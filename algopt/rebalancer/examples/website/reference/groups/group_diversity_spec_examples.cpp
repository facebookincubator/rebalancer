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
 * GroupDiversitySpec Reference Examples
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
#include <vector>

using namespace facebook::rebalancer::interface;

void quick_example() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("rack");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"rack0", {"shard0", "shard1", "shard2"}},
          {"rack1", {"shard3", "shard4", "shard5"}},
          {"rack2", {"shard6", "shard7", "shard8"}},
      });
  solver.addObjectDimension(
      "storage",
      std::map<std::string, double>{
          {"shard0", 100.0},
          {"shard1", 200.0},
          {"shard2", 150.0},
          {"shard3", 250.0},
          {"shard4", 300.0},
          {"shard5", 100.0},
          {"shard6", 200.0},
          {"shard7", 150.0},
          {"shard8", 250.0},
      });
  solver.addPartition(
      "database",
      std::map<std::string, std::vector<std::string>>{
          {"db_a", {"shard0", "shard1", "shard2"}},
          {"db_b", {"shard3", "shard4", "shard5"}},
          {"db_c", {"shard6", "shard7", "shard8"}},
      });

  // quick_example_start
  // Each rack must have storage from at least 3 different databases
  GroupDiversitySpec spec;
  spec.name() = "rack-storage-diversity";
  spec.scope() = "rack";
  spec.partition() = "database";
  spec.dimension() = "storage";

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 3.0;
  spec.limit() = std::move(limit);

  spec.bound() = GroupDiversityBound::MIN;

  solver.addConstraint(std::move(spec));
  // quick_example_end
}

void min_diversity() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("rack");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"rack0", {"shard0", "shard1", "shard2", "shard3", "shard4"}},
          {"rack1", {"shard5", "shard6", "shard7", "shard8", "shard9"}},
      });
  solver.addObjectDimension(
      "disk",
      std::map<std::string, double>{
          {"shard0", 10.0},
          {"shard1", 20.0},
          {"shard2", 30.0},
          {"shard3", 15.0},
          {"shard4", 25.0},
          {"shard5", 10.0},
          {"shard6", 20.0},
          {"shard7", 30.0},
          {"shard8", 15.0},
          {"shard9", 25.0},
      });
  solver.addPartition(
      "database",
      std::map<std::string, std::vector<std::string>>{
          {"db_a", {"shard0", "shard5"}},
          {"db_b", {"shard1", "shard6"}},
          {"db_c", {"shard2", "shard7"}},
          {"db_d", {"shard3", "shard8"}},
          {"db_e", {"shard4", "shard9"}},
      });

  // min_diversity_start
  // Ensure each rack has data from at least 5 different databases
  GroupDiversitySpec spec;
  spec.name() = "min-database-diversity";
  spec.scope() = "rack";
  spec.partition() = "database";
  spec.dimension() = "disk";

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 5.0;
  spec.limit() = std::move(limit);

  spec.bound() = GroupDiversityBound::MIN;

  solver.addConstraint(std::move(spec));
  // min_diversity_end
}

void max_diversity() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {"task3", "task4", "task5"}},
      });
  solver.addObjectDimension(
      "cpu",
      std::map<std::string, double>{
          {"task0", 2.0},
          {"task1", 3.0},
          {"task2", 1.0},
          {"task3", 4.0},
          {"task4", 2.0},
          {"task5", 5.0},
      });
  solver.addPartition(
      "service",
      std::map<std::string, std::vector<std::string>>{
          {"svc_a", {"task0", "task1"}},
          {"svc_b", {"task2", "task3"}},
          {"svc_c", {"task4", "task5"}},
      });

  // max_diversity_start
  // Limit each host to at most 10 different services (by CPU usage)
  GroupDiversitySpec spec;
  spec.name() = "max-service-diversity";
  spec.scope() = "host";
  spec.partition() = "service";
  spec.dimension() = "cpu";

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 10.0;
  spec.limit() = std::move(limit);

  spec.bound() = GroupDiversityBound::MAX;

  solver.addConstraint(std::move(spec));
  // max_diversity_end
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "Running all GroupDiversity examples...\n";

  std::cout << "\n1. Quick Example...\n";
  quick_example();

  std::cout << "\n2. Min Diversity...\n";
  min_diversity();

  std::cout << "\n3. Max Diversity...\n";
  max_diversity();

  std::cout << "\nAll GroupDiversity examples completed successfully!\n";
  return 0;
}
