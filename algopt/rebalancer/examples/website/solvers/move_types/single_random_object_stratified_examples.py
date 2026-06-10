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
SingleRandomObjectStratified Move Type Reference Examples

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
    GroupList,
    LocalSearchSolverSpec,
    ObjectsFromGroupsSpec,
    ObjectsToExploreOptions,
    SampleSize,
    SingleMoveTypeSpec,
    SingleRandomObjectStratifiedMoveTypeSpec,
)


def quick_example():
    """Quick example showing basic SingleRandomObjectStratified usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # Define object groups
    solver.addPartition(
        "size_class",
        {
            "small": ["task0", "task1"],
            "medium": ["task2"],
            "large": ["task3", "task4"],
        },
    )

    # quick_example_start
    # Stratified object sampling across size classes
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleRandomObjectStratifiedMoveTypeSpec(
                        objectsToExploreOptions=ObjectsToExploreOptions(
                            objectsFromGroupSpec=ObjectsFromGroupsSpec(
                                groupList=GroupList(
                                    partitionName="size_class",
                                ),
                            ),
                        ),
                        stratifiedSampleSize=SampleSize(
                            defaultSampleSize=300,  # 300 objects per size class
                        ),
                    ),
                ]
            )
        )
    )
    # quick_example_end

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
            "task1": 1.0,
            "task2": 2.0,
            "task3": 3.0,
            "task4": 3.0,
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


def size_stratified():
    """Sample across object sizes."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # size_stratified_start
    # Ensure coverage across object size classes
    # Assumes objects are partitioned into size-based groups
    solver.addPartition(
        "size_class",
        {
            "very_small": ["task0"],
            "small": ["task1"],
            "medium": ["task2"],
            "large": ["task3"],
            "very_large": ["task4"],
        },
    )

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleRandomObjectStratifiedMoveTypeSpec(
                        objectsToExploreOptions=ObjectsToExploreOptions(
                            objectsFromGroupSpec=ObjectsFromGroupsSpec(
                                groupList=GroupList(
                                    partitionName="size_class",
                                ),
                            ),
                        ),
                        stratifiedSampleSize=SampleSize(
                            defaultSampleSize=300,  # Sample 300 per size class
                        ),
                    ),
                ]
            )
        )
    )
    # size_stratified_end

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
            "task0": 0.5,
            "task1": 1.0,
            "task2": 2.0,
            "task3": 3.0,
            "task4": 4.0,
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


def type_stratified():
    """Ensure coverage across object types."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # type_stratified_start
    # Stratify by object type/workload
    solver.addPartition(
        "workload_type",
        {
            "batch": ["task0"],
            "realtime": ["task1"],
            "ml_training": ["task2"],
            "serving": ["task3"],
        },
    )

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleRandomObjectStratifiedMoveTypeSpec(
                        objectsToExploreOptions=ObjectsToExploreOptions(
                            objectsFromGroupSpec=ObjectsFromGroupsSpec(
                                groupList=GroupList(
                                    partitionName="workload_type",
                                ),
                            ),
                        ),
                        stratifiedSampleSize=SampleSize(
                            defaultSampleSize=200,  # Sample 200 per workload type
                        ),
                    ),
                ]
            )
        )
    )
    # type_stratified_end

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
            "task0": 1.0,
            "task1": 2.0,
            "task2": 3.0,
            "task3": 1.5,
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


def large_scale():
    """Millions of objects with stratification."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # large_scale_start
    # Very large: 3M objects across 5 size groups
    # Sample 300 per group = 1500 total (2000x reduction)
    solver.addPartition(
        "size_class",
        {
            "xs": ["task0"],
            "s": ["task1"],
            "m": ["task2"],
            "l": ["task3"],
            "xl": ["task4"],
        },
    )

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleRandomObjectStratifiedMoveTypeSpec(
                        objectsToExploreOptions=ObjectsToExploreOptions(
                            objectsFromGroupSpec=ObjectsFromGroupsSpec(
                                groupList=GroupList(
                                    partitionName="size_class",
                                ),
                            ),
                        ),
                        stratifiedSampleSize=SampleSize(
                            defaultSampleSize=300,  # Small sample, many strata
                        ),
                    ),
                ]
            )
        )
    )
    # large_scale_end

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
            "task0": 0.5,
            "task1": 1.0,
            "task2": 2.0,
            "task3": 3.0,
            "task4": 4.0,
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


def priority_stratified():
    """Sample by object priority."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # priority_stratified_start
    # Stratify by priority level, with higher sampling for critical objects
    solver.addPartition(
        "priority",
        {
            "critical": ["task0"],
            "high": ["task1"],
            "medium": ["task2"],
            "low": ["task3"],
        },
    )

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleRandomObjectStratifiedMoveTypeSpec(
                        objectsToExploreOptions=ObjectsToExploreOptions(
                            objectsFromGroupSpec=ObjectsFromGroupsSpec(
                                groupList=GroupList(
                                    partitionName="priority",
                                ),
                            ),
                        ),
                        stratifiedSampleSize=SampleSize(
                            defaultSampleSize=100,  # Default sample
                            # Can override per priority if needed
                        ),
                    ),
                    SingleMoveTypeSpec(),  # Final exhaustive pass
                ]
            )
        )
    )
    # priority_stratified_end

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
            "task0": 1.0,
            "task1": 1.5,
            "task2": 2.0,
            "task3": 0.5,
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
    print("=== SingleRandomObjectStratified Move Type Examples ===\n")

    print("Running quick_example...")
    quick_example()
    print("✓ quick_example completed\n")

    print("Running size_stratified...")
    size_stratified()
    print("✓ size_stratified completed\n")


if __name__ == "__main__":
    main()
