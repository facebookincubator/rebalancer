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

#include <gtest/gtest.h>

#include <map>
#include <string>

using namespace facebook::rebalancer::interface;

class NestedScopeLimitTest : public ::testing::TestWithParam<int> {
 protected:
  static std::unique_ptr<ProblemSolver> setUp() {
    auto solver =
        initializeTestProblemSolver({.executorThreadCount = GetParam()});
    solver->setObjectName("rack");
    solver->setContainerName("msb");

    solver->setAssignment(
        std::map<std::string, std::vector<std::string>>{
            {"msb0", {"rack0", "rack1", "rack2", "rack3"}}, {"msb1", {}}});

    ManifoldBackupParams params;
    params.uploadPolicy() = ManifoldUploadPolicy::NEVER;
    solver->setManifoldBackupParams(params);
    return solver;
  }
};

INSTANTIATE_TEST_CASE_P(ThreadCounts, NestedScopeLimitTest, testThreadCounts());

TEST_P(NestedScopeLimitTest, basic) {
  auto solver = setUp();
  const std::map<std::string, std::string> msbToDc = {
      {"msb0", "dc0"}, {"msb1", "dc0"}};
  // Create data center scope
  solver->addScope("dc", msbToDc);
  NestedScopeLimitSpec nestedScopeLimitSpec;
  nestedScopeLimitSpec.scope() = "msb";
  nestedScopeLimitSpec.dimension() = "rack_count";
  nestedScopeLimitSpec.outerScope() = "dc";
  nestedScopeLimitSpec.limit()->globalLimit() = 0.5;
  solver->addConstraint(nestedScopeLimitSpec);

  solver->addSolver(makeDefaultLocalSearchSolver());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto assignment = *solution.assignment();
  EXPECT_GT(*solution.initialObjective()->value(), 0);
  EXPECT_EQ(0, *solution.finalObjective()->value());
  std::map<std::string, int> msbCount;
  for (auto& [rack, msb] : assignment) {
    msbCount[msb]++;
  }
  EXPECT_EQ(2, msbCount.at("msb0"));
  EXPECT_EQ(2, msbCount.at("msb1"));
}
