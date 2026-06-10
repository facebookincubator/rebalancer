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
FixedDest Move Type Reference Examples

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
    FixedDestMoveTypeSpec,
    LocalSearchSolverSpec,
    SingleMoveTypeSpec,
)


def quick_example():
    """Quick example showing basic FixedDest usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("server")

    # quick_example_start
    # Move objects from hot container to specific destination
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    FixedDestMoveTypeSpec(
                        specialContainer="new_server",  # Fixed destination
                    ),
                ]
            )
        )
    )
    # quick_example_end

    # Setup problem with imbalanced load
    solver.setAssignment(
        {
            "server0": ["task0", "task1", "task2", "task3"],  # Overloaded
            "server1": ["task4", "task5"],
            "new_server": [],  # New server to fill
        }
    )

    solver.addObjectDimension(
        "cpu",
        {
            "task0": 1.5,
            "task1": 2.0,
            "task2": 1.0,
            "task3": 1.5,
            "task4": 2.0,
            "task5": 2.0,
        },
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="cpu-capacity",
                scope="server",
                dimension="cpu",
                limit=Limit(globalLimit=6.0),
            )
        )
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
    """Fill specific underutilized container."""
    # fill_container_start
    # Fill specific underutilized container
    solver = ProblemSolver(service_name="example", service_scope="test")

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    FixedDestMoveTypeSpec(
                        specialContainer="underutilized_server",
                    ),
                ]
            )
        )
    )
    # fill_container_end


def sampling():
    """Large hot container: sample 100 objects."""
    # sampling_start
    # Large hot container: sample 100 objects
    solver = ProblemSolver(service_name="example", service_scope="test")

    from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import SampleSize

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    FixedDestMoveTypeSpec(
                        specialContainer="destination_server",
                        sampleSize=SampleSize(
                            defaultSampleSize=100
                        ),  # Sample ~100 objects
                    ),
                ]
            )
        )
    )
    # sampling_end


def multi_dest():
    """Try multiple specific destinations in sequence."""
    # multi_dest_start
    # Try multiple specific destinations in sequence
    solver = ProblemSolver(service_name="example", service_scope="test")

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    FixedDestMoveTypeSpec(
                        specialContainer="server_A"
                    ),  # Try server A first
                    FixedDestMoveTypeSpec(specialContainer="server_B"),  # Then server B
                ]
            )
        )
    )
    # multi_dest_end


def server_migration():
    """Move objects to new server."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("server")

    # server_migration_start
    # Migrate tasks from overloaded servers to new server
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    FixedDestMoveTypeSpec(
                        specialContainer="new_server_01",  # New server
                    ),
                    SingleMoveTypeSpec(),  # Then explore all moves
                ]
            )
        )
    )
    # server_migration_end

    # Setup migration scenario
    solver.setAssignment(
        {
            "server0": ["task0", "task1", "task2"],
            "server1": ["task3", "task4"],
            "server2": ["task5"],
            "new_server_01": [],
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 3.0,
            "task1": 2.0,
            "task2": 1.0,
            "task3": 2.5,
            "task4": 2.5,
            "task5": 3.0,
        },
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="server",
                dimension="memory",
                limit=Limit(globalLimit=7.0),
            )
        )
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


def main():
    print("=== FixedDest Move Type Examples ===\n")

    print("Running quick_example...")
    quick_example()
    print("✓ quick_example completed\n")

    print("Running server_migration...")
    server_migration()
    print("✓ server_migration completed\n")


if __name__ == "__main__":
    main()
