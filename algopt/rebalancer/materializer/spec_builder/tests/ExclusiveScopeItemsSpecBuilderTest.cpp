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

#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/materializer/spec_builder/ExclusiveScopeItemsSpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {

class ExclusiveScopeItemsSpecBuilderTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse(
        {{"host1", {}},
         {"host2", {"task0", "task1", "task2", "task3"}},
         {"host3", {}}});

    co_await addPartition(
        "job", {{"job0", {"task0", "task1"}}, {"job1", {"task2", "task3"}}});

    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }

  static std::vector<interface::ScopeItemConflictInfo> makeConflictInfoList(
      const std::vector<std::pair<std::string, std::vector<std::string>>>&
          scopeItems);
};

std::vector<interface::ScopeItemConflictInfo>
ExclusiveScopeItemsSpecBuilderTest::makeConflictInfoList(
    const std::vector<std::pair<std::string, std::vector<std::string>>>&
        scopeItems) {
  std::vector<interface::ScopeItemConflictInfo> result;
  for (auto& [scopeItem, conflictingScopeItems] : scopeItems) {
    auto info = interface::ScopeItemConflictInfo();
    info.scopeItem() = scopeItem;
    std::vector<interface::ConflictingScopeItemInfo>
        conflictingScopeItemInfoList;
    for (auto& conflictingScopeItem : conflictingScopeItems) {
      auto conflictingScopeItemInfo = interface::ConflictingScopeItemInfo();
      conflictingScopeItemInfo.conflictingScopeItem() = conflictingScopeItem;
      conflictingScopeItemInfoList.push_back(
          std::move(conflictingScopeItemInfo));
    }
    info.conflictingScopeItemsWithOverlap() = conflictingScopeItemInfoList;
    result.push_back(std::move(info));
  }
  return result;
}

CO_TEST_F(ExclusiveScopeItemsSpecBuilderTest, Constraint) {
  std::vector<interface::ScopeItemConflictInfo> conflictInfoList =
      makeConflictInfoList({{"host1", {"host2"}}, {"host2", {"host3"}}});
  auto spec = interface::ExclusiveScopeItemsSpec();
  spec.name() = "test";
  spec.scope() = "host";
  spec.dimension() = "task_count";
  spec.conflictInfoList() = std::move(conflictInfoList);
  const ExclusiveScopeItemsSpecBuilder specBuilder(buildUniverse(), spec);

  EXPECT_EQ(
      "Spec test : specified conflicts of scope items cannot be utilized concurrently",
      specBuilder.description());

  const auto constraints =
      co_await specBuilder.constraints(expressionBuilder());
  // value will be 0 as only host2 has tasks
  EXPECT_NEAR(0, evaluate(constraints.at(0), deltaFromInitial({})), 1e-8);
  // value will be 1 as host1 and host2 both have tasks
  EXPECT_NEAR(
      1,
      evaluate(constraints.at(0), deltaFromInitial({{"task1", "host1"}})),
      1e-8);
  // value will be 0 as only one host in pair has task
  EXPECT_NEAR(
      0,
      evaluate(
          constraints.at(0),
          deltaFromInitial(
              {{"task0", "host1"},
               {"task1", "host1"},
               {"task2", "host3"},
               {"task3", "host3"}})),
      1e-8);
}

