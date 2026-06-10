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
UtilIncreaseCostSpec Reference Examples

This file demonstrates all the usage patterns for UtilIncreaseCostSpec shown in the
reference documentation. Each function is a complete, runnable example.
"""

# example_start
# pyre-strict
import datetime

from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    ConstraintSpecs,
    GoalSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    BalanceSpec,
    CapacitySpec,
    MinimizeMovementSpec,
    UtilIncreaseCostSpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


def quick_example():
    """Quick example showing basic UtilIncreaseCostSpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # quick_example_start
    # Penalize high CPU utilization
    solver.addGoal(
        GoalSpecs(
            utilIncreaseCostSpec=UtilIncreaseCostSpec(
                name="avoid-high-cpu",
                scope="host",
                dimension="cpu_usage",
                exponent=2.0,  # Quadratic penalty
            )
        ),
        weight=1.0,
    )
    # quick_example_end


def maintain_headroom():
    """Reserve capacity for traffic spikes."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # maintain_headroom_start
    # Keep hosts under 80% utilization
    solver.addGoal(
        GoalSpecs(
            utilIncreaseCostSpec=UtilIncreaseCostSpec(
                name="maintain-headroom",
                scope="host",
                dimension="cpu_usage",
                exponent=3.0,  # Strong penalty
                maxUtilization=0.8,  # Cap at 80%
            )
        ),
        weight=2.0,
    )
    # maintain_headroom_end


def avoid_hotspots():
    """Prevent overloaded containers."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # avoid_hotspots_start
    # Strongly discourage hot hosts
    solver.addGoal(
        GoalSpecs(
            utilIncreaseCostSpec=UtilIncreaseCostSpec(
                name="avoid-hotspots",
                scope="host",
                dimension="query_rate",
                exponent=4.0,  # Very strong penalty
            )
        ),
        weight=3.0,
    )
    # avoid_hotspots_end


def soft_capacity_limit():
    """Create a "soft" capacity that's lower than hard capacity."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # soft_capacity_limit_start
    # Hard capacity is 100, but strongly discourage going above 70
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="hard-capacity",
                scope="host",
                dimension="memory",
            )
        )
    )

    solver.addGoal(
        GoalSpecs(
            utilIncreaseCostSpec=UtilIncreaseCostSpec(
                name="soft-capacity-70pct",
                scope="host",
                dimension="memory",
                exponent=3.0,
                maxUtilization=0.7,  # Treat 70% as "full"
            )
        ),
        weight=5.0,  # High weight makes it nearly a constraint
    )
    # soft_capacity_limit_end


def multi_resource_headroom():
    """Maintain headroom on multiple resources."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # multi_resource_headroom_start
    # CPU headroom
    solver.addGoal(
        GoalSpecs(
            utilIncreaseCostSpec=UtilIncreaseCostSpec(
                name="cpu-headroom",
                scope="host",
                dimension="cpu_usage",
                exponent=2.5,
            )
        ),
        weight=1.0,
    )

    # Memory headroom
    solver.addGoal(
        GoalSpecs(
            utilIncreaseCostSpec=UtilIncreaseCostSpec(
                name="memory-headroom",
                scope="host",
                dimension="memory_usage",
                exponent=2.5,
            )
        ),
        weight=1.0,
    )

    # Disk I/O headroom
    solver.addGoal(
        GoalSpecs(
            utilIncreaseCostSpec=UtilIncreaseCostSpec(
                name="disk-headroom",
                scope="host",
                dimension="disk_iops",
                exponent=2.0,
            )
        ),
        weight=0.5,  # Less critical
    )
    # multi_resource_headroom_end


def rack_level_headroom():
    """Maintain headroom at rack level for network capacity."""
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

    # rack_level_headroom_start
    # Avoid overloading rack switches
    solver.addGoal(
        GoalSpecs(
            utilIncreaseCostSpec=UtilIncreaseCostSpec(
                name="rack-network-headroom",
                scope="rack",
                dimension="network_bandwidth",
                exponent=3.0,
                maxUtilization=0.75,  # Keep racks under 75%
            )
        ),
        weight=2.0,
    )
    # rack_level_headroom_end


