#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Complete Multi-Objective Optimization Example

This is a complete, runnable example demonstrating multi-objective optimization
for a storage cluster rebalancing problem. It shows how to:
- Set up an imbalanced initial assignment
- Define multiple dimensions (CPU, memory, data size)
- Use hybrid approach (tuples + weights) to balance objectives
- Analyze the results to understand the trade-offs
"""

import random

from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    GoalSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    BalanceSpec,
    BalanceSpecFormula,
    MinimizeMovementSpec,
    Priority,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)

# solution_start


def create_imbalanced_assignment():
    """Create an imbalanced initial state."""
    assignment = {}

    # 20 hosts
    for i in range(20):
        assignment[f"host{i}"] = []

    # 100 nodes - concentrate on first 5 hosts (imbalanced)
    for i in range(100):
        host_idx = i % 5  # Only use first 5 hosts
        assignment[f"host{host_idx}"].append(f"node{i}")

    return assignment


def solve_multi_objective():
    """Demonstrate multi-objective optimization."""
    solver = ProblemSolver(
        service_name="storage-rebalancer", service_scope="production"
    )

    solver.setObjectName("storage_node")
    solver.setContainerName("host")

    # Initial imbalanced assignment
    assignment = create_imbalanced_assignment()
    solver.setAssignment(assignment)

    # Node CPU usage (varies 0.5 to 8.0 cores)
    cpu_usage = {f"node{i}": 0.5 + random.random() * 7.5 for i in range(100)}
    solver.addObjectDimension("cpu_usage", cpu_usage)

    # Node memory usage (varies 1 to 16 GB)
    memory_usage = {f"node{i}": 1.0 + random.random() * 15.0 for i in range(100)}
    solver.addObjectDimension("memory_usage", memory_usage)

    # Node data size for movement cost (varies 10 to 500 GB)
    data_size = {f"node{i}": 10.0 + random.random() * 490.0 for i in range(100)}
    solver.addObjectDimension("data_size", data_size)

    # APPROACH: Hybrid (tuples + weights)
    # Tuple 0: Balance both CPU and memory (critical)
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-cpu",
                scope="host",
                dimension="cpu_usage",
                formula=BalanceSpecFormula.LEGACY,
                fixAverageToInitial=True,
            )
        ),
        weight=2.0,  # Slightly prefer CPU balance
        priority=Priority(tuple=0),
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-memory",
                scope="host",
                dimension="memory_usage",
                formula=BalanceSpecFormula.LEGACY,
                fixAverageToInitial=True,
            )
        ),
        weight=1.0,  # Within same tuple
        priority=Priority(tuple=0),
    )

    # Tuple 1: Minimize movement (secondary)
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-movement",
                scope="host",
                dimension="data_size",
            )
        ),
        weight=1.0,
        priority=Priority(tuple=1),
    )

    # Solve
    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=30000))
    )
    solution = solver.solve()

    # Analyze results
    print(f"Solution found in {solution.profile.solveTime}ms")
    print(f"Objective value: {solution.objectiveValue}")
    print(f"Nodes moved: {solution.profile.moveCount}")

    # Calculate utilization statistics
    host_cpu = {}
    host_memory = {}

    for host, nodes in solution.assignment.items():
        host_cpu[host] = sum(cpu_usage.get(n, 0) for n in nodes)
        host_memory[host] = sum(memory_usage.get(n, 0) for n in nodes)

    cpu_values = list(host_cpu.values())
    memory_values = list(host_memory.values())

    cpu_min, cpu_max = min(cpu_values), max(cpu_values)
    cpu_avg = sum(cpu_values) / len(cpu_values)
    mem_min, mem_max = min(memory_values), max(memory_values)
    mem_avg = sum(memory_values) / len(memory_values)

    print(f"\nCPU utilization:")
    print(f"  Min: {cpu_min:.1f}, Max: {cpu_max:.1f}, Avg: {cpu_avg:.1f}")
    print(
        f"  Range: {cpu_max - cpu_min:.1f} cores ({((cpu_max - cpu_min) / cpu_avg * 100):.1f}% of avg)"
    )

    print(f"\nMemory utilization:")
    print(f"  Min: {mem_min:.1f}, Max: {mem_max:.1f}, Avg: {mem_avg:.1f} GB")
    print(
        f"  Range: {mem_max - mem_min:.1f} GB ({((mem_max - mem_min) / mem_avg * 100):.1f}% of avg)"
    )

    data_moved = sum(
        data_size[node]
        for host, nodes in solution.assignment.items()
        for node in nodes
        if node not in assignment.get(host, [])
    )
    print(f"\nData moved: {data_moved:.1f} GB")

    return solution


# solution_end


if __name__ == "__main__":
    solve_multi_objective()
