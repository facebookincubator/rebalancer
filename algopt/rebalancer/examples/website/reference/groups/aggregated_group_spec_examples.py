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
AggregatedGroupSpec Reference Examples

This file demonstrates all usage patterns shown in the reference documentation.
Each function is a complete, runnable example.
"""

# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    AggregatedGroupSpec,
    AggregatedGroupSpecAggType,
    Limit,
    LimitType,
)


def quick_example():
    """Quick example showing basic AggregatedGroupSpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # quick_example_start
    # Limit the maximum group total on any host to 100GB
    solver.addConstraint(
        AggregatedGroupSpec(
            name="max-group-per-host",
            scope="host",
            dimension="memory",
            partitionName="service",
            limit=Limit(type=LimitType.ABSOLUTE, globalLimit=100.0),
            withinGroupAggregationType=AggregatedGroupSpecAggType.SUM,  # Sum within group
            groupAggregationType=AggregatedGroupSpecAggType.MAX,  # Take max across groups
            containerAggregationType=AggregatedGroupSpecAggType.MAX,  # Check each host
        )
    )
    # quick_example_end


def peak_group_total():
    """Limit peak group total per container."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("server")

    # peak_group_total_start
    # Ensure largest database on each server doesn't exceed 500GB
    solver.addConstraint(
        AggregatedGroupSpec(
            name="peak-database-limit",
            scope="server",
            dimension="storage",
            partitionName="database",
            limit=Limit(type=LimitType.ABSOLUTE, globalLimit=500.0),
            withinGroupAggregationType=AggregatedGroupSpecAggType.SUM,
            groupAggregationType=AggregatedGroupSpecAggType.MAX,
            containerAggregationType=AggregatedGroupSpecAggType.MAX,
        )
    )
    # peak_group_total_end


def top_n_groups():
    """Limit sum across top N groups."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # top_n_groups_start
    # Total of all largest groups per host must not exceed 1TB
    solver.addConstraint(
        AggregatedGroupSpec(
            name="total-peak-groups",
            scope="host",
            dimension="disk",
            partitionName="tenant",
            limit=Limit(type=LimitType.ABSOLUTE, globalLimit=1000.0),
            withinGroupAggregationType=AggregatedGroupSpecAggType.SUM,
            groupAggregationType=AggregatedGroupSpecAggType.MAX,
            containerAggregationType=AggregatedGroupSpecAggType.SUM,
        )
    )
    # top_n_groups_end


def weighted_aggregation():
    """Use LINEAR_SUM with custom contributions."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # weighted_aggregation_start
    # Weighted aggregation with custom per-host contributions
    solver.addConstraint(
        AggregatedGroupSpec(
            name="weighted-capacity",
            scope="host",
            dimension="cpu",
            partitionName="service",
            limit=Limit(type=LimitType.ABSOLUTE, globalLimit=100.0),
            withinGroupAggregationType=AggregatedGroupSpecAggType.SUM,
            groupAggregationType=AggregatedGroupSpecAggType.LINEAR_SUM,
            containerAggregationType=AggregatedGroupSpecAggType.SUM,
            contributions={
                "host1": Limit(type=LimitType.ABSOLUTE, globalLimit=2.0),  # 2x weight
                "host2": Limit(type=LimitType.ABSOLUTE, globalLimit=1.0),  # 1x weight
                "host3": Limit(type=LimitType.ABSOLUTE, globalLimit=0.5),  # 0.5x weight
            },
        )
    )
    # weighted_aggregation_end
