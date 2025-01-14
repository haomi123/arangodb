////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2022-2022 ArangoDB GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Michael Hackstein
////////////////////////////////////////////////////////////////////////////////

#include "ResponsibleServerList.h"

#include "Basics/debugging.h"
#include "Cluster/ClusterHelpers.h"

using namespace arangodb;

ServerID const& ResponsibleServerList::getLeader() const {
  TRI_ASSERT(!servers.empty());
  return servers.front();
}

bool ResponsibleServerList::operator==(
    ResponsibleServerList const& other) const {
  return ClusterHelpers::compareServerLists(servers, other.servers);
}
