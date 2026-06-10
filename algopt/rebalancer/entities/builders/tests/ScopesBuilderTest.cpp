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

#include "algopt/rebalancer/algopt_common/TestUtils.h"
#include "algopt/rebalancer/entities/builders/AssignmentBuilder.h"
#include "algopt/rebalancer/entities/builders/ScopesBuilder.h"
#include "algopt/rebalancer/entities/builders/tests/BuilderTestBase.h"

#include <folly/container/irange.h>
#include <folly/coro/BlockingWait.h>
#include <folly/coro/Collect.h>
#include <folly/coro/GtestHelpers.h>
#include <folly/portability/GTest.h>

namespace facebook::rebalancer::entities::tests {

class ScopesBuilderTest : public BuilderTestBase {
 protected:
  void SetUp() override {
    BuilderTestBase::SetUp();
    assignmentBuilder_ = std::make_unique<AssignmentBuilder>(idBuilder_);
    const Map<std::string, std::vector<std::string>> assignment{
        {"c1", {"o1", "o2"}},
        {"c2", {"o3"}},
        {"c3", {}},
    };
    assignmentBuilder_->set(assignment);
  }

  std::unique_ptr<AssignmentBuilder> assignmentBuilder_;
};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    ScopesBuilderTest,
    ::testing::Values(1, 3, 20));

CO_TEST_P(ScopesBuilderTest, AddScopeItemToContainers) {
  ScopesBuilder scopesBuilder(idBuilder_);
  const auto& containers = assignmentBuilder_->getContainers();

  const Map<std::string, std::vector<std::string>> regionScope{
      {"us-east", {"c1", "c2"}},
      {"us-west", {"c3"}},
  };
  const Map<std::string, std::vector<std::string>> rackScope{
      {"rack1", {"c1"}},
  };

  const auto regionId = scopesBuilder.makeScopeId("region");
  const auto rackId = scopesBuilder.makeScopeId("rack");
  co_await scopesBuilder.add(regionId, regionScope, containers);
  co_await scopesBuilder.add(rackId, rackScope, containers);

  const auto regionScopeData = co_await scopesBuilder.get(regionId);
  EXPECT_EQ(2, regionScopeData->scopeItemNameToId->size());
  const auto usEastId = regionScopeData->getScopeItemId("us-east");
  const auto& usEastContainers =
      regionScopeData->scopeItemIdToContainerIds->at(usEastId);
  EXPECT_EQ(usEastContainers.size(), 2);
  EXPECT_TRUE(usEastContainers.contains(containers->getId("c1")));
  EXPECT_TRUE(usEastContainers.contains(containers->getId("c2")));

  const auto rackScopeData = co_await scopesBuilder.get(rackId);
  EXPECT_EQ(1, rackScopeData->scopeItemNameToId->size());
  const auto rack1Id = rackScopeData->getScopeItemId("rack1");
  const auto& rack1Containers =
      rackScopeData->scopeItemIdToContainerIds->at(rack1Id);
  EXPECT_EQ(1, rack1Containers.size());

  // build
  const auto result = co_await scopesBuilder.build();
  EXPECT_EQ(2, result.scopeNameToId.size());
  EXPECT_EQ(regionId, result.scopeNameToId.at("region"));
  EXPECT_EQ(rackId, result.scopeNameToId.at("rack"));

  // Verify scopeIdToScopeItemNameToId
  EXPECT_EQ(result.scopeIdToScopeItemNameToId.size(), 2);
  const auto& regionItemNameToId =
      result.scopeIdToScopeItemNameToId.at(regionId);
  EXPECT_EQ(regionItemNameToId->size(), 2);
  EXPECT_TRUE(regionItemNameToId->contains("us-east"));
  EXPECT_TRUE(regionItemNameToId->contains("us-west"));

  // Verify scopeIdToScopeItemIdToContainerIds
  EXPECT_EQ(result.scopeIdToScopeItemIdToContainerIds.size(), 2);
  const auto& regionItems =
      result.scopeIdToScopeItemIdToContainerIds.at(regionId);
  EXPECT_EQ(2, regionItems->size());
  EXPECT_EQ(2, regionItems->at(usEastId).size());
  EXPECT_TRUE(regionItems->at(usEastId).contains(containers->getId("c1")));
  EXPECT_TRUE(regionItems->at(usEastId).contains(containers->getId("c2")));
}

