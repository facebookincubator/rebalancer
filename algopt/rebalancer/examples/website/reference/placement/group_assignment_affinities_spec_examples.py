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
GroupAssignmentAffinitiesSpec Reference Examples

This file demonstrates all usage patterns shown in the reference documentation.
Each function is a complete, runnable example.
"""

# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    GroupAssignmentAffinitiesSpec,
    GroupScopeItemAffinity,
)


def quick_example():
    """Quick example showing basic GroupAssignmentAffinitiesSpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # quick_example_start
    # Prefer service-A on host1
    solver.addGoal(
        GroupAssignmentAffinitiesSpec(
            name="service-placement-preferences",
            scope="host",
            partition="service",
            dimension="cpu",
            affinities=[
                GroupScopeItemAffinity(
                    group="service-A",
                    scopeItem="host1",
                    targetDimensionValue=1.0,  # 100% of service-A
                    affinity=10.0,  # Strong preference
                ),
            ],
        ),
        weight=1.0,
    )
    # quick_example_end


def designated_hosts():
    """Route services to designated hosts."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # designated_hosts_start
    # Route different services to specific hosts
    solver.addGoal(
        GroupAssignmentAffinitiesSpec(
            name="service-routing",
            scope="host",
            partition="service",
            dimension="memory",
            affinities=[
                # Critical service on premium host
                GroupScopeItemAffinity(
                    group="critical-service",
                    scopeItem="premium-host-1",
                    targetDimensionValue=1.0,
                    affinity=20.0,
                ),
                # Batch service on standard host
                GroupScopeItemAffinity(
                    group="batch-service",
                    scopeItem="standard-host-1",
                    targetDimensionValue=1.0,
                    affinity=10.0,
                ),
            ],
        ),
        weight=1.0,
    )
    # designated_hosts_end


def split_group():
    """Distribute group across multiple containers."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("datacenter")

    # split_group_start
    # Split database across 3 datacenters with specific fractions
    solver.addGoal(
        GroupAssignmentAffinitiesSpec(
            name="database-distribution",
            scope="datacenter",
            partition="database",
            dimension="storage",
            affinities=[
                GroupScopeItemAffinity(
                    group="user-db",
                    scopeItem="dc-west",
                    targetDimensionValue=0.5,  # 50% in west
                    affinity=15.0,
                ),
                GroupScopeItemAffinity(
                    group="user-db",
                    scopeItem="dc-east",
                    targetDimensionValue=0.3,  # 30% in east
                    affinity=15.0,
                ),
                GroupScopeItemAffinity(
                    group="user-db",
                    scopeItem="dc-central",
                    targetDimensionValue=0.2,  # 20% in central
                    affinity=15.0,
                ),
            ],
        ),
        weight=1.0,
    )
    # split_group_end


def priority_placement():
    """Use affinity scores for priority tiers."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # priority_placement_start
    # Different affinity strengths for different priority tiers
    solver.addGoal(
        GroupAssignmentAffinitiesSpec(
            name="priority-placement",
            scope="host",
            partition="service",
            dimension="cpu",
            affinities=[
                # P0: Very strong preference (must go here)
                GroupScopeItemAffinity(
                    group="p0-service",
                    scopeItem="ssd-host-1",
                    targetDimensionValue=1.0,
                    affinity=100.0,  # Very strong
                ),
                # P1: Strong preference (should go here)
                GroupScopeItemAffinity(
                    group="p1-service",
                    scopeItem="ssd-host-2",
                    targetDimensionValue=1.0,
                    affinity=50.0,  # Strong
                ),
                # P2: Weak preference (nice to have)
                GroupScopeItemAffinity(
                    group="p2-service",
                    scopeItem="standard-host-1",
                    targetDimensionValue=1.0,
                    affinity=10.0,  # Weak
                ),
            ],
        ),
        weight=1.0,
    )
    # priority_placement_end
