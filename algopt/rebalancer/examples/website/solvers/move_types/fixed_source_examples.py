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
FixedSource Move Type Reference Examples

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
    ScopeItemList,
    SingleFixedSourceMoveTypeSpec,
)


def quick_example():
    """Quick example showing basic FixedSource usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("server")

    # quick_example_start
    # Move objects from specific source to hot container
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleFixedSourceMoveTypeSpec(
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
            "old_server": ["task0", "task1", "task2"],  # Server to drain
            "server1": ["task3"],
            "server2": ["task4"],
            "server3": [],
        }
    )

    solver.addObjectDimension(
        "cpu",
        {
            "task0": 1.5,
            "task1": 2.0,
            "task2": 1.5,
            "task3": 2.0,
            "task4": 2.0,
        },
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="cpu-capacity",
                scope="server",
                dimension="cpu",
                limit=Limit(globalLimit=5.0),
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
    """Fill underutilized container from specific sources."""
    # fill_container_start
    # Fill underutilized container from specific sources
    solver = ProblemSolver(service_name="example", service_scope="test")

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleFixedSourceMoveTypeSpec(
                        scopeItemList=ScopeItemList(
                            itemList=["overloaded_server_A", "overloaded_server_B"]
                        ),
                    ),
                ]
            )
        )
    )
    # fill_container_end


def sampling():
    """Large source containers: sample 100 objects per source."""
    # sampling_start
    # Large source containers: sample 100 objects per source
    solver = ProblemSolver(service_name="example", service_scope="test")

    from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import SampleSize

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleFixedSourceMoveTypeSpec(
                        scopeItemList=ScopeItemList(
                            itemList=["large_source_1", "large_source_2"]
                        ),
                        sampleSize=SampleSize(
                            defaultSampleSize=100
                        ),  # Sample ~100 per source
                    ),
                ]
            )
        )
    )
    # sampling_end


def early_exit():
    """Stop after first source with improvement."""
    # early_exit_start
    # Stop after first source with improvement
    solver = ProblemSolver(service_name="example", service_scope="test")

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleFixedSourceMoveTypeSpec(
                        scopeItemList=ScopeItemList(
                            itemList=["source_A", "source_B", "source_C"]
                        ),
                        stopEarlyAtScopeItemThatImprovesObjective=True,
                    ),
                ]
            )
        )
    )
    # early_exit_end


def drain_servers():
    """Pull objects from servers being decommissioned."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("server")

    # drain_servers_start
    # Drain tasks from servers being decommissioned
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleFixedSourceMoveTypeSpec(
                        scopeItemList=ScopeItemList(
                            itemList=["old_server_01", "old_server_02", "old_server_03"]
                        ),
                    ),
                ]
            )
        )
    )
    # drain_servers_end

    # Setup multi-server drain
    solver.setAssignment(
        {
            "old_server_01": ["task0", "task1"],
            "old_server_02": ["task2"],
            "old_server_03": ["task3", "task4"],
            "server1": [],
            "server2": [],
            "server3": [],
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 2.0,
            "task1": 2.0,
            "task2": 3.0,
            "task3": 1.5,
            "task4": 1.5,
        },
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="server",
                dimension="memory",
                limit=Limit(globalLimit=6.0),
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
    print("=== FixedSource Move Type Examples ===\n")

    print("Running quick_example...")
    quick_example()
    print("✓ quick_example completed\n")

    print("Running drain_servers...")
    drain_servers()
    print("✓ drain_servers completed\n")


if __name__ == "__main__":
    main()
