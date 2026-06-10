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

#include "algopt/rebalancer/algopt_common/AssociativeMap.h"
#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/entities/Universe.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"
#include "algopt/rebalancer/solver/utils/Assignment.h"

#include <folly/SynchronizedPtr.h>

#include <cstdint>

namespace facebook::rebalancer {

/** Let S be a set of scope items, and G be a set of groups.
   For a group g in G, GroupScopeItemTransformUtil(g, S, TYPE) is the
   total "transformed" utilization of scope items in S with respect to
   the given dimension.

   TYPE defines the transformation function that is used. Currently supported
   types are
    - STEP: which succinctly represents the count of scope items with positive
      utilization namely, sum_{s in S} STEP(UTIL(g,s)). The transformation
      function here is STEP, that is f(x) = 1 if x > 0 and f(x) = 0 otherwise.
    - IDENTITY: represents total utilization for g in S, given by sum_{s in S}
      UTIL(g, s). The transformation function f is IDENTITY function f(x) = x
    - SQUARE: sum of squares of utilization for  g in S, given by sum_{s in S)
      UTIL(g, s)^2. The transformation function f is SQUARE function f(x) = x^2
    - STEP_MOD_K: for a given constant K, this represents the function sum_{s in
      S} STEP(UTIL(g, s) % K). In other words, the transformation function f is
      f(x) = 1 if x is NOT divisble by K and f(x) = 0 otherwise.

   In other words, GroupScopeItemTransformUtil(g, S) is the sum_{s in S}
   f(UTIL(g, s)) where f is defined by TYPE.

   One application of this expression is when we want to compute the sum of
   group utilization over all groups and scopeItems. Indeed writing that formula
   as UTIL(g, s) over all groups has quadratic complexity (representation size
   is |groups| * |scopeItems|). Using GroupScopeItemTransformUtil(g, S) will
   have linear complexity (representation size is linear in |groups|).

   With this expression, we intend to represent expression succinctly.

   The memory consumed by this expression is O(|g|) which when amortized over
   all groups has total memory of O(sum_g(|G|)) = ~O(|objects|) for a group-wise
   disjoint partition
*/

// Helper class and structs needed to store data for some transform functions
// e.g. STEP_MOD_K transform may need to store the value of K for each scopeItem
class ModKTransformData {
 public:
  explicit ModKTransformData(
      uint32_t defaultValue,
      entities::Map<entities::ScopeItemId, uint32_t> perScopeItemValue);

  uint32_t get(entities::ScopeItemId scopeItemId) const;

 private:
  uint32_t defaultK_;
  entities::Map<entities::ScopeItemId, uint32_t> perScopeItemValue_;
};

struct TransformFunctionData {
  std::optional<ModKTransformData> kForModKTransform;
};

class GroupScopeItemTransformUtil : public Expression {
 public:
  enum class TransformFunctionType {
    STEP,
    IDENTITY,
    SQUARE,
    STEP_MOD_K,
  };

  explicit GroupScopeItemTransformUtil(
      std::shared_ptr<const entities::Universe> universe,
      entities::PartitionId partitionId,
      entities::GroupId groupId,
      entities::DimensionId dimensionId,
      entities::ScopeId scopeId,
      const std::vector<entities::ScopeItemId>& allowedScopeItems,
      std::shared_ptr<const entities::Set<entities::ContainerId>> containersPtr,
      const Assignment& initialAssignment,
      folly::F14FastMap<entities::ScopeItemId, double> scopeItemWeights = {},
      double scopeItemDefaultWeight = 1,
      TransformFunctionType transformType = TransformFunctionType::STEP,
      double normalizationConstant = 1,
      TransformFunctionData transformFunctionData = TransformFunctionData());

  const std::string_view& getType() const override;

  std::optional<AffectedByChange> isAffectedByChange(
      const AffectedByChangeDecisionData& data) const override;

  double evaluate(
      const BottomToTopEvaluator& evaluator,
      const ChangeSet& changes) const override;

