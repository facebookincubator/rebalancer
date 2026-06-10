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
SingleGreedy Move Type Reference Examples

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
    SingleGreedyMoveTypeSpec,
    SingleMoveTypeSpec,
    SwapMoveTypeSpec,
)


def quick_example():
    """Quick example showing basic SingleGreedy usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # quick_example_start
    # Ultra-fast: stops at first improving move
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleGreedyMoveTypeSpec(),  # Fastest convergence
                ]
            )
        )
    )
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


def interactive():
    """Real-time response for dashboards and UIs."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-cpu",
                scope="host",
                dimension="cpu",
            )
        ),
        weight=1.0,
    )

    # interactive_start
    # Ultra-fast solver for UI/dashboard (100ms limit)
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                solveTime=0.1,  # 100ms limit for interactive response
                moveTypeList=[
                    SingleGreedyMoveTypeSpec(),  # First improvement = instant response
                ],
            )
        )
    )
    # interactive_end

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

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def continuous():
    """Frequent small adjustments."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # continuous_start
    # Run every 10 seconds for continuous rebalancing
    # Each run makes 1-2 quick improvements
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                solveTime=1,  # 1 second per run
                moveTypeList=[
                    SingleGreedyMoveTypeSpec(),  # Fast incremental improvements
                ],
            )
        )
    )
    # continuous_end

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


def combined():
    """Use greedy for quick wins, then thorough search."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # combined_start
    # Multi-stage: greedy for quick wins, then thorough search
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleGreedyMoveTypeSpec(),  # Quick improvements first
                    SingleMoveTypeSpec(),  # Then thorough exploration
                    SwapMoveTypeSpec(),  # Finally handle capacity constraints
                ]
            )
        )
    )
    # combined_end

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


def warmstart():
    """Fast initial solution before refinement."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # warmstart_start
    # Stage 1: Fast warm-start with greedy
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                solveTime=5,  # Quick 5 second warm-start
                moveTypeList=[
                    SingleGreedyMoveTypeSpec(),  # Get to "good enough" quickly
                ],
            )
        )
    )

    # Stage 2: Refine with thorough search
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                solveTime=60,  # Spend more time refining
                moveTypeList=[
                    SingleMoveTypeSpec(),  # Find best moves
                    SwapMoveTypeSpec(),  # Handle capacity constraints
                ],
            )
        )
    )
    # warmstart_end

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
            "task0": 2.0,
            "task1": 1.5,
            "task2": 2.5,
            "task3": 1.0,
            "task4": 2.0,
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


def main():
    print("=== SingleGreedy Move Type Examples ===\n")

    print("Running quick_example...")
    quick_example()
    print("✓ quick_example completed\n")

    print("Running interactive...")
    interactive()
    print("✓ interactive completed\n")


if __name__ == "__main__":
    main()
