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

#pragma once

#include "algopt/rebalancer/common/log/RebalancerLog.h"

#include <memory>
#include <vector>

namespace facebook {
namespace rebalancer {

class MultiLog : public RebalancerLog {
 public:
  explicit MultiLog(std::vector<std::shared_ptr<RebalancerLog>> loggers);
  void log(const ProblemDefinition& info) override;
  void log(const interface::GlobalObjectiveSummary& info) override;
  void log(const interface::ConstraintSummary& info) override;
  void log(const SolutionStats& info) override;
  void log(const interface::MovesSummary& info) override;
  void log(const SolverSummary& info) override;
  void log(const ExitStatusInfo& info) override;
  void log(const BenchmarkInfo& info) override;
  void log(const interface::FinalEvaluationSummary& info) override;
  void log(const TupperwareLogSummary& info) override;
  void log(const interface::LocalSearchProfile& info) override;
  void log(const interface::SpecMetadata& info) override;
  void log(const interface::HierarchicalProfileNode& info) override;
  void log(const SpecUsageInfo& /*info*/) override;
  void log(const ManifoldInfo& /*info*/) override;
  void log(const interface::thrift::Metrics& metrics) override;
  void log(const NodeSummary& /*info*/) override;

 private:
  std::vector<std::shared_ptr<RebalancerLog>> loggers;
};

} // namespace rebalancer
} // namespace facebook
