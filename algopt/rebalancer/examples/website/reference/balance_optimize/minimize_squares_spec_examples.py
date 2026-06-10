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
MinimizeSquaresSpec Reference Examples

This file demonstrates all the usage patterns for MinimizeSquaresSpec shown in the
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
    CapacitySpec,
    Filter,
    MinimizeMovementSpec,
    MinimizeSquaresSpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


def quick_example():
    """Quick example showing basic MinimizeSquaresSpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # quick_example_start
    # Strong balance enforcement via sum-of-squares
    solver.addGoal(
        GoalSpecs(
            minimizeSquaresSpec=MinimizeSquaresSpec(
                name="strong-balance",
                scope="host",
                dimension="cpu_usage",
                lowerBound=0.0,  # Minimum utilization
                upperBound=1.0,  # Maximum utilization (optional)
                pieceCount=100,  # Piecewise linear approximation pieces
            )
        ),
        weight=1.0,
    )
    # quick_example_end


def strong_load_balancing():
    """Enforce very even load distribution."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # strong_load_balancing_start
    # Aggressively balance CPU
    solver.addGoal(
        GoalSpecs(
            minimizeSquaresSpec=MinimizeSquaresSpec(
                name="even-cpu-distribution",
                scope="host",
                dimension="cpu_usage",
                lowerBound=0.0,
                upperBound=1.0,  # Utilization normalized to [0, 1]
            )
        ),
        weight=2.0,  # High weight for strong enforcement
    )
    # strong_load_balancing_end


def multi_resource_strong_balance():
    """Balance multiple resources with squared penalty."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # multi_resource_strong_balance_start
    # Strong CPU balance
    solver.addGoal(
        GoalSpecs(
            minimizeSquaresSpec=MinimizeSquaresSpec(
                name="cpu-balance",
                scope="host",
                dimension="cpu_usage",
            )
        ),
        weight=2.0,
    )

    # Strong memory balance
    solver.addGoal(
        GoalSpecs(
            minimizeSquaresSpec=MinimizeSquaresSpec(
                name="memory-balance",
                scope="host",
                dimension="memory_usage",
            )
        ),
        weight=2.0,
    )
    # multi_resource_strong_balance_end


def filtered_strong_balance():
    """Apply strong balancing only to specific hosts."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # filtered_strong_balance_start
    # Production hosts need very even distribution
    prod_hosts = ["prod_host_1", "prod_host_2", "prod_host_3"]

    solver.addGoal(
        GoalSpecs(
            minimizeSquaresSpec=MinimizeSquaresSpec(
                name="prod-strong-balance",
                scope="host",
                dimension="query_rate",
                filter=Filter(items=prod_hosts),
            )
        ),
        weight=3.0,
    )
    # filtered_strong_balance_end


def rack_level_balance():
    """Strong balance at rack level."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # Add rack scope
    solver.addScope(
        "rack",
        {
            "host0": "rack0",
            "host1": "rack0",
            "host2": "rack1",
            "host3": "rack1",
        },
    )

    # rack_level_balance_start
    # Ensure racks are evenly loaded
    solver.addGoal(
        GoalSpecs(
            minimizeSquaresSpec=MinimizeSquaresSpec(
                name="rack-balance",
                scope="rack",
                dimension="network_traffic",
            )
        ),
        weight=1.5,
    )
    # rack_level_balance_end


def upper_bound_for_headroom():
    """Use upperBound to create headroom target."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # upper_bound_headroom_start
    # Balance but keep utilization under 80%
    solver.addGoal(
        GoalSpecs(
            minimizeSquaresSpec=MinimizeSquaresSpec(
                name="balance-with-headroom",
                scope="host",
                dimension="cpu_usage",
                lowerBound=0.0,
                upperBound=0.8,  # Treat >80% same as 80% (softcap)
            )
        ),
        weight=2.0,
    )
    # upper_bound_headroom_end


