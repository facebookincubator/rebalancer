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
 * WorkingSetSpec Reference Examples
 *
 * This file demonstrates all usage patterns shown in the reference
 * documentation. Each function is a complete, runnable example.
 */

#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"

#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/init/Init.h>

#include <iostream>

using namespace facebook::rebalancer::interface;

void quick_example() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("service");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"frontend", "backend"}},
          {"host1", {"cache"}},
      });
  solver.addObjectDimension(
      "network",
      std::map<std::string, double>{
          {"frontend", 1.0}, {"backend", 1.0}, {"cache", 1.0}});

  // quick_example_start
  // Optimize placement to minimize latency between coordinated services
  WorkingSetSpec spec;
  spec.name() = "service-coordination";
  spec.scope() = "host";
  spec.dimension() = "network";

  WorkingUnit unit;
  unit.endpoints() = {"frontend", "backend", "cache"};
  unit.weight() = 1.0;

  spec.workingUnits() = {unit};
  spec.metric() = WorkingSetMetric::AVG;

  solver.addGoal(spec, 1.0);
  // quick_example_end
}

void microservice_coordination() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("service");
  solver.setContainerName("datacenter");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"dc0", {"web-server", "auth-service", "session-cache"}},
          {"dc1", {"ingestion", "processing", "storage"}},
      });
  solver.addObjectDimension(
      "network",
      std::map<std::string, double>{
          {"web-server", 1.0},
          {"auth-service", 1.0},
          {"session-cache", 1.0},
          {"ingestion", 1.0},
          {"processing", 1.0},
          {"storage", 1.0}});

  // microservice_coordination_start
  // Multiple working units for different service groups
  WorkingSetSpec spec;
  spec.name() = "microservice-latency";
  spec.scope() = "datacenter";
  spec.dimension() = "network";

  WorkingUnit webTier;
  webTier.endpoints() = {"web-server", "auth-service", "session-cache"};
  webTier.weight() = 1.0;

  WorkingUnit dataPipeline;
  dataPipeline.endpoints() = {"ingestion", "processing", "storage"};
  dataPipeline.weight() = 1.0;

  spec.workingUnits() = {std::move(webTier), std::move(dataPipeline)};

  solver.addGoal(std::move(spec), 1.0);
  // microservice_coordination_end
}

void weighted_units() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("service");
  solver.setContainerName("rack");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"rack0", {"api-gateway", "user-db", "auth"}},
          {"rack1", {"batch-worker", "analytics-db"}},
      });
  solver.addObjectDimension(
      "network",
      std::map<std::string, double>{
          {"api-gateway", 1.0},
          {"user-db", 1.0},
          {"auth", 1.0},
          {"batch-worker", 1.0},
          {"analytics-db", 1.0}});

  // weighted_units_start
  // Critical path gets higher weight
  WorkingSetSpec spec;
  spec.name() = "weighted-coordination";
  spec.scope() = "rack";
  spec.dimension() = "network";

  WorkingUnit critical;
  critical.endpoints() = {"api-gateway", "user-db", "auth"};
  critical.weight() = 10.0;

  WorkingUnit nonCritical;
  nonCritical.endpoints() = {"batch-worker", "analytics-db"};
  nonCritical.weight() = 1.0;

  spec.workingUnits() = {std::move(critical), std::move(nonCritical)};

  solver.addGoal(std::move(spec), 1.0);
  // weighted_units_end
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "Running all WorkingSet examples...\n";

  std::cout << "\n1. Quick Example...\n";
  quick_example();

  std::cout << "\n2. Microservice Coordination...\n";
  microservice_coordination();

  std::cout << "\n3. Weighted Units...\n";
  weighted_units();

  std::cout << "\nAll WorkingSet examples completed successfully!\n";
  return 0;
}