def progressive_penalty():
    """Different penalties for different utilization ranges."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # progressive_penalty_start
    # Moderate penalty up to 80%
    solver.addGoal(
        GoalSpecs(
            utilIncreaseCostSpec=UtilIncreaseCostSpec(
                name="moderate-penalty",
                scope="host",
                dimension="cpu_usage",
                exponent=2.0,
                maxUtilization=0.8,
            )
        ),
        weight=1.0,
    )

    # Severe penalty above 80%
    solver.addGoal(
        GoalSpecs(
            utilIncreaseCostSpec=UtilIncreaseCostSpec(
                name="severe-penalty-high",
                scope="host",
                dimension="cpu_usage",
                exponent=5.0,  # Very steep
            )
        ),
        weight=3.0,
    )
    # progressive_penalty_end


def dynamic_exponent_based_on_time():
    """Adjust penalty based on time of day."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # dynamic_exponent_start
    def get_exponent_for_time():
        """Higher penalty during peak hours."""
        hour = datetime.datetime.now().hour
        if 9 <= hour < 17:  # Business hours
            return 4.0  # Strong penalty (keep more headroom)
        else:  # Off-peak
            return 2.0  # Moderate penalty (can run hotter)

    solver.addGoal(
        GoalSpecs(
            utilIncreaseCostSpec=UtilIncreaseCostSpec(
                name="time-aware-headroom",
                scope="host",
                dimension="cpu_usage",
                exponent=get_exponent_for_time(),
            )
        ),
        weight=2.0,
    )
    # dynamic_exponent_end


def pitfall_exponent_too_high():
    """Example showing exponent pitfall and solution."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # pitfall_exponent_bad_start
    # BAD: Exponent too high
    solver.addGoal(
        GoalSpecs(
            utilIncreaseCostSpec=UtilIncreaseCostSpec(
                name="bad-exponent",
                scope="host",
                dimension="cpu",
                exponent=10.0,  # Too extreme!
            )
        ),
        weight=1.0,
    )
    # pitfall_exponent_bad_end

    # pitfall_exponent_good_start
    # GOOD: Reasonable exponent
    solver.addGoal(
        GoalSpecs(
            utilIncreaseCostSpec=UtilIncreaseCostSpec(
                name="good-exponent",
                scope="host",
                dimension="cpu",
                exponent=3.0,  # Strong but stable
            )
        ),
        weight=1.0,
    )
    # pitfall_exponent_good_end


def pitfall_conflicting_capacity():
    """Example showing capacity conflict pitfall and solution."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # pitfall_capacity_bad_start
    # BAD: maxUtilization doesn't match capacity
    solver.addGoal(
        GoalSpecs(
            utilIncreaseCostSpec=UtilIncreaseCostSpec(
                name="bad-max-util",
                scope="host",
                dimension="cpu",
                maxUtilization=0.5,  # Treat 50% as "full"
            )
        ),
        weight=10.0,  # Very high weight
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(name="capacity", scope="host", dimension="cpu")
        )  # Allows up to 100%
    )
    # Solver confused: can use up to 100% but heavily penalized above 50%
    # pitfall_capacity_bad_end

    # pitfall_capacity_good_start
    # GOOD: Clear operating range
    solver.addGoal(
        GoalSpecs(
            utilIncreaseCostSpec=UtilIncreaseCostSpec(
                name="good-max-util",
                scope="host",
                dimension="cpu",
                maxUtilization=0.8,  # Target 80% max
                exponent=3.0,
            )
        ),
        weight=2.0,
    )
    # pitfall_capacity_good_end


def pitfall_weight_too_low():
    """Example showing weight pitfall and solution."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # pitfall_weight_bad_start
    # BAD: Weight too low relative to other goals
    solver.addGoal(
        GoalSpecs(
            utilIncreaseCostSpec=UtilIncreaseCostSpec(
                name="low-weight-headroom", scope="host", dimension="cpu", exponent=2.0
            )
        ),
        weight=0.01,  # Dominated by other goals
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(name="balance", scope="host", dimension="cpu")
        ),
        weight=10.0,  # Will ignore headroom to achieve balance
    )
    # pitfall_weight_bad_end

    # pitfall_weight_good_start
    # GOOD: Weights reflect priorities
    solver.addGoal(
        GoalSpecs(
            utilIncreaseCostSpec=UtilIncreaseCostSpec(
                name="balanced-weight-headroom",
                scope="host",
                dimension="cpu",
                exponent=2.0,
            )
        ),
        weight=2.0,  # Headroom is important
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(name="balance", scope="host", dimension="cpu")
        ),
        weight=1.0,  # Balance is secondary
    )
    # pitfall_weight_good_end


def pitfall_missing_capacity():
    """Example showing missing capacity dimension pitfall and solution."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # pitfall_missing_capacity_bad_start
    # BAD: No capacity dimension
    solver.addObjectDimension("cpu_usage", {"task0": 1.0})
    # Missing: solver.addContainerDimension("cpu_capacity", ...)

    solver.addGoal(
        GoalSpecs(
            utilIncreaseCostSpec=UtilIncreaseCostSpec(
                name="missing-capacity", scope="host", dimension="cpu_usage"
            )
        ),  # Will fail!
        weight=1.0,
    )
    # pitfall_missing_capacity_bad_end

    # pitfall_missing_capacity_good_start
    # GOOD: Both usage and capacity defined
    solver.addObjectDimension("cpu_usage", {"task0": 1.0})
    solver.addContainerDimension("cpu_capacity", {"host0": 10.0})

    solver.addGoal(
        GoalSpecs(
            utilIncreaseCostSpec=UtilIncreaseCostSpec(
                name="with-capacity", scope="host", dimension="cpu_usage"
            )
        ),
        weight=1.0,
    )
    # pitfall_missing_capacity_good_end


