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

/**
 * AggregatedGroupSpec Reference Examples
 *
 * This file demonstrates all usage patterns shown in the reference
 * documentation. Each function is a complete, runnable example.
 */

#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"

#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/init/Init.h>

#include <iostream>
#include <map>
#include <string>

using namespace facebook::rebalancer::interface;

void quick_example() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}},
          {"host1", {"task2", "task3"}},
      });
  solver.addObjectDimension(
      "memory",
      std::map<std::string, double>{
          {"task0", 10.0},
          {"task1", 20.0},
          {"task2", 15.0},
          {"task3", 25.0},
      });
  solver.addPartition(
      "service",
      std::map<std::string, std::vector<std::string>>{
          {"svc_a", {"task0", "task1"}},
          {"svc_b", {"task2", "task3"}},
      });

  // quick_example_start
  // Limit the maximum group total on any host to 100GB
  AggregatedGroupSpec spec;
  spec.name() = "max-group-per-host";
  spec.scope() = "host";
  spec.dimension() = "memory";
  spec.partitionName() = "service";

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 100.0;
  spec.limit() = limit;

  spec.withinGroupAggregationType() = AggregatedGroupSpecAggType::SUM;
  spec.groupAggregationType() = AggregatedGroupSpecAggType::MAX;
  spec.containerAggregationType() = AggregatedGroupSpecAggType::MAX;

  solver.addConstraint(spec);
  // quick_example_end
}

void peak_group_total() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("server");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"server0", {"shard0", "shard1"}},
          {"server1", {"shard2", "shard3"}},
      });
  solver.addObjectDimension(
      "storage",
      std::map<std::string, double>{
          {"shard0", 100.0},
          {"shard1", 200.0},
          {"shard2", 150.0},
          {"shard3", 250.0},
      });
  solver.addPartition(
      "database",
      std::map<std::string, std::vector<std::string>>{
          {"db_a", {"shard0", "shard1"}},
          {"db_b", {"shard2", "shard3"}},
      });

  // peak_group_total_start
  // Ensure largest database on each server doesn't exceed 500GB
  AggregatedGroupSpec spec;
  spec.name() = "peak-database-limit";
  spec.scope() = "server";
  spec.dimension() = "storage";
  spec.partitionName() = "database";

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 500.0;
  spec.limit() = std::move(limit);

  spec.withinGroupAggregationType() = AggregatedGroupSpecAggType::SUM;
  spec.groupAggregationType() = AggregatedGroupSpecAggType::MAX;
  spec.containerAggregationType() = AggregatedGroupSpecAggType::MAX;

  solver.addConstraint(std::move(spec));
  // peak_group_total_end
}

void top_n_groups() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}},
          {"host1", {"task2", "task3"}},
      });
  solver.addObjectDimension(
      "disk",
      std::map<std::string, double>{
          {"task0", 100.0},
          {"task1", 200.0},
          {"task2", 150.0},
          {"task3", 250.0},
      });
  solver.addPartition(
      "tenant",
      std::map<std::string, std::vector<std::string>>{
          {"tenant_a", {"task0", "task1"}},
          {"tenant_b", {"task2", "task3"}},
      });

  // top_n_groups_start
  // Total of all largest groups per host must not exceed 1TB
  AggregatedGroupSpec spec;
  spec.name() = "total-peak-groups";
  spec.scope() = "host";
  spec.dimension() = "disk";
  spec.partitionName() = "tenant";

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 1000.0;
  spec.limit() = std::move(limit);

  spec.withinGroupAggregationType() = AggregatedGroupSpecAggType::SUM;
  spec.groupAggregationType() = AggregatedGroupSpecAggType::MAX;
  spec.containerAggregationType() = AggregatedGroupSpecAggType::SUM;

  solver.addConstraint(std::move(spec));
  // top_n_groups_end
}

void weighted_aggregation() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host1", {"task0", "task1"}},
          {"host2", {"task2", "task3"}},
          {"host3", {"task4", "task5"}},
      });
  solver.addObjectDimension(
      "cpu",
      std::map<std::string, double>{
          {"task0", 5.0},
          {"task1", 10.0},
          {"task2", 8.0},
          {"task3", 12.0},
          {"task4", 6.0},
          {"task5", 9.0},
      });
  solver.addPartition(
      "service",
      std::map<std::string, std::vector<std::string>>{
          {"svc_a", {"task0", "task1", "task2"}},
          {"svc_b", {"task3", "task4", "task5"}},
      });

  // weighted_aggregation_start
  // Weighted aggregation with custom per-host contributions
  AggregatedGroupSpec spec;
  spec.name() = "weighted-capacity";
  spec.scope() = "host";
  spec.dimension() = "cpu";
  spec.partitionName() = "service";

  Limit globalLimit;
  globalLimit.type() = LimitType::ABSOLUTE;
  globalLimit.globalLimit() = 100.0;
  spec.limit() = std::move(globalLimit);

  spec.withinGroupAggregationType() = AggregatedGroupSpecAggType::SUM;
  spec.groupAggregationType() = AggregatedGroupSpecAggType::SUM;
  spec.containerAggregationType() = AggregatedGroupSpecAggType::SUM;

  // Set per-host weights
  std::map<std::string, Limit> contributions;

  Limit weight1;
  weight1.type() = LimitType::ABSOLUTE;
  weight1.globalLimit() = 2.0;
  contributions["host1"] = weight1;

  Limit weight2;
  weight2.type() = LimitType::ABSOLUTE;
  weight2.globalLimit() = 1.0;
  contributions["host2"] = weight2;

  Limit weight3;
  weight3.type() = LimitType::ABSOLUTE;
  weight3.globalLimit() = 0.5;
  contributions["host3"] = weight3;

  spec.contributions() = std::move(contributions);

  solver.addConstraint(std::move(spec));
  // weighted_aggregation_end
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "Running all AggregatedGroup examples...\n";

  std::cout << "\n1. Quick Example...\n";
  quick_example();

  std::cout << "\n2. Peak Group Total...\n";
  peak_group_total();

  std::cout << "\n3. Top N Groups...\n";
  top_n_groups();

  std::cout << "\n4. Weighted Aggregation...\n";
  weighted_aggregation();

  std::cout << "\nAll AggregatedGroup examples completed successfully!\n";
  return 0;
}
