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

#include "algopt/rebalancer/common/log/InMemoryLog.h"

#include <algopt/rebalancer/interface/thrift/gen-cpp2/Metrics_types.h>
#include <algopt/rebalancer/interface/thrift/gen-cpp2/Types_types.h>

namespace facebook {
namespace rebalancer {

void InMemoryLog::log(const interface::MovesSummary& info) {
  moves.push_back(info);
}

void InMemoryLog::log(const interface::GlobalObjectiveSummary& info) {
  objectiveSummaries.push_back(info);
}

void InMemoryLog::log(const interface::ConstraintSummary& info) {
  constraintSummaries.push_back(info);
}

void InMemoryLog::log(const SolverSummary& info) {
  solverSummaries.push_back(info);
}

void InMemoryLog::log(const interface::FinalEvaluationSummary& info) {
  finalEvaluationSummary = info;
}

void InMemoryLog::log(const interface::LocalSearchProfile& info) {
  localSearchProfiles_.push_back(info);
}

void InMemoryLog::log(const interface::SpecMetadata& info) {
  specMetadata_.push_back(info);
}

std::vector<interface::MovesSummary> InMemoryLog::flushMoves() {
  return std::move(moves);
}

interface::GlobalObjectiveSummary InMemoryLog::getInitialObjective() const {
  checkObjectives();
  return objectiveSummaries.front();
}

interface::ConstraintSummary InMemoryLog::getInitialConstraint() const {
  checkConstraints();
  return constraintSummaries.front();
}

interface::GlobalObjectiveSummary InMemoryLog::getFinalObjective() const {
  checkObjectives();
  return objectiveSummaries.back();
}

interface::ConstraintSummary InMemoryLog::getFinalConstraint() const {
  checkConstraints();
  return constraintSummaries.back();
}

const std::vector<SolverSummary>& InMemoryLog::getSolverSummaries() const {
  return solverSummaries;
}

const std::optional<interface::FinalEvaluationSummary>&
InMemoryLog::getFinalEvaluationSummary() const {
  return finalEvaluationSummary;
}

void InMemoryLog::checkObjectives() const {
  if (objectiveSummaries.size() != 2) {
    throw std::runtime_error(
        "expected 2 objective summaries but found " +
        std::to_string(objectiveSummaries.size()));
  }
}

void InMemoryLog::checkConstraints() const {
  if (constraintSummaries.size() != 2) {
    throw std::runtime_error(
        "expected 2 constraint summaries but found " +
        std::to_string(constraintSummaries.size()));
  }
}

const std::vector<interface::LocalSearchProfile>&
InMemoryLog::getLocalSearchProfiles() const {
  return localSearchProfiles_;
}

const std::vector<interface::SpecMetadata>& InMemoryLog::getSpecMetadata()
    const {
  return specMetadata_;
}

void InMemoryLog::log(const interface::thrift::Metrics& info) {
  metrics_.push_back(info);
}

interface::thrift::Metrics InMemoryLog::getInitialMetrics() const {
  if (metrics_.empty()) {
    return interface::thrift::Metrics();
  }

  checkMetrics();
  return metrics_.front();
}

interface::thrift::Metrics InMemoryLog::getFinalMetrics() const {
  if (metrics_.empty()) {
    return interface::thrift::Metrics();
  }

  checkMetrics();
  return metrics_.back();
}

void InMemoryLog::checkMetrics() const {
  // if metrics are logged, then there should be exactly 2 summaries
  if (metrics_.size() > 0 && metrics_.size() != 2) {
    throw std::runtime_error(
        fmt::format(
            "expected 2 'metrics' info (initial and final), but found {} 'metrics' info",
            metrics_.size()));
  }
}

} // namespace rebalancer
} // namespace facebook
