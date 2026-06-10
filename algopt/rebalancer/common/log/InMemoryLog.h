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

namespace facebook {
namespace rebalancer {

class InMemoryLog : public RebalancerLog {
 public:
  void log(const interface::GlobalObjectiveSummary& info) override;
  void log(const interface::ConstraintSummary& info) override;
  void log(const interface::MovesSummary& info) override;
  void log(const SolverSummary& info) override;
  void log(const interface::FinalEvaluationSummary& info) override;
  void log(const interface::LocalSearchProfile& info) override;
  void log(const interface::SpecMetadata& info) override;
  void log(const interface::thrift::Metrics& info) override;

  std::vector<interface::MovesSummary> flushMoves();
  interface::GlobalObjectiveSummary getInitialObjective() const;
  interface::ConstraintSummary getInitialConstraint() const;
  interface::GlobalObjectiveSummary getFinalObjective() const;
  interface::ConstraintSummary getFinalConstraint() const;
  const std::vector<SolverSummary>& getSolverSummaries() const;
  const std::optional<interface::FinalEvaluationSummary>&
  getFinalEvaluationSummary() const;
  const std::vector<interface::LocalSearchProfile>& getLocalSearchProfiles()
      const;
  const std::vector<interface::SpecMetadata>& getSpecMetadata() const;
  interface::thrift::Metrics getInitialMetrics() const;
  interface::thrift::Metrics getFinalMetrics() const;

 private:
  void checkObjectives() const;
  void checkConstraints() const;
  void checkMetrics() const;

  std::vector<interface::MovesSummary> moves;
  std::vector<interface::GlobalObjectiveSummary> objectiveSummaries;
  std::vector<interface::ConstraintSummary> constraintSummaries;
  std::vector<SolverSummary> solverSummaries;
  std::optional<interface::FinalEvaluationSummary> finalEvaluationSummary;
  std::vector<interface::LocalSearchProfile> localSearchProfiles_;
  std::vector<interface::SpecMetadata> specMetadata_;
  std::vector<interface::thrift::Metrics> metrics_;
};

} // namespace rebalancer
} // namespace facebook
