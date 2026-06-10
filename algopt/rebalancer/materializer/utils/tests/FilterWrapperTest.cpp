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

#include "algopt/rebalancer/materializer/utils/FilterWrapper.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"

#include <folly/coro/BlockingWait.h>
#include <gtest/gtest.h>

template <class T>
std::set<T> makeSet(std::initializer_list<T> items) {
  return std::set<T>(items);
}

template <class T>
std::set<T> makeSet(const std::vector<T>& items) {
  return std::set<T>(items.begin(), items.end());
}

namespace facebook::rebalancer::materializer::tests {

class FilterWrapperTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse(
        {{"host0", {"task0", "task1", "task2", "task3", "task4"}},
         {"host1", {}},
         {"host2", {}},
         {"host3", {}}});

    co_await addPartition(
        "job",
        {{"job0", {"task0", "task2"}},
         {"job1", {"task1", "task3"}},
         {"job2", {"task4"}}});

    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }

  entities::ScopeId host() const {
    return scopeId("host");
  }
  entities::ScopeItemId host(int index) const {
    return scopeItemId("host", fmt::format("host{}", index));
  }
  entities::PartitionId job() const {
    return partitionId("job");
  }
  entities::GroupId job(int index) const {
    return groupId("job", fmt::format("job{}", index));
  }
};

TEST_F(FilterWrapperTest, DefaultFilter) {
  const auto universe = buildUniverse();
  interface::Filter filter;

  {
    const ScopeItemFilterWrapper wrapper(*universe, filter, host());

    EXPECT_EQ(
        makeSet({
            host(0),
            host(1),
            host(2),
            host(3),
        }),
        makeSet(wrapper.getScopeItemIds()));
    EXPECT_EQ(
        std::set<entities::ScopeItemId>{},
        makeSet(wrapper.getExcludedScopeItemIds()));

    EXPECT_TRUE(wrapper.isIncluded(host(0)));
    EXPECT_TRUE(wrapper.isIncluded(host(1)));
    EXPECT_TRUE(wrapper.isIncluded(host(2)));
    EXPECT_TRUE(wrapper.isIncluded(host(3)));
  }

  {
    filter.type() = interface::FilterType::GROUP;
    const GroupFilterWrapper wrapper(*universe, filter, job());

    EXPECT_EQ(
        makeSet({
            job(0),
            job(1),
            job(2),
        }),
        makeSet(wrapper.getGroupIds()));

    EXPECT_TRUE(wrapper.isIncluded(job(0)));
    EXPECT_TRUE(wrapper.isIncluded(job(1)));
    EXPECT_TRUE(wrapper.isIncluded(job(2)));
  }
}

TEST_F(FilterWrapperTest, NonEmptyBlacklist) {
  const auto universe = buildUniverse();
  interface::Filter filter;

  {
    filter.itemsBlacklist() = {"host1", "host3"};
    const ScopeItemFilterWrapper wrapper(*universe, filter, host());

    EXPECT_EQ(
        makeSet({
            host(0),
            host(2),
        }),
        makeSet(wrapper.getScopeItemIds()));
    EXPECT_EQ(
        makeSet({host(1), host(3)}),
        makeSet(wrapper.getExcludedScopeItemIds()));

    EXPECT_TRUE(wrapper.isIncluded(host(0)));
    EXPECT_FALSE(wrapper.isIncluded(host(1)));
    EXPECT_TRUE(wrapper.isIncluded(host(2)));
    EXPECT_FALSE(wrapper.isIncluded(host(3)));
  }

  {
    filter.itemsBlacklist() = {"job1", "job2"};
    filter.type() = interface::FilterType::GROUP;
    const GroupFilterWrapper wrapper(*universe, filter, job());

    EXPECT_EQ(
        makeSet({
            job(0),
        }),
        makeSet(wrapper.getGroupIds()));

    EXPECT_TRUE(wrapper.isIncluded(job(0)));
    EXPECT_FALSE(wrapper.isIncluded(job(1)));
    EXPECT_FALSE(wrapper.isIncluded(job(2)));
  }
}

TEST_F(FilterWrapperTest, EmptyBlacklist) {
  const auto universe = buildUniverse();
  interface::Filter filter;
  filter.itemsBlacklist() = {};

  {
    const ScopeItemFilterWrapper wrapper(*universe, filter, host());

    EXPECT_EQ(
        makeSet({
            host(0),
            host(1),
            host(2),
            host(3),
        }),
        makeSet(wrapper.getScopeItemIds()));
    EXPECT_EQ(
        std::set<entities::ScopeItemId>{},
        makeSet(wrapper.getExcludedScopeItemIds()));

    EXPECT_TRUE(wrapper.isIncluded(host(0)));
    EXPECT_TRUE(wrapper.isIncluded(host(1)));
    EXPECT_TRUE(wrapper.isIncluded(host(2)));
    EXPECT_TRUE(wrapper.isIncluded(host(3)));
  }

  {
    filter.type() = interface::FilterType::GROUP;
    const GroupFilterWrapper wrapper(*universe, filter, job());

    EXPECT_EQ(
        makeSet({
            job(0),
            job(1),
            job(2),
        }),
        makeSet(wrapper.getGroupIds()));

    EXPECT_TRUE(wrapper.isIncluded(job(0)));
    EXPECT_TRUE(wrapper.isIncluded(job(1)));
    EXPECT_TRUE(wrapper.isIncluded(job(2)));
  }
}

