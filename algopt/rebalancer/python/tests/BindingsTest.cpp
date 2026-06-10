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

// C++ tests for the nanobind binding's helper logic. Mirrors the cases in
// interface/fb/polyglot/ProblemSolverBindingTests.cpp but exercises the same
// underlying behavior through the JSON-shaped surface that the Python module
// uses.

#include "algopt/rebalancer/algopt_common/TestUtils.h"
#include "algopt/rebalancer/algopt_common/Utils.h"
#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/ProblemSolverFactory.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSolver_types.h"

#include <gtest/gtest.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include <string>

namespace facebook::rebalancer::python::tests {

using Serializer = apache::thrift::SimpleJSONSerializer;

namespace {
template <typename T>
std::string toJson(const T& value) {
  std::string out;
  Serializer::serialize(value, &out);
  return out;
}
} // namespace

// We re-import the wrapper class by linking against the same translation unit;
// for simplicity the test invokes the underlying ProblemSolver directly with
// JSON-deserialized inputs, which is the same path the binding takes.
class BindingsTest : public ::testing::Test {
 protected:
  static std::unique_ptr<interface::ProblemSolver> makeSolver() {
    auto solver = interface::ProblemSolverFactory::makeProblemSolver(
        "rebalancer", "tests", /* canExecuteAsync= */ false);
    solver->setContainerName("container");
    solver->setObjectName("object");
    solver->shouldUseDynamicObjectOrdering(true);
    const std::unordered_map<std::string, std::vector<std::string>> assignment{
        {"c1", {"o1", "o2"}}, {"c2", {}}};
    solver->setAssignment(assignment);
    const std::unordered_map<std::string, std::vector<std::string>> partition{
        {"o1", {"o1"}}, {"o2", {"o2"}}};
    solver->addPartition("all objects", partition);
    return solver;
  }
};

TEST_F(BindingsTest, EmptyGoalSpecJsonRoundTripsToEmptyUnion) {
  // The binding deserializes JSON into a thrift union and rejects empty unions.
  // Empty JSON object yields an empty union.
  interface::GoalSpecs spec;
  Serializer::deserialize<interface::GoalSpecs>("{}", spec);
  REBALANCER_EXPECT_RUNTIME_ERROR(
      algopt::utils::throwIfUnionIsEmpty(spec),
      "Thrift union of type 'GoalSpecs' is empty. Please initialize it with a valid type.");
}

TEST_F(BindingsTest, EmptyConstraintSpecJsonRoundTripsToEmptyUnion) {
  interface::ConstraintSpecs spec;
  Serializer::deserialize<interface::ConstraintSpecs>("{}", spec);
  REBALANCER_EXPECT_RUNTIME_ERROR(
      algopt::utils::throwIfUnionIsEmpty(spec),
      "Thrift union of type 'ConstraintSpecs' is empty. Please initialize it with a valid type.");
}

TEST_F(BindingsTest, SetCompactAssignmentRoundTripsThroughBalanceSolve) {
  auto solver = interface::ProblemSolverFactory::makeProblemSolver(
      "rebalancer", "tests", /* canExecuteAsync= */ false);
  solver->setContainerName("host");
  solver->setObjectName("task");
  solver->shouldUseDynamicObjectOrdering(true);

  solver->setAssignment(
      std::unordered_map<std::string, std::unordered_map<std::string, int64_t>>{
          {"host0", {{"taskA", 3}}}, {"host1", {{"taskA", 1}}}});

  solver->addObjectDimension(
      "cpu", std::unordered_map<std::string, double>{{"taskA", 10.0}}, 0.0);
  solver->addContainerDimension(
      "cpu",
      std::unordered_map<std::string, double>{
          {"host0", 100.0}, {"host1", 100.0}},
      0.0);

  interface::BalanceSpec balanceSpec;
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu";
  balanceSpec.formula() = interface::BalanceSpecFormula::LEGACY;
  balanceSpec.fixAverageToInitial() = true;
  balanceSpec.name() = "balance_cpu";
  solver->addGoal(balanceSpec, 1.0, std::nullopt);

  interface::LocalSearchSolverSpec lsSpec;
  lsSpec.solveTime() = 5;
  interface::MoveTypeSpec moveType;
  moveType.singleMoveTypeSpec() = interface::SingleMoveTypeSpec{};
  lsSpec.moveTypeList()->push_back(moveType);
  solver->addSolver(lsSpec);

  const auto solution = solver->solve();

  // Verify the JSON serializer round-trips the solution shape used by the
  // Python module.
  const std::string solutionJson = toJson(solution);
  ASSERT_FALSE(solutionJson.empty());

  interface::AssignmentSolution roundtripped;
  Serializer::deserialize<interface::AssignmentSolution>(
      solutionJson, roundtripped);

  const auto& finalCounts =
      *roundtripped.compactAssignment()->objectsPerContainer();
  EXPECT_EQ(finalCounts.at("host0").at("taskA"), 2);
  EXPECT_EQ(finalCounts.at("host1").at("taskA"), 2);
}

} // namespace facebook::rebalancer::python::tests