CO_TEST_P(ScopesBuilderTest, AddContainerToScopeItems) {
  ScopesBuilder scopesBuilder(idBuilder_);
  const auto containers = assignmentBuilder_->getContainers();

  const auto scopeId = scopesBuilder.makeScopeId("region");

  const Map<std::string, std::string> containerToRegion{
      {"c1", "us-east"},
      {"c2", "us-east"},
      {"c3", "us-west"},
  };

  co_await scopesBuilder.add(scopeId, containerToRegion, containers);

  const auto scopeData = co_await scopesBuilder.get(scopeId);

  EXPECT_EQ(2, scopeData->scopeItemNameToId->size());
  EXPECT_EQ(2, scopeData->scopeItemIdToContainerIds->size());
  EXPECT_EQ(2, scopeData->scopeItemIdToContainerIds->size());
  const auto usEastId = scopeData->getScopeItemId("us-east");
  const auto& usEastContainers =
      scopeData->scopeItemIdToContainerIds->at(usEastId);
  EXPECT_EQ(usEastContainers.size(), 2);
  EXPECT_TRUE(usEastContainers.contains(containers->getId("c1")));
  EXPECT_TRUE(usEastContainers.contains(containers->getId("c2")));

  const auto usWestId = scopeData->getScopeItemId("us-west");
  const auto& usWestContainers =
      scopeData->scopeItemIdToContainerIds->at(usWestId);
  EXPECT_EQ(1, usWestContainers.size());
  EXPECT_TRUE(usWestContainers.contains(containers->getId("c3")));
}

