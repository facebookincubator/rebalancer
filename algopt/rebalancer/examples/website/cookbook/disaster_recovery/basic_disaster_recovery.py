#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Basic Disaster Recovery Placement Example

Demonstrates how to place replicated database shards with diversity constraints
to ensure fault tolerance against rack and datacenter failures.

This example shows:
- Placement of 100 database shards (3 replicas each = 300 objects total)
- 30 servers across 6 racks in 2 datacenters
- Rack diversity constraint (max 1 replica per shard per rack)
- Datacenter diversity constraint (min 2 DCs per shard)
- Server capacity constraints
- Load balancing goals
"""

# solution_start
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    ConstraintSpecs,
    GoalSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    BalanceSpec,
    BalanceSpecFormula,
    CapacitySpec,
    GroupCountSpec,
    GroupCountSpecBound,
    GroupCountSpecDefinition,
    Limit,
    LimitType,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


def place_replicas_with_diversity():
    # Create solver
    solver = ProblemSolver(service_name="replica-placer", service_scope="production")

    # Define objects and containers
    solver.setObjectName("replica")
    solver.setContainerName("server")

    # Create initial assignment (currently violates diversity)
    current_assignment = {}
    for i in range(30):
        current_assignment[f"server{i}"] = []

    # Place all replicas on first 10 servers (BAD - no diversity)
    replica_idx = 0
    for shard in range(100):
        for replica in range(3):
            server_idx = replica_idx % 10
            current_assignment[f"server{server_idx}"].append(
                f"shard{shard}_replica{replica}"
            )
            replica_idx += 1

    solver.setAssignment(current_assignment)

    # Define rack topology (6 racks, 5 servers each)
    server_to_rack = {}
    for rack in range(6):
        for server_in_rack in range(5):
            server_idx = rack * 5 + server_in_rack
            server_to_rack[f"server{server_idx}"] = f"rack{rack}"
    solver.addScope("rack", server_to_rack)

    # Define datacenter topology (2 DCs, 3 racks each)
    rack_to_dc = {
        "rack0": "dc1",
        "rack1": "dc1",
        "rack2": "dc1",
        "rack3": "dc2",
        "rack4": "dc2",
        "rack5": "dc2",
    }
    solver.addScope("datacenter", rack_to_dc)

    # Define shard partition (group replicas by shard)
    shard_partition = {}
    for shard in range(100):
        shard_partition[f"shard{shard}"] = [
            f"shard{shard}_replica{r}" for r in range(3)
        ]
    solver.addPartition("shard", shard_partition)

    # Replica storage sizes (all same size for simplicity)
    replica_storage = {
        f"shard{shard}_replica{r}": 50.0  # 50GB each
        for shard in range(100)
        for r in range(3)
    }
    solver.addObjectDimension("storage_gb", replica_storage)

    # Server storage capacities
    server_capacity = {
        f"server{i}": 600.0  # 600GB per server
        for i in range(30)
    }
    solver.addContainerDimension("storage_capacity_gb", server_capacity)

    # CONSTRAINT 1: Max 1 replica per shard per rack (rack diversity)
    solver.addConstraint(
        ConstraintSpecs(
            groupCountSpec=GroupCountSpec(
                name="rack-diversity",
                scope="rack",
                partitionName="shard",
                bound=GroupCountSpecBound.MAX,
                limit=Limit(type=LimitType.ABSOLUTE, globalLimit=1),
                definition=GroupCountSpecDefinition.DURING,  # Even during migration!
            )
        )
    )

    # CONSTRAINT 2: Each shard in at least 2 datacenters (DC diversity)
    solver.addConstraint(
        ConstraintSpecs(
            groupCountSpec=GroupCountSpec(
                name="datacenter-diversity",
                scope="datacenter",
                partitionName="shard",
                bound=GroupCountSpecBound.MIN,
                limit=Limit(type=LimitType.ABSOLUTE, globalLimit=2),
                definition=GroupCountSpecDefinition.AFTER,
            )
        )
    )

    # CONSTRAINT 3: Don't exceed server storage capacity
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="storage-capacity",
                scope="server",
                dimension="storage_gb",
            )
        )
    )

    # GOAL 1: Balance replica count across servers
    # Add count dimension
    replica_count = {
        f"shard{shard}_replica{r}": 1.0 for shard in range(100) for r in range(3)
    }
    solver.addObjectDimension("count", replica_count)

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-replica-count",
                scope="server",
                dimension="count",
                formula=BalanceSpecFormula.LEGACY,
                fixAverageToInitial=True,
            )
        ),
        weight=1.0,
    )

    # GOAL 2: Balance storage across servers
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-storage",
                scope="server",
                dimension="storage_gb",
                formula=BalanceSpecFormula.LEGACY,
                fixAverageToInitial=True,
            )
        ),
        weight=1.0,
    )

    # Use Local Search solver
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                timeLimitMs=60000,  # 60 second time limit
            )
        )
    )

    # Solve
    solution = solver.solve()

    # Analyze solution
    print(f"Solution found in {solution.profile.solveTime}ms")
    print(f"Objective value: {solution.objectiveValue}")
    print(f"Replicas moved: {solution.profile.moveCount}")

    # Verify diversity constraints
    print("\nDiversity verification:")
    verify_diversity(solution.assignment, shard_partition, server_to_rack, rack_to_dc)

    # Print server utilization
    print("\nServer utilization:")
    print(
        f"{'Server':<10} {'Rack':<8} {'DC':<6} {'Replicas':<10} {'Storage (GB)':<15} {'Utilization %'}"
    )
    print("-" * 70)

    for server in sorted(solution.assignment.keys()):
        replicas = solution.assignment[server]
        rack = server_to_rack[server]
        dc = rack_to_dc[rack]
        total_storage = sum(replica_storage.get(r, 0) for r in replicas)
        utilization = (total_storage / server_capacity[server]) * 100

        print(
            f"{server:<10} {rack:<8} {dc:<6} {len(replicas):<10} "
            f"{total_storage:<15.0f} {utilization:.1f}%"
        )

    return solution


def verify_diversity(assignment, shard_partition, server_to_rack, rack_to_dc):
    """Verify diversity constraints are satisfied."""
    # Build reverse mapping: rack -> replicas
    rack_replicas = {}
    for server, replicas in assignment.items():
        rack = server_to_rack[server]
        if rack not in rack_replicas:
            rack_replicas[rack] = []
        rack_replicas[rack].extend(replicas)

    # Check rack diversity (max 1 replica per shard per rack)
    rack_violations = 0
    for shard, replicas in shard_partition.items():
        for rack, rack_reps in rack_replicas.items():
            count = sum(1 for r in replicas if r in rack_reps)
            if count > 1:
                rack_violations += 1
                print(f"  ❌ Shard {shard} has {count} replicas on {rack}")

    if rack_violations == 0:
        print(f"  ✅ Rack diversity satisfied (no shard has >1 replica per rack)")

    # Check DC diversity (min 2 DCs per shard)
    dc_violations = 0
    for shard, replicas in shard_partition.items():
        dcs_with_replica = set()
        for server, server_reps in assignment.items():
            if any(r in server_reps for r in replicas):
                rack = server_to_rack[server]
                dc = rack_to_dc[rack]
                dcs_with_replica.add(dc)

        if len(dcs_with_replica) < 2:
            dc_violations += 1
            print(f"  ❌ Shard {shard} only in {len(dcs_with_replica)} DC(s)")

    if dc_violations == 0:
        print(f"  ✅ DC diversity satisfied (all shards in ≥2 datacenters)")


if __name__ == "__main__":
    place_replicas_with_diversity()
# solution_end
