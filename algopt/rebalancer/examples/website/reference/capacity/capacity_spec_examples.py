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
CapacitySpec Reference Examples

This file demonstrates all the usage patterns for CapacitySpec shown in the
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
    CapacitySpec,
    CapacitySpecBound,
    GroupUtilizationBound,
    Limit,
    LimitType,
    MinimizeMovementSpec,
    UtilizationBound,
    UtilizationBoundType,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


def quick_example():
    """Quick example showing basic CapacitySpec usage as constraint and goal."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # quick_example_start
    # As a constraint (hard): memory must not exceed capacity
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="host",
                dimension="memory",
                limit=Limit(globalLimit=64.0),  # Max 64GB per host
            )
        )
    )

    # As a goal (soft): prefer to stay under CPU capacity
    solver.addGoal(
        GoalSpecs(
            capacitySpec=CapacitySpec(
                name="cpu-soft-limit",
                scope="host",
                dimension="cpu",
                limit=Limit(globalLimit=16.0),  # Prefer max 16 cores
            )
        ),
        weight=10.0,
    )
    # quick_example_end


def global_limit_example():
    """Global limit - same limit for all scope items."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # global_limit_start
    # All hosts have 64GB capacity
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="host",
                dimension="memory",
                limit=Limit(globalLimit=64.0),
            )
        )
    )
    # global_limit_end


def per_scope_item_limits_example():
    """Per-scope-item limits - different limits for different scope items."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # per_scope_item_limits_start
    # Different hosts have different capacities
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="host",
                dimension="memory",
                limit=Limit(
                    scopeItemLimits={"host0": 32.0, "host1": 64.0, "host2": 128.0}
                ),
            )
        )
    )
    # per_scope_item_limits_end


def relative_limit_example():
    """Relative limits - limit as a fraction of a capacity dimension."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # relative_limit_example_start
    # Use 80% of available capacity
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="host",
                dimension="memory",
                limit=Limit(globalLimit=0.8, limitType=LimitType.RELATIVE),
            )
        )
    )
    # relative_limit_example_end


def minimum_utilization_example():
    """Minimum utilization - ensure minimum utilization using MIN bound."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # minimum_utilization_start
    # Ensure at least 10 objects per host (minimize containers)
    solver.addGoal(
        GoalSpecs(
            capacitySpec=CapacitySpec(
                name="min-objects-per-host",
                scope="host",
                dimension="object_count",
                limit=Limit(globalLimit=10.0),
                bound=CapacitySpecBound.MIN,
            )
        ),
        weight=1.0,
    )
    # minimum_utilization_end


def automatic_capacity_matching_example():
    """Automatic capacity matching with relative limits."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # Setup
    memory_per_object = {"task0": 2.0, "task1": 4.0, "task2": 3.0}
    capacity_per_host = {"host0": 64.0, "host1": 64.0, "host2": 128.0}

    # automatic_capacity_matching_start
    # Define object utilization
    solver.addObjectDimension("memory", memory_per_object)

    # Define container capacity
    solver.addContainerDimension("memory_capacity", capacity_per_host)

    # Capacity spec with relative limit
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-limit",
                scope="host",
                dimension="memory",  # Utilization
                limit=Limit(
                    globalLimit=0.9,  # 90% of capacity
                    limitType=LimitType.RELATIVE,
                ),
            )
        )
    )

    # Rebalancer computes: memory / memory_capacity ≤ 0.9 for each host
    # automatic_capacity_matching_end


def basic_capacity_constraint_example():
    """Basic capacity constraint - don't exceed host memory."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # Setup
    memory_per_task = {"task0": 2.0, "task1": 4.0, "task2": 3.0}
    memory_per_host = {"host0": 64.0, "host1": 64.0, "host2": 128.0}

    # basic_capacity_constraint_start
    solver.addObjectDimension("memory", memory_per_task)
    solver.addContainerDimension("memory_capacity", memory_per_host)

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-limit", scope="host", dimension="memory"
            )
        )
    )
    # basic_capacity_constraint_end


def multi_resource_capacity_example():
    """Multi-resource capacity - constrain both CPU and memory."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # multi_resource_capacity_start
    # CPU constraint
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="cpu-capacity",
                scope="host",
                dimension="cpu",
                limit=Limit(globalLimit=16.0),
            )
        )
    )

    # Memory constraint
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="host",
                dimension="memory",
                limit=Limit(globalLimit=64.0),
            )
        )
    )
    # multi_resource_capacity_end


