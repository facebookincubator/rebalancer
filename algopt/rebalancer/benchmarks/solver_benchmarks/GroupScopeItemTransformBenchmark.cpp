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

#include "algopt/rebalancer/entities/tests/UniverseBuilderTestUtils.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"
#include "algopt/rebalancer/solver/expressions/GroupScopeItemTransformUtil.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"
#include "algopt/rebalancer/solver/expressions/Orchestrator.h"
#include "algopt/rebalancer/solver/expressions/TopToBottomEvaluator.h"
#include "algopt/rebalancer/solver/utils/Assignment.h"
#include "algopt/rebalancer/solver/utils/ChangeSet.h"

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/coro/BlockingWait.h>
#include <folly/init/Init.h>
#include <folly/Random.h>

using namespace facebook::rebalancer;

namespace {

struct BenchmarkUniverse : public entities::tests::UniverseBuilderTestUtils {
  std::shared_ptr<const entities::Universe> buildWithScopeAndPartition(
      int objectCount,
      int containerCount,
      int groupCount) {
    entities::Map<std::string, std::vector<std::string>> initialAssignment;
    for (const auto i : folly::irange(containerCount)) {
      initialAssignment[fmt::format("container{}", i)] = {};
    }
    for (const auto i : folly::irange(objectCount)) {
      initialAssignment[fmt::format("container{}", i % containerCount)]
          .push_back(fmt::format("object{}", i));
    }
    setInitialAssignment(initialAssignment);

    entities::Map<std::string, std::vector<std::string>> scopeItemToContainers;
    for (const auto i : folly::irange(containerCount)) {
      scopeItemToContainers[fmt::format("scopeItem{}", i)] = {
          fmt::format("container{}", i)};
    }
    folly::coro::blockingWait(addScope("scope0", scopeItemToContainers));

    entities::Map<std::string, std::vector<std::string>> groupToObjects;
    for (const auto i : folly::irange(objectCount)) {
      groupToObjects[fmt::format("group{}", i % groupCount)].push_back(
          fmt::format("object{}", i));
    }
    folly::coro::blockingWait(addPartition("part0", groupToObjects));

    entities::Map<std::string, double> objectToValue;
    for (const auto i : folly::irange(objectCount)) {
      objectToValue[fmt::format("object{}", i)] = 1.0;
    }
    folly::coro::blockingWait(addObjectDimension("dim0", objectToValue, 0.0));

    return buildUniverse();
  }
};

entities::ObjectId object(
    int i,
    const std::shared_ptr<const entities::Universe>& u) {
  return u->getObjectId(fmt::format("object{}", i));
}

entities::ContainerId container(
    int i,
    const std::shared_ptr<const entities::Universe>& u) {
  return u->getContainerId(fmt::format("container{}", i));
}

ChangeSet makeSingleMoveChangeSet(
    entities::ObjectId objectId,
    entities::ContainerId src,
    entities::ContainerId dst) {
  return ChangeSet({Change(objectId, src, -1), Change(objectId, dst, 1)});
}

double
evaluateExpr(Orchestrator& orchestrator, Expression* expr, Context& context) {
  orchestrator.evaluate(expr, context);
  const auto val = context.val().get(expr->getId());
  return val.has_value() ? val.value() : expr->value;
}

double partialApplyExpr(
    Orchestrator& orchestrator,
    Expression* expr,
    const Assignment& assignment,
    Context& context) {
  orchestrator.apply(context, assignment);
  const auto val = context.apply().get(expr->getId());
  return val.has_value() ? val.value() : expr->value;
}

// Builds a GroupScopeItemTransformUtil expression tree with STEP transform and
// initializes an Orchestrator, ready for evaluate/partialApply benchmarking.
struct BenchmarkFixture {
  std::shared_ptr<const entities::Universe> universe;
  Assignment assignment;
  ExprPtr root;
  Context context;
  Orchestrator orchestrator;

