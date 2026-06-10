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
#include "algopt/rebalancer/interface/thrift/ThriftUtils.h"

#include <gtest/gtest.h>

#include <map>
#include <string>

namespace thriftUtils = facebook::rebalancer::interface::thriftUtils;

namespace facebook::rebalancer::interface::tests {

class RoutingLatencyTest
    : public ::testing::TestWithParam<std::tuple<int, bool>> {
 protected:
  std::vector<std::vector<std::string>> routingLogic1 = {
      {"region1"},
      {"region0", "region2"}};
  std::vector<std::vector<std::string>> routingLogic2 = {
      {"region0", "region1", "region2"}};
  std::vector<std::vector<std::string>> routingLogic3 = {
      {"region2"},
      {"region0", "region1"}};

  std::unordered_map<std::string, GroupRoutingRings> getGroupToRoutingRings(
      bool useDefaultRoutingLogic) {
    std::unordered_map<std::string, GroupRoutingRings> groupToRoutingRings;

    constexpr double totalTrafficUnitsPerService = 100;
    // 98% of the traffic for service0 originates in region0, 2% at region1,

    GroupRoutingRings service0Rings;
    service0Rings.routingRings()->push_back(getRoutingRing(
        "region0", // origin
        0.98 * totalTrafficUnitsPerService, // originTraffic
        useDefaultRoutingLogic
            ? std::nullopt
            : std::make_optional(routingLogic1) // routingLogic
        ));

    service0Rings.routingRings()->push_back(getRoutingRing(
        "region1", // origin
        0.02 * totalTrafficUnitsPerService, // originTraffic
        useDefaultRoutingLogic
            ? std::nullopt
            : std::make_optional(routingLogic2) // routingLogic
        ));

    groupToRoutingRings.emplace("service0", service0Rings);

    // 100% of the traffic for service1 originates in region2.
    GroupRoutingRings service1Rings;
    service1Rings.routingRings()->push_back(getRoutingRing(
        "region2", // origin
        totalTrafficUnitsPerService, // originTraffic
        useDefaultRoutingLogic
            ? std::nullopt
            : std::make_optional(routingLogic3) // routingLogic
        ));

    groupToRoutingRings.emplace("service1", service1Rings);

    return groupToRoutingRings;
  }

  std::unordered_map<std::string, std::vector<std::vector<std::string>>>
  getDefaultOriginToDestinationScopeItemSets(bool useDefaultRoutingLogic) {
    std::unordered_map<std::string, std::vector<std::vector<std::string>>>
        defaultMap;
    if (useDefaultRoutingLogic) {
      defaultMap.emplace("region0", routingLogic1);
      defaultMap.emplace("region1", routingLogic2);
      defaultMap.emplace("region2", routingLogic3);
    }
    return defaultMap;
  }

  void setUpProblem(
      bool useDefaultRoutingLogic,
      interface::RoutingLatencyMetricInfo aggregationMetric,
      std::optional<Filter> groupFilter = std::nullopt,
      double globalLimit = 0.0,
      double weightForExtraAvgLatency = 1.0) {
    // We simulate 2 services with 2 replicas each.
    solver_ =
        initializeTestProblemSolver({.executorThreadCount = numThreads()});
    solver_->setObjectName("replica");
    solver_->setContainerName("region");

    solver_->setAssignment(
        std::vector<std::pair<std::string, std::vector<std::string>>>{
            {"region0", {"service1:0"}},
            {"region1", {"service0:0", "service1:1"}},
            {"region2", {"service0:1"}},
        });

    // Group replicas of the same service together in a partition.
    replicaToService_ = {
        {"service0:0", "service0"},
        {"service0:1", "service0"},
        {"service1:0", "service1"},
        {"service1:1", "service1"},
    };
    solver_->addPartition("service", replicaToService_);

    {
      // define routingConfig
      const std::unordered_map<
          std::string,
          std::unordered_map<std::string, double>>
          originToDestinationLatency = {
              {"region0", {{"region0", 5}, {"region1", 10}, {"region2", 5}}},
              {"region1", {{"region0", 5}, {"region1", 0}, {"region2", 15}}},
              {"region2", {{"region0", 50}, {"region1", 40}, {"region2", 0}}}};

      solver_->addRoutingConfig(
          "routingConfig1",
          "region",
          "service",
          getGroupToRoutingRings(useDefaultRoutingLogic),
          originToDestinationLatency,
          getDefaultOriginToDestinationScopeItemSets(useDefaultRoutingLogic));
    }

    // At most one replica of the same service can be placed in the same
    // region.
    {
      GroupCountSpec spec;
      spec.scope() = "region";
      spec.partitionName() = "service";
      spec.dimension() = "replica_count";
      spec.limit()->globalLimit() = 1;
      solver_->addConstraint(spec);
    }

    {
      RoutingLatencySpec spec;
      spec.scope() = "region";
      spec.partition() = "service";
      spec.routingConfigName() = "routingConfig1";

      spec.limit()->globalLimit() = globalLimit;
      spec.latencyMetric() = aggregationMetric;

      if (groupFilter.has_value()) {
        spec.filter() = std::move(groupFilter.value());
      }

      if (std::get<1>(GetParam())) {
        spec.includeWeightedAvgLatencyMetricIfLimitViolated() =
            weightForExtraAvgLatency;
      }

      solver_->addGoal(spec);
    }

    solver_->addSolver(makeDefaultLocalSearchSolver());

    solver_->publishMetrics();
  }

  static RoutingRing getRoutingRing(
      const std::string& origin,
      double originTraffic,
      const std::optional<std::vector<std::vector<std::string>>>&
          destinationScopeItemSets) {
    RoutingRing ring;
    ring.originScopeItem() = origin;
    ring.originTraffic() = originTraffic;
    if (destinationScopeItemSets.has_value()) {
      ring.destinationScopeItemSets() = destinationScopeItemSets.value();
    }

    return ring;
  }

  static int numThreads() {
    return std::get<0>(GetParam());
  }

  static bool includeAvgLatencyMetricIfViolated() {
    return std::get<1>(GetParam());
  }

  std::unique_ptr<ProblemSolver> solver_;
  std::map<std::string, std::string> replicaToService_;
};

static void verifyRoutingLatencyMetrics(
    interface::RoutingLatencyMetricInfo metric,
    const std::string& routingConfigName,
    const AssignmentSolution& solution,
    const std::vector<std::pair<std::string, double>>&
        expectedInitialGroupLatencyPairs,
    std::vector<std::pair<std::string, double>>
        expectedFinalGroupLatencyPairs) {
  auto& actualInitialGroupLatencyMetrics =
      *solution.initialMetrics()
           ->routingConfigToGroupLatencyMetrics()
           ->at(routingConfigName)
           .groupToMetricValues();
  auto& actualFinalGroupLatencyMetrics =
      *solution.finalMetrics()
           ->routingConfigToGroupLatencyMetrics()
           ->at(routingConfigName)
           .groupToMetricValues();

  auto getLatency = [&metric](auto& groupLatencyMetrics, const auto& group) {
    for (auto& latencyMetrics : groupLatencyMetrics.at(group)) {
      if (latencyMetrics.metric() == metric) {
        return *latencyMetrics.value();
      }
    }
    throw std::runtime_error(
        fmt::format(
            "{} not found for group {}",
            apache::thrift::util::enumNameSafe(*metric.type()),
            group));
  };

  auto assertEqual = [&](auto& actualGroupLatencyMetrics,
                         auto& expectedGroupLatencyMetrics) {
    for (auto& [group, latency] : expectedGroupLatencyMetrics) {
      EXPECT_NEAR(getLatency(actualGroupLatencyMetrics, group), latency, 1e-8);
    }
  };

  assertEqual(
      actualInitialGroupLatencyMetrics, expectedInitialGroupLatencyPairs);
  assertEqual(actualFinalGroupLatencyMetrics, expectedFinalGroupLatencyPairs);
}

static void verifyTrafficMetrics(
    const std::string& routingConfigName,
    const AssignmentSolution& solution,
    const std::vector<std::tuple<
        std::string /*tenant*/,
        std::string /*sourceRegion*/,
        std::string /*destinationRegion*/,
        double>>& expectedInitialTenantRegionTraffic,
    const std::vector<
        std::tuple<std::string, std::string, std::string, double>>&
        expectedFinalTenantRegionTraffic) {
  auto& actualInitialTrafficMetrics =
      solution.initialMetrics()->routingConfigToGroupTrafficMetrics()->at(
          routingConfigName);
  auto& actualFinalTrafficMetrics =
      solution.finalMetrics()->routingConfigToGroupTrafficMetrics()->at(
          routingConfigName);

  auto assertEqual = [&](const auto& actualTrafficMetrics,
                         const auto& expectedTrafficMetrics) {
    for (auto& [tenant, sourceRegion, destinationRegion, traffic] :
         expectedTrafficMetrics) {
      auto& tenantSourceSummary = actualTrafficMetrics.groupToSourceTraffic()
                                      ->at(tenant)
                                      .sourceToDestinationTraffic()
                                      ->at(sourceRegion);
      auto value = folly::get_ptr(
          *tenantSourceSummary.scopeItemToValue(), destinationRegion);

      if (!value) {
        // not all destinations maybe part of the map associated with a
        // source; if a destination is not present, then traffic to it is 0
        EXPECT_NEAR(0.0, traffic, 1e-8);
      } else {
        EXPECT_NEAR(*value, traffic, 1e-8);
      }
    }
  };

  assertEqual(actualInitialTrafficMetrics, expectedInitialTenantRegionTraffic);
  assertEqual(actualFinalTrafficMetrics, expectedFinalTenantRegionTraffic);
}

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    RoutingLatencyTest,
    ::testing::Combine(testThreadCounts(), ::testing::Bool()));