def hierarchical_capacity_example():
    """Hierarchical capacity - apply capacity at multiple levels."""
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

    # hierarchical_capacity_start
    # Host-level capacity (fine-grained)
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="host-cpu",
                scope="host",
                dimension="cpu",
                limit=Limit(globalLimit=16.0),
            )
        )
    )

    # Rack-level capacity (coarse-grained)
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="rack-cpu",
                scope="rack",
                dimension="cpu",
                limit=Limit(globalLimit=128.0),  # 8 hosts x 16 cores
            )
        )
    )
    # hierarchical_capacity_end


def soft_capacity_oversubscription_example():
    """Soft capacity (oversubscription) - allow temporary capacity violations."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # soft_capacity_oversubscription_start
    # Hard constraint: never exceed 200% of capacity
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="hard-limit",
                scope="host",
                dimension="memory",
                limit=Limit(globalLimit=128.0),  # 2x capacity
            )
        )
    )

    # Soft goal: prefer to stay under 100% capacity
    solver.addGoal(
        GoalSpecs(
            capacitySpec=CapacitySpec(
                name="soft-limit",
                scope="host",
                dimension="memory",
                limit=Limit(globalLimit=64.0),  # 1x capacity
            )
        ),
        weight=10.0,
    )
    # soft_capacity_oversubscription_end


def reserve_capacity_example():
    """Reserve capacity - keep some capacity free."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # reserve_capacity_start
    # Use at most 80% of capacity (reserve 20%)
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="reserved-capacity",
                scope="host",
                dimension="cpu",
                limit=Limit(globalLimit=0.8, limitType=LimitType.RELATIVE),
            )
        )
    )
    # reserve_capacity_end


def group_utilization_bounds_example():
    """Advanced: Group utilization bounds - limit utilization from specific groups."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # Setup partition
    object_to_tenant = {
        "task0": "tenant_a",
        "task1": "tenant_a",
        "task2": "tenant_b",
        "task3": "tenant_b",
    }

    # group_utilization_bounds_start
    # Partition: group by tenant
    solver.addPartition("tenant", object_to_tenant)

    # Constraint: Each tenant can use at most 20GB per host
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="per-tenant-memory",
                scope="host",
                dimension="memory",
                utilizationBound=UtilizationBound(
                    groupUtilizationBound=GroupUtilizationBound(
                        partitionName="tenant",
                        boundType=UtilizationBoundType.UPPER,
                        perGroupValues=Limit(globalLimit=20.0),
                    )
                ),
            )
        )
    )
    # group_utilization_bounds_end


def combining_capacity_and_balance_example():
    """Combining capacity + balance - respect capacity while balancing load."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # combining_capacity_balance_start
    # Constraint: capacity
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="cpu-capacity",
                scope="host",
                dimension="cpu",
                limit=Limit(globalLimit=16.0),
            )
        )
    )

    # Goal: balance
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(name="balance-cpu", scope="host", dimension="cpu")
        ),
        weight=1.0,
    )
    # combining_capacity_balance_end


def combining_capacity_and_minimize_movement_example():
    """Combining capacity + minimize movement - respect capacity with minimal disruption."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # combining_capacity_minimize_movement_start
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="host",
                dimension="memory",
                limit=Limit(globalLimit=64.0),
            )
        )
    )

    solver.addGoal(
        GoalSpecs(minimizeMovementSpec=MinimizeMovementSpec(name="minimize-moves")),
        weight=1.0,
    )
    # combining_capacity_minimize_movement_end


def troubleshooting_capacity_dimension_example():
    """Troubleshooting: Ensure container dimension exists for capacity."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # Setup
    capacity_per_host = {"host0": 64.0, "host1": 64.0}

    # troubleshooting_capacity_dimension_start
    solver.addContainerDimension("cpu_capacity", capacity_per_host)

    # Or use dimension name that matches
    solver.addContainerDimension("cpu", capacity_per_host)
    # troubleshooting_capacity_dimension_end


