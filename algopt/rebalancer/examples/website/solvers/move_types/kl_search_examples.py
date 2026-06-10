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
KLSearch Move Type Reference Examples

This file demonstrates all usage patterns shown in the reference documentation.
Each function is a complete, runnable example.
"""

# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    GoalSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import BalanceSpec
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    KLSearchMoveTypeSpec,
    LocalSearchSolverSpec,
    SingleMoveTypeSpec,
)


def quick_example():
    """Quick example showing basic KLSearch usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("server")

    # quick_example_start
    # Enable KL Search for advanced optimization
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    KLSearchMoveTypeSpec(),  # No parameters needed
                ]
            )
        )
    )
    # quick_example_end

    # Setup imbalanced problem that may have local optima
    solver.setAssignment(
        {
            "server0": ["task0", "task1", "task2", "task3"],
            "server1": ["task4"],
            "server2": [],
        }
    )

    solver.addObjectDimension(
        "cpu",
        {
            "task0": 1.5,
            "task1": 2.0,
            "task2": 1.5,
            "task3": 1.0,
            "task4": 4.0,
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


def basic_usage():
    """Use KL Search to escape local optima."""
    # basic_usage_start
    # Use KL Search to escape local optima
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("server")

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    KLSearchMoveTypeSpec(),
                ]
            )
        )
    )
    # basic_usage_end


def combined():
    """Start with fast moves, then use KL Search for refinement."""
    # combined_start
    # Start with fast moves, then use KL Search for refinement
    solver = ProblemSolver(service_name="example", service_scope="test")

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleMoveTypeSpec(),  # Fast initial optimization
                    KLSearchMoveTypeSpec(),  # Refine and escape local optima
                ]
            )
        )
    )
    # combined_end


def balancing():
    """Escape local optima when balancing load."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("node")

    # balancing_start
    # Use KL Search when simple moves plateau during load balancing
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleMoveTypeSpec(),  # Initial balancing
                    KLSearchMoveTypeSpec(),  # Escape local optima for better balance
                ]
            )
        )
    )
    # balancing_end

    # Setup problem with potential local optima
    solver.setAssignment(
        {
            "node0": ["shard0", "shard1", "shard2"],
            "node1": ["shard3", "shard4"],
            "node2": ["shard5"],
            "node3": [],
        }
    )

    solver.addObjectDimension(
        "load",
        {
            "shard0": 2.0,
            "shard1": 1.5,
            "shard2": 2.5,
            "shard3": 3.0,
            "shard4": 2.0,
            "shard5": 4.0,
        },
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(name="balance-load", scope="node", dimension="load")
        ),
        weight=1.0,
    )

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def main():
    print("=== KLSearch Move Type Examples ===\n")

    print("Running quick_example...")
    quick_example()
    print("✓ quick_example completed\n")

    print("Running balancing...")
    balancing()
    print("✓ balancing completed\n")


if __name__ == "__main__":
    main()
