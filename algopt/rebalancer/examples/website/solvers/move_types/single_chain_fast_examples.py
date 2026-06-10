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
SingleChainFast Move Type Reference Examples

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
    SingleChainFastMoveTypeSpec,
    SingleChainMoveTypeSpec,
    SingleFastMoveTypeSpec,
)


def quick_example():
    """Quick example showing basic SingleChainFast usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # quick_example_start
    # Fast chain moves with early exit
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleChainFastMoveTypeSpec(),  # Stop at first improving chain
                ]
            )
        )
    )
    # quick_example_end

    # Setup capacity-constrained problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1", "task2"],  # Overloaded
            "host1": ["task3", "task4"],  # At capacity
            "host2": [],  # Empty
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 2.5,
            "task1": 3.0,
            "task2": 1.5,
            "task3": 3.5,
            "task4": 3.5,
        },
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="host",
                dimension="memory",
                limit=Limit(globalLimit=7.0),
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


def fast_chains():
    """Fastest chain moves (early exit + parallelism)."""
    # fast_chains_start
    # Fastest chain moves (early exit + parallelism)
    solver = ProblemSolver(service_name="example", service_scope="test")

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleChainFastMoveTypeSpec(),
                ]
            )
        )
    )
    # fast_chains_end


def restricted():
    """Fast chains restricted to same type."""
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
    # Fast chains restricted to same type
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleChainFastMoveTypeSpec(
                        partitionNameToExploreChainsWithinObjectGroup="task_type",
                    ),
                ]
            )
        )
    )
    # restricted_end


def fixed_dest():
    """Fast chains to specific destination."""
    # fixed_dest_start
    # Fast chains to specific destination
    solver = ProblemSolver(service_name="example", service_scope="test")

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleChainFastMoveTypeSpec(
                        specialFastColdContainer="new_server",  # Fixed destination
                    ),
                ]
            )
        )
    )
    # fixed_dest_end


def multistage():
    """Fast chains first, then thorough if needed."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("server")

    # multistage_start
    # Multi-stage: fast first, quality later
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleFastMoveTypeSpec(),  # Simple moves first
                    SingleChainFastMoveTypeSpec(),  # Fast chains for quick wins
                    SingleChainMoveTypeSpec(),  # Thorough chains if needed
                ]
            )
        )
    )
    # multistage_end

    # Setup problem with tight constraints
    solver.setAssignment(
        {
            "server0": ["task0", "task1", "task2", "task3"],
            "server1": ["task4"],
            "server2": ["task5"],
        }
    )

    solver.addObjectDimension(
        "cpu",
        {
            "task0": 1.5,
            "task1": 2.0,
            "task2": 1.0,
            "task3": 1.5,
            "task4": 5.0,
            "task5": 1.0,
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


def main():
    print("=== SingleChainFast Move Type Examples ===\n")

    print("Running quick_example...")
    quick_example()
    print("✓ quick_example completed\n")

    print("Running multistage...")
    multistage()
    print("✓ multistage completed\n")


if __name__ == "__main__":
    main()
