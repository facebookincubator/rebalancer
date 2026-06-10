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

#include "algopt/rebalancer/materializer/spec_builder/DisasterRecoveryCapacitySpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

#include <memory>

namespace facebook::rebalancer::materializer::tests {

class DisasterRecoveryCapacitySpecBuilderTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse({
        {"host0", {"task0", "task1", "task2", "task3"}},
        {"host1", {"task4", "task5", "task6", "task7"}},
        {"host2", {}},
    });

    co_await addObjectDimension(
        "cpu",
        {
            {task(0), 1},
            {task(1), 2},
            {task(2), 5},
            {task(3), 10},
            {task(4), 1},
            {task(5), 2},
            {task(6), 5},
            {task(7), 10},
        },
        0);

    co_await addScopeDimension(
        "cpu",
        scopeId("host"),
        {
            {"host0", 2},
            {"host1", 4},
            {"host2", 0},
        },
        0);

    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }

  entities::ObjectId task(int index) const {
    return objectId(fmt::format("task{}", index));
  }
};

TEST_F(DisasterRecoveryCapacitySpecBuilderTest, CheckDescription) {
  interface::DisasterRecoveryCapacitySpec spec;
  spec.scope() = "host";
  spec.dimension() = "cpu";
  spec.sharedDisasterGroups() = {};

  const DisasterRecoveryCapacitySpecBuilder specBuilder(buildUniverse(), spec);
  EXPECT_EQ(
      "Disaster recovery usage for scope = host and dimension = cpu",
      specBuilder.description());
}

TEST_F(DisasterRecoveryCapacitySpecBuilderTest, SpecInfo) {
  interface::DisasterRecoveryCapacitySpec spec;
  spec.scope() = "host";
  spec.dimension() = "cpu";
  spec.sharedDisasterGroups() = {};

  const DisasterRecoveryCapacitySpecBuilder specBuilder(buildUniverse(), spec);

  auto expectedSpecInfo =
      SpecParameters{.name = "", .scope = "host", .dimension = "cpu"};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());
}

CO_TEST_F(DisasterRecoveryCapacitySpecBuilderTest, OnlyPrimary) {
  interface::DisasterRecoveryCapacitySpec spec;
  spec.scope() = "host";
  spec.dimension() = "cpu";
  spec.primaryToSetOfSecondaryObjects() = {
      {"task0", {"task1"}},
      {"task2", {"task3"}},
      {"task4", {"task5"}},
      {"task6", {"task7"}},
  };

  const DisasterRecoveryCapacitySpecBuilder specBuilder(buildUniverse(), spec);

  auto goal = co_await specBuilder.goalCoro(expressionBuilder());
  /*
  This test only checks for primaryUsage calculations.

  host0: PrimaryUsage = 1 (from task0) + 5 (task2) = 6
        DisasterRecoveryUsage: 0 (because if host1 is down, there is no task
        in
  host1 that can act secondary task of any task in host1)

  host1: PrimaryUsage = 1 (from task4) + 5 (task6) = 6
        DisasterRecoveryUsage: 0 (because if host0 is down, there is no task
        in
  host1 that can act secondary task of any task in host1)

  Note that the expression w.r.t. to a host is normalized by the average "cpu"
  values of hosts (which is 2)
  Goal value = (6 - 2)/2 + (6 - 4)/2 = 3
  */

  EXPECT_NEAR(3.0, evaluate(goal, deltaFromInitial({})), 1e-8);
}

