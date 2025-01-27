/**
 *    Copyright (C) 2015 MongoDB Inc.
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
 *    must comply with the GNU Affero General Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#include "mongo/platform/basic.h"

#include "mongo/s/catalog/catalog_manager_mock.h"

#include "mongo/base/status.h"
#include "mongo/s/catalog/type_collection.h"
#include "mongo/s/catalog/type_database.h"
#include "mongo/s/catalog/type_settings.h"

namespace mongo {

using std::string;
using std::vector;

CatalogManagerMock::CatalogManagerMock() {
    _mockDistLockMgr = stdx::make_unique<DistLockManagerMock>();
}
CatalogManagerMock::~CatalogManagerMock() = default;

Status CatalogManagerMock::startup() {
    return Status::OK();
}

void CatalogManagerMock::shutDown(bool allowNetworking) {}

Status CatalogManagerMock::shardCollection(OperationContext* txn,
                                           const string& ns,
                                           const ShardKeyPattern& fieldsAndOrder,
                                           bool unique,
                                           const vector<BSONObj>& initPoints,
                                           const std::set<ShardId>& initShardIds) {
    return Status::OK();
}

StatusWith<string> CatalogManagerMock::addShard(OperationContext* txn,
                                                const std::string* shardProposedName,
                                                const ConnectionString& shardConnectionString,
                                                const long long maxSize) {
    return Status::OK();
}

StatusWith<ShardDrainingStatus> CatalogManagerMock::removeShard(OperationContext* txn,
                                                                const string& name) {
    return ShardDrainingStatus::COMPLETED;
}

Status CatalogManagerMock::updateDatabase(const string& dbName, const DatabaseType& db) {
    return Status::OK();
}

StatusWith<OpTimePair<DatabaseType>> CatalogManagerMock::getDatabase(const string& dbName) {
    return OpTimePair<DatabaseType>();
}

Status CatalogManagerMock::updateCollection(const string& collNs, const CollectionType& coll) {
    return Status::OK();
}

StatusWith<OpTimePair<CollectionType>> CatalogManagerMock::getCollection(const string& collNs) {
    return OpTimePair<CollectionType>();
}

Status CatalogManagerMock::getCollections(const string* dbName,
                                          vector<CollectionType>* collections,
                                          repl::OpTime* optime) {
    return Status::OK();
}

Status CatalogManagerMock::dropCollection(OperationContext* txn, const NamespaceString& ns) {
    return {ErrorCodes::InternalError, "Method not implemented"};
}

Status CatalogManagerMock::getDatabasesForShard(const string& shardName, vector<string>* dbs) {
    return Status::OK();
}

Status CatalogManagerMock::getChunks(const BSONObj& filter,
                                     const BSONObj& sort,
                                     boost::optional<int> limit,
                                     std::vector<ChunkType>* chunks,
                                     repl::OpTime* opTime) {
    return Status::OK();
}

Status CatalogManagerMock::getTagsForCollection(const string& collectionNs,
                                                vector<TagsType>* tags) {
    return Status::OK();
}

StatusWith<string> CatalogManagerMock::getTagForChunk(const string& collectionNs,
                                                      const ChunkType& chunk) {
    return string();
}

Status CatalogManagerMock::getAllShards(vector<ShardType>* shards) {
    return Status::OK();
}

bool CatalogManagerMock::runUserManagementWriteCommand(const string& commandName,
                                                       const string& dbname,
                                                       const BSONObj& cmdObj,
                                                       BSONObjBuilder* result) {
    return true;
}

bool CatalogManagerMock::runReadCommand(const std::string& dbname,
                                        const BSONObj& cmdObj,
                                        BSONObjBuilder* result) {
    return true;
}

bool CatalogManagerMock::runUserManagementReadCommand(const string& dbname,
                                                      const BSONObj& cmdObj,
                                                      BSONObjBuilder* result) {
    return true;
}

Status CatalogManagerMock::applyChunkOpsDeprecated(const BSONArray& updateOps,
                                                   const BSONArray& preCondition) {
    return Status::OK();
}

void CatalogManagerMock::logAction(const ActionLogType& actionLog) {}

void CatalogManagerMock::logChange(const string& clientAddress,
                                   const string& what,
                                   const string& ns,
                                   const BSONObj& detail) {}

StatusWith<SettingsType> CatalogManagerMock::getGlobalSettings(const string& key) {
    return SettingsType();
}

void CatalogManagerMock::writeConfigServerDirect(const BatchedCommandRequest& request,
                                                 BatchedCommandResponse* response) {}

DistLockManager* CatalogManagerMock::getDistLockManager() {
    return _mockDistLockMgr.get();
}

Status CatalogManagerMock::_checkDbDoesNotExist(const std::string& dbName, DatabaseType* db) {
    return Status::OK();
}

StatusWith<std::string> CatalogManagerMock::_generateNewShardName() {
    return {ErrorCodes::InternalError, "Method not implemented"};
}

Status CatalogManagerMock::checkAndUpgrade(bool checkOnly) {
    return Status::OK();
}

}  // namespace mongo