def troubleshooting_relative_limit_example():
    """Troubleshooting: Provide matching capacity dimension for relative limits."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # Setup
    memory_per_task = {"task0": 2.0, "task1": 4.0}
    capacity_per_host = {"host0": 64.0, "host1": 64.0}

    # troubleshooting_relative_limit_start
    # Utilization dimension (object)
    solver.addObjectDimension("memory", memory_per_task)

    # Capacity dimension (container) - must exist!
    solver.addContainerDimension("memory_capacity", capacity_per_host)

    # Now relative limit works
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-limit",
                scope="host",
                dimension="memory",
                limit=Limit(globalLimit=0.8, limitType=LimitType.RELATIVE),
            )
        )
    )
    # troubleshooting_relative_limit_end


def troubleshooting_zero_utilization_example():
    """Troubleshooting: Allow zero utilization if empty containers are acceptable."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # troubleshooting_zero_utilization_start
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="cpu-capacity",
                scope="host",
                dimension="cpu",
                limit=Limit(globalLimit=16.0),
                zeroAllowed=True,
            )
        )
    )
    # troubleshooting_zero_utilization_end


def complete_example():
    """
    Complete runnable example demonstrating CapacitySpec.

    This example shows how to use CapacitySpec to enforce capacity constraints
    while balancing CPU and memory across hosts.
    """
    # Create solver
    solver = ProblemSolver(service_name="capacity-example", service_scope="production")

    solver.setObjectName("task")
    solver.setContainerName("host")

    # Initial assignment (some hosts overloaded)
    assignment = {
        "host0": ["task0", "task1", "task2", "task3", "task4"],
        "host1": ["task5", "task6"],
        "host2": ["task7"],
        "host3": [],
    }
    solver.setAssignment(assignment)

    # Task resources
    cpu_usage = {f"task{i}": 2.0 + (i % 3) for i in range(8)}
    memory_usage = {f"task{i}": 4.0 + (i % 4) for i in range(8)}

    solver.addObjectDimension("cpu", cpu_usage)
    solver.addObjectDimension("memory", memory_usage)

    # Host capacities
    host_cpu_capacity = {f"host{i}": 16.0 for i in range(4)}
    host_memory_capacity = {f"host{i}": 32.0 for i in range(4)}

    solver.addContainerDimension("cpu_capacity", host_cpu_capacity)
    solver.addContainerDimension("memory_capacity", host_memory_capacity)

    # Add capacity constraints (hard limits)
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="cpu-capacity",
                scope="host",
                dimension="cpu",
                limit=Limit(globalLimit=16.0),
            )
        )
    )
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="host",
                dimension="memory",
                limit=Limit(globalLimit=32.0),
            )
        )
    )

    # Add balance goals (soft objectives)
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(name="balance-cpu", scope="host", dimension="cpu")
        ),
        weight=1.0,
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-memory", scope="host", dimension="memory"
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
    print("Running all Capacity examples...")

    print("\n1. Quick Example...")
    quick_example()

    print("\n2. Global Limit Example...")
    global_limit_example()

    print("\n3. Per Scope Item Limits Example...")
    per_scope_item_limits_example()

    print("\n4. Relative Limit Example...")
    relative_limit_example()

    print("\n5. Minimum Utilization Example...")
    minimum_utilization_example()

    print("\n6. Automatic Capacity Matching Example...")
    automatic_capacity_matching_example()

    print("\n7. Basic Capacity Constraint Example...")
    basic_capacity_constraint_example()

    print("\n8. Multi Resource Capacity Example...")
    multi_resource_capacity_example()

    print("\n9. Hierarchical Capacity Example...")
    hierarchical_capacity_example()

    print("\n10. Soft Capacity Oversubscription Example...")
    soft_capacity_oversubscription_example()

    print("\n11. Reserve Capacity Example...")
    reserve_capacity_example()

    print("\n12. Group Utilization Bounds Example...")
    group_utilization_bounds_example()

    print("\n13. Combining Capacity And Balance Example...")
    combining_capacity_and_balance_example()

    print("\n14. Combining Capacity And Minimize Movement Example...")
    combining_capacity_and_minimize_movement_example()

    print("\n15. Troubleshooting Capacity Dimension Example...")
    troubleshooting_capacity_dimension_example()

    print("\n16. Troubleshooting Relative Limit Example...")
    troubleshooting_relative_limit_example()

    print("\n17. Troubleshooting Zero Utilization Example...")
    troubleshooting_zero_utilization_example()

    print("\n18. Complete Example...")
    complete_example()

    print("\n✓ All Capacity examples completed successfully!")
# example_end
