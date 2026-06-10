#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Time-Based Migration Gates

Demonstrates how to implement approval gates between migration rounds with
preview and validation steps. This variation shows:
- Previewing migration plan before execution
- Manual approval gates
- Configurable wait periods between rounds
- Ability to pause and adjust migration
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
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


def solve_migration_round(current_assignment, round_num, hosts_per_dc):
    """Solve migration for a single round."""
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
            balanceSpec=BalanceSpec(
                name="balance-memory",
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

    return solution


def print_round_preview(solution, old_assignment, round_num):
    """Display preview of migration plan before execution."""
    print(f"\n{'=' * 80}")
    print(f"MIGRATION PLAN PREVIEW - ROUND {round_num}")
    print(f"{'=' * 80}")

    new_assignment = solution.assignment

    # Count workloads in each DC
    old_dc_count = sum(
        len(workloads)
        for host, workloads in new_assignment.items()
        if host.startswith("old_dc_")
    )
    new_dc_count = sum(
        len(workloads)
        for host, workloads in new_assignment.items()
        if host.startswith("new_dc_")
    )

    # Count moves
    moved_count = 0
    moves_to_new_dc = 0
    for workload_id in range(200):
        workload_name = f"workload_{workload_id}"

        old_host = None
        for host, workloads in old_assignment.items():
            if workload_name in workloads:
                old_host = host
                break

        new_host = None
        for host, workloads in new_assignment.items():
            if workload_name in workloads:
                new_host = host
                break

        if old_host != new_host:
            moved_count += 1
            if new_host and new_host.startswith("new_dc_"):
                moves_to_new_dc += 1

    print(f"\nProposed Changes:")
    print(f"  Workloads to move: {moved_count}")
    print(f"  Moving to DC-NEW: {moves_to_new_dc}")
    print(f"  Moving to DC-OLD: {moved_count - moves_to_new_dc}")
    print(f"\nResulting Distribution:")
    print(f"  DC-OLD: {old_dc_count}/200 ({(old_dc_count / 200) * 100:.1f}%)")
    print(f"  DC-NEW: {new_dc_count}/200 ({(new_dc_count / 200) * 100:.1f}%)")
    print(f"\nSolution Quality:")
    print(f"  Objective value: {solution.objectiveValue}")
    print(f"  Solve time: {solution.profile.solveTime}ms")
    print(f"{'=' * 80}")


# variation_start
def migrate_with_gates():
    """
    Migration with manual approval gates.

    Features:
    - Preview migration plan before execution
    - Manual approval required for each round
    - Ability to pause and adjust parameters
    - Configurable wait periods between rounds
    """
    TOTAL_ROUNDS = 5
    WORKLOAD_COUNT = 200
    HOSTS_PER_DC = 30
    WAIT_HOURS = 24  # Wait period between rounds (hours)

    current_assignment = initialize_old_datacenter(WORKLOAD_COUNT, HOSTS_PER_DC)

    print("Starting migration with approval gates")
    print(f"{'=' * 80}")
    print(f"Total rounds: {TOTAL_ROUNDS}")
    print(f"Wait period between rounds: {WAIT_HOURS} hours")
    print(f"{'=' * 80}\n")

    for round_num in range(TOTAL_ROUNDS):
        print(f"\n{'=' * 80}")
        print(f"ROUND {round_num + 1}/{TOTAL_ROUNDS}")
        print(f"{'=' * 80}")

        # Solve round to get migration plan
        print("\nComputing migration plan...")
        solution = solve_migration_round(
            current_assignment,
            round_num,
            HOSTS_PER_DC,
        )

        # Show preview of what will happen
        print_round_preview(solution, current_assignment, round_num + 1)

        # Wait for manual approval
        approval = input("\nProceed with this round? (yes/no/abort): ").strip().lower()

        if approval == "abort":
            print("\nMigration ABORTED by user.")
            print("Current state preserved. No changes applied.")
            return

        if approval != "yes":
            print("\nRound SKIPPED. Migration paused.")
            print("You can adjust parameters and retry this round.")
            print("Current state preserved.")
            break

        # Apply the migration
        print("\nApplying migration moves...")
        current_assignment = solution.assignment

        # Count actual result
        new_dc_count = sum(
            len(workloads)
            for host, workloads in current_assignment.items()
            if host.startswith("new_dc_")
        )

        print(f"✓ Round {round_num + 1} complete: {new_dc_count}/200 in DC-NEW")

        # Wait period before next round (simulated)
        if round_num < TOTAL_ROUNDS - 1:
            print(f"\nWaiting {WAIT_HOURS} hours before next round...")
            print("(In production, monitor metrics during this period)")
            print("(Simulating wait - press Enter to continue)")
            input()
            # In real world: time.sleep(WAIT_HOURS * 3600)

    print("\nMigration sequence complete!")


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


if __name__ == "__main__":
    migrate_with_gates()
