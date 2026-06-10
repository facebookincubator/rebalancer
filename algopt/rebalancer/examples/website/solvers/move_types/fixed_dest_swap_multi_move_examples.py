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
FixedDestSwapMultiMove Move Type Reference Examples

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
    FixedDestSwapMultiMoveTypeSpec,
    LocalSearchSolverSpec,
)


def quick_example():
    """Quick example showing basic FixedDestSwapMultiMove usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("server")

    # quick_example_start
    # Swap object bundles between hot container and specific destination
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    FixedDestSwapMultiMoveTypeSpec(
                        specialContainer="swap_partner",  # Fixed destination
                    ),
                ]
            )
        )
    )
    # quick_example_end

    # Setup swap scenario
    solver.setAssignment(
        {
            "server0": ["task0", "task1", "task2"],
            "swap_partner": ["task3", "task4", "task5"],
            "server2": [],
        }
    )

    solver.addObjectDimension(
        "cpu",
        {
            "task0": 2.0,
            "task1": 1.0,
            "task2": 1.0,
            "task3": 1.5,
            "task4": 1.5,
            "task5": 1.0,
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


def adaptive_swap():
    """Adaptive 1:k swaps based on capacity dimension."""
    # adaptive_swap_start
    # Adaptive 1:k swaps based on capacity dimension
    solver = ProblemSolver(service_name="example", service_scope="test")

    from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
        RasLocalSearchMetadata,
    )

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    FixedDestSwapMultiMoveTypeSpec(
                        specialContainer="new_server_type",
                        greedyOnSrc=True,  # Required for adaptive
                        rasLocalSearchMetadata=RasLocalSearchMetadata(
                            swapRatioDimension="capacity",  # Dimension for ratios
                            useAdaptiveAllotments=True,  # Enable adaptive mode
                        ),
                    ),
                ]
            )
        )
    )
    # adaptive_swap_end


def greedy():
    """Greedy source selection for faster search."""
    # greedy_start
    # Greedy source selection for faster search
    solver = ProblemSolver(service_name="example", service_scope="test")

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    FixedDestSwapMultiMoveTypeSpec(
                        specialContainer="swap_destination",
                        greedyOnSrc=True,  # Exit early on first improvement
                    ),
                ]
            )
        )
    )
    # greedy_end


def traditional_swap():
    """Standard bundle swaps between containers."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("server")

    # traditional_swap_start
    # Traditional 1:1 bundle swaps
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    FixedDestSwapMultiMoveTypeSpec(
                        specialContainer="destination_server",
                        greedyOnSrc=False,  # Evaluate all combinations
                    ),
                ]
            )
        )
    )
    # traditional_swap_end

    # Setup balanced swap problem
    solver.setAssignment(
        {
            "server0": ["task0", "task1"],
            "destination_server": ["task2", "task3"],
            "server2": ["task4"],
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 3.0,
            "task1": 2.0,
            "task2": 1.5,
            "task3": 1.5,
            "task4": 2.0,
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


def sampling():
    """Limit evaluations with source and destination sampling."""
    # sampling_start
    # Limit evaluations with source and destination sampling
    solver = ProblemSolver(service_name="example", service_scope="test")

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    FixedDestSwapMultiMoveTypeSpec(
                        specialContainer="swap_partner",
                        maxSamplesPerEquivSet=10,  # Sample equiv sets
                        maxSampleSizeOnSrc=20,  # Max 20 source bundles
                        maxSampleSizeOnDst=30,  # Max 30 dest bundles
                    ),
                ]
            )
        )
    )
    # sampling_end


def main():
    print("=== FixedDestSwapMultiMove Move Type Examples ===\n")

    print("Running quick_example...")
    quick_example()
    print("✓ quick_example completed\n")

    print("Running traditional_swap...")
    traditional_swap()
    print("✓ traditional_swap completed\n")


if __name__ == "__main__":
    main()