CO_TEST_P(ScopesBuilderTest, ConcurrentAddAndGet) {
  ScopesBuilder scopesBuilder(idBuilder_);

  AssignmentBuilder customAssignmentBuilder(idBuilder_);
  customAssignmentBuilder.set(generateInitialAssignment(
      ProblemConfig{.numObjects = 10'000, .numContainers = 333}));
  const auto containers = customAssignmentBuilder.getContainers();

  constexpr int nScopes = 500;
  std::vector<ScopeId> scopeIds;
  scopeIds.reserve(nScopes);
  for (const auto i : folly::irange(nScopes)) {
    scopeIds.push_back(scopesBuilder.makeScopeId(fmt::format("scope_{}", i)));
  }

  constexpr int nScopeItems = 71;
  const auto scopeItemNameToContainerNames =
      generateScopeItemNameToContainerNames(nScopeItems, *containers);

  std::vector<folly::coro::Task<void>> tasks;
  tasks.reserve(nScopes * 2);
  for (const auto i : folly::irange(nScopes)) {
    tasks.push_back(scopesBuilder.add(
        scopeIds[i], scopeItemNameToContainerNames, containers));

    tasks.push_back(
        folly::coro::co_invoke(
            [&scopesBuilder, nScopeItems, i, scopeId = scopeIds[i]]()
                -> folly::coro::Task<void> {
              executeRandomDelay();
              const auto data = co_await scopesBuilder.get(scopeId);
              EXPECT_EQ(nScopeItems, data->scopeItemNameToId->size());
              EXPECT_EQ(nScopeItems, data->scopeItemIdToContainerIds->size());
              executeRandomDelay();
              EXPECT_EQ(
                  scopeId,
                  scopesBuilder.getScopeId(fmt::format("scope_{}", i)));
            }));
  }

  co_await folly::coro::co_withExecutor(
      executor.get(), folly::coro::collectAllRange(std::move(tasks)));

  const auto result = co_await scopesBuilder.build();

  EXPECT_EQ(nScopes, result.scopeNameToId.size());
  for (const auto& [scopeId, scopeItemNameToId] :
       result.scopeIdToScopeItemNameToId) {
    EXPECT_EQ(nScopeItems, scopeItemNameToId->size());
  }
  for (const auto& [scopeId, scopeItemIdToContainerIds] :
       result.scopeIdToScopeItemIdToContainerIds) {
    EXPECT_EQ(nScopeItems, scopeItemIdToContainerIds->size());
  }
}

TEST_P(ScopesBuilderTest, DuplicateMakeScopeIdThrows) {
  ScopesBuilder scopesBuilder(idBuilder_);
  std::ignore = scopesBuilder.makeScopeId("region");
  REBALANCER_EXPECT_RUNTIME_ERROR(
      scopesBuilder.makeScopeId("region"),
      "Unexpected call to makeScopeId with a previously added name 'region'");
}

CO_TEST_P(ScopesBuilderTest, Summarize) {
  ScopesBuilder builder(idBuilder_);
  const auto containers = assignmentBuilder_->getContainers();

  const auto regionId = builder.makeScopeId("region");
  const Map<std::string, std::vector<std::string>> regionScope{
      {"us-east", {"c1", "c2"}},
      {"us-west", {"c3"}},
  };
  co_await builder.add(regionId, regionScope, containers);

  const auto result = co_await builder.summarize();

  const std::string expected =
      "Scopes:\n"
      "  region [ us-east us-west ]\n";
  EXPECT_EQ(expected, result);
}

CO_TEST_P(ScopesBuilderTest, BuildAddScopeAndRebuild) {
  ScopesBuilder scopesBuilder(idBuilder_);
  const auto containers = assignmentBuilder_->getContainers();

  const auto scope1Id = scopesBuilder.makeScopeId("scope1");
  const Map<std::string, std::vector<std::string>> scope1{
      {"item1", {"c1", "c2"}},
  };
  co_await scopesBuilder.add(scope1Id, scope1, containers);

  const auto result1 = co_await scopesBuilder.build();
  EXPECT_EQ(1, result1.scopeNameToId.size());
  EXPECT_EQ(scope1Id, result1.scopeNameToId.at("scope1"));
  EXPECT_EQ(1, result1.scopeIdToScopeItemIdToContainerIds.size());
  EXPECT_EQ(1, result1.scopeIdToScopeItemNameToId.size());
  EXPECT_EQ(1, result1.scopeIdToScopeItemNameToId.at(scope1Id)->size());

  const auto scope2Id = scopesBuilder.makeScopeId("scope2");
  const Map<std::string, std::vector<std::string>> scope2{
      {"itemA", {"c1"}},
      {"itemB", {"c3"}},
  };
  co_await scopesBuilder.add(scope2Id, scope2, containers);

  const auto result2 = co_await scopesBuilder.build();

  // Verify that result1 was not mutated by the second build
  EXPECT_EQ(1, result1.scopeNameToId.size());
  EXPECT_EQ(scope1Id, result1.scopeNameToId.at("scope1"));
  EXPECT_EQ(1, result1.scopeIdToScopeItemIdToContainerIds.size());
  EXPECT_EQ(1, result1.scopeIdToScopeItemNameToId.size());
  EXPECT_EQ(1, result1.scopeIdToScopeItemNameToId.at(scope1Id)->size());

  EXPECT_EQ(2, result2.scopeNameToId.size());
  EXPECT_EQ(scope2Id, result2.scopeNameToId.at("scope2"));
  EXPECT_EQ(2, result2.scopeIdToScopeItemIdToContainerIds.size());
  EXPECT_EQ(2, result2.scopeIdToScopeItemNameToId.size());
  EXPECT_EQ(2, result2.scopeIdToScopeItemNameToId.at(scope2Id)->size());
}

} // namespace facebook::rebalancer::entities::tests
