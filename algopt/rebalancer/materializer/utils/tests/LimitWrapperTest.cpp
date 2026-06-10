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
#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/materializer/utils/LimitWrapper.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"

#include <folly/coro/BlockingWait.h>
#include <gtest/gtest.h>

#include <memory>

namespace facebook::rebalancer::materializer::tests {
class LimitWrapperTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse(
        {{"host0", {"task1"}},
         {"host1", {"task2"}},
         {"host2", {"task3", "task4"}}});

    co_await addPartition(
        "job", {{"job1", {"task1", "task2"}}, {"job2", {"task3", "task4"}}});
    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }

  entities::ScopeId host() const {
    return scopeId("host");
  }

  entities::ScopeItemId host(int index) const {
    return scopeItemId("host", fmt::format("{}{}", "host", index));
  }

  entities::PartitionId job() const {
    return partitionId("job");
  }

  entities::GroupId job(int index) const {
    return groupId("job", fmt::format("job{}", index));
  }
};

TEST_F(LimitWrapperTest, DefaultLimit) {
  const interface::Limit limit;

  const LimitWrapper wrapper(buildUniverse(), limit, host());

  EXPECT_EQ(interface::LimitType::RELATIVE, wrapper.getType());
  EXPECT_EQ(1.0, wrapper.getLimit(host(0)));
  EXPECT_EQ(1.0, wrapper.getLimit(host(1)));
  EXPECT_EQ(1.0, wrapper.getLimit(host(2)));
}

TEST_F(LimitWrapperTest, ScopeItemLimit) {
  interface::Limit limit;
  limit.type() = interface::LimitType::ABSOLUTE;
  limit.globalLimit() = 2.7;
  limit.scopeItemLimits() = {{"host0", 1.5}, {"host1", 3.5}};

  const LimitWrapper wrapper(buildUniverse(), limit, host());

  EXPECT_EQ(interface::LimitType::ABSOLUTE, wrapper.getType());
  EXPECT_EQ(1.5, wrapper.getLimit(host(0)));
  EXPECT_EQ(3.5, wrapper.getLimit(host(1)));
  EXPECT_EQ(2.7, wrapper.getLimit(host(2)));
}

TEST_F(LimitWrapperTest, GroupItemLimit) {
  interface::Limit limit;
  limit.type() = interface::LimitType::ABSOLUTE;
  limit.globalLimit() = 2.7;
  limit.groupLimits() = {{"job1", 1}, {"job2", 3}};

  const LimitWrapper wrapper(buildUniverse(), limit, host(), job());

  EXPECT_EQ(1, wrapper.getLimit(job(1)));
  EXPECT_EQ(3, wrapper.getLimit(job(2)));
}

TEST_F(LimitWrapperTest, AllGroupItemLimits) {
  interface::Limit limit;
  limit.type() = interface::LimitType::ABSOLUTE;
  limit.globalLimit() = 1;
  limit.groupLimits() = {{"job2", 2}};
  const entities::Map<entities::GroupId, double> parsedLimits =
      LimitWrapper::getAllGroupLimits(buildUniverse(), job(), limit);
  EXPECT_EQ(2, parsedLimits.size());
  EXPECT_EQ(1, parsedLimits.at(job(1)));
  EXPECT_EQ(2, parsedLimits.at(job(2)));
}

TEST_F(LimitWrapperTest, ScopeItemTogroupItemLimits) {
  interface::Limit limit;
  limit.type() = interface::LimitType::ABSOLUTE;
  limit.globalLimit() = 2.7;
  limit.scopeItemToGroupLimits() = {{"host0", {{"job1", 1}}}};
  const LimitWrapper wrapper(buildUniverse(), limit, host(), job());

  EXPECT_EQ(1, wrapper.getLimit(host(0), job(1)));
}

