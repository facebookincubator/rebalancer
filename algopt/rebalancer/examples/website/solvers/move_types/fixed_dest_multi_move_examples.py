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
FixedDestMultiMove Move Type Reference Examples

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
    FixedDestMultiMoveTypeSpec,
    LocalSearchSolverSpec,
)


def quick_example():
    """Quick example showing basic FixedDestMultiMove usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("server")

    # quick_example_start
    # Move object bundles from hot container to specific destination
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    FixedDestMultiMoveTypeSpec(
                        specialContainer="new_server",  # Fixed destination
                    ),
                ]
            )
        )
    )
    # quick_example_end

    # Setup problem
    solver.setAssignment(
        {
            "server0": ["task0", "task1", "task2"],
            "server1": ["task3"],
            "new_server": [],
        }
    )

    solver.addObjectDimension(
        "cpu",
        {
            "task0": 1.0,
            "task1": 1.0,
            "task2": 1.0,
            "task3": 3.0,
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


def fill_container():
    """Fill specific underutilized container with object bundles."""
    # fill_container_start
    # Fill specific underutilized container with object bundles
    solver = ProblemSolver(service_name="example", service_scope="test")

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    FixedDestMultiMoveTypeSpec(
                        specialContainer="underutilized_server",
                    ),
                ]
            )
        )
    )
    # fill_container_end


def sampling():
    """Limit evaluations by sampling equivalent sets."""
    # sampling_start
    # Limit evaluations by sampling equivalent sets
    solver = ProblemSolver(service_name="example", service_scope="test")

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    FixedDestMultiMoveTypeSpec(
                        specialContainer="destination_server",
                        maxSamplesPerEquivSet=10,  # Evaluate up to 10 per equiv set
                    ),
                ]
            )
        )
    )
    # sampling_end


def ras_migration():
    """Move RAS bundles to new server."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("server")

    # ras_migration_start
    # Migrate RAS task bundles to new server
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    FixedDestMultiMoveTypeSpec(
                        specialContainer="new_ras_server",  # New server
                    ),
                ]
            )
        )
    )
    # ras_migration_end

    # Setup RAS problem
    solver.setAssignment(
        {
            "server0": ["task0", "task1", "task2", "task3"],
            "server1": ["task4", "task5"],
            "new_ras_server": [],
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 2.0,
            "task1": 2.0,
            "task2": 1.5,
            "task3": 1.5,
            "task4": 3.0,
            "task5": 2.0,
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


def ras_metadata():
    """RAS stackable solve with metadata."""
    # ras_metadata_start
    # RAS stackable solve with metadata
    solver = ProblemSolver(service_name="example", service_scope="test")

    from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
        RasLocalSearchMetadata,
    )

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    FixedDestMultiMoveTypeSpec(
                        specialContainer="ras_destination",
                        maxSamplesPerEquivSet=5,
                        rasLocalSearchMetadata=RasLocalSearchMetadata(),
                    ),
                ]
            )
        )
    )
    # ras_metadata_end


def main():
    print("=== FixedDestMultiMove Move Type Examples ===\n")

    print("Running quick_example...")
    quick_example()
    print("✓ quick_example completed\n")

    print("Running ras_migration...")
    ras_migration()
    print("✓ ras_migration completed\n")


if __name__ == "__main__":
    main()
