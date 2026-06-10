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
FixedSourceMultiMove Move Type Reference Examples

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
    FixedSrcMultiMoveTypeSpec,
    LocalSearchSolverSpec,
)


def quick_example():
    """Quick example showing basic FixedSourceMultiMove usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("server")

    # quick_example_start
    # Move object bundles from specific source to hot container
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    FixedSrcMultiMoveTypeSpec(
                        specialContainer="old_server",  # Fixed source
                    ),
                ]
            )
        )
    )
    # quick_example_end

    # Setup drain scenario
    solver.setAssignment(
        {
            "old_server": ["task0", "task1", "task2"],
            "server1": ["task3"],
            "server2": [],
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
    """Fill underutilized container by pulling bundles from specific source."""
    # fill_container_start
    # Fill underutilized container by pulling bundles from specific source
    solver = ProblemSolver(service_name="example", service_scope="test")

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    FixedSrcMultiMoveTypeSpec(
                        specialContainer="overloaded_server",
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
                    FixedSrcMultiMoveTypeSpec(
                        specialContainer="source_server",
                        maxSamplesPerEquivSet=10,  # Evaluate up to 10 per equiv set
                    ),
                ]
            )
        )
    )
    # sampling_end


def drain_server():
    """Pull RAS bundles from server being decommissioned."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("server")

    # drain_server_start
    # Drain RAS task bundles from server being decommissioned
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    FixedSrcMultiMoveTypeSpec(
                        specialContainer="old_ras_server",  # Server to drain
                    ),
                ]
            )
        )
    )
    # drain_server_end

    # Setup RAS drain problem
    solver.setAssignment(
        {
            "old_ras_server": ["task0", "task1", "task2", "task3"],
            "server1": ["task4"],
            "server2": ["task5"],
            "server3": [],
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 1.5,
            "task1": 1.5,
            "task2": 2.0,
            "task3": 2.0,
            "task4": 3.0,
            "task5": 3.0,
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
                    FixedSrcMultiMoveTypeSpec(
                        specialContainer="ras_source",
                        maxSamplesPerEquivSet=5,
                        rasLocalSearchMetadata=RasLocalSearchMetadata(),
                    ),
                ]
            )
        )
    )
    # ras_metadata_end


def main():
    print("=== FixedSourceMultiMove Move Type Examples ===\n")

    print("Running quick_example...")
    quick_example()
    print("✓ quick_example completed\n")

    print("Running drain_server...")
    drain_server()
    print("✓ drain_server completed\n")


if __name__ == "__main__":
    main()