CO_TEST_F(DisasterRecoveryCapacitySpecBuilderTest, DisasterGroupAndDeltaTest) {
  interface::DisasterRecoveryCapacitySpec spec;
  spec.scope() = "host";
  spec.dimension() = "cpu";
  spec.primaryToSetOfSecondaryObjects() = {
      {"task0", {"task4"}},
      {"task1", {"task5"}},
      {"task2", {"task6"}},
      {"task3", {"task7"}},
  };

  const auto universe = buildUniverse();
  auto& exprBuilder = expressionBuilder();
  const DisasterRecoveryCapacitySpecBuilder specBuilder(universe, spec);

  auto goal = co_await specBuilder.goalCoro(exprBuilder);

  /*
  Initially, we have:
  host0: PrimaryUsage = 1 (from task0) + 2 (task1) + 5 (task2) + 10 (task3) =
  18
        DisasterRecoveryUsage: 0 (because if host1 is down, there is no task
        in
  host0 that can act secondary task of any task in host1)

  host1: PrimaryUsage = 0 (none of tasks in host1 are 'primary')
        DisasterRecoveryUsage: 18 (because if host0 is down, then all tasks
        in
  host0 have a secondary task in host1)

  Note that the expression w.r.t. to a host is normalized by the average "cpu"
  values of hosts (which is 2)
  Goal_value = (18 - 2)/2 + (18 - 4)/2 = 15
  */
  EXPECT_NEAR(15.0, evaluate(goal, deltaFromInitial({})), 1e-8);

  /*
    If task3 is in host2, then:

    host0: PrimaryUsage = 1 (from task0) + 2 (task1) + 5 (task2) = 8
    DisasterRecoveryUsage: 0 (because if host1/host2 is down, there is no
    task in host0 that can act secondary task of any task in host1)

    host1: PrimaryUsage = 0 (none of tasks in host1 are 'primary')
          DisasterRecoveryUsage:
                if host0 is down: 1 + 2 + 5 = 8
                if host2 is down: 10

                Therefore, DisasterRecoveryUsage = Max(8, 10) = 10

    host2: PrimaryUsage = 10 (from task3)
    DisasterRecoveryUsage: 0 (because if host0/host1 is down, there is no
    task in host2 that can act secondary task of any task in host0/host1)

    Goal_value = (8 - 2)/2 + (10 - 4)/2 + (10 - 0)/2 = 10
  */
  EXPECT_NEAR(
      11.0, evaluate(goal, deltaFromInitial({{"task3", "host2"}})), 1e-8);

  /*
  Now, if we make {host0, host2} as a disaster group and have task3 assigned
  to host2, then the only change from above is that DisasterRecoveryUsage of
  host1 is now 10 + 8 (instead of Max(10, 8) as in the previous case). So,
  goal_value = (10 - 2)/2 + (18 - 4)/2 + (8 - 0)/2 = 15.
  */
  spec.sharedDisasterGroups() = {{"host0", "host2"}};
  const DisasterRecoveryCapacitySpecBuilder specWithDisasterGroupsBuilder(
      universe, spec);
  auto goalWithDisasterGroups =
      co_await specWithDisasterGroupsBuilder.goalCoro(exprBuilder);

  EXPECT_NEAR(
      15.0,
      evaluate(goalWithDisasterGroups, deltaFromInitial({{"task3", "host2"}})),
      1e-8);
}