TEST_P(RoutingLatencyTest, BasicAvg) {
  // minimize sum of average latencies of the tenants;
  // When using average latency metric, there is no difference between the test
  // cases 'IncludeAvgLatencyMetricIfViolated' and
  // 'DoNotIncludeAvgLatencyMetricIfViolated'
  auto metric = thriftUtils::makeRoutingLatencyMetric(
      interface::RoutingLatencyMetric::AVG);
  setUpProblem(false, metric);

  // Get a solution from Rebalancer.
  auto solution = solver_->solve();
  auto assignment = *solution.assignment();

  // In the initial solution:
  //  Service0:
  //    - 98% of traffic is from region0:
  //      -- all of it (98%) goes to region1 (10ms)
  //    - 2% of traffic is from region1
  //      -- 1% goes to region1 (0ms), 1% to region2 (15ms)
  //  avg latency = 0.98 * 10 + 0.01*0 + 0.01 * 15 = 9.8 + 0.15 = 9.95
  //
  //  Service1:
  //    - 100% of traffic is from region2:
  //      -- 50% goes to region0 (50ms), 50% to region1 (40ms)
  //  avg latency = 0.5*50 + 0.5*40 = 45

  // initial objective value = max(0, 9.95 - 0) + max(0, 45 - 0) = 54.95ms
  EXPECT_NEAR(54.95, *solution.initialObjective()->value(), 1e-8);

  // In the expected best solution:
  //
  // 1) move a replica of service0 from region1 to region0. This enables 98%
  // from region0 of traffic to be evenly spread across region0 (5ms) and
  // region2 (5ms). Also, after this 2% traffic from region 1 is equally spread
  // to region0(5ms) and region2 (15ms).
  // Overall avg latency to 0.98*5 + 0.01*5 + 0.01*15 = 4.9 + 0.2 = 5.10
  //
  // 2) move a replica of service1 to region2. This enables that the 100% of its
  // traffic can be sent to region2 (0ms latency).

  // final objective value = max(0, 5.10 - 0) + max(0, 0 - 0) = 5.10ms
  EXPECT_NEAR(5.10, *solution.finalObjective()->value(), 1e-8);

  std::unordered_map<std::string, std::unordered_map<std::string, int>>
      regionServiceCount;
  for (auto& [traffic, region] : assignment) {
    auto service = replicaToService_.at(traffic);
    ++regionServiceCount[region][service];
  }

  // we expect a replica of service0 to have moved from region1 to region0
  // we also expect that region2 to have a got a replica of service1
  EXPECT_EQ(1, regionServiceCount["region0"]["service0"]);
  EXPECT_EQ(0, regionServiceCount["region1"]["service0"]);
  EXPECT_EQ(1, regionServiceCount["region2"]["service0"]);
  EXPECT_EQ(1, regionServiceCount["region2"]["service1"]);

  // verify initial and final latency metrics
  verifyRoutingLatencyMetrics(
      metric,
      "routingConfig1",
      solution,
      {{"service0", 9.95}, {"service1", 45}}, // inital latency values
      {{"service0", 5.10}, {"service1", 0}}); // final latency values

  // verify initial and final traffic metrics
  verifyTrafficMetrics(
      "routingConfig1",
      solution,
      {
          {"service0", "region0", "region0", 0.0},
          {"service0", "region0", "region1", 0.98},
          {"service0", "region0", "region2", 0.0},
          //
          {"service0", "region1", "region0", 0.0},
          {"service0", "region1", "region1", 0.01},
          {"service0", "region1", "region2", 0.01},
          //
          {"service1", "region2", "region0", 0.5},
          {"service1", "region2", "region1", 0.5},
          {"service1", "region2", "region2", 0.0},
      }, // initial metrics (tenant, sourceRegion, destinationRegion, value)
      {
          {"service0", "region0", "region0", 0.49},
          {"service0", "region0", "region1", 0.0},
          {"service0", "region0", "region2", 0.49},
          //
          {"service0", "region1", "region0", 0.01},
          {"service0", "region1", "region1", 0.0},
          {"service0", "region1", "region2", 0.01},
          //
          {"service1", "region2", "region0", 0.0},
          {"service1", "region2", "region1", 0.0},
          {"service1", "region2", "region2", 1.0},
      } // final metrics (tenant, sourceRegion, destinationRegion, value)
  );
}