  void updateEquivalenceSets(EquivalenceSets& equivalenceSets) const override;

  virtual algopt::lp::Expression lp(
      const LpEvaluator& evaluator,
      bool minimizing,
      const interface::OptimalSolverSpec& configs) override;

  virtual bool hasNoLpIntent() const override;

  ExpressionProperties getProperties() const override;

 private:
  void set_directly_affected_containers();

  double applyAssignment(const Assignment& assignment);

  double innerFullApply(
      const TopToBottomEvaluator& evaluator,
      const Assignment& assignment) override;

  double innerPartialApply(
      const BottomToTopEvaluator& evaluator,
      const Assignment& assignment,
      const ChangeSet& changes) override;

  std::string innerDigest(size_t /*maxChildren*/) const override;

  // computes delta per container into the provided map (cleared first)
  void computeDeltaPerContainer(
      const ChangeSet& changes,
      folly::F14FastMap<entities::ContainerId, double>& deltaPerContainer)
      const;

  // computes delta per scope item into the provided map (cleared first)
  void computeDeltaPerScopeItem(
      const ChangeSet& changes,
      folly::F14FastMap<entities::ScopeItemId, double>& deltaPerScopeItem)
      const;

  Bounds innerLowerAndUpperBounds(Context& context, const BoundConstraints& bc)
      const override;

  double getWeight(entities::ScopeItemId scopeItemId) const;

  template <typename ScopeItemCollection>
  double getMaxPossibleExpressionValue(
      const ScopeItemCollection& allowedScopeItems) const;

  double getNormalizedValue(double val, double normalizationConst);

  double getObjectValue(
      entities::ObjectId objectId,
      entities::ContainerId containerId) const;

  ExprPtr getExprForLp(const Problem& problem) const;

  void validateTransformFunctionData() const;

 private:
  const std::shared_ptr<const entities::Universe> universe_;
  const entities::ScopeId scopeId_;
  const entities::Scope& scope_;
  const entities::PartitionId partitionId_;
  const entities::GroupId groupId_;
  double maxPossibleValue_ = 0;

  // maintains a dynamic map of contribution to the utilization of scopeItems by
  // objects in this group. This will be updated with every apply(..) Note that
  // the total size of this map is O(|group|) because we only store containers
  // that have non-zero utilization wrt to the group.
  folly::F14FastMap<
      entities::ScopeItemId,
      algopt::SumMap<entities::ContainerId, double>>
      scopeItemsToContainerUtil_;

  // Maintains utilization of every scopeItem that contains at least one object
  // of this group. Also updated with every apply(..)
  algopt::SumMap<entities::ScopeItemId, double> scopeItemUtil_;

  // stores the relevant set of objects and containers
  std::shared_ptr<entities::Set<entities::ObjectId>> relevantObjectsPtr_;
  std::shared_ptr<const entities::Set<entities::ContainerId>> containersPtr_;

  // stores the transformed object values (if the dimension is dynamic
  // dimension, stores max values)
  PackerMap<entities::ObjectId, double> objectValues_;
  // stores the transformed object values for dynamic dimensions
  PackerMap<entities::ObjectId, PackerMap<entities::ContainerId, double>>
      objectValuesDynamic_;
  // stores object values per scope item for dynamic dimensions
  PackerMap<entities::ScopeItemId, PackerMap<entities::ObjectId, double>>
      objectValuesForScopeItem_;

  // stores scopeItem weights
  folly::F14FastMap<entities::ScopeItemId, double> scopeItemWeights_;
  double scopeItemDefaultWeight_ = 1;
  const TransformFunctionType transformType_;
  const std::function<double(double, entities::ScopeItemId)> transformFunction_;
  const entities::ObjectScalarDimension& dimension_;
  const std::vector<entities::ScopeItemId> allowedScopeItems_;
  folly::SynchronizedPtr<ExprPtr> exprForLp_{nullptr};
  TransformFunctionData transformFunctionData_;
};

} // namespace facebook::rebalancer