  BenchmarkFixture(int objectCount, int containerCount, int groupCount) {
    BenchmarkUniverse builder;
    universe = builder.buildWithScopeAndPartition(
        objectCount, containerCount, groupCount);

    assignment = Assignment(universe->getContainers().getInitialAssignment());

    const auto partId = builder.partitionId("part0");
    const auto dimId = builder.dimensionId("dim0");
    const auto scopeId = builder.scopeId("scope0");

    std::vector<entities::ScopeItemId> scopeItems;
    for (const auto i : folly::irange(containerCount)) {
      scopeItems.push_back(
          builder.scopeItemId(scopeId, fmt::format("scopeItem{}", i)));
    }

    auto containers = std::make_shared<entities::Set<entities::ContainerId>>();
    for (const auto i : folly::irange(containerCount)) {
      containers->insert(container(i, universe));
    }

    root = const_expr(0, universe);
    for (const auto g : folly::irange(groupCount)) {
      const auto gId = builder.groupId(partId, fmt::format("group{}", g));
      auto expr = std::make_shared<GroupScopeItemTransformUtil>(
          universe,
          partId,
          gId,
          dimId,
          scopeId,
          scopeItems,
          containers,
          assignment,
          folly::F14FastMap<entities::ScopeItemId, double>{},
          1.0,
          GroupScopeItemTransformUtil::TransformFunctionType::STEP);
      inplace_add(root, expr, universe);
    }

    root->fullApply(TopToBottomEvaluator(context), assignment);

    orchestrator.init(
        std::vector<Expression*>{root.get()},
        AffectedByChangeDecisionData(objectCount, containerCount));
  }
};

} // namespace

BENCHMARK(GroupScopeItemTransformEvaluate) {
  // Measures evaluate() throughput for GroupScopeItemTransformUtil with STEP
  // transform — the hot path for group diversity constraint evaluation during
  // local search.
  folly::BenchmarkSuspender suspend;
  constexpr int containerCount = 100;
  constexpr int objectCount = 10000;
  constexpr int groupCount = 500;
  BenchmarkFixture fixture(objectCount, containerCount, groupCount);
  folly::Random::DefaultGenerator rng(0 /* seed */);

  constexpr int nEvals = 200000;
  suspend.dismiss();

  for ([[maybe_unused]] const auto _ : folly::irange(nEvals)) {
    const auto randObj = folly::Random::rand32(rng) % objectCount;
    const auto srcCon = randObj % containerCount;
    const auto dstCon = (randObj + 1) % containerCount;
    auto changeSet = makeSingleMoveChangeSet(
        object(randObj, fixture.universe),
        container(srcCon, fixture.universe),
        container(dstCon, fixture.universe));

    fixture.context.clear();
    fixture.context.changes() = changeSet;
    evaluateExpr(fixture.orchestrator, fixture.root.get(), fixture.context);
  }
}

BENCHMARK(GroupScopeItemTransformPartialApply) {
  // Measures partialApply() throughput for GroupScopeItemTransformUtil with
  // STEP transform. partialApply commits the move into the assignment,
  // benchmarking the incremental update path.
  folly::BenchmarkSuspender suspend;
  constexpr int containerCount = 100;
  constexpr int objectCount = 10000;
  constexpr int groupCount = 500;
  BenchmarkFixture fixture(objectCount, containerCount, groupCount);
  folly::Random::DefaultGenerator rng(0 /* seed */);

  constexpr int nMoves = 100000;
  suspend.dismiss();

  for ([[maybe_unused]] const auto _ : folly::irange(nMoves)) {
    const auto randObj = folly::Random::rand32(rng) % objectCount;
    const auto srcCon = randObj % containerCount;
    const auto dstCon = (randObj + 1) % containerCount;
    auto changeSet = makeSingleMoveChangeSet(
        object(randObj, fixture.universe),
        container(srcCon, fixture.universe),
        container(dstCon, fixture.universe));

    fixture.context.clear();
    fixture.context.changes() = changeSet;
    partialApplyExpr(
        fixture.orchestrator,
        fixture.root.get(),
        fixture.assignment,
        fixture.context);
  }
}

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);
  folly::runBenchmarks();
  return 0;
}
