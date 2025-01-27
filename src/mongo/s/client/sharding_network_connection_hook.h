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

#pragma once

#include "mongo/executor/network_connection_hook.h"

namespace mongo {

/**
 * An implementation of NetworkConnectionHook for handling sharding-specific operations such
 * as sending sharding initialization information to shards and indicating up the call stack that
 * swapping the active catalog manager is needed during upgrade to CSRS.
 */
class ShardingNetworkConnectionHook final : public executor::NetworkConnectionHook {
public:
    ShardingNetworkConnectionHook() = default;
    virtual ~ShardingNetworkConnectionHook() = default;

    /**
     * Looks for the presence of a configsvr field in the ismaster response.  If no such field
     * exits, does nothing and returns Status::OK().  If the field is present, asks the grid
     * whether swapping catalog managers is needed and returns its response.
     */
    Status validateHost(const HostAndPort& remoteHost,
                        const executor::RemoteCommandResponse& isMasterReply) override;

    /**
     * Implementation of validateHost can be called without a ShardingNetworkConnectionHook
     * instance.
     */
    static Status validateHostImpl(const HostAndPort& remoteHost,
                                   const executor::RemoteCommandResponse& isMasterReply);

    /**
     * Currently a no-op.
     */
    StatusWith<boost::optional<executor::RemoteCommandRequest>> makeRequest(
        const HostAndPort& remoteHost) override;

    /**
     * Currently a no-op.
     */
    Status handleReply(const HostAndPort& remoteHost,
                       executor::RemoteCommandResponse&& response) override;
};

}  // namespace mongo
