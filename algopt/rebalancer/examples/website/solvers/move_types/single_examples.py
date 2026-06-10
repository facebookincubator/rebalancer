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
Single Move Type Reference Examples

This file demonstrates all the usage patterns for Single move type shown in the
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
    SingleMoveTypeSpec,
    SwapMoveTypeSpec,
)


def quick_example():
    """Quick example showing basic Single move type usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # quick_example_start
    # Use Single move type (most basic - move one object at a time)
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleMoveTypeSpec(),  # Evaluate all single-object moves
                ]
            )
        )
    )
    # quick_example_end

    # Setup problem: imbalanced tasks across hosts
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
            "task0": 2.0,
            "task1": 1.5,
            "task2": 3.0,
            "task3": 1.0,
            "task4": 2.5,
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


def basic_configuration():
    """Basic configuration - Single as foundation move type."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # basic_configuration_start
    # Single move type is the foundation - include it in almost all configurations
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleMoveTypeSpec(),  # Always start with Single
                ]
            )
        )
    )
    # basic_configuration_end

    # Setup simple problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1", "task2"],
            "host1": [],
            "host2": [],
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


def combined_with_others():
    """Combine Single with other move types."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # combined_with_others_start
    # Typical pattern: Single as foundation, other types for special cases
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleMoveTypeSpec(),  # Try simple moves first
                    SwapMoveTypeSpec(),  # Then try swaps for capacity-constrained
                ]
            )
        )
    )
    # combined_with_others_end

    # Setup capacity-constrained problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1"],
            "host1": ["task2"],
            "host2": [],
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 3.0,
            "task1": 1.0,
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


def load_balancing():
    """Use Single for load balancing tasks across hosts."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # load_balancing_start
    # Balance CPU across hosts using Single moves
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="cpu-capacity",
                scope="host",
                dimension="cpu",
                limit=Limit(globalLimit=16.0),  # Max 16 cores per host
            )
        )
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(name="balance-cpu", scope="host", dimension="cpu")
        ),
        weight=1.0,
    )

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleMoveTypeSpec(),  # Single moves are sufficient for balancing
                ]
            )
        )
    )
    # load_balancing_end

    # Setup imbalanced problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1", "task2", "task3", "task4", "task5"],
            "host1": [],
            "host2": [],
            "host3": [],
        }
    )

    solver.addObjectDimension(
        "cpu",
        {
            "task0": 2.0,
            "task1": 3.0,
            "task2": 1.5,
            "task3": 2.5,
            "task4": 1.0,
            "task5": 2.0,
        },
    )

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def main():
    print("=== Single Move Type Examples ===\n")

    print("Running quick_example...")
    quick_example()
    print("✓ quick_example completed\n")

    print("Running load_balancing...")
    load_balancing()
    print("✓ load_balancing completed\n")


if __name__ == "__main__":
    main()
