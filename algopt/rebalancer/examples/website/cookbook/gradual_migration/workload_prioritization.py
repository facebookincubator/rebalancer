#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Workload Prioritization Strategy

Demonstrates how to migrate workloads by priority tiers, moving low-risk
workloads first and critical workloads last. This variation shows:
- Categorizing workloads into risk tiers
- Staged migration based on risk level
- Protecting critical workloads until final rounds
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


# variation_start
# Define workload tiers by risk level
workload_tiers = {
    "tier1_low_risk": [f"workload_{i}" for i in range(0, 50)],  # 50 low-risk workloads
    "tier2_medium_risk": [
        f"workload_{i}" for i in range(50, 150)
    ],  # 100 medium-risk workloads
    "tier3_critical": [
        f"workload_{i}" for i in range(150, 200)
    ],  # 50 critical workloads
}


def solve_prioritized_round(current_assignment, round_num, total_rounds, hosts_per_dc):
    """
    Solve migration round with workload prioritization.

    Round 1-2: Tier 1 only (low risk)
    Round 3-4: Tier 1 + Tier 2 (low + medium risk)
    Round 5: All tiers (including critical)
    """
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

    # Determine which tiers to protect based on round number
    workloads_to_pin = []

    if round_num < 2:  # Round 1-2: Only migrate Tier 1
        workloads_to_pin.extend(workload_tiers["tier2_medium_risk"])
        workloads_to_pin.extend(workload_tiers["tier3_critical"])
        print(f"  TIER STRATEGY: Migrating Tier 1 (low risk) only")
        print(
            f"  Pinning: {len(workload_tiers['tier2_medium_risk'])} medium-risk + "
            f"{len(workload_tiers['tier3_critical'])} critical"
        )

    elif round_num < 4:  # Round 3-4: Migrate Tier 1 + Tier 2
        workloads_to_pin.extend(workload_tiers["tier3_critical"])
        print(f"  TIER STRATEGY: Migrating Tier 1-2 (low and medium risk)")
        print(f"  Pinning: {len(workload_tiers['tier3_critical'])} critical workloads")

    else:  # Round 5: Migrate all tiers
        print(f"  TIER STRATEGY: Migrating ALL tiers (including critical)")
        # Don't pin any tier-based workloads

    # Pin workloads not yet eligible for migration
    if workloads_to_pin:
        solver.addConstraint(
            ConstraintSpecs(
                avoidMovingSpec=AvoidMovingSpec(
                    name="pin-protected-tiers",
                    objects=workloads_to_pin,
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


def analyze_migration_by_tier(assignment):
    """Analyze how many workloads of each tier have been migrated."""
    tier_counts = {}

    for tier_name, tier_workloads in workload_tiers.items():
        migrated_count = 0
        for host, workloads in assignment.items():
            if host.startswith("new_dc_"):
                for workload in workloads:
                    if workload in tier_workloads:
                        migrated_count += 1

        tier_counts[tier_name] = {
            "total": len(tier_workloads),
            "migrated": migrated_count,
            "pct": (migrated_count / len(tier_workloads)) * 100,
        }

    return tier_counts


def prioritized_migration():
    """Run migration with workload prioritization by risk tier."""
    WORKLOAD_COUNT = 200
    HOSTS_PER_DC = 30
    TOTAL_ROUNDS = 5

    current_assignment = initialize_old_datacenter(WORKLOAD_COUNT, HOSTS_PER_DC)

    print("Starting migration with workload prioritization")
    print(f"{'=' * 80}")
    print(f"Tier 1 (Low Risk):    {len(workload_tiers['tier1_low_risk'])} workloads")
    print(f"Tier 2 (Medium Risk): {len(workload_tiers['tier2_medium_risk'])} workloads")
    print(f"Tier 3 (Critical):    {len(workload_tiers['tier3_critical'])} workloads")
    print(f"{'=' * 80}\n")

    for round_num in range(TOTAL_ROUNDS):
        print(f"\nROUND {round_num + 1}/{TOTAL_ROUNDS}")
        print(f"{'=' * 80}")

        solution = solve_prioritized_round(
            current_assignment,
            round_num,
            TOTAL_ROUNDS,
            HOSTS_PER_DC,
        )

        current_assignment = solution.assignment

        # Analyze migration by tier
        tier_analysis = analyze_migration_by_tier(current_assignment)

        print(f"\nRound {round_num + 1} complete - Migration by tier:")
        for tier_name, stats in tier_analysis.items():
            print(
                f"  {tier_name}: {stats['migrated']}/{stats['total']} "
                f"({stats['pct']:.1f}%) migrated"
            )

    print("\nMigration complete!")


if __name__ == "__main__":
    prioritized_migration()
