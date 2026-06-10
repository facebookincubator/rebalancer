#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Gradual Datacenter Migration Example

Demonstrates how to migrate workloads gradually from an old datacenter to a new
datacenter over multiple rounds to minimize risk and disruption. This example
shows:
- Multi-round migration strategy
- Pinning already-migrated workloads to prevent ping-pong
- Protecting critical workloads in early rounds
- Gradually freezing the old datacenter
- Balancing load during migration
"""

import random

# solution_start
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


def gradual_datacenter_migration():
    """Migrate workloads gradually from old DC to new DC over multiple rounds."""

    # Migration configuration
    TOTAL_ROUNDS = 5
    WORKLOAD_COUNT = 200
    HOSTS_PER_DC = 30

    # Initial state: All workloads in DC-OLD
    current_assignment = initialize_old_datacenter(WORKLOAD_COUNT, HOSTS_PER_DC)

    # Track migration progress
    migration_history = []

    for round_num in range(TOTAL_ROUNDS):
        print(f"\n{'=' * 80}")
        print(f"ROUND {round_num + 1}/{TOTAL_ROUNDS}")
        print(f"{'=' * 80}")

        # Determine migration target for this round
        target_migrated_pct = ((round_num + 1) / TOTAL_ROUNDS) * 100
        target_migrated_count = int((target_migrated_pct / 100) * WORKLOAD_COUNT)

        print(
            f"Target: {target_migrated_count} workloads in DC-NEW ({target_migrated_pct:.0f}%)"
        )

        # Solve for this round
        solution = solve_migration_round(
            current_assignment=current_assignment,
            round_num=round_num,
            total_rounds=TOTAL_ROUNDS,
            target_new_dc_count=target_migrated_count,
            hosts_per_dc=HOSTS_PER_DC,
        )

        # Analyze round results
        round_stats = analyze_round(
            solution.assignment,
            current_assignment,
            round_num,
        )

        migration_history.append(round_stats)

        # Update current state for next round
        current_assignment = solution.assignment

        print(f"\nRound {round_num + 1} complete:")
        print(f"  Workloads in DC-NEW: {round_stats['new_dc_count']}")
        print(f"  Workloads moved this round: {round_stats['moved_count']}")
        print(f"  Cumulative moves: {sum(r['moved_count'] for r in migration_history)}")

    # Final summary
    print_migration_summary(migration_history)

    return migration_history


def initialize_old_datacenter(workload_count, hosts_per_dc):
    """Create initial assignment: all workloads in DC-OLD."""
    assignment = {}

    # DC-OLD hosts
    for i in range(hosts_per_dc):
        assignment[f"old_dc_host_{i}"] = []

    # DC-NEW hosts (empty initially)
    for i in range(hosts_per_dc):
        assignment[f"new_dc_host_{i}"] = []

    # Distribute workloads across old DC (imbalanced)
    for i in range(workload_count):
        host_idx = i % hosts_per_dc
        assignment[f"old_dc_host_{host_idx}"].append(f"workload_{i}")

    return assignment


def solve_migration_round(
    current_assignment,
    round_num,
    total_rounds,
    target_new_dc_count,
    hosts_per_dc,
):
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
        **{f"old_dc_host_{i}": 20.0 for i in range(hosts_per_dc)},  # Old: 20 cores
        **{
            f"new_dc_host_{i}": 32.0 for i in range(hosts_per_dc)
        },  # New: 32 cores (better)
    }
    host_mem_capacity = {
        **{f"old_dc_host_{i}": 32.0 for i in range(hosts_per_dc)},  # Old: 32 GB
        **{
            f"new_dc_host_{i}": 64.0 for i in range(hosts_per_dc)
        },  # New: 64 GB (better)
    }

    solver.addContainerDimension("cpu_capacity", host_cpu_capacity)
    solver.addContainerDimension("memory_capacity", host_mem_capacity)

    # CONSTRAINT: Capacity limits
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

    # STRATEGY: Pin workloads already migrated to new DC
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

    # STRATEGY: In early rounds, keep critical workloads stable
    if round_num < total_rounds - 2:  # First 3 of 5 rounds
        critical_workloads = [
            f"workload_{i}" for i in range(0, 20)
        ]  # First 20 are critical
        solver.addConstraint(
            ConstraintSpecs(
                avoidMovingSpec=AvoidMovingSpec(
                    name="pin-critical-early",
                    objects=critical_workloads,
                )
            )
        )
        print(f"  Pinned {len(critical_workloads)} critical workloads (early round)")

    # STRATEGY: Block old DC from accepting NEW placements (gradual freeze)
    # In later rounds, start freezing old DC
    if round_num >= total_rounds // 2:  # Round 3+ of 5
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
        print(f"  Old DC frozen (no new placements)")

    # GOAL 1: Balance within each datacenter
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

    # GOAL 2: Minimize movement
    # Higher weight in later rounds (more stable)
    movement_weight = 0.5 + (round_num / total_rounds) * 2.0
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-movement",
                scope="host",
                dimension="cpu_usage",
            )
        ),
        weight=movement_weight,
    )

    # Solve
    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=30000))
    )
    solution = solver.solve()

    print(f"  Solution found in {solution.profile.solveTime}ms")
    print(f"  Objective value: {solution.objectiveValue}")

    return solution


def analyze_round(new_assignment, old_assignment, round_num):
    """Analyze results of a migration round."""
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
    for workload in range(200):
        workload_name = f"workload_{workload}"

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

    return {
        "round": round_num + 1,
        "old_dc_count": old_dc_count,
        "new_dc_count": new_dc_count,
        "moved_count": moved_count,
    }


def print_migration_summary(migration_history):
    """Print summary of entire migration."""
    print(f"\n{'=' * 80}")
    print("MIGRATION SUMMARY")
    print(f"{'=' * 80}")
    print(f"{'Round':<8} {'DC-OLD':<12} {'DC-NEW':<12} {'Moved':<12} {'Cumulative'}")
    print("-" * 80)

    cumulative_moves = 0
    for stats in migration_history:
        cumulative_moves += stats["moved_count"]
        print(
            f"{stats['round']:<8} {stats['old_dc_count']:<12} "
            f"{stats['new_dc_count']:<12} {stats['moved_count']:<12} {cumulative_moves}"
        )

    total_rounds = len(migration_history)
    avg_moves_per_round = cumulative_moves / total_rounds

    print(f"\nTotal moves: {cumulative_moves}")
    print(f"Average per round: {avg_moves_per_round:.1f}")
    print(f"Final state: {migration_history[-1]['new_dc_count']}/200 in DC-NEW")


if __name__ == "__main__":
    gradual_datacenter_migration()
# solution_end