def combining_with_balance_spec():
    """Use both for balanced distribution with headroom."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # combining_balance_start
    # Primary: Maintain headroom
    solver.addGoal(
        GoalSpecs(
            utilIncreaseCostSpec=UtilIncreaseCostSpec(
                name="headroom",
                scope="host",
                dimension="cpu_usage",
                exponent=2.5,
            )
        ),
        weight=2.0,
    )

    # Secondary: Even distribution among non-full hosts
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance",
                scope="host",
                dimension="cpu_usage",
            )
        ),
        weight=1.0,
    )
    # combining_balance_end


def combining_with_capacity_spec():
    """Soft limit below hard limit."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # combining_capacity_start
    # Hard limit: 100% capacity
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="hard-limit",
                scope="host",
                dimension="memory",
            )
        )
    )

    # Soft limit: strongly discourage above 75%
    solver.addGoal(
        GoalSpecs(
            utilIncreaseCostSpec=UtilIncreaseCostSpec(
                name="soft-limit-75",
                scope="host",
                dimension="memory",
                exponent=4.0,
                maxUtilization=0.75,
            )
        ),
        weight=5.0,  # High weight = nearly a constraint
    )
    # combining_capacity_end


def combining_with_minimize_movement():
    """Balance headroom with minimal disruption."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # combining_minimize_movement_start
    # Maintain headroom
    solver.addGoal(
        GoalSpecs(
            utilIncreaseCostSpec=UtilIncreaseCostSpec(
                name="headroom",
                scope="host",
                dimension="cpu_usage",
                exponent=2.0,
            )
        ),
        weight=2.0,
    )

    # But minimize churn
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="min-moves",
                scope="host",
                dimension="cpu_usage",
            )
        ),
        weight=1.0,
    )
    # combining_minimize_movement_end


def verify_headroom(solution, cpu_usage, cpu_capacity, target_max_util=0.8):
    """Verify containers stay under target utilization.

    Args:
        solution: Solver solution
        cpu_usage: Object CPU usage
        cpu_capacity: Container CPU capacity
        target_max_util: Target maximum utilization (default 80%)

    Returns:
        True if all containers under target
    """
    # verify_headroom_start
    violations = []

    for host, objects in solution.assignment.items():
        cpu_used = sum(cpu_usage.get(obj, 0) for obj in objects)
        cpu_cap = cpu_capacity[host]
        utilization = cpu_used / cpu_cap

        if utilization > target_max_util:
            violations.append(
                {
                    "host": host,
                    "utilization": utilization,
                    "cpu_used": cpu_used,
                    "cpu_capacity": cpu_cap,
                }
            )

    if not violations:
        print(f"✅ All hosts under {target_max_util * 100}% utilization")
        return True
    else:
        print(
            f"❌ {len(violations)} hosts exceed {target_max_util * 100}% utilization:"
        )
        for v in violations[:5]:  # Show first 5
            print(
                f"   {v['host']}: {v['utilization'] * 100:.1f}% "
                f"({v['cpu_used']:.1f}/{v['cpu_capacity']:.1f})"
            )
        return False
    # verify_headroom_end


def analyze_utilization_distribution(solution, cpu_usage, cpu_capacity):
    """Analyze how well headroom goal worked.

    Args:
        solution: Solver solution
        cpu_usage: Object CPU usage
        cpu_capacity: Container CPU capacity
    """
    # analyze_utilization_start
    utilizations = []

    for host, objects in solution.assignment.items():
        cpu_used = sum(cpu_usage.get(obj, 0) for obj in objects)
        cpu_cap = cpu_capacity[host]
        if cpu_cap > 0:
            util = cpu_used / cpu_cap
            utilizations.append(util)

    utilizations.sort()

    print("Utilization distribution:")
    print(f"  Min: {min(utilizations) * 100:.1f}%")
    print(f"  25th percentile: {utilizations[len(utilizations) // 4] * 100:.1f}%")
    print(f"  Median: {utilizations[len(utilizations) // 2] * 100:.1f}%")
    print(f"  75th percentile: {utilizations[3 * len(utilizations) // 4] * 100:.1f}%")
    print(f"  Max: {max(utilizations) * 100:.1f}%")

    # Count hosts in utilization ranges
    ranges = [(0, 0.5), (0.5, 0.7), (0.7, 0.85), (0.85, 1.0), (1.0, float("inf"))]
    for low, high in ranges:
        count = sum(1 for u in utilizations if low <= u < high)
        pct = (count / len(utilizations)) * 100
        print(f"  {low * 100:.0f}-{high * 100:.0f}%: {count} hosts ({pct:.1f}%)")
    # analyze_utilization_end


def complete_example():
    """
    Complete runnable example demonstrating UtilIncreaseCostSpec.

    This example shows how to use UtilIncreaseCostSpec to maintain headroom
    on hosts while balancing CPU and memory usage.
    """
    # Create solver
    solver = ProblemSolver(
        service_name="util-increase-example", service_scope="production"
    )

    solver.setObjectName("task")
    solver.setContainerName("host")

    # Initial imbalanced assignment (some hosts overloaded)
    assignment = {
        "host0": ["task0", "task1", "task2", "task3", "task4", "task5"],
        "host1": ["task6", "task7", "task8"],
        "host2": ["task9"],
        "host3": [],
    }
    solver.setAssignment(assignment)

    # Task resources
    cpu_usage = {f"task{i}": 2.0 + (i % 3) for i in range(10)}
    memory_usage = {f"task{i}": 4.0 + (i % 4) for i in range(10)}

    solver.addObjectDimension("cpu", cpu_usage)
    solver.addObjectDimension("memory", memory_usage)

    # Host capacities
    host_cpu_capacity = {f"host{i}": 20.0 for i in range(4)}
    host_memory_capacity = {f"host{i}": 40.0 for i in range(4)}

    solver.addContainerDimension("cpu_capacity", host_cpu_capacity)
    solver.addContainerDimension("memory_capacity", host_memory_capacity)

    # Add hard capacity constraints
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

    # Add headroom goals - penalize high utilization
    solver.addGoal(
        GoalSpecs(
            utilIncreaseCostSpec=UtilIncreaseCostSpec(
                name="cpu-headroom",
                scope="host",
                dimension="cpu",
                exponent=3.0,  # Strong penalty for high CPU
                maxUtilization=0.8,  # Target 80% max
            )
        ),
        weight=2.0,
    )

    solver.addGoal(
        GoalSpecs(
            utilIncreaseCostSpec=UtilIncreaseCostSpec(
                name="memory-headroom",
                scope="host",
                dimension="memory",
                exponent=2.5,  # Moderate penalty for memory
                maxUtilization=0.85,  # Target 85% max
            )
        ),
        weight=1.5,
    )

    # Add balance goal as secondary objective
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-cpu",
                scope="host",
                dimension="cpu",
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

    # Verify headroom maintained
    print("\nVerifying headroom:")
    verify_headroom(solution, cpu_usage, host_cpu_capacity, target_max_util=0.8)

    print("\nUtilization analysis:")
    analyze_utilization_distribution(solution, cpu_usage, host_cpu_capacity)

    return solution


if __name__ == "__main__":
    print("Running all UtilIncreaseCost examples...")

    print("\n1. Quick Example...")
    quick_example()

    print("\n2. Maintain Headroom...")
    maintain_headroom()

    print("\n3. Avoid Hotspots...")
    avoid_hotspots()

    print("\n4. Soft Capacity Limit...")
    soft_capacity_limit()

    print("\n5. Multi Resource Headroom...")
    multi_resource_headroom()

    print("\n6. Rack Level Headroom...")
    rack_level_headroom()

    print("\n7. Progressive Penalty...")
    progressive_penalty()

    print("\n8. Dynamic Exponent Based On Time...")
    dynamic_exponent_based_on_time()

    print("\n9. Pitfall Exponent Too High...")
    pitfall_exponent_too_high()

    print("\n10. Pitfall Conflicting Capacity...")
    pitfall_conflicting_capacity()

    print("\n11. Pitfall Weight Too Low...")
    pitfall_weight_too_low()

    print("\n12. Pitfall Missing Capacity...")
    pitfall_missing_capacity()

    print("\n13. Combining With Balance Spec...")
    combining_with_balance_spec()

    print("\n14. Combining With Capacity Spec...")
    combining_with_capacity_spec()

    print("\n15. Combining With Minimize Movement...")
    combining_with_minimize_movement()

    print("\n16. Complete Example...")
    complete_example()

    print("\n✓ All UtilIncreaseCost examples completed successfully!")
# example_end
