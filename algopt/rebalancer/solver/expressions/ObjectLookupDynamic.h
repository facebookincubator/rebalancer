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
#include "algopt/rebalancer/solver/expressions/LinearSum.h"
#include "algopt/rebalancer/solver/expressions/ObjectLookup.h"

namespace facebook::rebalancer {

/** ObjectLookup over scopeItem s1 is simply a weighted sum
 * of objects in S where weights are determined by the dimension's values.
 *
 * -- For static dimension, the weights are independent of the scopeItem s1.
 * -- For dynamic dimension, the weights are dependent on the scopeItem s1.
 *
 * Specifically, suppose that the value of dynamic dimension depends on the
 * dimension's scope say S_d. Therefore, if we want to compute utilization of a
 * dynamic dimension for a given scopeItem s1 (of a potentially different scope
 * S), we can no longer use traditional ObjectLookup.
 *
 * This is because S_d may split the containers of s1 into multiple disjoint
 * sets and for each such set, we will have different values of objects =>
 * different ObjectVector => different ObjectLookup.
 *
 * Currently, we express it is a sum of ObjectLookup over set of containers
 * defined by image of s1 on scope S_d. That is, scope S will split the
 * containers of scopeItem s1 into a disjoint collection of sets say C1, C2, ..
 * Ck. Each such set corresponds to an ObjectLookup expression on the
 * corresponding container set Ci.
 *
 * Eventually, we will like to create a new expression that does the right thing
 * for dynamic dimensions whithout expressing it as a sum of ObjectLookups. This
 * is tracked in T243122344.
 *
 * We introduce this special expression for following reasons:
 * - It enables optimizations in hottest container ordering that depend on
 *  'directlyAffectedContainers'. For this reason, we explicitly set the
 *  'directlyAffectedContainers' in the constructor.
 *
 * - For debugability, as it clearly encapsulates ObjectLookups for a dynamic
 *  dimension
 *
 * - Eventually, when this expression is implemented in a compact way (currently
 * it is just a sum of ObjectLookups), it will save memory and computation.
 */

class ObjectLookupDynamic : public Expression {
 public:
  /** Expects that @param dimension is dynamic and container sets of each
   * ObjectLookup in @param sumOfObjectLookups belong to different scopeItems of
   * @param dimension's scope. NOTE: the current implementation uses a sum of
   * ObjectLookups but this could change with a different implementation.
   */
  explicit ObjectLookupDynamic(
      ExprPtr sumOfObjectLookups,
      const entities::ObjectScalarDimension& dimension,
      std::shared_ptr<const entities::Universe> universe);

  const std::string_view& getType() const override;

  std::optional<AffectedByChange> isAffectedByChange(
      const AffectedByChangeDecisionData& data) const override;

  double evaluate(
      const BottomToTopEvaluator& evaluator,
      const ChangeSet& changes) const override;

  void updateEquivalenceSets(EquivalenceSets& equivalenceSets) const override;

  algopt::lp::Expression lp(
      const LpEvaluator& evaluator,
      bool minimizing,
      const interface::OptimalSolverSpec& configs) override;

  bool hasNoLpIntent() const override;

  ExpressionProperties getProperties() const override;

  void lpIntent(const LpEvaluator& evaluator, bool minimizing) override;

 private:
  double innerFullApply(
      const TopToBottomEvaluator& evaluator,
      const Assignment& assignment) override;

  double innerPartialApply(
      const BottomToTopEvaluator& evaluator,
      const Assignment& assignment,
      const ChangeSet& changes) override;

  std::string innerDigest(size_t maxChildren = 10) const override;

  Bounds innerLowerAndUpperBounds(Context& context, const BoundConstraints& bc)
      const override;

  // currently, this expression is modeled using a sum of ObjectLookup, one on
  // each ContainerSet in containerSets_. This function returns that expression
  // (which is the only child of this expression)
  LinearSum* getSumOfLookups() const;

 private:
  std::shared_ptr<PackerSet<entities::ContainerId>> allContainers_ =
      std::make_shared<PackerSet<entities::ContainerId>>();
  static constexpr std::string_view type = "ObjectLookupDynamic";
};

} // namespace facebook::rebalancer
