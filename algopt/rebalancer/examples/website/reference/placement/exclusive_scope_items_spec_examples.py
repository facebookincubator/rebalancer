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
ExclusiveScopeItemsSpec Reference Examples

This file demonstrates all usage patterns shown in the reference documentation.
Each function is a complete, runnable example.
"""

# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    ConflictingScopeItemInfo,
    ExclusiveScopeItemsSpec,
    ScopeItemConflictInfo,
)


def quick_example():
    """Quick example showing basic ExclusiveScopeItemsSpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("rack")

    # quick_example_start
    # Rack1 and Rack2 are mutually exclusive
    solver.addConstraint(
        ExclusiveScopeItemsSpec(
            name="rack-exclusivity",
            scope="rack",
            dimension="storage",
            conflictInfoList=[
                ScopeItemConflictInfo(
                    scopeItem="rack1",
                    conflictingScopeItemsWithOverlap=[
                        ConflictingScopeItemInfo(
                            conflictingScopeItem="rack2", overlap=1.0
                        ),
                    ],
                ),
            ],
        )
    )
    # quick_example_end


def rack_anti_affinity():
    """Rack anti-affinity for database replicas."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("rack")

    # rack_anti_affinity_start
    # Replicas of same database cannot use racks 1 and 2 together
    solver.addConstraint(
        ExclusiveScopeItemsSpec(
            name="replica-rack-anti-affinity",
            scope="rack",
            dimension="storage",
            partitionName="database",  # Exclusivity per database
            conflictInfoList=[
                ScopeItemConflictInfo(
                    scopeItem="rack1",
                    conflictingScopeItemsWithOverlap=[
                        ConflictingScopeItemInfo(conflictingScopeItem="rack2"),
                    ],
                ),
                ScopeItemConflictInfo(
                    scopeItem="rack3",
                    conflictingScopeItemsWithOverlap=[
                        ConflictingScopeItemInfo(conflictingScopeItem="rack4"),
                    ],
                ),
            ],
        )
    )
    # rack_anti_affinity_end


def datacenter_exclusivity():
    """Datacenter exclusivity for services."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("datacenter")

    # datacenter_exclusivity_start
    # DC-West and DC-East cannot both be used by same service
    solver.addConstraint(
        ExclusiveScopeItemsSpec(
            name="datacenter-exclusivity",
            scope="datacenter",
            dimension="cpu",
            partitionName="service",
            conflictInfoList=[
                ScopeItemConflictInfo(
                    scopeItem="dc-west",
                    conflictingScopeItemsWithOverlap=[
                        ConflictingScopeItemInfo(conflictingScopeItem="dc-east"),
                    ],
                ),
            ],
        )
    )
    # datacenter_exclusivity_end