TEST_P(RoutingLatencyTest, BasicP99) {
  // minimize sum of P99 latencies of the tenants
  auto metric = thriftUtils::makeRoutingLatencyMetric(
      interface::RoutingLatencyMetric::PERCENTILE, 99);
  setUpProblem(
      false,
      metric,
      std::nullopt,
      0.0 /*globalLimit*/,
      2.0 /*weightForExtraAvgLatency*/);

  // Get a solution from Rebalancer.
  auto solution = solver_->solve();
  auto assignment = *solution.assignment();

  // In the initial solution:
  //  Service0:
  //    - 98% of traffic is from region0:
  //      -- all of it (98%) goes to region1 (10ms)
  //    - 2% of traffic is from region1
  //      -- 1% goes to region1 (0ms), 1% to region2 (15ms)
  //  p99 latency = 10ms
  //  avg latency = 0.98 * 10 + 0.01*0 + 0.01 * 15 = 9.8 + 0.15 = 9.95
  //
  //  Service1:
  //    - 100% of traffic is from region2:
  //      -- 50% goes to region0 (50ms), 50% to region1 (40ms)
  //  p99 latency = 50ms
  //  avg latency = 0.5*50 + 0.5*40 = 45ms

  if (includeAvgLatencyMetricIfViolated()) {
    // Service0's contribution to objective = max(0, 10-0) + 2.0 * 9.95 = 29.90
    // (there is a term (10 - 0) since 0 is the global limit, and a term (2.0
    // * 9.95) since there is a limit violation and weight is 2.0)

    // Service1's contribution to objective = max(0, 50-0) + 2.0 * 45 = 140
    // (there is a term (50 - 0) since 0 is the global limit, and a term (2.0
    // * 45) since there is a limit violation and weight is 2.0)

    // initial objective value = 29.9 + 140 = 169.90ms
    EXPECT_NEAR(169.90, *solution.initialObjective()->value(), 1e-8);
  } else {
    // initial objective value = max(0, 10-0) + max(0, 50-0) = 60ms
    EXPECT_NEAR(60, *solution.initialObjective()->value(), 1e-8);
  }

  // In the expected best solution:
  //
  // 1) move a replica of service0 from region1 to region0. This enables 98% of
  // traffic to be evenly spread across region0 (5ms) and region2 (5ms),
  // thus reducing its p99 latency to 5ms (since 2% total traffic from
  // region1 is equally spread across region0 and region2, and so region2
  // gets at most 1% of the total traffic; p100/max is still 15ms), and avg
  // latency to 0.98*5 + 0.01*5 + 0.01*15 = 5.1ms
  //
  // 2) move a replica of service1 to region2. This enables that the 100% of its
  // traffic can be sent to region2 (0ms latency). p99 and avg latency = 0.0ms

  if (includeAvgLatencyMetricIfViolated()) {
    // Service0's contribution to objective = max(0, 5-0) + 2 * 5.1 = 15.2
    // Service1's contribution to objective = max(0, 0-0) + 0 * 0 = 0

    // final objective value = 15.2 + 0 = 15.2ms
    EXPECT_NEAR(15.2, *solution.finalObjective()->value(), 1e-8);
  } else {
    // final objective value = max(0, 5 - 0) + max(0, 0 - 0) = 5ms
    EXPECT_NEAR(5, *solution.finalObjective()->value(), 1e-8);
  }

  std::unordered_map<std::string, std::unordered_map<std::string, int>>
      regionServiceCount;
  for (auto& [traffic, region] : assignment) {
    auto service = replicaToService_.at(traffic);
    ++regionServiceCount[region][service];
  }

  // we expect a replica of service0 to have moved from region1 to region0
  // we also expect that region2 to have a got a replica of service1
  EXPECT_EQ(1, regionServiceCount["region0"]["service0"]);
  EXPECT_EQ(0, regionServiceCount["region1"]["service0"]);
  EXPECT_EQ(1, regionServiceCount["region2"]["service0"]);
  EXPECT_EQ(1, regionServiceCount["region2"]["service1"]);

  // verify initial and final P99 latency metrics
  verifyRoutingLatencyMetrics(
      metric,
      "routingConfig1",
      solution,
      {{"service0", 10}, {"service1", 50}}, // inital P99 latency values
      {{"service0", 5}, {"service1", 0}}); // final P99 latency values

  if (includeAvgLatencyMetricIfViolated()) {
    // verify initial and final AVG latency metrics
    verifyRoutingLatencyMetrics(
        thriftUtils::makeRoutingLatencyMetric(
            interface::RoutingLatencyMetric::AVG),
        "routingConfig1",
        solution,
        {{"service0", 9.95}, {"service1", 45}}, // inital AVG latency values
        {{"service0", 5.10}, {"service1", 0}}); // final AVG latency values
  }
}

