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
SingleRandomStratified Move Type Reference Examples

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
    DestinationsToExploreOptions,
    LocalSearchSolverSpec,
    MoveToCurrentScopeItemSpec,
    SampleSize,
    SingleMoveTypeSpec,
    SingleRandomStratifiedMoveTypeSpec,
)


def quick_example():
    """Quick example showing basic SingleRandomStratified usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # Define region scope
    solver.addScope(
        "region",
        {
            "host0": "region0",
            "host1": "region0",
            "host2": "region1",
            "host3": "region1",
        },
    )

    # quick_example_start
    # Stratified sampling across regions
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleRandomStratifiedMoveTypeSpec(
                        destinationsToExplore=DestinationsToExploreOptions(
                            moveToCurrentScopeItem=MoveToCurrentScopeItemSpec(
                                scopeNameForExploringMovesToCurrentScopeItem="region",
                            ),
                        ),
                        stratifiedSampleSize=SampleSize(
                            defaultSampleSize=100,  # 100 samples per region
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
            "host0": ["task0", "task1"],
            "host1": [],
            "host2": ["task2"],
            "host3": [],
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


def region_stratified():
    """Sample across regions for coverage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # Define region scope
    solver.addScope(
        "region",
        {
            "host0": "region0",
            "host1": "region0",
            "host2": "region1",
            "host3": "region1",
        },
    )

    # region_stratified_start
    # Ensure coverage across all regions
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleRandomStratifiedMoveTypeSpec(
                        destinationsToExplore=DestinationsToExploreOptions(
                            moveToCurrentScopeItem=MoveToCurrentScopeItemSpec(
                                scopeNameForExploringMovesToCurrentScopeItem="region",
                            ),
                        ),
                        stratifiedSampleSize=SampleSize(
                            defaultSampleSize=50,  # Sample 50 containers per region
                        ),
                    ),
                ]
            )
        )
    )
    # region_stratified_end

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
            "task0": 2.0,
            "task1": 1.5,
            "task2": 1.5,
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


def size_stratified():
    """Ensure coverage across container sizes."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # Define size class scope
    solver.addScope(
        "size_class",
        {
            "host0": "small",
            "host1": "small",
            "host2": "large",
            "host3": "large",
        },
    )

    # size_stratified_start
    # Stratify by container size class
    # Assumes containers are organized into size-based scope items
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleRandomStratifiedMoveTypeSpec(
                        destinationsToExplore=DestinationsToExploreOptions(
                            moveToCurrentScopeItem=MoveToCurrentScopeItemSpec(
                                scopeNameForExploringMovesToCurrentScopeItem="size_class",
                            ),
                        ),
                        stratifiedSampleSize=SampleSize(
                            defaultSampleSize=100,  # 100 samples per size class
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
            "host0": ["task0", "task1"],
            "host1": [],
            "host2": ["task2"],
            "host3": [],
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


def large_scale():
    """Very large problems with stratification."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # Define region scope
    solver.addScope(
        "region",
        {
            "host0": "region0",
            "host1": "region0",
            "host2": "region1",
            "host3": "region1",
        },
    )

    # large_scale_start
    # Large-scale: 100K containers across 10 regions
    # Sample 50 per region = 500 total evaluations
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleRandomStratifiedMoveTypeSpec(
                        destinationsToExplore=DestinationsToExploreOptions(
                            moveToCurrentScopeItem=MoveToCurrentScopeItemSpec(
                                scopeNameForExploringMovesToCurrentScopeItem="region",
                            ),
                        ),
                        stratifiedSampleSize=SampleSize(
                            defaultSampleSize=50,  # Small sample, many strata
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
            "host0": ["task0", "task1", "task2", "task3"],
            "host1": [],
            "host2": ["task4"],
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


def adaptive_sample():
    """Tune sample size by stratum."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # Define region scope
    solver.addScope(
        "region",
        {
            "host0": "region0",
            "host1": "region0",
            "host2": "region1",
            "host3": "region1",
        },
    )

    # adaptive_sample_start
    # Start with small samples, increase later
    # Stage 1: Quick exploration
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleRandomStratifiedMoveTypeSpec(
                        destinationsToExplore=DestinationsToExploreOptions(
                            moveToCurrentScopeItem=MoveToCurrentScopeItemSpec(
                                scopeNameForExploringMovesToCurrentScopeItem="region",
                            ),
                        ),
                        stratifiedSampleSize=SampleSize(
                            defaultSampleSize=25,  # Quick initial pass
                        ),
                    ),
                ]
            )
        )
    )

    # Stage 2: Refined search
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleRandomStratifiedMoveTypeSpec(
                        destinationsToExplore=DestinationsToExploreOptions(
                            moveToCurrentScopeItem=MoveToCurrentScopeItemSpec(
                                scopeNameForExploringMovesToCurrentScopeItem="region",
                            ),
                        ),
                        stratifiedSampleSize=SampleSize(
                            defaultSampleSize=100,  # More thorough
                        ),
                    ),
                    SingleMoveTypeSpec(),  # Final exhaustive pass
                ]
            )
        )
    )
    # adaptive_sample_end

    # Setup problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1"],
            "host1": [],
            "host2": ["task2"],
            "host3": [],
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


def main():
    print("=== SingleRandomStratified Move Type Examples ===\n")

    print("Running quick_example...")
    quick_example()
    print("✓ quick_example completed\n")

    print("Running region_stratified...")
    region_stratified()
    print("✓ region_stratified completed\n")


if __name__ == "__main__":
    main()