TEST_F(LimitWrapperTest, GroupsOverride) {
  const auto universe = buildUniverse();
  // 1. Test a scenario with scopeItemLimits
  {
    interface::Limit limit;
    limit.type() = interface::LimitType::RELATIVE;
    limit.globalLimit() = 3;
    limit.scopeItemLimits() = {{"host0", 1.5}, {"host1", 3.5}};

    const LimitWrapper wrapper(universe, limit, host(), job());

    const entities::Map<entities::GroupId, double> expected0 = {
        {job(1), 1.5}, {job(2), 1.5}};
    EXPECT_EQ(expected0, wrapper.getGroupsOverride(host(0)));
    const entities::Map<entities::GroupId, double> expected1 = {
        {job(1), 3.5}, {job(2), 3.5}};
    EXPECT_EQ(expected1, wrapper.getGroupsOverride(host(1)));
  }

  // 2. Test a scenario with groupLimits
  {
    interface::Limit limit;
    limit.type() = interface::LimitType::RELATIVE;
    limit.globalLimit() = 3;
    limit.groupLimits() = {{"job1", 1.5}, {"job2", 3.5}};

    const LimitWrapper wrapper(universe, limit, host(), job());

    const entities::Map<entities::GroupId, double> expected = {
        {job(1), 1.5}, {job(2), 3.5}};
    EXPECT_EQ(expected, wrapper.getGroupsOverride(host(0)));
    EXPECT_EQ(expected, wrapper.getGroupsOverride(host(1)));
  }
  // 3. Test a scenario where both scopeItemLimits and groupLimits are specified
  {
    interface::Limit limit;
    limit.type() = interface::LimitType::RELATIVE;
    limit.globalLimit() = 3;
    limit.scopeItemLimits() = {{"host0", 1.5}};
    limit.groupLimits() = {{"job1", 2.5}, {"job2", 3.5}};

    // if both limits are provided we use the scopeItemLimits, specifically for
    // host0 we have scopeItemLimit of 1.5 while job limit of 2.5 and 3.5. In
    // this case we choose scopeItem value as the override

    const LimitWrapper wrapper(universe, limit, host(), job());

    const entities::Map<entities::GroupId, double> expected0 = {
        {job(1), 1.5}, {job(2), 1.5}};
    const entities::Map<entities::GroupId, double> expected1 = {
        {job(1), 2.5}, {job(2), 3.5}};
    EXPECT_EQ(expected0, wrapper.getGroupsOverride(host(0)));
    EXPECT_EQ(expected1, wrapper.getGroupsOverride(host(1)));
  }
  // 4. Test a scenario with incomplete groupScopeItemLimits
  {
    interface::Limit limit;
    limit.type() = interface::LimitType::RELATIVE;
    limit.globalLimit() = 3;
    limit.scopeItemLimits() = {{"host0", 1.5}, {"host1", 3.5}};
    limit.scopeItemToGroupLimits() = {{"host0", {{"job1", 1}}}};

    const LimitWrapper wrapper(universe, limit, host(), job());

    const entities::Map<entities::GroupId, double> expected0 = {
        {job(1), 1}, {job(2), 1.5}};
    const entities::Map<entities::GroupId, double> expected1 = {
        {job(1), 3.5}, {job(2), 3.5}};
    EXPECT_EQ(expected0, wrapper.getGroupsOverride(host(0)));
    EXPECT_EQ(expected1, wrapper.getGroupsOverride(host(1)));
  }
}

TEST_F(LimitWrapperTest, CheckAndGetIntegralScopeItemLimits) {
  const auto universe = buildUniverse();
  // Test case for successful integral limit conversion
  {
    interface::Limit limit;
    limit.type() = interface::LimitType::ABSOLUTE;
    limit.globalLimit() = 2.0;
    limit.scopeItemLimits() = {{"host0", 1.0}, {"host1", 3.0}, {"host2", 5.0}};

    const LimitWrapper wrapper(universe, limit, host());

    const auto integralLimits =
        wrapper.checkAndGetPositiveIntegerScopeItemLimits();
    EXPECT_EQ(3, integralLimits.size());
    EXPECT_EQ(1, integralLimits.at(host(0)));
    EXPECT_EQ(3, integralLimits.at(host(1)));
    EXPECT_EQ(5, integralLimits.at(host(2)));
  }

  // Test case for exception when non-integral limits are present
  {
    interface::Limit limit;
    limit.type() = interface::LimitType::ABSOLUTE;
    limit.globalLimit() = 2.0;
    limit.scopeItemLimits() = {{"host0", 1.5}, {"host1", 3.0}};

    const LimitWrapper wrapper(universe, limit, host());

    REBALANCER_EXPECT_RUNTIME_ERROR(
        wrapper.checkAndGetPositiveIntegerScopeItemLimits(),
        "Limit 1.5 for scopeItem host0 is not a positive integer");
  }

  // Test case for empty scopeItemLimits
  {
    interface::Limit limit;
    limit.type() = interface::LimitType::ABSOLUTE;
    limit.globalLimit() = 2.0;
    // No scopeItemLimits specified

    const LimitWrapper wrapper(universe, limit, host());

    const auto integralLimits =
        wrapper.checkAndGetPositiveIntegerScopeItemLimits();
    EXPECT_TRUE(integralLimits.empty());
  }
}

} // namespace facebook::rebalancer::materializer::tests
