#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Prefer Rack Locality Variation

Demonstrates adding a secondary goal to prefer keeping replicas of the same shard
closer together (in fewer racks) while still maintaining diversity requirements.

This is useful when network latency between racks matters and you want to balance
fault tolerance with performance by preferring replicas to be in fewer racks
(but still respecting the hard constraint of max 1 replica per rack per shard).
"""

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
    ColocateGroupsSpec,
    ColocateGroupsSpecBound,
    GroupCountSpec,
    GroupCountSpecBound,
    GroupCountSpecDefinition,
    Limit,
    LimitType,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


def place_replicas_with_rack_locality_preference():
    """Place replicas preferring rack locality as a secondary goal."""
    solver = ProblemSolver(service_name="replica-placer", service_scope="production")

    solver.setObjectName("replica")
    solver.setContainerName("server")

    # Create initial assignment
    current_assignment = {}
    for i in range(30):
        current_assignment[f"server{i}"] = []

    replica_idx = 0
    for shard in range(100):
        for replica in range(3):
            server_idx = replica_idx % 10
            current_assignment[f"server{server_idx}"].append(
                f"shard{shard}_replica{replica}"
            )
            replica_idx += 1

    solver.setAssignment(current_assignment)

    # Define rack topology
    server_to_rack = {}
    for rack in range(6):
        for server_in_rack in range(5):
            server_idx = rack * 5 + server_in_rack
            server_to_rack[f"server{server_idx}"] = f"rack{rack}"
    solver.addScope("rack", server_to_rack)

    # Define datacenter topology
    rack_to_dc = {
        "rack0": "dc1",
        "rack1": "dc1",
        "rack2": "dc1",
        "rack3": "dc2",
        "rack4": "dc2",
        "rack5": "dc2",
    }
    solver.addScope("datacenter", rack_to_dc)

    # Define shard partition
    shard_partition = {}
    for shard in range(100):
        shard_partition[f"shard{shard}"] = [
            f"shard{shard}_replica{r}" for r in range(3)
        ]
    solver.addPartition("shard", shard_partition)

    # Replica storage sizes
    replica_storage = {
        f"shard{shard}_replica{r}": 50.0 for shard in range(100) for r in range(3)
    }
    solver.addObjectDimension("storage_gb", replica_storage)

    # Server capacities
    server_capacity = {f"server{i}": 600.0 for i in range(30)}
    solver.addContainerDimension("storage_capacity_gb", server_capacity)

    # Hard constraints
    solver.addConstraint(
        ConstraintSpecs(
            groupCountSpec=GroupCountSpec(
                name="rack-diversity",
                scope="rack",
                partitionName="shard",
                bound=GroupCountSpecBound.MAX,
                limit=Limit(type=LimitType.ABSOLUTE, globalLimit=1),
                definition=GroupCountSpecDefinition.DURING,
            )
        )
    )

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

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="storage-capacity",
                scope="server",
                dimension="storage_gb",
            )
        )
    )

    # Balance goals
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

    # variation_start
    # Secondary goal: prefer fewer racks per shard
    solver.addGoal(
        GoalSpecs(
            colocateGroupsSpec=ColocateGroupsSpec(
                name="prefer-rack-locality",
                scope="rack",
                partitionName="shard",
                bound=ColocateGroupsSpecBound.MAX,
                limits=Limit(type=LimitType.ABSOLUTE, globalLimit=2),
            )
        ),
        weight=0.3,  # Lower priority
    )
    # variation_end

    # Use Local Search solver
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                timeLimitMs=60000,
            )
        )
    )

    # Solve
    solution = solver.solve()

    print(f"Solution found in {solution.profile.solveTime}ms")
    print(f"Objective value: {solution.objectiveValue}")
    print(f"Replicas moved: {solution.profile.moveCount}")

    return solution


if __name__ == "__main__":
    place_replicas_with_rack_locality_preference()