CO_TEST_F(ExclusiveScopeItemsSpecBuilderTest, ConstraintPerGroup) {
  std::vector<interface::ScopeItemConflictInfo> conflictInfoList =
      makeConflictInfoList({{"host1", {"host2"}}, {"host2", {"host3"}}});

  auto spec = interface::ExclusiveScopeItemsSpec();
  spec.name() = "test";
  spec.scope() = "host";
  spec.dimension() = "task_count";
  spec.conflictInfoList() = std::move(conflictInfoList);
  spec.partitionName() = "job";
  const ExclusiveScopeItemsSpecBuilder specBuilder(buildUniverse(), spec);

  EXPECT_EQ(
      "Spec test : specified conflicts of scope items cannot be utilized concurrently "
      "by objects that belong to same group of partition job",
      specBuilder.description());

  const auto constraints =
      co_await specBuilder.constraints(expressionBuilder());
  // one constraint per group
  EXPECT_EQ(2, constraints.size());
  auto violation =
      constraints.at(0).constraintExpr + constraints.at(1).constraintExpr;

  // all tasks are in one host initially
  EXPECT_NEAR(0, evaluate(violation, deltaFromInitial({})), 1e-8);

  // job0 is host2 and job1 is host3, this is ok because for a given group
  // conflicting hosts are not used
  EXPECT_NEAR(
      0,
      evaluate(
          violation,
          deltaFromInitial({{"task2", "host3"}, {"task3", "host3"}})),
      1e-8);

  // job0 is host2 and job1 is in host1 and host3, this is also ok because for a
  // given group conflicting hosts are not used
  EXPECT_NEAR(
      0,
      evaluate(
          violation,
          deltaFromInitial({{"task2", "host1"}, {"task3", "host3"}})),
      1e-8);

  // job0 and job1 are split between host1 and host3, this is also ok because
  // for a given group conflicting hosts are not used
  EXPECT_NEAR(
      0,
      evaluate(
          violation,
          deltaFromInitial(
              {{"task0", "host1"},
               {"task1", "host3"},
               {"task2", "host1"},
               {"task3", "host3"}})),
      1e-8);
  // job1 is split between host2 and host3, this is NOT ok because
  // for a given group conflicting hosts are used
  EXPECT_NEAR(
      1, evaluate(violation, deltaFromInitial({{"task3", "host3"}})), 1e-8)
      << digest(violation, {{"task3", "host3"}});
  // job0 is split between host1 and host2, this is also NOT ok because
  // for a given group conflicting hosts are used
  EXPECT_NEAR(
      1, evaluate(violation, deltaFromInitial({{"task0", "host1"}})), 1e-8)
      << digest(violation, {{"task0", "host1"}});
  // job0, job1 are split between hosts, this is also NOT ok because
  // for a both groups conflicting hosts are used
  EXPECT_NEAR(
      2,
      evaluate(
          violation,
          deltaFromInitial({{"task0", "host1"}, {"task3", "host3"}})),
      1e-8)
      << digest(violation, {{"task0", "host1"}, {"task3", "host3"}});
}

CO_TEST_F(
    ExclusiveScopeItemsSpecBuilderTest,
    EmptyConflictingScopeItemsConstraints) {
  std::vector<interface::ScopeItemConflictInfo> conflictInfoList =
      makeConflictInfoList({{"host1", {}}, {"host2", {}}});

  auto spec = interface::ExclusiveScopeItemsSpec();
  spec.name() = "test";
  spec.scope() = "host";
  spec.dimension() = "task_count";
  spec.conflictInfoList() = std::move(conflictInfoList);
  const ExclusiveScopeItemsSpecBuilder specBuilder(buildUniverse(), spec);

  const auto constraints =
      co_await specBuilder.constraints(expressionBuilder());
  // no constraints as there are no conflicting scope items
  EXPECT_EQ(0, constraints.size());
}

CO_TEST_F(
    ExclusiveScopeItemsSpecBuilderTest,
    EmptyConflictingScopeItemsConstraintsPerGroup) {
  auto spec = interface::ExclusiveScopeItemsSpec();
  spec.name() = "test";
  spec.scope() = "host";
  spec.dimension() = "task_count";
  spec.conflictInfoList() =
      makeConflictInfoList({{"host1", {}}, {"host2", {}}});
  spec.partitionName() = "job";
  const ExclusiveScopeItemsSpecBuilder specBuilder(buildUniverse(), spec);

  const auto constraints =
      co_await specBuilder.constraints(expressionBuilder());
  // no constraints as there are no conflicting scope items
  EXPECT_EQ(0, constraints.size());
}

TEST_F(ExclusiveScopeItemsSpecBuilderTest, SpecInfo) {
  interface::ExclusiveScopeItemsSpec spec;
  spec.name() = "test";
  spec.scope() = "host";
  spec.conflictInfoList() = makeConflictInfoList({{"host1", {"host2"}}});
  spec.dimension() = "task_count";

  const ExclusiveScopeItemsSpecBuilder specBuilder(buildUniverse(), spec);

  const auto expectedSpecInfo = SpecParameters{
      .name = "test", .scope = "host", .dimension = "task_count", .size = 1};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());
}
} // namespace facebook::rebalancer::materializer::tests
