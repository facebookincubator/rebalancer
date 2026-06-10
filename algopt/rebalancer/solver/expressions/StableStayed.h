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

#include "algopt/rebalancer/algopt_common/AssociativeHybridMap.h"
#include "algopt/rebalancer/solver/expressions/Evaluator.h"
#include "algopt/rebalancer/solver/expressions/ObjectLookup.h"
#include "algopt/rebalancer/solver/if/gen-cpp2/packer_types.h"
#include "algopt/rebalancer/solver/utils/BoundConstraints.h"
#include "algopt/rebalancer/solver/utils/Util.h"

namespace facebook::rebalancer {

class Expression;
class Change;
class ObjectVector;

class StableStayed : public ObjectLookup {
 public:
  explicit StableStayed(
      std::shared_ptr<ObjectVector> initialObjectVector,
      std::shared_ptr<ObjectVector> fullObjectVector,
      std::shared_ptr<const PackerSet<entities::ContainerId>> containersPtr,
      std::shared_ptr<const entities::Universe> universe,
      const Assignment& initialAssignment);

  void updateEquivalenceSets(EquivalenceSets& equivSets) const override;

  using Expression::lp;
  virtual algopt::lp::Expression lp(
      const LpEvaluator& evaluator,
      bool minimizing,
      const interface::OptimalSolverSpec& configs) override;

  virtual const std::string_view& getType() const override;

  virtual bool hasNoLpIntent() const override;

  virtual bool needsEquivalenceSetBasedPostProcessing() const override;

 private:
  std::shared_ptr<ObjectVector> fullObjectVector_;
};
} // namespace facebook::rebalancer
