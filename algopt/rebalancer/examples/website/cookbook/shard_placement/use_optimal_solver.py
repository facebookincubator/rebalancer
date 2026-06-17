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
Database Shard Placement with Optimal Solver

This variation uses the optimal solver for small problems (<30 shards) to get
provably optimal solutions. Useful when you need the absolute best placement.
"""

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
    OptimalSolverPackage,
    OptimalSolverSpec,
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
        "server0": [f"shard{i}" for i in range(30)],
        "server1": [f"shard{i}" for i in range(30, 45)],
        "server2": [f"shard{i}" for i in range(45, 50)],
        "server3": [],
        "server4": [],
        "server5": [],
        "server6": [],
        "server7": [],
        "server8": [],
        "server9": [],
    }
    solver.setAssignment(current_assignment)

    # Shard storage sizes (GB)
    shard_storage = {f"shard{i}": 50 + (i * 10) % 450 for i in range(50)}
    solver.addObjectDimension("storage_gb", shard_storage)

    # Shard IOPS requirements
    shard_iops = {f"shard{i}": 500 + (i * 100) % 4500 for i in range(50)}
    solver.addObjectDimension("iops", shard_iops)

    # Server storage capacities
    server_storage_capacity = {
        "server0": 3000,
        "server1": 3000,
        "server2": 5000,
        "server3": 5000,
        "server4": 8000,
        "server5": 3000,
        "server6": 3000,
        "server7": 5000,
        "server8": 5000,
        "server9": 8000,
    }
    solver.addContainerDimension("storage_capacity_gb", server_storage_capacity)

    # Server IOPS capacities
    server_iops_capacity = {f"server{i}": 50000 for i in range(10)}
    solver.addContainerDimension("iops_capacity", server_iops_capacity)

    # Constraints
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="storage-capacity",
                scope="server",
                dimension="storage_gb",
            )
        )
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="iops-capacity",
                scope="server",
                dimension="iops",
            )
        )
    )

    # Goals
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

    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-movement",
                scope="server",
                dimension="storage_gb",
            )
        ),
        weight=0.2,
    )

    # variation_start
    solver.addSolver(
        SolverSpecs(
            optimalSolverSpec=OptimalSolverSpec(
                solverPackage=OptimalSolverPackage.HIGHS,
                timeLimitMs=120000,  # 2 minute timeout
            )
        )
    )
    # variation_end

    # Solve
    solution = solver.solve()

    # Print results
    print(f"Solution found in {solution.profile.solveTime}ms")
    print(f"Objective value: {solution.objectiveValue}")
    print(f"Shards moved: {solution.profile.moveCount}")

    return solution


if __name__ == "__main__":
    place_database_shards()
