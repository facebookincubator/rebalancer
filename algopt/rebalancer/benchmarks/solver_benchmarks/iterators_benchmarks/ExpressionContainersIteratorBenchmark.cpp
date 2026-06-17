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

#include "algopt/rebalancer/solver/expressions/LinearSum.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"
#include "algopt/rebalancer/solver/expressions/tests/ExpressionUtils.h"
#include "algopt/rebalancer/solver/expressions/TopToBottomEvaluator.h"
#include "algopt/rebalancer/solver/iterators/ExpressionContainersIterator.h"
#include "algopt/rebalancer/solver/tests/IdConverterTestUtils.h"
#include "algopt/rebalancer/solver/utils/Assignment.h"

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::packer::tests {

BENCHMARK(AllChildrenYieldSameAffectedContainers) {
  const std::shared_ptr<const entities::Universe> universe =
      std::make_shared<const entities::Universe>();
  // when all child nodes affect the same set of containers, we do not expand
  // the parent but just add all affected containers of the parent to the
  // incremental priority queue. This Benchmark checks that indeed happens, for
  // if not, it will take forever to compute the hottest container ordering

  // each lookup affects all containers
  folly::BenchmarkSuspender suspend;
  const int lookupCount = 100E3;
  const Assignment assignment(
      {{container(1), {object(1)}},
       {container(2), {object(2)}},
       {container(3), {object(3)}}});
  auto allContainers = std::make_shared<PackerSet<entities::ContainerId>>(
      PackerSet<entities::ContainerId>{
          container(1), container(2), container(3)});
  ExprPtr obj = const_expr(0, universe);
  for (const auto _ : folly::irange(lookupCount)) {
    auto lookup = object_lookup(
        makeObjectVector(
            PackerMap<entities::ObjectId, double>{}, 1, 4, universe),
        allContainers,
        universe,
        assignment);
    inplace_add(obj, lookup, universe);
  }

  Context context;
  obj->fullApply(TopToBottomEvaluator(context), assignment);
  obj->init_unconstrained_bounds(context);

  auto objective = GlobalObjective::Builder{}
                       .addToObjective(0, std::move(obj), universe)
                       .build(universe);
  auto objectiveExpr = objective.getOnlyObjective();
  objectiveExpr->fullApply(TopToBottomEvaluator(context), assignment);
  objectiveExpr->init_unconstrained_bounds(context);
  suspend.dismiss();

  const DescendingExpressionContainersTraversal descending(objective.getView());
  const PackerSet<entities::ContainerId> descendingHottestContainers(
      descending.begin(), descending.end());
  EXPECT_EQ(descendingHottestContainers.size(), 3);
}

BENCHMARK(UpdateChildPotentialsOnlyIfRequired) {
  folly::BenchmarkSuspender suspend;
  const std::shared_ptr<const entities::Universe> universe =
      std::make_shared<const entities::Universe>();
  // This benchmark shows the usefulness of updating the child potentials only
  // if required (after a fullApply/partialApply), as opposed to recomputing
  // them always
  const int groupCount = 500e3;
  constexpr int containerCountAffectingGroup = 5;
  const int containerCount = groupCount * containerCountAffectingGroup;
  const int objectCount = 100e3;

  const Assignment emptyAssignment;

  auto objectVector = makeObjectVector(
      PackerMap<entities::ObjectId, double>{}, 1, objectCount, universe);

  auto obj = const_expr(0, universe);
  for (const auto i : folly::irange(groupCount)) {
    const int x = i * containerCountAffectingGroup;
    const int y = (i + 1) * containerCountAffectingGroup;
    PackerMap<ExprPtr, double> childToCoeff;
    for (int j = x; j < y; ++j) {
      auto lookup = object_lookup(
          objectVector,
          std::make_shared<PackerSet<entities::ContainerId>>(
              PackerSet<entities::ContainerId>{container(j)}),
          universe,
          emptyAssignment);
      childToCoeff[lookup] = 1;
    }
    inplace_add(
        obj, std::make_shared<LinearSum>(universe, 1, childToCoeff), universe);
  }

  // initially all objects are in container 0
  PackerMap<entities::ContainerId, std::vector<entities::ObjectId>>
      containerToObjects;
  for (const auto i : folly::irange(containerCount)) {
    containerToObjects[container(i)] = {};
  }
  for (const auto i : folly::irange(objectCount)) {
    containerToObjects[container(0)].push_back(object(i));
  }

  const Assignment assignment(containerToObjects);

  auto objective = GlobalObjective::Builder{}
                       .addToObjective(0, std::move(obj), universe)
                       .build(universe);

  Context context;
  auto objectiveExpr = objective.getOnlyObjective();
  objectiveExpr->fullApply(TopToBottomEvaluator(context), assignment);
  objectiveExpr->init_unconstrained_bounds(context);

  Orchestrator orchestrator;
  orchestrator.init(
      std::vector<Expression*>{objectiveExpr.get()},
      AffectedByChangeDecisionData(objectCount, containerCount));
  suspend.dismiss();

  constexpr int nMoves = 500;
  // perform 'nMoves'; for nMoves = 500, hottest container during each move
  // will be container 0 and the move will be container1
  for (const auto i : folly::irange(nMoves)) {
    const DescendingExpressionContainersTraversal descending(
        objective.getView(), true /*skipOptimalExpressions*/);

    auto iterator = descending.begin();
    auto worstContainer = *iterator;
    context.clear();

    // make a move, where object i is moved from the worst container
    auto changes = std::vector<Change>{
        Change(object(i), worstContainer, -1),
        Change(
            object(i),
            container((worstContainer.asInt() + 1) % containerCount),
            1)};
    ChangeSet changeSet(changes);

    context.changes() = std::move(changeSet);

    orchestrator.apply(context, assignment);
  }
  suspend.rehire();
}

BENCHMARK(RefreshDescendingChildPotentialsAfterFewChildChanges) {
  folly::BenchmarkSuspender suspend;
  const std::shared_ptr<const entities::Universe> universe =
      std::make_shared<const entities::Universe>();
  // Flat LinearSum with `childCount` single-container children. Each move
  // changes two children's values. Only time to refresh child potentials
  // refresh is timed
  constexpr int childCount = 100'000;
  constexpr int objectCount = 20'000;

  const Assignment emptyAssignment;
  auto objectVector = makeObjectVector(
      PackerMap<entities::ObjectId, double>{}, 1, objectCount, universe);

  PackerMap<ExprPtr, double> childToCoeff;
  for (const auto i : folly::irange(childCount)) {
    childToCoeff[object_lookup(
        objectVector,
        std::make_shared<PackerSet<entities::ContainerId>>(
            PackerSet<entities::ContainerId>{container(i)}),
        universe,
        emptyAssignment)] = 1;
  }
  auto sum = std::make_shared<LinearSum>(universe, 0, childToCoeff);

  // All objects start in container(0).
  PackerMap<entities::ContainerId, std::vector<entities::ObjectId>>
      containerToObjects;
  for (const auto i : folly::irange(childCount)) {
    containerToObjects[container(i)] = {};
  }
  for (const auto i : folly::irange(objectCount)) {
    containerToObjects[container(0)].push_back(object(i));
  }
  const Assignment assignment(containerToObjects);

  Context context;
  sum->fullApply(TopToBottomEvaluator(context), assignment);
  sum->init_unconstrained_bounds(context);

  Orchestrator orchestrator;
  orchestrator.init(
      std::vector<Expression*>{sum.get()},
      AffectedByChangeDecisionData(objectCount, childCount));

  // Pre-build the cache so the first timed iteration measures a refresh,
  // not an initial full build.
  sum->getDescendingChildPotentials();

  constexpr int nMoves = 500;
  for (const auto i : folly::irange(nMoves)) {
    context.clear();
    context.changes() = ChangeSet(
        {Change(object(i), container(0), -1),
         Change(object(i), container(1), 1)});
    orchestrator.apply(context, assignment);

    suspend.dismiss();
    sum->getDescendingChildPotentials();
    suspend.rehire();
  }
}

BENCHMARK(PruneOptimalSubgraph) {
  const std::shared_ptr<const entities::Universe> universe =
      std::make_shared<const entities::Universe>();
  // This benchmark shows the usefulness of pruning away optimal subgraphs when
  // doing preorder traversal
  folly::BenchmarkSuspender suspend;
  const int nEmptyContainers = 100e3;
  const int objectCount = 100e3;
  const int containerCount = nEmptyContainers +
      2; // all containers expect containers 0 and 1 are always emoty

  auto objectVector = makeObjectVector(
      PackerMap<entities::ObjectId, double>{}, 1, objectCount, universe);
  const Assignment emptyAssignment;

  auto obj0 = const_expr(0, universe);
  for (const auto i : folly::irange(nEmptyContainers)) {
    PackerMap<ExprPtr, double> childToCoeff;
    auto lookup = object_lookup(
        objectVector,
        std::make_shared<PackerSet<entities::ContainerId>>(
            // note that there is no lookup w.r.t. containers 0 and 1
            PackerSet<entities::ContainerId>{container(i + 2)}),
        universe,
        emptyAssignment);
    childToCoeff[lookup] = 1;
    inplace_add(
        obj0, std::make_shared<LinearSum>(universe, 1, childToCoeff), universe);
  }

  // incentive to empty container0
  auto obj1 = object_lookup(
      objectVector,
      std::make_shared<PackerSet<entities::ContainerId>>(
          PackerSet<entities::ContainerId>{container(0)}),
      universe,
      emptyAssignment);

  // initially all objects are in container 0
  PackerMap<entities::ContainerId, std::vector<entities::ObjectId>>
      containerToObjects;
  for (const auto i : folly::irange(containerCount)) {
    containerToObjects[container(i)] = {};
  }
  for (const auto i : folly::irange(objectCount)) {
    containerToObjects[container(0)].push_back(object(i));
  }
  const Assignment assignment(containerToObjects);

  auto objective = GlobalObjective::Builder{}
                       .addToObjective(0, std::move(obj0), universe)
                       .addToObjective(1, obj1, universe)
                       .build(universe);

  Context context;
  objective.fullApply(TopToBottomEvaluator(context), assignment);
  objective.initUnconstrainedBounds();

  std::vector<Expression*> allObjExprs;
  for (const auto& obj : objective) {
    allObjExprs.push_back(obj.get());
  }

  Orchestrator orchestrator;
  orchestrator.init(
      std::move(allObjExprs),
      AffectedByChangeDecisionData(objectCount, containerCount));
  suspend.dismiss();

  constexpr int nMoves = 50e3;
  // perform 'nMoves'; each move will be from container 0
  interface::HottestTraversalConfig traversalConfig;
  traversalConfig.pruneOptimalSubgraphs() = true;
  for (const auto i : folly::irange(nMoves)) {
    const DescendingExpressionContainersTraversal descending(
        objective.getView(), /*skipOptimalExpressions=*/true, traversalConfig);

    auto iterator = descending.begin();
    auto worstContainer = *iterator;

    folly::BenchmarkSuspender suspend1;
    // make a move, where object i is moved from the worst container (expected
    // to be container(0)) to container(1)
    ASSERT_EQ(worstContainer, container(0));
    context.clear();
    auto changes = std::vector<Change>{
        Change(object(i), worstContainer, -1),
        Change(object(i), container(1), 1)};
    ChangeSet changeSet(changes);
    context.changes() = std::move(changeSet);
    orchestrator.apply(context, assignment);
    suspend1.dismiss();
  }
}

} // namespace facebook::rebalancer::packer::tests

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  folly::runBenchmarks();

  return 0;
}
