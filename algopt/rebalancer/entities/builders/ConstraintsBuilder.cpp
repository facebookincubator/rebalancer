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

#include "algopt/rebalancer/entities/builders/ConstraintsBuilder.h"

#include "algopt/rebalancer/entities/builders/BuilderUtils.h"

namespace facebook::rebalancer::entities {

namespace {
constexpr static std::string_view kMakeConstraintId{"makeConstraintId"};
}

ConstraintId ConstraintsBuilder::makeConstraintId(
    const std::string& constraintName) {
  return idBuilder_.makeIdFromName(
      constraintName,
      constraintNameToId_,
      constraintIdToData_,
      kMakeConstraintId);
}

folly::coro::Task<ConstraintsBuilderResult> ConstraintsBuilder::build() const {
  ConstraintsBuilderResult result;
  co_await constraintIdToData_.waitAndReadFromEach(
      [&](ConstraintId id, const ConstraintData& data) {
        result.constraintIdToConstraint.emplace(id, data.constraint);
      });
  result.constraintNameToId = *constraintNameToId_.rlock();

  co_return result;
}

folly::coro::Task<std::string> ConstraintsBuilder::summarize() const {
  co_return summarizeNames("Constraints", *constraintNameToId_.rlock());
}

} // namespace facebook::rebalancer::entities