class DisasterRecoveryCapacitySpecBuilderMultiDimTest
    : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse({
        {"host0", {"r1_p", "r5_s"}},
        {"host1", {"r2_p"}},
        {"host2", {"r3_p", "r1_s", "r5_p"}},
        {"host3", {"r4_p"}},
        {"host4", {"r4_s", "r2_s", "r3_s"}},
    });

    // Multi-value dimension with 4 time periods
    const auto numObjects = getNumObjects();
    std::vector<entities::ObjectIdToDoubleMap> loadMaps;
    loadMaps.reserve(4);
    loadMaps.emplace_back(
        entities::Map<entities::ObjectId, double>{
            {objectId("r1_p"), 1},
            {objectId("r1_s"), 1},
            {objectId("r2_p"), 2.3},
            {objectId("r2_s"), 2.3},
            {objectId("r3_p"), 0.4},
            {objectId("r3_s"), 0.4},
            {objectId("r4_p"), 1.4},
            {objectId("r4_s"), 1.4},
            {objectId("r5_p"), 0.5},
            {objectId("r5_s"), 0.5}},
        /*defaultValue=*/0.0,
        /*totalSize=*/numObjects);
    loadMaps.emplace_back(
        entities::Map<entities::ObjectId, double>{
            {objectId("r1_p"), 1.1},
            {objectId("r1_s"), 1.1},
            {objectId("r2_p"), 2.2},
            {objectId("r2_s"), 2.2},
            {objectId("r3_p"), 0.2},
            {objectId("r3_s"), 0.2},
            {objectId("r4_p"), 1.8},
            {objectId("r4_s"), 1.8},
            {objectId("r5_p"), 0.5},
            {objectId("r5_s"), 0.5}},
        /*defaultValue=*/0.0,
        /*totalSize=*/numObjects);
    loadMaps.emplace_back(
        entities::Map<entities::ObjectId, double>{
            {objectId("r1_p"), 1.2},
            {objectId("r1_s"), 1.2},
            {objectId("r2_p"), 2.1},
            {objectId("r2_s"), 2.1},
            {objectId("r3_p"), 0.3},
            {objectId("r3_s"), 0.3},
            {objectId("r4_p"), 1.3},
            {objectId("r4_s"), 1.3},
            {objectId("r5_p"), 0.5},
            {objectId("r5_s"), 0.5}},
        /*defaultValue=*/0.0,
        /*totalSize=*/numObjects);
    loadMaps.emplace_back(
        entities::Map<entities::ObjectId, double>{
            {objectId("r1_p"), 1.3},
            {objectId("r1_s"), 1.3},
            {objectId("r2_p"), 2},
            {objectId("r2_s"), 2},
            {objectId("r3_p"), 0.5},
            {objectId("r3_s"), 0.5},
            {objectId("r4_p"), 1.5},
            {objectId("r4_s"), 1.5},
            {objectId("r5_p"), 0.5},
            {objectId("r5_s"), 0.5}},
        /*defaultValue=*/0.0,
        /*totalSize=*/numObjects);
    co_await universeBuilder_.addObjectDimension(
        universeBuilder_.makeObjectDimensionId("load"),
        entities::ObjectDimensionData{
            .dimension = std::make_unique<const entities::ObjectDimension>(
                std::move(loadMaps))});

    co_await addScopeDimension(
        "load", scopeId("host"), {{"host0", 0}, {"host1", 0}, {"host2", 0}}, 0);

    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }
};

CO_TEST_F(
    DisasterRecoveryCapacitySpecBuilderMultiDimTest,
    MultiValueDimension) {
  interface::DisasterRecoveryCapacitySpec spec;
  spec.scope() = "host";
  spec.dimension() = "load";

  spec.primaryToSetOfSecondaryObjects() = {
      {"r1_p", {"r1_s"}},
      {"r2_p", {"r2_s"}},
      {"r3_p", {"r3_s"}},
      {"r4_p", {"r4_s"}},
      {"r5_p", {"r5_s"}},
  };

  spec.sharedDisasterGroups() = {
      {"host0", "host1"},
      {"host1", "host2", "host3"},
  };

  const DisasterRecoveryCapacitySpecBuilder specBuilder(buildUniverse(), spec);
  auto goal = co_await specBuilder.goalCoro(expressionBuilder());

  /*
    For each host, we want to compute PrimaryUsage (usage from tasks of the
    form "_p") and DisasterRecoveryUsage (added usage due to disasters). Note
    that each is a vector of length 4, since there are 4 values for "load"

    host0:
            PrimaryUsage = {1, 1.1, 1.2, 1.3}

            DisasterRecoveryUsage:
             when ("host4") is down: {0, 0, 0, 0}
             when ("host1","host2", "host3") is down = {0.5, 0.5, 0.5, 0.5}

             maxDisasterRecoveryUsage = {0.5, 0.5, 0.5, 0.5}

            TotalUsage = {1.5, 1.6, 1.7, 1.8}

    host1:
            PrimaryUsage = {2.3, 2.2, 2.1, 2}

            DisasterRecoveryUsage:
             when ("host4") is down: {0, 0, 0, 0}

             maxDisasterRecoveryUsage = {0, 0, 0, 0}

            TotalUsage = {2.3, 2.2, 2.1, 2}

    host2:
            PrimaryUsage = {0.4, 0.2, 0.3, 0.5} + {0.5, 0.5, 0.5, 0.5}
                         = {0.9, 0.7, 0.8, 1.0} (for r3_p and r5_p)

            DisasterRecoveryUsage:
             when ("host4") is down: {0, 0, 0, 0}
             when ("host0", "host1") is down: {1, 1.1, 1.2, 1.3}

             maxDisasterRecoveryUsage = {1, 1.1, 1.2, 1.3}

            TotalUsage = {1.9, 1.8, 2.0, 2.3}

    host3:
            PrimaryUsage = {1.4, 1.8, 1.3, 1.5}

            DisasterRecoveryUsage:
             when ("host4") is down: {0, 0, 0, 0}
             when ("host0", "host1") is down: {0, 0, 0, 0}

             maxDisasterRecoveryUsage = {0, 0, 0, 0}

            TotalUsage = {1.4, 1.8, 1.3, 1.5}

    host4:
            PrimaryUsage = {0, 0, 0, 0}

            DisasterRecoveryUsage:
             when ("host0", "host1") is down: {2.3, 2.2, 2.1, 2}
             when ("host1","host2", "host3") is down:
             {2.3, 2.2, 2.1, 2} +
             {0.4, 0.2, 0.3, 0.5} +
             {1.4, 1.8, 1.3, 1.5}

             = {4.1, 4.2, 3.7, 4.0}

             maxDisasterRecoveryUsage = {4.1, 4.2, 3.7, 4.0}

            TotalUsage = {4.1, 4.2, 3.7, 4.0}

    Goal_value = Sum of peak usages = 1.8 + 2.3 + 2.3 + 1.8 + 4.2 = 12.4
  */

  EXPECT_NEAR(12.4, evaluate(goal, deltaFromInitial({})), 1e-8);
}

class DisasterRecoveryMultipleSecondaryTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    // define initial assignment
    initialAssignment = {
        {"host0", {"r1_p", "r4_s1", "r3_s2"}},
        {"host1", {"r2_p", "r1_s2", "r4_s2", "r2_s2"}},
        {"host2", {"r3_p", "r1_s1", "r5_s1", "r4_s3"}},
        {"host3", {"r4_p", "r1_s3", "r2_s1"}},
        {"host4", {"r5_p", "r3_s1", "r2_s3"}},
    };

    setUpUniverse(initialAssignment);

    // map objects to their load values
    entities::Map<std::string, double> objectToLoad;
    objectToLoad = {
        {"r1_p", 15},
        {"r2_p", 25},
        {"r3_p", 10},
        {"r4_p", 20},
        {"r5_p", 5},
    };

    // primary object can have multiple secondary objects
    primaryToSetOfSecondaryObjects = {
        {"r1_p", {"r1_s1", "r1_s2", "r1_s3"}},
        {"r2_p", {"r2_s1", "r2_s2", "r2_s3"}},
        {"r3_p", {"r3_s1", "r3_s2"}},
        {"r4_p", {"r4_s1", "r4_s2", "r4_s3"}},
        {"r5_p", {"r5_s1"}},
    };

    // for this example, we assume that all the secondary objects of a
    // primary object have the same "load" value as the primary object
    entities::Map<entities::ObjectId, double> objectIdToLoad;
    for (auto& [primaryObject, secondaryObjects] :
         primaryToSetOfSecondaryObjects) {
      objectIdToLoad[objectId(primaryObject)] = objectToLoad.at(primaryObject);
      for (auto& obj : secondaryObjects) {
        objectIdToLoad[objectId(obj)] =
            objectIdToLoad.at(objectId(primaryObject));
      }
    }

    co_await addObjectDimension("load", objectIdToLoad, 0);

    // all the hosts have 0 load value
    co_await addScopeDimension(
        "load", scopeId("host"), entities::Map<std::string, double>(), 0);

    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }

  entities::Map<std::string, std::vector<std::string>> initialAssignment;
  std::map<std::string, std::vector<std::string>>
      primaryToSetOfSecondaryObjects;
};