TEST_F(FilterWrapperTest, NonEmptyWhitelist) {
  const auto universe = buildUniverse();
  interface::Filter filter;
  {
    filter.itemsWhitelist() = {"host1", "host3"};
    const ScopeItemFilterWrapper wrapper(*universe, filter, host());

    EXPECT_EQ(
        makeSet({
            host(1),
            host(3),
        }),
        makeSet(wrapper.getScopeItemIds()));
    EXPECT_EQ(
        makeSet({host(0), host(2)}),
        makeSet(wrapper.getExcludedScopeItemIds()));

    EXPECT_FALSE(wrapper.isIncluded(host(0)));
    EXPECT_TRUE(wrapper.isIncluded(host(1)));
    EXPECT_FALSE(wrapper.isIncluded(host(2)));
    EXPECT_TRUE(wrapper.isIncluded(host(3)));
  }

  {
    filter.itemsWhitelist() = {"job0", "job2"};
    filter.type() = interface::FilterType::GROUP;
    const GroupFilterWrapper wrapper(*universe, filter, job());

    EXPECT_EQ(
        makeSet({
            job(0),
            job(2),
        }),
        makeSet(wrapper.getGroupIds()));

    EXPECT_TRUE(wrapper.isIncluded(job(0)));
    EXPECT_FALSE(wrapper.isIncluded(job(1)));
    EXPECT_TRUE(wrapper.isIncluded(job(2)));
  }
}

TEST_F(FilterWrapperTest, EmptyWhitelist) {
  const auto universe = buildUniverse();
  interface::Filter filter;
  filter.itemsWhitelist() = {};

  {
    const ScopeItemFilterWrapper wrapper(*universe, filter, host());

    EXPECT_EQ(
        makeSet<entities::ScopeItemId>({}), makeSet(wrapper.getScopeItemIds()));
    EXPECT_EQ(
        makeSet({host(0), host(1), host(2), host(3)}),
        makeSet(wrapper.getExcludedScopeItemIds()));

    EXPECT_FALSE(wrapper.isIncluded(host(0)));
    EXPECT_FALSE(wrapper.isIncluded(host(1)));
    EXPECT_FALSE(wrapper.isIncluded(host(2)));
    EXPECT_FALSE(wrapper.isIncluded(host(3)));
  }

  {
    filter.type() = interface::FilterType::GROUP;
    const GroupFilterWrapper wrapper(*universe, filter, job());

    EXPECT_EQ(makeSet<entities::GroupId>({}), makeSet(wrapper.getGroupIds()));

    EXPECT_FALSE(wrapper.isIncluded(job(0)));
    EXPECT_FALSE(wrapper.isIncluded(job(1)));
    EXPECT_FALSE(wrapper.isIncluded(job(2)));
  }
}

TEST_F(FilterWrapperTest, EmptyMethod) {
  const auto universe = buildUniverse();
  // Test empty filter (no whitelist or blacklist)
  {
    const interface::Filter filter;
    const ScopeItemFilterWrapper wrapper(*universe, filter, host());
    EXPECT_TRUE(wrapper.empty());
  }

  {
    interface::Filter filter;
    filter.type() = interface::FilterType::GROUP;
    const GroupFilterWrapper wrapper(*universe, filter, job());
    EXPECT_TRUE(wrapper.empty());
  }

  // Test filter with non-empty blacklist
  {
    interface::Filter filterWithBlacklist;
    filterWithBlacklist.itemsBlacklist() = {"host1", "host3"};
    const ScopeItemFilterWrapper wrapper(
        *universe, filterWithBlacklist, host());
    EXPECT_FALSE(wrapper.empty());
  }

  // Test filter with empty blacklist (should be considered non-empty since
  // blacklist is explicitly set)
  {
    interface::Filter filterWithEmptyBlacklist;
    filterWithEmptyBlacklist.itemsBlacklist() = {};
    const ScopeItemFilterWrapper wrapper(
        *universe, filterWithEmptyBlacklist, host());
    EXPECT_FALSE(wrapper.empty());
  }

  // Test filter with non-empty whitelist
  {
    interface::Filter filterWithWhitelist;
    filterWithWhitelist.itemsWhitelist() = {"job0", "job2"};
    filterWithWhitelist.type() = interface::FilterType::GROUP;
    const GroupFilterWrapper wrapper(*universe, filterWithWhitelist, job());
    EXPECT_FALSE(wrapper.empty());
  }

  // Test filter with empty whitelist (should be considered non-empty since
  // whitelist is set)
  {
    interface::Filter filterWithEmptyWhitelist;
    filterWithEmptyWhitelist.itemsWhitelist() = {};
    const ScopeItemFilterWrapper wrapper(
        *universe, filterWithEmptyWhitelist, host());
    EXPECT_FALSE(wrapper.empty());
  }
}

} // namespace facebook::rebalancer::materializer::tests
