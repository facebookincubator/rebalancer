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
SwapSampled Move Type Reference Examples

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
    SampleSize,
    SingleMoveTypeSpec,
    SwapMoveTypeSpec,
)


def quick_example():
    """Quick example showing basic SwapSampled usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # quick_example_start
    # Use SwapMoveTypeSpec with sampling for large problems
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleMoveTypeSpec(),
                    SwapMoveTypeSpec(
                        sampleSize=SampleSize(
                            defaultSampleSize=100,  # Sample 100 objects on each side
                        ),
                    ),
                ]
            )
        )
    )
    # quick_example_end

    # Setup capacity-constrained problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1", "task2"],  # 12GB
            "host1": ["task3", "task4"],  # 8GB
            "host2": ["task5"],  # 4GB
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 4.0,
            "task1": 4.0,
            "task2": 4.0,
            "task3": 4.0,
            "task4": 4.0,
            "task5": 4.0,
        },
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="host",
                dimension="memory",
                limit=Limit(globalLimit=16.0),
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


def basic_sampling():
    """Basic sampling configuration."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # basic_sampling_start
    # Reasonable sample size for most large problems
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SwapMoveTypeSpec(
                        sampleSize=SampleSize(
                            defaultSampleSize=100,
                        ),
                    ),
                ]
            )
        )
    )
    # basic_sampling_end

    # Setup problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1"],  # 8GB - full
            "host1": ["task2", "task3"],  # 8GB - full
            "host2": ["task4"],  # 4GB
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 4.0,
            "task1": 4.0,
            "task2": 4.0,
            "task3": 4.0,
            "task4": 4.0,
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


def per_object_sampling():
    """Different sample sizes for different objects."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # per_object_sampling_start
    # Higher sample size for important objects
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SwapMoveTypeSpec(
                        sampleSize=SampleSize(
                            defaultSampleSize=50,  # Default for most objects
                            objectToSampleSize={
                                "critical_obj1": 200,  # More sampling for critical objects
                                "critical_obj2": 200,
                            },
                        ),
                    ),
                ]
            )
        )
    )
    # per_object_sampling_end

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


def large_problem():
    """Aggressive sampling for very large problems."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # large_problem_start
    # Small sample for problems with 100K+ objects
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SwapMoveTypeSpec(
                        sampleSize=SampleSize(
                            defaultSampleSize=50,  # Aggressive sampling for speed
                        ),
                    ),
                ]
            )
        )
    )
    # large_problem_end

    # Setup problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1", "task2"],  # 12GB
            "host1": ["task3", "task4"],  # 8GB
            "host2": ["task5", "task6"],  # 8GB
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 4.0,
            "task1": 4.0,
            "task2": 4.0,
            "task3": 4.0,
            "task4": 4.0,
            "task5": 4.0,
            "task6": 4.0,
        },
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="host",
                dimension="memory",
                limit=Limit(globalLimit=12.0),
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


def greedy_sampling():
    """Combine sampling with greedy flags for maximum speed."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # greedy_sampling_start
    # Sample + early exit = maximum speed
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SwapMoveTypeSpec(
                        sampleSize=SampleSize(
                            defaultSampleSize=50,
                        ),
                        greedyOnSrc=True,  # Exit early when trying source objects
                        greedyOnDst=True,  # Exit early when trying destination objects
                    ),
                ]
            )
        )
    )
    # greedy_sampling_end

    # Setup problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1"],  # 8GB - full
            "host1": ["task2", "task3"],  # 8GB - full
            "host2": ["task4", "task5"],  # 8GB - full
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 4.0,
            "task1": 4.0,
            "task2": 4.0,
            "task3": 4.0,
            "task4": 4.0,
            "task5": 4.0,
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


def adaptive_sampling():
    """Multi-stage with increasing sample sizes."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # adaptive_sampling_start
    # Start with small sample, increase in later stages
    # Note: This would require LocalSearchStageSolverSpec
    # Showing concept with single-stage for simplicity
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleMoveTypeSpec(),
                    SwapMoveTypeSpec(
                        sampleSize=SampleSize(
                            defaultSampleSize=100,  # Start moderate
                        ),
                    ),
                    SwapMoveTypeSpec(
                        sampleSize=SampleSize(
                            defaultSampleSize=500,  # Increase for refinement
                        ),
                    ),
                ]
            )
        )
    )
    # adaptive_sampling_end

    # Setup problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1", "task2"],  # 9GB
            "host1": ["task3", "task4"],  # 6GB
            "host2": ["task5"],  # 3GB
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 3.0,
            "task1": 3.0,
            "task2": 3.0,
            "task3": 3.0,
            "task4": 3.0,
            "task5": 3.0,
        },
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="host",
                dimension="memory",
                limit=Limit(globalLimit=12.0),
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
    print("=== SwapSampled Move Type Examples ===\n")

    print("Running quick_example...")
    quick_example()
    print("✓ quick_example completed\n")

    print("Running basic_sampling...")
    basic_sampling()
    print("✓ basic_sampling completed\n")


if __name__ == "__main__":
    main()