TEST_P(RoutingLatencyTest, BasicMAX) {
  // minimize sum of MAX/P100 latencies of the tenants
  auto metric = thriftUtils::makeRoutingLatencyMetric(
      interface::RoutingLatencyMetric::PERCENTILE, 100);
  setUpProblem(true, metric, std::nullopt, 10.0 /*globalLimit*/);

  // Get a solution from Rebalancer.
  auto solution = solver_->solve();
  auto assignment = *solution.assignment();

  // In the initial solution:
  //  Service0:
  //    - 98% of traffic is from region0:
  //      -- all of it (98%) goes to region1 (10ms)
  //    - 2% of traffic is from region1
  //      -- 1% goes to region1 (0ms), 1% to region2 (15ms)
  //  max/p100 latency value = 15ms
  //
  //  Service1:
  //    - 100% of traffic is from region2:
  //      -- 50% goes to region0 (50ms), 50% to region1 (40ms)
  //  max/p100 latency value = 50ms

  if (includeAvgLatencyMetricIfViolated()) {
    // Service0's contribution to objective = max(0, 15-10) + 1 * 9.95 = 14.95
    // (there is a term (15 - 10) since 10 is the global limit, and a term (1
    // * 9.95) since there is a limit violation)

    // Service1's contribution to objective = max(0, 50-10) + 1 * 45 = 85
    // (there is a term (50 - 10) since 10 is the global limit, and a term (1
    // * 45) since there is a limit violation)

    // initial objective value = 14.95 + 85 = 114.95ms
    EXPECT_NEAR(99.95, *solution.initialObjective()->value(), 1e-8);
  } else {
    // initial objective value = max(0, 15 - 10) + max(0, 50 - 10) = 45ms
    EXPECT_NEAR(45, *solution.initialObjective()->value(), 1e-8);
  }

  // In the expected best solution:
  //
  // 1) move a replica of service0 from region2 to region0. This enables 98% of
  // traffic to be sent to region1 (10ms) and 2% of the traffic from region1 to
  // be equally split between region0 (5ms) and region1 (0ms), thus resulting in
  // overall maxLatency of 10ms, and avg latency of 0.98*10 + 0.01*5 + 0.01*0
  // = 9.85;
  //
  // 2) move a replica of service1 to region2. This enables that the 100% of its
  // traffic can be sent to region2 (0ms latency). Both max and avg latencies
  // are zero;

  if (includeAvgLatencyMetricIfViolated()) {
    // Service0's contribution to objective = max(0, 10 - 10) + 0 * 9.85 = 0 (0
    // * 9.85 since there is no limit violation)
    // Service1's contribution to objective = max(0, 0 - 10) + 0 * 0 = 0

    // final objective value = 0 + 0 = 0ms
    EXPECT_NEAR(0.0, *solution.finalObjective()->value(), 1e-8);
  } else {
    // final objective value = max(0, 10 - 10) + max(0, 0 - 10) = 0ms
    EXPECT_NEAR(0.0, *solution.finalObjective()->value(), 1e-8);
  }

  std::unordered_map<std::string, std::unordered_map<std::string, int>>
      regionServiceCount;
  for (auto& [traffic, region] : assignment) {
    auto service = replicaToService_.at(traffic);
    ++regionServiceCount[region][service];
  }

  // we expect a replica of service0 to have moved from region2 to region0
  // we also expect that region2 to have a got a replica of service1
  EXPECT_EQ(1, regionServiceCount["region0"]["service0"]);
  EXPECT_EQ(1, regionServiceCount["region1"]["service0"]);
  EXPECT_EQ(0, regionServiceCount["region2"]["service0"]);
  EXPECT_EQ(1, regionServiceCount["region2"]["service1"]);

  // verify initial and final MAX latency metrics
  verifyRoutingLatencyMetrics(
      metric,
      "routingConfig1",
      solution,
      {{"service0", 15}, {"service1", 50}}, // inital MAX latency values
      {{"service0", 10}, {"service1", 0}}); // final MAX latency values

  if (includeAvgLatencyMetricIfViolated()) {
    // verify initial and final AVG latency metrics
    verifyRoutingLatencyMetrics(
        thriftUtils::makeRoutingLatencyMetric(
            interface::RoutingLatencyMetric::AVG),
        "routingConfig1",
        solution,
        {{"service0", 9.95}, {"service1", 45}}, // inital AVG latency values
        {{"service0", 9.85}, {"service1", 0}}); // final AVG latency values
  }
}

TEST_P(RoutingLatencyTest, BasicAvgWithFilter) {
  // The following is the exact same test as BasicAvg, but just that Service0 is
  // filtered out; for explanation see BasicAvg test

  Filter filter;
  filter.type() = FilterType::GROUP;
  filter.itemsBlacklist() = {"service0"};

  auto metric = thriftUtils::makeRoutingLatencyMetric(
      interface::RoutingLatencyMetric::AVG);
  setUpProblem(true, metric, std::move(filter));

  // Get a solution from Rebalancer.
  auto solution = solver_->solve();
  auto assignment = *solution.assignment();

  EXPECT_NEAR(45, *solution.initialObjective()->value(), 1e-8);
  EXPECT_NEAR(0, *solution.finalObjective()->value(), 1e-8);

  // verify initial and final latency metrics
  verifyRoutingLatencyMetrics(
      metric,
      "routingConfig1",
      solution,
      {{"service1", 45}}, // inital latency values
      {{"service1", 0}}); // final latency values
}

} // namespace facebook::rebalancer::interface::tests
