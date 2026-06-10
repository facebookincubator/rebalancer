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
GroupDiversitySpec Reference Examples

This file demonstrates all usage patterns shown in the reference documentation.
Each function is a complete, runnable example.
"""

# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    GroupDiversityBound,
    GroupDiversitySpec,
    Limit,
    LimitType,
)


def quick_example():
    """Quick example showing basic GroupDiversitySpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("rack")

    # quick_example_start
    # Each rack must have storage from at least 3 different databases
    solver.addConstraint(
        GroupDiversitySpec(
            name="rack-storage-diversity",
            scope="rack",
            partition="database",
            dimension="storage",
            limit=Limit(type=LimitType.ABSOLUTE, globalLimit=3.0),
            bound=GroupDiversityBound.MIN,
        )
    )
    # quick_example_end


def min_diversity():
    """Minimum storage diversity across racks."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("rack")

    # min_diversity_start
    # Ensure each rack has data from at least 5 different databases
    solver.addConstraint(
        GroupDiversitySpec(
            name="min-database-diversity",
            scope="rack",
            partition="database",
            dimension="disk",
            limit=Limit(type=LimitType.ABSOLUTE, globalLimit=5.0),
            bound=GroupDiversityBound.MIN,
        )
    )
    # min_diversity_end


def max_diversity():
    """Maximum service diversity per host."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # max_diversity_start
    # Limit each host to at most 10 different services (by CPU usage)
    solver.addConstraint(
        GroupDiversitySpec(
            name="max-service-diversity",
            scope="host",
            partition="service",
            dimension="cpu",
            limit=Limit(type=LimitType.ABSOLUTE, globalLimit=10.0),
            bound=GroupDiversityBound.MAX,
        )
    )
    # max_diversity_end
