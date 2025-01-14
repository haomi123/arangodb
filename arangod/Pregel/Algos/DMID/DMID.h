////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2023 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
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
/// @author Simon Grätzer
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Pregel/Algorithm.h"
#include "Pregel/Algos/DMID/DMIDValue.h"
#include "Pregel/Algos/DMID/DMIDMessage.h"

namespace arangodb {
namespace pregel {
namespace algos {

/// https://github.com/Rofti/DMID

struct DMIDType {
  using Vertex = DMIDValue;
  using Edge = float;
  using Message = DMIDMessage;
};

struct DMID : public SimpleAlgorithm<DMIDValue, float, DMIDMessage> {
  unsigned _maxCommunities = 1;

 public:
  explicit DMID(VPackSlice userParams)
      : SimpleAlgorithm<DMIDValue, float, DMIDMessage>(userParams) {
    arangodb::velocypack::Slice val = userParams.get("maxCommunities");
    if (val.isInteger()) {
      _maxCommunities = (unsigned)std::min(
          (uint64_t)32, std::max(val.getUInt(), (uint64_t)0));
    }
  }

  [[nodiscard]] auto name() const -> std::string_view override {
    return "DMID";
  };

  std::shared_ptr<GraphFormat<DMIDValue, float> const> inputFormat()
      const override;
  MessageFormat<DMIDMessage>* messageFormat() const override;
  [[nodiscard]] auto messageFormatUnique() const
      -> std::unique_ptr<message_format> override;

  VertexComputation<DMIDValue, float, DMIDMessage>* createComputation(
      std::shared_ptr<WorkerConfig const>) const override;

  [[nodiscard]] auto workerContext(
      std::unique_ptr<AggregatorHandler> readAggregators,
      std::unique_ptr<AggregatorHandler> writeAggregators,
      velocypack::Slice userParams) const -> WorkerContext* override;
  [[nodiscard]] auto workerContextUnique(
      std::unique_ptr<AggregatorHandler> readAggregators,
      std::unique_ptr<AggregatorHandler> writeAggregators,
      velocypack::Slice userParams) const
      -> std::unique_ptr<WorkerContext> override;

  [[nodiscard]] auto masterContext(
      std::unique_ptr<AggregatorHandler> aggregators,
      arangodb::velocypack::Slice userParams) const -> MasterContext* override;
  [[nodiscard]] auto masterContextUnique(
      uint64_t vertexCount, uint64_t edgeCount,
      std::unique_ptr<AggregatorHandler> aggregators,
      arangodb::velocypack::Slice userParams) const
      -> std::unique_ptr<MasterContext> override;

  IAggregator* aggregator(std::string const& name) const override;
};
}  // namespace algos
}  // namespace pregel
}  // namespace arangodb
