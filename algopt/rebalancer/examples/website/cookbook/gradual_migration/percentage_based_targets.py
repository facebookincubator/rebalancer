#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Percentage-Based Migration Targets

Demonstrates how to configure explicit percentage targets for each migration
round with different strategies per round. This variation shows:
- Custom migration schedule with specific percentages
- Different strategies for different rounds
- Flexibility in migration pacing
"""

import random

from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    ConstraintSpecs,
    GoalSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    AvoidMovingSpec,
    BalanceSpec,
    BalanceSpecFormula,
    CapacitySpec,
    MinimizeMovementSpec,
    NonAcceptingSpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


# variation_start
# Define custom migration schedule with specific targets and strategies per round
migration_schedule = [
    {"round": 1, "target_pct": 10, "pin_critical": True},
    {"round": 2, "target_pct": 30, "pin_critical": True},
    {"round": 3, "target_pct": 60, "pin_critical": False},
    {"round": 4, "target_pct": 90, "freeze_old": True},
    {"round": 5, "target_pct": 100, "freeze_old": True},
]


def solve_with_custom_schedule(current_assignment, round_config, hosts_per_dc):
    """Solve migration round using custom schedule configuration."""
    round_num = round_config["round"] - 1
    target_pct = round_config["target_pct"]
    pin_critical = round_config.get("pin_critical", False)
    freeze_old = round_config.get("freeze_old", False)

    solver = ProblemSolver(
        service_name=f"dc-migration-round-{round_num}", service_scope="production"
    )

    solver.setObjectName("workload")
    solver.setContainerName("host")
    solver.setAssignment(current_assignment)

    # Define datacenter scope
    host_to_dc = {}
    for i in range(hosts_per_dc):
        host_to_dc[f"old_dc_host_{i}"] = "dc_old"
        host_to_dc[f"new_dc_host_{i}"] = "dc_new"
    solver.addScope("datacenter", host_to_dc)

    # Workload resource usage
    workload_cpu = {f"workload_{i}": 2.0 + random.random() * 4.0 for i in range(200)}
    workload_mem = {f"workload_{i}": 4.0 + random.random() * 8.0 for i in range(200)}

    solver.addObjectDimension("cpu_usage", workload_cpu)
    solver.addObjectDimension("memory_usage", workload_mem)

    # Host capacities
    host_cpu_capacity = {
        **{f"old_dc_host_{i}": 20.0 for i in range(hosts_per_dc)},
        **{f"new_dc_host_{i}": 32.0 for i in range(hosts_per_dc)},
    }
    host_mem_capacity = {
        **{f"old_dc_host_{i}": 32.0 for i in range(hosts_per_dc)},
        **{f"new_dc_host_{i}": 64.0 for i in range(hosts_per_dc)},
    }

    solver.addContainerDimension("cpu_capacity", host_cpu_capacity)
    solver.addContainerDimension("memory_capacity", host_mem_capacity)

    # Capacity constraints
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="cpu-capacity", scope="host", dimension="cpu_usage"
            )
        )
    )
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity", scope="host", dimension="memory_usage"
            )
        )
    )

    # Pin already-migrated workloads
    already_migrated = []
    for host, workloads in current_assignment.items():
        if host.startswith("new_dc_"):
            already_migrated.extend(workloads)

    if already_migrated:
        solver.addConstraint(
            ConstraintSpecs(
                avoidMovingSpec=AvoidMovingSpec(
                    name="pin-already-migrated",
                    objects=already_migrated,
                )
            )
        )
        print(f"  Pinned {len(already_migrated)} already-migrated workloads")

    # Pin critical workloads if specified in schedule
    if pin_critical:
        critical_workloads = [f"workload_{i}" for i in range(0, 20)]
        solver.addConstraint(
            ConstraintSpecs(
                avoidMovingSpec=AvoidMovingSpec(
                    name="pin-critical-early",
                    objects=critical_workloads,
                )
            )
        )
        print(f"  Pinned {len(critical_workloads)} critical workloads (per schedule)")

    # Freeze old DC if specified in schedule
    if freeze_old:
        old_dc_hosts = [f"old_dc_host_{i}" for i in range(hosts_per_dc)]
        solver.addConstraint(
            ConstraintSpecs(
                nonAcceptingSpec=NonAcceptingSpec(
                    name="freeze-old-dc",
                    scope="host",
                    items=old_dc_hosts,
                )
            )
        )
        print(f"  Old DC frozen (per schedule)")

    # Balance goals
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-cpu-per-dc",
                scope="host",
                dimension="cpu_usage",
                formula=BalanceSpecFormula.LEGACY,
            )
        ),
        weight=2.0,
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-memory-per-dc",
                scope="host",
                dimension="memory_usage",
                formula=BalanceSpecFormula.LEGACY,
            )
        ),
        weight=2.0,
    )

    # Minimize movement
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-movement",
                scope="host",
                dimension="cpu_usage",
            )
        ),
        weight=1.0,
    )

    # Solve
    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=30000))
    )
    solution = solver.solve()

    print(f"  Target: {target_pct}% in DC-NEW")
    print(f"  Solution found in {solution.profile.solveTime}ms")

    return solution


# variation_end


def initialize_old_datacenter(workload_count, hosts_per_dc):
    """Create initial assignment: all workloads in DC-OLD."""
    assignment = {}

    for i in range(hosts_per_dc):
        assignment[f"old_dc_host_{i}"] = []
        assignment[f"new_dc_host_{i}"] = []

    for i in range(workload_count):
        host_idx = i % hosts_per_dc
        assignment[f"old_dc_host_{host_idx}"].append(f"workload_{i}")

    return assignment


def percentage_based_migration():
    """Run migration using percentage-based custom schedule."""
    WORKLOAD_COUNT = 200
    HOSTS_PER_DC = 30

    current_assignment = initialize_old_datacenter(WORKLOAD_COUNT, HOSTS_PER_DC)

    print("Starting migration with custom percentage-based schedule")
    print(f"{'=' * 80}\n")

    for round_config in migration_schedule:
        print(f"\nROUND {round_config['round']}")
        print(f"{'=' * 80}")

        solution = solve_with_custom_schedule(
            current_assignment,
            round_config,
            HOSTS_PER_DC,
        )

        current_assignment = solution.assignment

        # Count workloads in new DC
        new_dc_count = sum(
            len(workloads)
            for host, workloads in current_assignment.items()
            if host.startswith("new_dc_")
        )
        actual_pct = (new_dc_count / WORKLOAD_COUNT) * 100

        print(f"\nRound {round_config['round']} complete:")
        print(f"  Target: {round_config['target_pct']}%")
        print(f"  Actual: {new_dc_count}/200 workloads ({actual_pct:.1f}%)")

    print("\nMigration complete!")


if __name__ == "__main__":
    percentage_based_migration()
