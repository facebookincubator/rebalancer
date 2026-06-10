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

#include "algopt/rebalancer/interface/tests/utils.h"

#include <folly/container/irange.h>
#include <gtest/gtest.h>

using namespace facebook::rebalancer::interface;

class FloatingPointViolationTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    FloatingPointViolationTest,
    testThreadCounts());

TEST_P(FloatingPointViolationTest, MoveShouldNotBePerformed) {
  /*Test Case: Container Capacity Utilization
This test should fail. We have two containers with a maximum capacity
utilization constraint of 1 for each.
One container is full, while the other is not.

The first container has an extremely large storage capacity (1e15). The
objects in the second container are relatively small (500 << 1e15).
When we attempt to free the second container, we expect no objects to be moved
because the first container is already full. However, due to floating-point
precision issues, the test incorrectly moves objects despite the hard
constraint.

This occurs because the small increases in capacity are deemed insignificant,
leading to incorrect behavior. Expected Result: No objects should be moved from
the second container. Actual Result: Objects are moved due to floating-point
precision errors, violating the hard constraint.
  */
  constexpr int numObjects = 1000;

  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("objects");
  solver->setContainerName("containers");
  std::map<std::string, std::vector<std::string>> initialAssignment = {
      {"container0", {"large_object"}},
  };
  for (const auto i : folly::irange(numObjects)) {
    initialAssignment["container1"].push_back(fmt::format("small_object{}", i));
  }

  solver->setAssignment(initialAssignment);

  facebook::rebalancer::entities::Map<std::string, double> objectSizes = {
      {"large_object", 1e15},
  };
  for (const auto i : folly::irange(numObjects)) {
    objectSizes[fmt::format("small_object{}", i)] = 500;
  }
  solver->addObjectDimension("storage", objectSizes);

  const facebook::rebalancer::entities::Map<std::string, double>
      containerSizes = {
          {"container0", 1e15},
          {"container1", 1e15},
      };

  solver->addContainerDimension("storage", containerSizes);

  CapacitySpec capacitySpec;
  capacitySpec.scope() = "containers";
  capacitySpec.dimension() = "storage";
  capacitySpec.limit()->type() = LimitType::RELATIVE;
  capacitySpec.limit()->globalLimit() = 1.0;
  solver->addConstraint(capacitySpec, ConstraintPolicy::HARD);

  ToFreeSpec toFreeSpec;
  toFreeSpec.containers() = {"container1"};
  solver->addGoal(toFreeSpec);

  facebook::algopt::common::thrift::PrecisionTolerances tolerances;
  tolerances.relative() = 1e-13;
  tolerances.absolute() = 1e-13;
  solver->setPrecision(tolerances);

  solver->addSolver(makeDefaultLocalSearchSolver());

  auto solution = solver->solve();

  auto& assignment = *solution.assignment();

  std::map<std::string, int> hostToTaskCount;
  for (auto& [task, host] : assignment) {
    ++hostToTaskCount[host];
  }

  EXPECT_EQ(1, hostToTaskCount["container0"]);
  EXPECT_EQ(numObjects, hostToTaskCount["container1"]);
}