def piecewise_approximation_tuning():
    """Adjust pieceCount for accuracy vs performance."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # piecewise_tuning_high_accuracy_start
    # High accuracy (slower solve)
    solver.addGoal(
        GoalSpecs(
            minimizeSquaresSpec=MinimizeSquaresSpec(
                name="high-accuracy-balance",
                scope="host",
                dimension="cpu_usage",
                pieceCount=200,  # More pieces = more accurate approximation
            )
        ),
        weight=1.0,
    )
    # piecewise_tuning_high_accuracy_end


def piecewise_fast_solve():
    """Fast solve with fewer pieces."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # piecewise_tuning_fast_start
    # Fast solve (less accurate)
    solver.addGoal(
        GoalSpecs(
            minimizeSquaresSpec=MinimizeSquaresSpec(
                name="fast-balance",
                scope="host",
                dimension="cpu_usage",
                pieceCount=20,  # Fewer pieces = faster but rougher
            )
        ),
        weight=1.0,
    )
    # piecewise_tuning_fast_end


def combining_with_capacity():
    """Balance within capacity limits."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # combining_capacity_start
    # Hard: Capacity
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="capacity",
                dimension="memory",
            )
        )
    )

    # Soft: Strong balance (within capacity)
    solver.addGoal(
        GoalSpecs(
            minimizeSquaresSpec=MinimizeSquaresSpec(
                name="balance",
                dimension="memory",
            )
        ),
        weight=2.0,
    )
    # combining_capacity_end


def combining_with_minimize_movement():
    """Balance but limit churn."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # combining_minimize_movement_start
    # Primary: Strong balance
    solver.addGoal(
        GoalSpecs(
            minimizeSquaresSpec=MinimizeSquaresSpec(
                name="balance",
                dimension="cpu_usage",
            )
        ),
        weight=3.0,
    )

    # Secondary: Limit movement
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-moves",
                dimension="cpu_usage",
            )
        ),
        weight=1.0,
    )
    # combining_minimize_movement_end


def verification_example():
    """Verify balance quality."""

    # verification_example_start
    def verify_balance_quality(solution, dimension_values, capacity_values):
        """Measure balance quality using sum of squares.

        Args:
            solution: Solver solution
            dimension_values: Object dimension values
            capacity_values: Container capacity values

        Returns:
            Sum of squares value
        """
        sum_of_squares = 0.0
        utilizations = []

        for container, objects in solution.assignment.items():
            used = sum(dimension_values.get(obj, 0.0) for obj in objects)
            capacity = capacity_values.get(container, 1.0)
            utilization = used / capacity if capacity > 0 else 0

            utilizations.append(utilization)
            sum_of_squares += utilization**2

        avg_util = sum(utilizations) / len(utilizations) if utilizations else 0
        variance = (
            sum((u - avg_util) ** 2 for u in utilizations) / len(utilizations)
            if utilizations
            else 0
        )

        print(f"Balance metrics:")
        print(f"  Sum of squares: {sum_of_squares:.4f}")
        print(f"  Average utilization: {avg_util:.2%}")
        print(f"  Variance: {variance:.4f}")
        print(f"  Std dev: {variance**0.5:.2%}")

        return sum_of_squares

    # verification_example_end
    return verify_balance_quality


