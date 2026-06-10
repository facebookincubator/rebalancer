#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Canary Migration Strategy

Demonstrates how to migrate a small canary group first before proceeding with
full migration. This variation shows:
- Forced canary workload placement to new DC
- Validation phase before continuing
- Risk mitigation through staged rollout
"""

import random

from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    ConstraintSpecs,
    GoalSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    AvoidAssignment,
    AvoidAssignmentsSpec,
    AvoidMovingSpec,
    BalanceSpec,
    BalanceSpecFormula,
    CapacitySpec,
    MinimizeMovementSpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


# variation_start
def solve_canary_round(current_assignment, hosts_per_dc):
    """Round 1: Migrate canary workloads (5%) to new DC for validation."""
    solver = ProblemSolver(
        service_name="dc-migration-canary", service_scope="production"
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

    # Define canary workloads (small subset, ~5%)
    canary_workloads = [f"workload_{i}" for i in range(10)]
    print(f"  Canary workloads: {canary_workloads}")

    # FORCE canary workloads to new DC by forbidding old DC placement
    old_dc_hosts = [f"old_dc_host_{i}" for i in range(hosts_per_dc)]
    solver.addConstraint(
        ConstraintSpecs(
            avoidAssignmentsSpec=AvoidAssignmentsSpec(
                name="force-canary-to-new-dc",
                scope="host",
                assignments=[
                    AvoidAssignment(object=workload, scopeItems=old_dc_hosts)
                    for workload in canary_workloads
                ],
            )
        )
    )
    print(f"  Forcing {len(canary_workloads)} canary workloads to DC-NEW")

    # Balance goals
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-cpu",
                scope="host",
                dimension="cpu_usage",
                formula=BalanceSpecFormula.LEGACY,
            )
        ),
        weight=2.0,
    )

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

    print(f"  Canary migration complete in {solution.profile.solveTime}ms")

    return solution


# variation_end


def solve_full_migration_round(current_assignment, round_num, hosts_per_dc):
    """Rounds 2+: Proceed with full migration if canary successful."""
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

    # Balance goals
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-cpu",
                scope="host",
                dimension="cpu_usage",
                formula=BalanceSpecFormula.LEGACY,
            )
        ),
        weight=2.0,
    )

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

    print(f"  Migration round complete in {solution.profile.solveTime}ms")

    return solution


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


def validate_canary():
    """Simulate canary validation (in real world, check metrics)."""
    print("\n  Validating canary deployment...")
    print("  Checking error rates... OK")
    print("  Checking latency... OK")
    print("  Checking resource utilization... OK")
    print("  Canary validation PASSED")
    return True


def canary_migration():
    """Run migration with canary-first strategy."""
    WORKLOAD_COUNT = 200
    HOSTS_PER_DC = 30
    TOTAL_ROUNDS = 4  # Canary + 3 full rounds

    current_assignment = initialize_old_datacenter(WORKLOAD_COUNT, HOSTS_PER_DC)

    print("Starting canary migration strategy")
    print(f"{'=' * 80}\n")

    # ROUND 1: Canary migration (5%)
    print(f"ROUND 1: CANARY (5%)")
    print(f"{'=' * 80}")

    canary_solution = solve_canary_round(current_assignment, HOSTS_PER_DC)
    current_assignment = canary_solution.assignment

    new_dc_count = sum(
        len(workloads)
        for host, workloads in current_assignment.items()
        if host.startswith("new_dc_")
    )
    print(f"  Canary deployed: {new_dc_count}/200 workloads")

    # Validation phase (would wait hours/days in real world)
    if not validate_canary():
        print("\nCanary validation FAILED. Aborting migration.")
        return

    print("\n" + "=" * 80)
    print("CANARY SUCCESSFUL - Proceeding with full migration")
    print("=" * 80)

    # ROUNDS 2+: Full migration
    for round_num in range(1, TOTAL_ROUNDS):
        print(f"\nROUND {round_num + 1}: FULL MIGRATION")
        print(f"{'=' * 80}")

        solution = solve_full_migration_round(
            current_assignment,
            round_num,
            HOSTS_PER_DC,
        )

        current_assignment = solution.assignment

        new_dc_count = sum(
            len(workloads)
            for host, workloads in current_assignment.items()
            if host.startswith("new_dc_")
        )
        pct = (new_dc_count / WORKLOAD_COUNT) * 100

        print(
            f"  Round {round_num + 1} complete: {new_dc_count}/200 workloads ({pct:.1f}%)"
        )

    print("\nMigration complete!")


if __name__ == "__main__":
    canary_migration()
