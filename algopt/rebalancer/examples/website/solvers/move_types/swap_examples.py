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
Swap Move Type Reference Examples

This file demonstrates all the usage patterns for Swap move type shown in the
reference documentation. Each function is a complete, runnable example.
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
    MoveToCurrentScopeItemSpec,
    SampleSize,
    SingleMoveTypeSpec,
    SwapMoveTypeSpec,
)


def quick_example():
    """Quick example showing basic Swap move type usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # quick_example_start
    # Use Swap for capacity-constrained problems
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleMoveTypeSpec(),  # Try single moves first
                    SwapMoveTypeSpec(),  # Then swaps for capacity constraints
                ]
            )
        )
    )
    # quick_example_end

    # Setup capacity-constrained problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1"],  # Full (4GB)
            "host1": ["task2"],  # Full (4GB)
            "host2": [],  # Empty
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 2.0,
            "task1": 2.0,
            "task2": 4.0,
        },
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="host",
                dimension="memory",
                limit=Limit(globalLimit=4.0),
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


def basic_capacity_constrained():
    """Basic capacity-constrained problem with Swap."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # basic_capacity_constrained_start
    # Capacity is tight - need swaps to make progress
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="host",
                dimension="memory",
                limit=Limit(globalLimit=64.0),
            )
        )
    )

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleMoveTypeSpec(),  # Single moves when there's room
                    SwapMoveTypeSpec(),  # Swaps when capacity is tight
                ]
            )
        )
    )
    # basic_capacity_constrained_end

    # Setup problem where hosts are at capacity
    solver.setAssignment(
        {
            "host0": ["task0", "task1", "task2"],  # 64GB - full
            "host1": ["task3", "task4"],  # 64GB - full
            "host2": ["task5"],  # 32GB - partial
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 32.0,
            "task1": 16.0,
            "task2": 16.0,
            "task3": 48.0,
            "task4": 16.0,
            "task5": 32.0,
        },
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


def greedy_swap():
    """Use greedy flags for faster convergence."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # greedy_swap_start
    # Exit early when improvement found (faster)
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SwapMoveTypeSpec(
                        greedyOnSrc=True,  # Try src objects one by one, exit on improvement
                        greedyOnDst=True,  # Try dst objects one by one, exit on improvement
                    ),
                ]
            )
        )
    )
    # greedy_swap_end

    # Setup capacity-constrained problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1", "task2"],  # 48GB
            "host1": ["task3", "task4"],  # 32GB
            "host2": ["task5"],  # 16GB
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 16.0,
            "task1": 16.0,
            "task2": 16.0,
            "task3": 16.0,
            "task4": 16.0,
            "task5": 16.0,
        },
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="host",
                dimension="memory",
                limit=Limit(globalLimit=64.0),
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


def sampled_swap():
    """Sample objects for large problems."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # sampled_swap_start
    # For large problems, sample subset of objects
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SwapMoveTypeSpec(
                        sampleSize=SampleSize(
                            defaultSampleSize=100,  # Sample 100 objects on each side
                        ),
                    ),
                ]
            )
        )
    )
    # sampled_swap_end

    # Setup capacity-constrained problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1"],  # 8GB - full
            "host1": ["task2", "task3"],  # 6GB
            "host2": ["task4"],  # 3GB
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 5.0,
            "task1": 3.0,
            "task2": 3.0,
            "task3": 3.0,
            "task4": 3.0,
        },
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="host",
                dimension="memory",
                limit=Limit(globalLimit=8.0),
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


def swap_within_groups():
    """Only swap objects within the same partition group."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # swap_within_groups_start
    # Only swap objects from the same group (e.g., same database shard)
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SwapMoveTypeSpec(
                        partitionNameToExploreSwapsWithinObjectGroup="shard",
                    ),
                ]
            )
        )
    )
    # swap_within_groups_end

    # Setup problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1"],  # 8GB - full
            "host1": ["task2"],  # 4GB
            "host2": ["task3"],  # 4GB
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 4.0,
            "task1": 4.0,
            "task2": 4.0,
            "task3": 4.0,
        },
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="host",
                dimension="memory",
                limit=Limit(globalLimit=8.0),
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


def restricted_destinations():
    """Limit which containers to explore for swaps."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # restricted_destinations_start
    # Only swap within the same rack (limit destinations)
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SwapMoveTypeSpec(
                        destinationsToExplore=MoveToCurrentScopeItemSpec(
                            scopeNameForExploringMovesToCurrentScopeItem="rack",
                        ),
                    ),
                ]
            )
        )
    )
    # restricted_destinations_end

    # Setup problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1"],  # 6GB
            "host1": ["task2"],  # 3GB
            "host2": ["task3"],  # 3GB
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 3.0,
            "task1": 3.0,
            "task2": 3.0,
            "task3": 3.0,
        },
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="host",
                dimension="memory",
                limit=Limit(globalLimit=8.0),
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


def main():
    print("=== Swap Move Type Examples ===\n")

    print("Running quick_example...")
    quick_example()
    print("✓ quick_example completed\n")

    print("Running basic_capacity_constrained...")
    basic_capacity_constrained()
    print("✓ basic_capacity_constrained completed\n")


if __name__ == "__main__":
    main()
