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
SingleColdestStratified Move Type Reference Examples

This file demonstrates all usage patterns shown in the reference documentation.
Each function is a complete, runnable example.

NOTE: SingleColdestStratified uses legacy string-based move type specification.
"""

# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    GoalSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    BalanceSpec,
    MinimizeContainersSpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
    MoveTypeSpec,
)


def quick_example():
    """Quick example showing basic SingleColdestStratified usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # quick_example_start
    # Move to coldest containers per stratum (legacy string-based)
    spec = LocalSearchSolverSpec(
        stratifiedSampleSize=10,  # Sample 10 coldest per stratum
        moveTypeList=[
            MoveTypeSpec(moveTypeName="SINGLE_COLDEST_STRATIFIED"),
        ],
    )
    solver.addSolver(SolverSpecs(localSearchSolverSpec=spec))
    # quick_example_end

    # Setup problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1", "task2"],
            "host1": [],
            "host2": [],
        }
    )

    solver.addObjectDimension(
        "cpu",
        {
            "task0": 1.0,
            "task1": 1.0,
            "task2": 1.0,
        },
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(name="balance-cpu", scope="host", dimension="cpu")
        ),
        weight=1.0,
    )

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def bin_packing():
    """Fill underutilized containers."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # bin_packing_start
    # Bin-packing: fill coldest (most empty) containers
    solver.addGoal(
        GoalSpecs(
            minimizeContainersSpec=MinimizeContainersSpec(
                name="minimize-containers",
                scope="container",
            )
        ),
        weight=10.0,
    )

    spec = LocalSearchSolverSpec(
        stratifiedSampleSize=10,  # Try 10 coldest per stratum
        moveTypeList=[
            MoveTypeSpec(moveTypeName="SINGLE_COLDEST_STRATIFIED"),
        ],
    )
    solver.addSolver(SolverSpecs(localSearchSolverSpec=spec))
    # bin_packing_end

    # Setup problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1"],
            "host1": [],
            "host2": ["task2"],
        }
    )

    solver.addObjectDimension(
        "cpu",
        {
            "task0": 1.0,
            "task1": 1.0,
            "task2": 1.0,
        },
    )

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def capacity_balancing():
    """Balance load while respecting regions."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("server")

    # capacity_balancing_start
    # Move to coldest servers in each region for capacity balancing
    spec = LocalSearchSolverSpec(
        stratifiedSampleSize=20,  # Sample 20 coldest per region
        moveTypeList=[
            MoveTypeSpec(moveTypeName="SINGLE_COLDEST_STRATIFIED"),
        ],
    )
    solver.addSolver(SolverSpecs(localSearchSolverSpec=spec))
    # capacity_balancing_end

    # Setup problem
    solver.setAssignment(
        {
            "server0": ["task0", "task1", "task2"],
            "server1": [],
            "server2": [],
        }
    )

    solver.addObjectDimension(
        "cpu",
        {
            "task0": 2.0,
            "task1": 1.5,
            "task2": 1.5,
        },
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(name="balance-cpu", scope="server", dimension="cpu")
        ),
        weight=1.0,
    )

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def random_sampling():
    """Add randomness to avoid local optima."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # random_sampling_start
    # Include random sampling for diversity: k coldest + k random
    spec = LocalSearchSolverSpec(
        stratifiedSampleSize=10,  # 10 coldest
        includeEqualSizeRandomSampleForSingleColdestMoveType=True,  # + 10 random = 20 total
        moveTypeList=[
            MoveTypeSpec(moveTypeName="SINGLE_COLDEST_STRATIFIED"),
        ],
    )
    solver.addSolver(SolverSpecs(localSearchSolverSpec=spec))
    # random_sampling_end

    # Setup problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1"],
            "host1": [],
            "host2": [],
        }
    )

    solver.addObjectDimension(
        "cpu",
        {
            "task0": 1.0,
            "task1": 1.0,
        },
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(name="balance-cpu", scope="host", dimension="cpu")
        ),
        weight=1.0,
    )

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def decommission():
    """Empty servers by moving to coldest alternatives."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("server")

    # decommission_start
    # Decommission servers: move shards to coldest available servers
    # Use with toFree or nonAccepting constraints to mark servers for decommission
    spec = LocalSearchSolverSpec(
        stratifiedSampleSize=15,  # Try 15 coldest alternatives per region
        moveTypeList=[
            MoveTypeSpec(moveTypeName="SINGLE_COLDEST_STRATIFIED"),
        ],
    )
    solver.addSolver(SolverSpecs(localSearchSolverSpec=spec))
    # decommission_end

    # Setup problem
    solver.setAssignment(
        {
            "server0": ["shard0", "shard1", "shard2"],
            "server1": [],
            "server2": [],
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "shard0": 2.0,
            "shard1": 1.5,
            "shard2": 1.5,
        },
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-memory", scope="server", dimension="memory"
            )
        ),
        weight=1.0,
    )

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def main():
    print("=== SingleColdestStratified Move Type Examples ===\n")

    print("Running quick_example...")
    quick_example()
    print("✓ quick_example completed\n")

    print("Running bin_packing...")
    bin_packing()
    print("✓ bin_packing completed\n")


if __name__ == "__main__":
    main()
