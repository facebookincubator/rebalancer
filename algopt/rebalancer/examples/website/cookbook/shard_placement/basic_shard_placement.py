# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Database Shard Placement

This example demonstrates how to place database shards across servers with
capacity constraints and balance goals.

Problem: 50 database shards of varying sizes need to be distributed across 10
servers with different storage capacities. Shards have different storage and
IOPS requirements. The current placement is highly imbalanced.

Goal: Place shards to balance both storage and IOPS load while respecting
capacity limits and minimizing data movement.
"""

# solution_start
# pyre-strict
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
    MinimizeMovementSpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


def place_database_shards():
    # Create solver
    solver = ProblemSolver(
        service_name="database-rebalancer", service_scope="production"
    )

    # Define objects and containers
    solver.setObjectName("shard")
    solver.setContainerName("server")

    # Current imbalanced assignment
    current_assignment = {
        "server0": [f"shard{i}" for i in range(30)],  # Overloaded
        "server1": [f"shard{i}" for i in range(30, 45)],
        "server2": [f"shard{i}" for i in range(45, 50)],
        "server3": [],  # Empty
        "server4": [],
        "server5": [],
        "server6": [],
        "server7": [],
        "server8": [],
        "server9": [],
    }
    solver.setAssignment(current_assignment)

    # Shard storage sizes (GB) - varies from 10GB to 500GB
    shard_storage = {
        f"shard{i}": 50 + (i * 10) % 450  # Varies 50-500GB
        for i in range(50)
    }
    solver.addObjectDimension("storage_gb", shard_storage)

    # Shard IOPS requirements - varies from 100 to 5000
    shard_iops = {
        f"shard{i}": 500 + (i * 100) % 4500  # Varies 500-5000
        for i in range(50)
    }
    solver.addObjectDimension("iops", shard_iops)

    # Server storage capacities (GB) - different sizes
    server_storage_capacity = {
        "server0": 3000,
        "server1": 3000,
        "server2": 5000,
        "server3": 5000,
        "server4": 8000,  # Large server
        "server5": 3000,
        "server6": 3000,
        "server7": 5000,
        "server8": 5000,
        "server9": 8000,  # Large server
    }
    solver.addContainerDimension("storage_capacity_gb", server_storage_capacity)

    # Server IOPS capacities
    server_iops_capacity = {
        f"server{i}": 50000  # 50K IOPS per server
        for i in range(10)
    }
    solver.addContainerDimension("iops_capacity", server_iops_capacity)

    # CONSTRAINT 1: Don't exceed storage capacity
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="storage-capacity",
                scope="server",
                dimension="storage_gb",
            )
        )
    )

    # CONSTRAINT 2: Don't exceed IOPS capacity
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="iops-capacity",
                scope="server",
                dimension="iops",
            )
        )
    )

    # GOAL 1: Balance storage across servers
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

    # GOAL 2: Balance IOPS across servers
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-iops",
                scope="server",
                dimension="iops",
                formula=BalanceSpecFormula.LEGACY,
                fixAverageToInitial=True,
            )
        ),
        weight=1.0,
    )

    # GOAL 3: Minimize data movement
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-movement",
                scope="server",
                dimension="storage_gb",  # Weight by data size
            )
        ),
        weight=0.2,  # Lower priority than balance
    )

    # Use Local Search solver
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                timeLimitMs=30000,  # 30 second time limit
            )
        )
    )

    # Solve
    solution = solver.solve()

    # Analyze solution
    print(f"Solution found in {solution.profile.solveTime}ms")
    print(f"Objective value: {solution.objectiveValue}")
    print(f"Shards moved: {solution.profile.moveCount}")

    # Print server utilization
    print("\nServer utilization:")
    print(
        f"{'Server':<10} {'Shards':<8} {'Storage (GB)':<15} {'Storage %':<12} {'IOPS':<10} {'IOPS %'}"
    )
    print("-" * 75)

    for server in sorted(solution.assignment.keys()):
        shards = solution.assignment[server]
        total_storage = sum(shard_storage.get(s, 0) for s in shards)
        total_iops = sum(shard_iops.get(s, 0) for s in shards)
        storage_capacity = server_storage_capacity[server]
        iops_capacity = server_iops_capacity[server]
        storage_pct = (total_storage / storage_capacity) * 100
        iops_pct = (total_iops / iops_capacity) * 100

        print(
            f"{server:<10} {len(shards):<8} {total_storage:<15.0f} {storage_pct:<12.1f} "
            f"{total_iops:<10.0f} {iops_pct:.1f}%"
        )

    return solution


if __name__ == "__main__":
    place_database_shards()
# solution_end
