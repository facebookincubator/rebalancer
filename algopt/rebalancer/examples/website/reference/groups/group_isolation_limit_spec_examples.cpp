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
 * GroupIsolationLimitSpec Reference Examples

This file demonstrates all the usage patterns for GroupIsolationLimitSpec shown
in the reference documentation. Each function is a complete, runnable example.
 *
 * This file demonstrates all the usage patterns for GroupIsolationLimitSpec
shown in the
 * reference documentation. Each function is a complete, runnable example.
 */

// example_start
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
  /**
   * Quick example showing basic GroupIsolationLimitSpec usage.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {"task3", "task4", "task5"}},
      });
  solver.addPartition(
      "service",
      std::map<std::string, std::vector<std::string>>{
          {"svc_a", {"task0", "task1"}},
          {"svc_b", {"task2", "task3"}},
          {"svc_c", {"task4", "task5"}},
      });

  // quick_example_start
  // Limit each host to max 3 different services
  GroupIsolationLimitSpec spec;
  spec.name() = "limit-service-mixing";
  spec.scope() = "host";
  spec.partitionName() = "service";

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 3; // Max 3 different services per host
  spec.limit() = limit;

  spec.groupsAllowed() = 1; // Default (not typically changed)

  solver.addConstraint(spec);
  // quick_example_end
}

void limit_service_mixing() {
  /**
   * Prevent too many services on same host.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"web_0", "web_1", "api_0", "db_0"}},
          {"host1", {"web_2", "api_1", "api_2", "db_1", "cache_0", "cache_1"}},
      });

  // limit_service_mixing_start
  // Service partition
  std::map<std::string, std::vector<std::string>> services = {
      {"web", {"web_0", "web_1", "web_2"}},
      {"api", {"api_0", "api_1", "api_2"}},
      {"db", {"db_0", "db_1"}},
      {"cache", {"cache_0", "cache_1"}},
  };
  solver.addPartition("service", std::move(services));

  // Max 2 different services per host
  GroupIsolationLimitSpec spec;
  spec.name() = "limit-service-mixing";
  spec.scope() = "host";
  spec.partitionName() = "service";

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 2; // Max 2 services per host
  spec.limit() = std::move(limit);

  solver.addConstraint(std::move(spec));
  // limit_service_mixing_end
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "Running all examples...\n\n";

  std::cout << "1. Quick Example...\n";
  quick_example();

  std::cout << "2. Limit Service Mixing...\n";
  limit_service_mixing();

  std::cout << "\n✓ All examples completed successfully!\n";
  return 0;
}
// example_end