def complete_example():
    """
    Complete runnable example demonstrating MinimizeSquaresSpec.

    This example shows how to use MinimizeSquaresSpec to achieve very even
    distribution of CPU and memory across hosts using quadratic penalty.
    """
    # Create solver
    solver = ProblemSolver(
        service_name="minimize-squares-example", service_scope="production"
    )

    solver.setObjectName("task")
    solver.setContainerName("host")

    # Initial imbalanced assignment
    assignment = {
        "host0": ["task0", "task1", "task2", "task3", "task4", "task5"],
        "host1": ["task6"],
        "host2": [],
        "host3": ["task7", "task8", "task9"],
    }
    solver.setAssignment(assignment)

    # Task resources (normalized to [0, 1] range for utilization)
    cpu_usage = {f"task{i}": 0.1 + (i % 3) * 0.05 for i in range(10)}
    memory_usage = {f"task{i}": 0.15 + (i % 4) * 0.05 for i in range(10)}

    solver.addObjectDimension("cpu_usage", cpu_usage)
    solver.addObjectDimension("memory_usage", memory_usage)

    # Host capacities (normalized to 1.0 for utilization)
    host_cpu_capacity = {f"host{i}": 1.0 for i in range(4)}
    host_memory_capacity = {f"host{i}": 1.0 for i in range(4)}

    solver.addContainerDimension("cpu_capacity", host_cpu_capacity)
    solver.addContainerDimension("memory_capacity", host_memory_capacity)

    # Add capacity constraints
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="cpu-capacity", scope="host", dimension="cpu_usage"
            )
        )
    )
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity", scope="host", dimension="memory_usage"
            )
        )
    )

    # Add strong balance goals using MinimizeSquaresSpec
    solver.addGoal(
        GoalSpecs(
            minimizeSquaresSpec=MinimizeSquaresSpec(
                name="strong-cpu-balance",
                scope="host",
                dimension="cpu_usage",
                lowerBound=0.0,
                upperBound=1.0,
                pieceCount=100,
            )
        ),
        weight=2.0,
    )

    solver.addGoal(
        GoalSpecs(
            minimizeSquaresSpec=MinimizeSquaresSpec(
                name="strong-memory-balance",
                scope="host",
                dimension="memory_usage",
                lowerBound=0.0,
                upperBound=1.0,
                pieceCount=100,
            )
        ),
        weight=2.0,
    )

    # Add minimize movement as secondary goal
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-moves",
                dimension="cpu_usage",
            )
        ),
        weight=0.5,
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

    # Calculate balance metrics
    cpu_utilizations = []
    memory_utilizations = []

    for host in ["host0", "host1", "host2", "host3"]:
        tasks = solution.assignment.get(host, [])
        cpu = sum(cpu_usage.get(task, 0.0) for task in tasks)
        memory = sum(memory_usage.get(task, 0.0) for task in tasks)

        cpu_utilizations.append(cpu)
        memory_utilizations.append(memory)
        print(f"\n{host}:")
        print(f"  Tasks: {len(tasks)}")
        print(f"  CPU: {cpu:.2%}")
        print(f"  Memory: {memory:.2%}")

    # Calculate sum of squares
    cpu_sum_squares = sum(u**2 for u in cpu_utilizations)
    memory_sum_squares = sum(u**2 for u in memory_utilizations)

    print(f"\nBalance quality:")
    print(f"  CPU sum of squares: {cpu_sum_squares:.4f}")
    print(f"  Memory sum of squares: {memory_sum_squares:.4f}")

    cpu_avg = sum(cpu_utilizations) / len(cpu_utilizations)
    cpu_variance = sum((u - cpu_avg) ** 2 for u in cpu_utilizations) / len(
        cpu_utilizations
    )
    print(f"  CPU std dev: {cpu_variance**0.5:.2%}")

    return solution


if __name__ == "__main__":
    print("Running all MinimizeSquares examples...")

    print("\n1. Quick Example...")
    quick_example()

    print("\n2. Strong Load Balancing...")
    strong_load_balancing()

    print("\n3. Multi Resource Strong Balance...")
    multi_resource_strong_balance()

    print("\n4. Filtered Strong Balance...")
    filtered_strong_balance()

    print("\n5. Rack Level Balance...")
    rack_level_balance()

    print("\n6. Upper Bound For Headroom...")
    upper_bound_for_headroom()

    print("\n7. Piecewise Approximation Tuning...")
    piecewise_approximation_tuning()

    print("\n8. Piecewise Fast Solve...")
    piecewise_fast_solve()

    print("\n9. Combining With Capacity...")
    combining_with_capacity()

    print("\n10. Combining With Minimize Movement...")
    combining_with_minimize_movement()

    print("\n11. Verification Example...")
    verification_example()

    print("\n12. Complete Example...")
    complete_example()

    print("\n✓ All MinimizeSquares examples completed successfully!")
# example_end
