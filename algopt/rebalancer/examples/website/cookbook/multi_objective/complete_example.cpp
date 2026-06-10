// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

/**
 * Complete Multi-Objective Optimization Example
 *
 * This is a complete, runnable example demonstrating multi-objective
 * optimization for a storage cluster rebalancing problem. It shows how to:
 * - Set up an imbalanced initial assignment
 * - Define multiple dimensions (CPU, memory, data size)
 * - Use hybrid approach (tuples + weights) to balance objectives
 * - Analyze the results to understand the trade-offs
 */

#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/SolverSpecs_types.h"

#include <fmt/format.h>
#include <folly/container/irange.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/init/Init.h>

#include <iostream>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

// solution_start

std::map<std::string, std::vector<std::string>> create_imbalanced_assignment() {
  // Create an imbalanced initial state
  std::map<std::string, std::vector<std::string>> assignment;

  // 20 hosts
  for (const auto i : folly::irange(20)) {
    assignment[fmt::format("host{}", i)] = {};
  }

  // 100 nodes - concentrate on first 5 hosts (imbalanced)
  for (const auto i : folly::irange(100)) {
    int host_idx = i % 5; // Only use first 5 hosts
    assignment[fmt::format("host{}", host_idx)].push_back(
        fmt::format("node{}", i));
  }

  return assignment;
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  // Demonstrate multi-objective optimization
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "storage-rebalancer", "production");

  solver.setObjectName("storage_node");
  solver.setContainerName("host");

  // Initial imbalanced assignment
  auto assignment = create_imbalanced_assignment();
  solver.setAssignment(assignment);

  // Random number generator
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> cpu_dist(0.5, 8.0);
  std::uniform_real_distribution<> mem_dist(1.0, 16.0);
  std::uniform_real_distribution<> data_dist(10.0, 500.0);

  // Node CPU usage (varies 0.5 to 8.0 cores)
  std::map<std::string, double> cpu_usage;
  for (const auto i : folly::irange(100)) {
    cpu_usage[fmt::format("node{}", i)] = cpu_dist(gen);
  }
  solver.addObjectDimension("cpu_usage", cpu_usage);

  // Node memory usage (varies 1 to 16 GB)
  std::map<std::string, double> memory_usage;
  for (const auto i : folly::irange(100)) {
    memory_usage[fmt::format("node{}", i)] = mem_dist(gen);
  }
  solver.addObjectDimension("memory_usage", memory_usage);

  // Node data size for movement cost (varies 10 to 500 GB)
  std::map<std::string, double> data_size;
  for (const auto i : folly::irange(100)) {
    data_size[fmt::format("node{}", i)] = data_dist(gen);
  }
  solver.addObjectDimension("data_size", data_size);

  // APPROACH: Hybrid (tuples + weights)
  // Tuple 0: Balance both CPU and memory (critical)
  BalanceSpec balanceCpu;
  balanceCpu.name() = "balance-cpu";
  balanceCpu.scope() = "host";
  balanceCpu.dimension() = "cpu_usage";
  balanceCpu.formula() = BalanceSpecFormula::LEGACY;
  balanceCpu.fixAverageToInitial() = true;

  solver.addGoal(balanceCpu, 2.0, 0); // Slightly prefer CPU balance

  BalanceSpec balanceMem;
  balanceMem.name() = "balance-memory";
  balanceMem.scope() = "host";
  balanceMem.dimension() = "memory_usage";
  balanceMem.formula() = BalanceSpecFormula::LEGACY;
  balanceMem.fixAverageToInitial() = true;

  solver.addGoal(balanceMem, 1.0, 0); // Within same tuple

  // Tuple 1: Minimize movement (secondary)
  MinimizeMovementSpec minMove;
  minMove.name() = "minimize-movement";
  minMove.scope() = "host";
  minMove.dimension() = "data_size";

  solver.addGoal(minMove, 1.0, 1);

  // Solve
  LocalSearchSolverSpec localSearch;
  localSearch.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  localSearch.solveTime() = 30000;
  solver.addSolver(localSearch);

  auto solution = solver.solve();

  // Analyze results
  std::cout << fmt::format(
      "AssignmentSolution found in {}ms\n", "<solve time>");
  std::cout << fmt::format(
      "Objective value: {}\n", *solution.finalObjective()->value());
  std::cout << fmt::format("Nodes moved: {}\n", "<move count>");

  // Calculate utilization statistics
  std::map<std::string, double> host_cpu;
  std::map<std::string, double> host_memory;

  // Build reverse map: host -> total usage
  for (const auto& [node, host] : *solution.assignment()) {
    if (host_cpu.find(host) == host_cpu.end()) {
      host_cpu[host] = 0.0;
      host_memory[host] = 0.0;
    }
    host_cpu[host] += cpu_usage.at(node);
    host_memory[host] += memory_usage.at(node);
  }

  std::vector<double> cpu_values;
  std::vector<double> memory_values;
  for (const auto& [host, val] : host_cpu) {
    cpu_values.push_back(val);
    memory_values.push_back(host_memory[host]);
  }

  auto cpu_min = *std::min_element(cpu_values.begin(), cpu_values.end());
  auto cpu_max = *std::max_element(cpu_values.begin(), cpu_values.end());
  auto cpu_avg = std::accumulate(cpu_values.begin(), cpu_values.end(), 0.0) /
      cpu_values.size();

  auto mem_min = *std::min_element(memory_values.begin(), memory_values.end());
  auto mem_max = *std::max_element(memory_values.begin(), memory_values.end());
  auto mem_avg =
      std::accumulate(memory_values.begin(), memory_values.end(), 0.0) /
      memory_values.size();

  std::cout << fmt::format("\nCPU utilization:\n");
  std::cout << fmt::format(
      "  Min: {:.1f}, Max: {:.1f}, Avg: {:.1f}\n", cpu_min, cpu_max, cpu_avg);
  std::cout << fmt::format(
      "  Range: {:.1f} cores ({:.1f}% of avg)\n",
      cpu_max - cpu_min,
      (cpu_max - cpu_min) / cpu_avg * 100);

  std::cout << fmt::format("\nMemory utilization:\n");
  std::cout << fmt::format(
      "  Min: {:.1f}, Max: {:.1f}, Avg: {:.1f} GB\n",
      mem_min,
      mem_max,
      mem_avg);
  std::cout << fmt::format(
      "  Range: {:.1f} GB ({:.1f}% of avg)\n",
      mem_max - mem_min,
      (mem_max - mem_min) / mem_avg * 100);

  // Calculate data moved
  // Build reverse map of original assignment: object -> original_host
  std::map<std::string, std::string> original_location;
  for (const auto& [host, nodes] : assignment) {
    for (const auto& node : nodes) {
      original_location[node] = host;
    }
  }

  double data_moved = 0.0;
  for (const auto& [node, new_host] : *solution.assignment()) {
    auto it = original_location.find(node);
    if (it == original_location.end() || it->second != new_host) {
      data_moved += data_size.at(node);
    }
  }

  std::cout << fmt::format("\nData moved: {:.1f} GB\n", data_moved);

  return 0;
}

// solution_end
