// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "algopt/rebalancer/common/log/MultiLog.h"

namespace facebook {
namespace rebalancer {

template <class T>
void logToAll(
    const T& info,
    std::vector<std::shared_ptr<RebalancerLog>>& loggers) {
  for (auto& logger : loggers) {
    logger->log(info);
  }
}

MultiLog::MultiLog(std::vector<std::shared_ptr<RebalancerLog>> loggers)
    : loggers(std::move(loggers)) {}

void MultiLog::log(const ProblemDefinition& info) {
  logToAll(info, loggers);
}

void MultiLog::log(const interface::GlobalObjectiveSummary& info) {
  logToAll(info, loggers);
}

void MultiLog::log(const interface::ConstraintSummary& info) {
  logToAll(info, loggers);
}

void MultiLog::log(const SolutionStats& info) {
  logToAll(info, loggers);
}

void MultiLog::log(const interface::MovesSummary& info) {
  logToAll(info, loggers);
}

void MultiLog::log(const SolverSummary& info) {
  logToAll(info, loggers);
}

void MultiLog::log(const ExitStatusInfo& info) {
  logToAll(info, loggers);
}

void MultiLog::log(const BenchmarkInfo& info) {
  logToAll(info, loggers);
}

void MultiLog::log(const interface::FinalEvaluationSummary& info) {
  logToAll(info, loggers);
}

void MultiLog::log(const TupperwareLogSummary& info) {
  logToAll(info, loggers);
}

void MultiLog::log(const interface::LocalSearchProfile& info) {
  logToAll(info, loggers);
}

void MultiLog::log(const interface::SpecMetadata& info) {
  logToAll(info, loggers);
}

void MultiLog::log(const interface::HierarchicalProfileNode& info) {
  logToAll(info, loggers);
}

void MultiLog::log(const SpecUsageInfo& info) {
  logToAll(info, loggers);
}

void MultiLog::log(const ManifoldInfo& info) {
  logToAll(info, loggers);
}

void MultiLog::log(const interface::thrift::Metrics& metrics) {
  logToAll(metrics, loggers);
}

void MultiLog::log(const NodeSummary& info) {
  logToAll(info, loggers);
}

} // namespace rebalancer
} // namespace facebook
