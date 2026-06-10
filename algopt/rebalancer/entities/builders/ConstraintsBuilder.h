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

#include "algopt/rebalancer/entities/builders/AsyncValueMap.h"
#include "algopt/rebalancer/entities/builders/IdBuilder.h"
#include "algopt/rebalancer/entities/Constraint.h"
#include "algopt/rebalancer/entities/Identifiers.h"

#include <folly/container/F14Map.h>
#include <folly/coro/Collect.h>
#include <folly/coro/Task.h>
#include <folly/futures/SharedPromise.h>

namespace facebook::rebalancer::entities {

struct ConstraintData {
  std::shared_ptr<entities::Constraint> constraint;
};

struct ConstraintsBuilderResult {
  Map<std::string, ConstraintId> constraintNameToId;
  std::map<ConstraintId, std::shared_ptr<Constraint>> constraintIdToConstraint;
};

class ConstraintsBuilder {
 public:
  explicit ConstraintsBuilder(IdBuilder& idBuilder) : idBuilder_(idBuilder) {}

  folly::coro::Task<void> add(
      ConstraintId constraintId,
      ConstraintData constraintData) {
    co_return constraintIdToData_.set(constraintId, std::move(constraintData));
  }

  ConstraintId makeConstraintId(const std::string& name);

  folly::coro::Task<ConstraintsBuilderResult> build() const;

  folly::coro::Task<std::string> summarize() const;

 private:
  IdBuilder& idBuilder_;
  AsyncValueMap<ConstraintId, entities::ConstraintData> constraintIdToData_;
  folly::Synchronized<Map<std::string, ConstraintId>> constraintNameToId_;
};

} // namespace facebook::rebalancer::entities
