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

#include "PageRank.h"
#include "Pregel/Aggregator.h"
#include "Pregel/GraphFormat.h"
#include "Pregel/Iterators.h"
#include "Pregel/MasterContext.h"
#include "Pregel/Utils.h"
#include "Pregel/VertexComputation.h"

using namespace arangodb;
using namespace arangodb::pregel;
using namespace arangodb::pregel::algos;

static float EPS = 0.00001f;
static std::string const kConvergence = "convergence";

struct PRWorkerContext : public WorkerContext {
  PRWorkerContext(std::unique_ptr<AggregatorHandler> readAggregators,
                  std::unique_ptr<AggregatorHandler> writeAggregators)
      : WorkerContext(std::move(readAggregators),
                      std::move(writeAggregators)){};

  float commonProb = 0;
  void preGlobalSuperstep(uint64_t gss) override {
    if (vertexCount() > 0) {
      if (gss == 0) {
        commonProb = 1.0f / vertexCount();
      } else {
        commonProb = 0.15f / vertexCount();
      }
    }
  }
};

PageRank::PageRank(VPackSlice const& params)
    : SimpleAlgorithm(params), _useSource(params.hasKey("sourceField")) {}

/// will use a seed value for pagerank if available
struct SeededPRGraphFormat final : public NumberGraphFormat<float, float> {
  SeededPRGraphFormat(std::string const& source, std::string const& result,
                      float vertexNull)
      : NumberGraphFormat(source, result, vertexNull, 0.0f) {}
};

std::shared_ptr<GraphFormat<float, float> const> PageRank::inputFormat() const {
  if (_useSource && !_sourceField.empty()) {
    return std::make_shared<SeededPRGraphFormat>(_sourceField, _resultField,
                                                 -1.0f);
  } else {
    return std::make_shared<VertexGraphFormat<float, float>>(_resultField,
                                                             -1.0f);
  }
}

struct PRComputation : public VertexComputation<float, float, float> {
  PRComputation() {}
  void compute(MessageIterator<float> const& messages) override {
    PRWorkerContext const* ctx = static_cast<PRWorkerContext const*>(context());
    float* ptr = mutableVertexData();
    float copy = *ptr;

    // initialize vertices to initial weight, unless there was a seed weight
    if (globalSuperstep() == 0) {
      if (*ptr < 0) {
        *ptr = ctx->commonProb;
      }
    } else {
      float sum = 0.0f;
      for (const float* msg : messages) {
        sum += *msg;
      }
      *ptr = 0.85f * sum + ctx->commonProb;
    }
    float diff = fabs(copy - *ptr);
    aggregate<float>(kConvergence, diff);

    size_t numEdges = getEdgeCount();
    if (numEdges > 0) {
      float val = *ptr / numEdges;
      sendMessageToAllNeighbours(val);
    }
  }
};

VertexComputation<float, float, float>* PageRank::createComputation(
    std::shared_ptr<WorkerConfig const> config) const {
  return new PRComputation();
}

[[nodiscard]] auto PageRank::workerContext(
    std::unique_ptr<AggregatorHandler> readAggregators,
    std::unique_ptr<AggregatorHandler> writeAggregators,
    velocypack::Slice userParams) const -> WorkerContext* {
  return new PRWorkerContext(std::move(readAggregators),
                             std::move(writeAggregators));
}
[[nodiscard]] auto PageRank::workerContextUnique(
    std::unique_ptr<AggregatorHandler> readAggregators,
    std::unique_ptr<AggregatorHandler> writeAggregators,
    velocypack::Slice userParams) const -> std::unique_ptr<WorkerContext> {
  return std::make_unique<PRWorkerContext>(std::move(readAggregators),
                                           std::move(writeAggregators));
}

struct PRMasterContext : public MasterContext {
  float _threshold = EPS;
  explicit PRMasterContext(uint64_t vertexCount, uint64_t edgeCount,
                           std::unique_ptr<AggregatorHandler> aggregators,
                           VPackSlice params)
      : MasterContext(vertexCount, edgeCount, std::move(aggregators)) {
    VPackSlice t = params.get("threshold");
    _threshold = t.isNumber() ? t.getNumber<float>() : EPS;
  }

  void preApplication() override {
    LOG_TOPIC("e0598", DEBUG, Logger::PREGEL)
        << "Using threshold " << _threshold << " for pagerank";
  }

  bool postGlobalSuperstep() override {
    float const* diff = getAggregatedValue<float>(kConvergence);
    return globalSuperstep() < 1 || *diff > _threshold;
  };
};

[[nodiscard]] auto PageRank::masterContext(
    std::unique_ptr<AggregatorHandler> aggregators,
    arangodb::velocypack::Slice userParams) const -> MasterContext* {
  return new PRMasterContext(0, 0, std::move(aggregators), userParams);
}
[[nodiscard]] auto PageRank::masterContextUnique(
    uint64_t vertexCount, uint64_t edgeCount,
    std::unique_ptr<AggregatorHandler> aggregators,
    arangodb::velocypack::Slice userParams) const
    -> std::unique_ptr<MasterContext> {
  return std::make_unique<PRMasterContext>(vertexCount, edgeCount,
                                           std::move(aggregators), userParams);
}

IAggregator* PageRank::aggregator(std::string const& name) const {
  if (name == kConvergence) {
    return new MaxAggregator<float>(-1, false);
  }
  return nullptr;
}
