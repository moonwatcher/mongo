/**
 *    Copyright (C) 2012 10gen Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the GNU Affero General Public License in all respects
 *    for all of the code used other than as permitted herein. If you modify
 *    file(s) with this exception, you may extend this exception to your
 *    version of the file(s), but you are not obligated to do so. If you do not
 *    wish to do so, delete this exception statement from your version. If you
 *    delete this exception statement from all source files in the program,
 *    then also delete it in the license file.
 */

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kSharding

#include "mongo/platform/basic.h"

#include "mongo/s/catalog/legacy/cluster_client_internal.h"

#include <vector>

#include "mongo/client/connpool.h"
#include "mongo/db/field_parser.h"
#include "mongo/s/catalog/catalog_manager.h"
#include "mongo/s/catalog/type_mongos.h"
#include "mongo/s/catalog/type_shard.h"
#include "mongo/s/client/shard_registry.h"
#include "mongo/s/grid.h"
#include "mongo/util/log.h"
#include "mongo/util/stringutils.h"

namespace mongo {

using std::endl;
using std::string;
using std::unique_ptr;
using std::vector;
using mongoutils::str::stream;

Status checkClusterMongoVersions(CatalogManager* catalogManager, const string& minMongoVersion) {
    unique_ptr<ScopedDbConnection> connPtr;

    //
    // Find mongos pings in config server
    //

    try {
        connPtr.reset(
            new ScopedDbConnection(grid.shardRegistry()->getConfigServerConnectionString(), 30));
        ScopedDbConnection& conn = *connPtr;
        unique_ptr<DBClientCursor> cursor(_safeCursor(conn->query(MongosType::ConfigNS, Query())));

        while (cursor->more()) {
            BSONObj pingDoc = cursor->next();

            auto mongosTypeResult = MongosType::fromBSON(pingDoc);
            if (!mongosTypeResult.isOK()) {
                warning() << "could not parse ping document: " << pingDoc << " : "
                          << mongosTypeResult.getStatus().toString();
                continue;
            }
            const MongosType& ping = mongosTypeResult.getValue();

            string mongoVersion = "2.0";
            // Hack to determine older mongos versions from ping format
            if (ping.isWaitingSet())
                mongoVersion = "2.2";
            if (ping.isMongoVersionSet() && ping.getMongoVersion() != "") {
                mongoVersion = ping.getMongoVersion();
            }

            Date_t lastPing = ping.getPing();

            Minutes quietIntervalMins{0};
            Date_t currentJsTime = jsTime();
            if (currentJsTime >= lastPing) {
                quietIntervalMins = duration_cast<Minutes>(currentJsTime - lastPing);
            }

            // We assume that anything that hasn't pinged in 5 minutes is probably down
            if (quietIntervalMins >= Minutes{5}) {
                log() << "stale mongos detected " << durationCount<Minutes>(quietIntervalMins)
                      << " minutes ago, network location is " << pingDoc["_id"].String()
                      << ", not checking version";
            } else {
                if (versionCmp(mongoVersion, minMongoVersion) < 0) {
                    return Status(
                        ErrorCodes::RemoteValidationError,
                        stream() << "version " << mongoVersion << " detected on mongos at "
                                 << ping.getName() << ", but version >= " << minMongoVersion
                                 << " required; you must wait 5 minutes "
                                 << "after shutting down a pre-" << minMongoVersion << " mongos");
                }
            }
        }
    } catch (const DBException& e) {
        return e.toStatus("could not read mongos pings collection");
    }

    connPtr->done();

    //
    // Load shards from config server
    //

    vector<HostAndPort> servers;

    try {
        vector<ShardType> shards;
        Status status = catalogManager->getAllShards(&shards);
        if (!status.isOK()) {
            return status;
        }

        for (const ShardType& shard : shards) {
            Status status = shard.validate();
            if (!status.isOK()) {
                return Status(ErrorCodes::UnsupportedFormat,
                              stream() << "shard " << shard.toBSON()
                                       << " failed validation: " << causedBy(status));
            }

            const auto shardConnStatus = ConnectionString::parse(shard.getHost());
            if (!shardConnStatus.isOK()) {
                return Status(ErrorCodes::UnsupportedFormat,
                              stream() << "invalid shard host " << shard.getHost()
                                       << " read from the config server"
                                       << shardConnStatus.getStatus().toString());
            }

            vector<HostAndPort> shardServers = shardConnStatus.getValue().getServers();
            servers.insert(servers.end(), shardServers.begin(), shardServers.end());
        }
    } catch (const DBException& e) {
        return e.toStatus("could not read shards collection");
    }

    // Add config servers to list of servers to check version against
    vector<HostAndPort> configServers =
        grid.shardRegistry()->getConfigServerConnectionString().getServers();
    servers.insert(servers.end(), configServers.begin(), configServers.end());

    //
    // We've now got all the shard info from the config server, start contacting the shards
    // and config servers and verifying their versions.
    //

    for (vector<HostAndPort>::iterator serverIt = servers.begin(); serverIt != servers.end();
         ++serverIt) {
        // Note: This will *always* be a single-host connection
        ConnectionString serverLoc(*serverIt);
        dassert(serverLoc.type() == ConnectionString::MASTER ||
                serverLoc.type() == ConnectionString::CUSTOM);  // for dbtests

        log() << "checking that version of host " << serverLoc << " is compatible with "
              << minMongoVersion << endl;

        unique_ptr<ScopedDbConnection> serverConnPtr;

        bool resultOk;
        BSONObj buildInfo;

        try {
            serverConnPtr.reset(new ScopedDbConnection(serverLoc, 30));
            ScopedDbConnection& serverConn = *serverConnPtr;

            resultOk = serverConn->runCommand("admin", BSON("buildInfo" << 1), buildInfo);
        } catch (const DBException& e) {
            warning() << "could not run buildInfo command on " << serverLoc.toString() << " "
                      << causedBy(e) << ". Please ensure that this server is up and at a "
                                        "version >= " << minMongoVersion;
            continue;
        }

        // TODO: Make running commands saner such that we can consolidate error handling
        if (!resultOk) {
            return Status(ErrorCodes::UnknownError,
                          stream() << DBClientConnection::getLastErrorString(buildInfo)
                                   << causedBy(buildInfo.toString()));
        }

        serverConnPtr->done();

        verify(buildInfo["version"].type() == String);
        string mongoVersion = buildInfo["version"].String();

        if (versionCmp(mongoVersion, minMongoVersion) < 0) {
            return Status(ErrorCodes::RemoteValidationError,
                          stream() << "version " << mongoVersion << " detected on mongo "
                                                                    "server at "
                                   << serverLoc.toString() << ", but version >= " << minMongoVersion
                                   << " required");
        }
    }

    return Status::OK();
}

// Helper function for safe cursors
DBClientCursor* _safeCursor(unique_ptr<DBClientCursor> cursor) {
    // TODO: Make error handling more consistent, it's annoying that cursors error out by
    // throwing exceptions *and* being empty
    uassert(16625, str::stream() << "cursor not found, transport error", cursor.get());
    return cursor.release();
}
}
