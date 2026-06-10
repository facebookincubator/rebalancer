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
SingleFast Move Type Reference Examples

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
    LocalSearchSolverSpec,
    MoveToCurrentScopeItemSpec,
    SingleFastMoveTypeSpec,
    SingleMoveTypeSpec,
)


def quick_example():
    """Quick example showing basic SingleFast usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # quick_example_start
    # Use SingleFast for faster convergence (stops after finding improvement)
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleFastMoveTypeSpec(),  # Fast early-exit single moves
                ]
            )
        )
    )
    # quick_example_end

    # Setup problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1", "task2", "task3"],
            "host1": [],
            "host2": [],
        }
    )

    solver.addObjectDimension(
        "cpu",
        {
            "task0": 2.0,
            "task1": 1.5,
            "task2": 2.5,
            "task3": 1.0,
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


def basic_configuration():
    """Basic configuration with SingleFast."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # basic_configuration_start
    # SingleFast is good default for faster convergence
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleFastMoveTypeSpec(),  # Use fast variant by default
                ]
            )
        )
    )
    # basic_configuration_end

    # Setup problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1"],
            "host1": [],
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


def combined_with_single():
    """Try SingleFast first, fall back to thorough Single."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # combined_with_single_start
    # Try fast moves first, then thorough search if needed
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleFastMoveTypeSpec(),  # Try fast moves first
                    SingleMoveTypeSpec(),  # Fall back to thorough search
                ]
            )
        )
    )
    # combined_with_single_end

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
            "task0": 2.0,
            "task1": 1.5,
            "task2": 1.5,
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


def more_objects():
    """Explore more objects before early termination."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # more_objects_start
    # Explore 5 objects before stopping (better quality)
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleFastMoveTypeSpec(
                        minHotObjects=5,  # Explore 5 objects before early exit
                    ),
                ]
            )
        )
    )
    # more_objects_end

    # Setup problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1", "task2", "task3", "task4"],
            "host1": [],
            "host2": [],
        }
    )

    solver.addObjectDimension(
        "cpu",
        {
            "task0": 1.0,
            "task1": 2.0,
            "task2": 1.5,
            "task3": 2.5,
            "task4": 1.0,
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


def restrict_destinations():
    """Limit search space for even faster convergence."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # Define rack scope
    solver.addScope(
        "rack",
        {
            "host0": "rack0",
            "host1": "rack0",
            "host2": "rack1",
            "host3": "rack1",
        },
    )

    # restrict_destinations_start
    # Only explore moves within the same rack
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleFastMoveTypeSpec(
                        destinationsToExplore=MoveToCurrentScopeItemSpec(
                            scopeNameForExploringMovesToCurrentScopeItem="rack",
                        ),
                    ),
                ]
            )
        )
    )
    # restrict_destinations_end

    # Setup problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1", "task2"],
            "host1": [],
            "host2": ["task3"],
            "host3": [],
        }
    )

    solver.addObjectDimension(
        "cpu",
        {
            "task0": 1.0,
            "task1": 1.0,
            "task2": 1.0,
            "task3": 1.0,
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


def interactive():
    """Fast responses for interactive applications."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # interactive_start
    # Fast solver for UI/dashboard (1 second limit)
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(name="balance-cpu", scope="host", dimension="cpu")
        ),
        weight=1.0,
    )

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                solveTime=1,  # 1 second limit for interactive response
                moveTypeList=[
                    SingleFastMoveTypeSpec(),  # Fast moves only
                ],
            )
        )
    )
    # interactive_end

    # Setup problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1", "task2", "task3", "task4"],
            "host1": [],
            "host2": [],
            "host3": [],
        }
    )

    solver.addObjectDimension(
        "cpu",
        {
            "task0": 2.0,
            "task1": 1.5,
            "task2": 2.5,
            "task3": 1.0,
            "task4": 2.0,
        },
    )

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def main():
    print("=== SingleFast Move Type Examples ===\n")

    print("Running quick_example...")
    quick_example()
    print("✓ quick_example completed\n")

    print("Running interactive...")
    interactive()
    print("✓ interactive completed\n")


if __name__ == "__main__":
    main()
