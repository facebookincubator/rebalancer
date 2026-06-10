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
BalanceSpec Reference Examples

This file demonstrates all the usage patterns for BalanceSpec shown in the
reference documentation. Each function is a complete, runnable example.
"""

# example_start
# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    ConstraintSpecs,
    GoalSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    BalanceSpec,
    BalanceSpecBoundType,
    BalanceSpecFormula,
    CapacitySpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


def quick_example():
    """Quick example showing basic BalanceSpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # quick_example_start
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-cpu",
                scope="host",
                dimension="cpu_usage",
                formula=BalanceSpecFormula.LEGACY,
                fixAverageToInitial=True,
            )
        ),
        weight=1.0,
    )
    # quick_example_end


def basic_load_balancing():
    """Basic load balancing - balance task count across hosts."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # basic_load_balancing_start
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-tasks",
                scope="host",
                dimension="task_count",  # Auto-created for object count
                formula=BalanceSpecFormula.LEGACY,
            )
        ),
        weight=1.0,
    )
    # basic_load_balancing_end


def balance_with_fixed_average():
    """Balance with fixed average - maintain total load while rebalancing."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # balance_fixed_average_start
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-cpu",
                scope="host",
                dimension="cpu",
                formula=BalanceSpecFormula.LEGACY,
                fixAverageToInitial=True,  # Don't change total CPU
            )
        ),
        weight=1.0,
    )
    # balance_fixed_average_end


def multi_level_balancing():
    """Multi-level balancing - balance at both host and rack level."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # Add rack scope
    host_to_rack = {
        "host0": "rack0",
        "host1": "rack0",
        "host2": "rack1",
        "host3": "rack1",
    }
    solver.addScope("rack", host_to_rack)

    # multi_level_balancing_start
    # Fine-grained: balance across hosts
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(name="balance-hosts", scope="host", dimension="cpu")
        ),
        weight=1.0,
    )

    # Coarse-grained: balance across racks
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(name="balance-racks", scope="rack", dimension="cpu")
        ),
        weight=0.5,  # Less important than host-level
    )
    # multi_level_balancing_end


def balance_with_upper_bound():
    """Balance with upper bound - allow some imbalance, but not too much."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # balance_upper_bound_start
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-memory",
                scope="host",
                dimension="memory",
                upperBound=1.2,  # Allow up to 120% of average
                boundType=BalanceSpecBoundType.RELATIVE,
            )
        ),
        weight=1.0,
    )
    # balance_upper_bound_end


def balance_with_datacenter_drain():
    """Balance across datacenters when draining one DC."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # Add datacenter scope
    host_to_dc = {
        "host0": "dc1",
        "host1": "dc1",
        "host2": "dc2",
        "host3": "dc2",
    }
    solver.addScope("datacenter", host_to_dc)

    # balance_datacenter_drain_start
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-across-dcs",
                scope="datacenter",
                dimension="objects",
                includeInInitialAverage=["dc-to-drain"],  # Include even though draining
                fixAverageToInitial=True,
            )
        ),
        weight=1.0,
    )
    # balance_datacenter_drain_end


def combining_with_capacity():
    """Common pattern: balance while respecting capacity constraints."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # Set up dimensions and capacities
    solver.addObjectDimension("memory", {"task0": 4.0, "task1": 8.0})
    solver.addContainerDimension("memory_capacity", {"host0": 32.0, "host1": 32.0})

    # combining_capacity_start
    # Constraint: don't exceed capacity
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity", scope="host", dimension="memory"
            )
        )
    )

    # Goal: balance memory usage
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-memory", scope="host", dimension="memory"
            )
        ),
        weight=1.0,
    )
    # combining_capacity_end


def complete_example():
    """
    Complete runnable example demonstrating BalanceSpec.

    This example shows how to use BalanceSpec to balance CPU and memory
    across hosts while respecting capacity constraints.
    """
    # Create solver
    solver = ProblemSolver(service_name="balance-example", service_scope="production")

    solver.setObjectName("task")
    solver.setContainerName("host")

    # Initial imbalanced assignment
    assignment = {
        "host0": ["task0", "task1", "task2", "task3", "task4"],
        "host1": ["task5"],
        "host2": [],
        "host3": ["task6", "task7", "task8", "task9"],
    }
    solver.setAssignment(assignment)

    # Task resources
    cpu_usage = {f"task{i}": 1.0 + (i % 3) for i in range(10)}
    memory_usage = {f"task{i}": 2.0 + (i % 4) for i in range(10)}

    solver.addObjectDimension("cpu", cpu_usage)
    solver.addObjectDimension("memory", memory_usage)

    # Host capacities
    host_cpu_capacity = {f"host{i}": 20.0 for i in range(4)}
    host_memory_capacity = {f"host{i}": 40.0 for i in range(4)}

    solver.addContainerDimension("cpu_capacity", host_cpu_capacity)
    solver.addContainerDimension("memory_capacity", host_memory_capacity)

    # Add capacity constraints
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="cpu-capacity", scope="host", dimension="cpu"
            )
        )
    )
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity", scope="host", dimension="memory"
            )
        )
    )

    # Add balance goals
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-cpu",
                scope="host",
                dimension="cpu",
                formula=BalanceSpecFormula.LEGACY,
                fixAverageToInitial=True,
            )
        ),
        weight=1.0,
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-memory",
                scope="host",
                dimension="memory",
                formula=BalanceSpecFormula.LEGACY,
                fixAverageToInitial=True,
            )
        ),
        weight=1.0,
    )

    # Solve
    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=10000))
    )
    solution = solver.solve()

    # Print results
    print(f"Solution found in {solution.profile.solveTime}ms")
    print(f"Objective value: {solution.objectiveValue}")
    print(f"Tasks moved: {solution.profile.moveCount}")

    return solution


if __name__ == "__main__":
    print("Running all Balance examples...")

    print("\n1. Quick Example...")
    quick_example()

    print("\n2. Basic Load Balancing...")
    basic_load_balancing()

    print("\n3. Balance With Fixed Average...")
    balance_with_fixed_average()

    print("\n4. Multi Level Balancing...")
    multi_level_balancing()

    print("\n5. Balance With Upper Bound...")
    balance_with_upper_bound()

    print("\n6. Balance With Datacenter Drain...")
    balance_with_datacenter_drain()

    print("\n7. Combining With Capacity...")
    combining_with_capacity()

    print("\n8. Complete Example...")
    complete_example()

    print("\n✓ All Balance examples completed successfully!")
# example_end