CO_TEST_F(DisasterRecoveryMultipleSecondaryTest, MultipleSecondaryObjects) {
  interface::DisasterRecoveryCapacitySpec spec;
  spec.scope() = "host";
  spec.dimension() = "load";
  spec.primaryToSetOfSecondaryObjects() = primaryToSetOfSecondaryObjects;
  spec.sharedDisasterGroups() = {
      {"host0", "host1"},
      {"host1", "host2", "host3"},
      {"host0", "host3"},
      {"host4"},
  };

  const DisasterRecoveryCapacitySpecBuilder specBuilder(buildUniverse(), spec);

  auto goal = co_await specBuilder.goalCoro(expressionBuilder());
  /*
    For each host, we want to compute PrimaryUsage (usage from tasks of the
    form "_p") and DisasterRecoveryUsage (added usage due to disasters).

    host0:
            PrimaryUsage = 15 (from "r1_p")

            DisasterRecoveryUsage:
             when ("host1", "host2", "host3") is down:
             = 20 (from "r4_s1"); "r2_s1", "r2_s2" are down as they are in
                  "host3" and "host1", respectively; "host4" has "r2_s3" and
                  "r3_s1", so those loads will be handled there)
             = 20

             when ("host4") is down:
             = 0 (since "r5_s1" is in "host2")

             maxDisasterRecoveryUsage = 20

            TotalUsage = 15 + 20 = 35

    host1:
            PrimaryUsage = 25 (from "r2_p")

            DisasterRecoveryUsage:
             when ("host0", "host3") is down:
             = 20 (from "r4_s2"; "host2" has "r1_s1" and so it will handle
             that
                   load; "r4_s1" is not available because it is in "host0")

             when ("host4") is down:
             = 0 (since "host2" has "r5_s1", so it will handle that load)

             maxDisasterRecoveryUsage = 20

            TotalUsage = 25 + 20 = 45

    host2:
            PrimaryUsage = 10 (from "r3_p")

            DisasterRecoveryUsage:
             when ("host0", "host1") is down:
             = 15 (from "r1_s1"; "host3" has "r2_s1", so that load will be
                   handled there)
             = 15

             when ("host0", "host3") is down:
             = 15 (from "r1_s1"; "r4_s1" is down as it as in "host0", "r4_s2"
             is
                   in "host1", so that load will be handled there)
             = 15

             when ("host4") is down:
             = 5 (from "r5_s1")

             maxDisasterRecoveryUsage = max(15, 15, 5) = 15

            TotalUsage = 10 + 15 = 25

    host3:
            PrimaryUsage = 20 (from "r4_p")

            DisasterRecoveryUsage:
             when ("host0", "host1") is down:
             = 25 (from "r2_s1"; "host2" has "r1_s1" and so that
                  load will be handled there)

             when ("host4") is down:
             = 0 (since "host2" has "r5_s1", so it will handle that load)

             maxDisasterRecoveryUsage = 25

            TotalUsage = 20 + 25 = 45

    host4:
            PrimaryUsage = 5 (from "r5_p")

            DisasterRecoveryUsage:
             when ("host0", "host1") is down:
             = 0 ("host2" has "r1_s1", "host3" has "r2_s1", so those loads
             will
                  be handled there)

             when ("host1", "host2", "host3") is down:
             = 25 (from "r2_s3"; "r2_s1" and "r2_s2" are down as they are in
                   "host3", "host1", respectively) + 10 (from "r3_s1";
                   "host0" has "r4_s1", so that load will be handled there)
             = 35

             when ("host0", "host3") is down:
             = 0 ("host2" has "r1_s1" and "host1" has "r4_s2" so those loads
                  will be handled there; "r4_s1" is down since it is in
                  "host0")

             maxDisasterRecoveryUsage = max(0, 35, 0) = 35

            TotalUsage = 5 + 35 = 40

    Goal_value = Sum of peak usages = 35 + 45 + 25 + 45 + 40 = 190
  */

  EXPECT_NEAR(190, evaluate(goal, deltaFromInitial({})), 1e-8);
}
} // namespace facebook::rebalancer::materializer::tests
