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
ColocateGroupsSpec Reference Examples

This file demonstrates all the usage patterns for ColocateGroupsSpec shown in the
reference documentation. Each function is a complete, runnable example.
"""

# example_start
# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    ConstraintSpecs,
    GoalSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    ColocateGroupsSpec,
    ColocateGroupsSpecBound,
    Limit,
    LimitType,
)


def quick_example():
    """Quick example showing basic ColocateGroupsSpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # quick_example_start
    # Colocate each tenant's objects on at most 3 hosts
    solver.addConstraint(
        ConstraintSpecs(
            colocateGroupsSpec=ColocateGroupsSpec(
                name="colocate-tenants",
                scope="host",
                partitionName="tenant",
                bound=ColocateGroupsSpecBound.MAX,
                limits=Limit(type=LimitType.ABSOLUTE, globalLimit=3),
            )
        )
    )
    # quick_example_end


def colocate_related_objects():
    """Keep tenant's objects together on few hosts."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("obj")
    solver.setContainerName("host")

    # colocate_related_objects_start
    # Define tenant partition
    solver.addPartition(
        "tenant",
        {
            "tenant_a": ["obj_1", "obj_2", "obj_3", "obj_4"],
            "tenant_b": ["obj_5", "obj_6", "obj_7"],
        },
    )

    # Each tenant on at most 2 hosts
    solver.addConstraint(
        ConstraintSpecs(
            colocateGroupsSpec=ColocateGroupsSpec(
                name="colocate-tenants",
                scope="host",
                partitionName="tenant",
                bound=ColocateGroupsSpecBound.MAX,
                limits=Limit(type=LimitType.ABSOLUTE, globalLimit=2),
            )
        )
    )
    # colocate_related_objects_end


def spread_for_fault_tolerance():
    """Ensure groups spread across sufficient racks."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # spread_for_fault_tolerance_start
    # Each replica group on at least 3 racks
    solver.addConstraint(
        ConstraintSpecs(
            colocateGroupsSpec=ColocateGroupsSpec(
                name="spread-replicas",
                scope="rack",
                partitionName="replica",
                bound=ColocateGroupsSpecBound.MIN,
                limits=Limit(type=LimitType.ABSOLUTE, globalLimit=3),
            )
        )
    )
    # spread_for_fault_tolerance_end


def prefer_colocation():
    """Soft preference for colocation."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # prefer_colocation_start
    # Try to colocate, but don't fail if can't
    solver.addGoal(
        GoalSpecs(
            colocateGroupsSpec=ColocateGroupsSpec(
                name="prefer-colocation",
                scope="host",
                partitionName="tenant",
                bound=ColocateGroupsSpecBound.MAX,
                limits=Limit(type=LimitType.ABSOLUTE, globalLimit=1),
                squares=True,  # Stronger penalty for spread
            )
        ),
        weight=0.5,
    )
    # prefer_colocation_end


def weighted_scope_items():
    """Prefer colocation on certain scope items."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # weighted_scope_items_start
    # Prefer spreading to high-capacity hosts (lower weight = better for colocation)
    solver.addConstraint(
        ConstraintSpecs(
            colocateGroupsSpec=ColocateGroupsSpec(
                name="weighted-colocation",
                scope="host",
                partitionName="tenant",
                bound=ColocateGroupsSpecBound.MAX,
                limits=Limit(type=LimitType.ABSOLUTE, globalLimit=5),
                scopeItemWeights={
                    "high_capacity_host_1": 0.5,  # Prefer these
                    "high_capacity_host_2": 0.5,
                    "low_capacity_host_3": 1.0,  # Default
                },
            )
        )
    )
    # weighted_scope_items_end


def relative_to_group_size():
    """Limit spread proportionally."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # relative_to_group_size_start
    # Large groups can spread more, small groups must colocate
    solver.addConstraint(
        ConstraintSpecs(
            colocateGroupsSpec=ColocateGroupsSpec(
                name="proportional-spread",
                scope="host",
                partitionName="shard",
                bound=ColocateGroupsSpecBound.MAX,
                limits=Limit(
                    type=LimitType.RELATIVE_TO_GROUP,
                    relativeLimitPercentage=0.3,  # Max 30% of hosts
                ),
            )
        )
    )
    # relative_to_group_size_end


if __name__ == "__main__":
    print("Running all ColocateGroupsSpec examples...\n")

    print("1. Quick Example...")
    quick_example()

    print("\n2. Colocate Related Objects...")
    colocate_related_objects()

    print("\n3. Spread For Fault Tolerance...")
    spread_for_fault_tolerance()

    print("\n4. Prefer Colocation...")
    prefer_colocation()

    print("\n5. Weighted Scope Items...")
    weighted_scope_items()

    print("\n6. Relative To Group Size...")
    relative_to_group_size()

    print("\n✓ All ColocateGroupsSpec examples completed successfully!")
# example_end
