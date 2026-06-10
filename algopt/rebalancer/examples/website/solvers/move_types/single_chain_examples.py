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
SingleChain Move Type Reference Examples

This file demonstrates all usage patterns shown in the reference documentation.
Each function is a complete, runnable example.
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
    CapacitySpec,
    Limit,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
    SingleChainMoveTypeSpec,
    SingleEndChainMoveTypeSpec,
    SingleMoveTypeSpec,
)


def quick_example():
    """Quick example showing basic SingleChain usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # quick_example_start
    # 2-object chains: hot container gives one, receives another
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleChainMoveTypeSpec(),  # Balanced chain moves
                ]
            )
        )
    )
    # quick_example_end

    # Setup capacity-constrained problem requiring chain moves
    solver.setAssignment(
        {
            "host0": ["task0", "task1", "task2", "task3"],  # At capacity, imbalanced
            "host1": ["task4"],  # At capacity
            "host2": [],  # Empty
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 3.0,
            "task1": 1.0,
            "task2": 2.0,
            "task3": 2.0,
            "task4": 4.0,
        },
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="host",
                dimension="memory",
                limit=Limit(globalLimit=8.0),  # Tight capacity
            )
        )
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-memory", scope="host", dimension="memory"
            )
        ),
        weight=1.0,
    )

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def basic():
    """Hot container must remain balanced (one out, one in)."""
    # basic_start
    # Hot container must remain balanced (one out, one in)
    solver = ProblemSolver(service_name="example", service_scope="test")

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleChainMoveTypeSpec(),
                ]
            )
        )
    )
    # basic_end


def restricted():
    """Only replace hot object with same type."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    # Define object type partition
    solver.addPartition(
        "task_type",
        {
            "batch": ["task1", "task2"],
            "realtime": ["task3", "task4"],
            "ml": ["task5", "task6"],
        },
    )

    # restricted_start
    # Only replace hot object with same type
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleChainMoveTypeSpec(
                        partitionNameToExploreChainsWithinObjectGroup="task_type",
                    ),
                ]
            )
        )
    )
    # restricted_end


def fixed_dest():
    """Move hot objects to specific destination, find replacements."""
    # fixed_dest_start
    # Move hot objects to specific destination, find replacements
    solver = ProblemSolver(service_name="example", service_scope="test")

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleChainMoveTypeSpec(
                        specialColdContainer="new_server",  # Fixed destination
                    ),
                ]
            )
        )
    )
    # fixed_dest_end


def combined():
    """Use with other move types."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # combined_start
    # Multi-stage: simple moves first, then chains
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleMoveTypeSpec(),  # Try simple moves first
                    SingleEndChainMoveTypeSpec(),  # Then end chains (more common)
                    SingleChainMoveTypeSpec(),  # Finally middle chains if needed
                ]
            )
        )
    )
    # combined_end

    # Setup tight capacity problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1", "task2"],
            "host1": ["task3", "task4"],
            "host2": ["task5"],
        }
    )

    solver.addObjectDimension(
        "cpu",
        {
            "task0": 2.0,
            "task1": 3.0,
            "task2": 1.0,
            "task3": 2.5,
            "task4": 1.5,
            "task5": 4.0,
        },
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="cpu-capacity",
                scope="host",
                dimension="cpu",
                limit=Limit(globalLimit=6.0),
            )
        )
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
    print("=== SingleChain Move Type Examples ===\n")

    print("Running quick_example...")
    quick_example()
    print("✓ quick_example completed\n")

    print("Running combined...")
    combined()
    print("✓ combined completed\n")


if __name__ == "__main__":
    main()
