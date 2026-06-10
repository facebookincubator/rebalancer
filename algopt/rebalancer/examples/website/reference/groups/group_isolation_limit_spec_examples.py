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
GroupIsolationLimitSpec Reference Examples

This file demonstrates all the usage patterns for GroupIsolationLimitSpec shown in the
reference documentation. Each function is a complete, runnable example.
"""

# example_start
# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    GroupIsolationLimitSpec,
    Limit,
    LimitType,
)


def quick_example():
    """Quick example showing basic GroupIsolationLimitSpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # quick_example_start
    # Limit each host to max 3 different services
    solver.addConstraint(
        GroupIsolationLimitSpec(
            name="limit-service-mixing",
            scope="host",
            partitionName="service",
            limit=Limit(
                type=LimitType.ABSOLUTE,
                globalLimit=3,  # Max 3 different services per host
            ),
            groupsAllowed=1,  # Default (not typically changed)
        )
    )
    # quick_example_end


def limit_service_mixing():
    """Prevent too many services on same host."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # limit_service_mixing_start
    # Service partition
    services = {
        "web": ["web_0", "web_1", "web_2"],
        "api": ["api_0", "api_1", "api_2"],
        "db": ["db_0", "db_1"],
        "cache": ["cache_0", "cache_1"],
    }
    solver.addPartition("service", services)

    # Max 2 different services per host
    solver.addConstraint(
        GroupIsolationLimitSpec(
            name="limit-service-mixing",
            scope="host",
            partitionName="service",
            limit=Limit(
                type=LimitType.ABSOLUTE,
                globalLimit=2,  # Max 2 services per host
            ),
        )
    )
    # limit_service_mixing_end


if __name__ == "__main__":
    print("Running all GroupIsolationLimitSpec examples...\n")

    print("1. Quick Example...")
    quick_example()

    print("\n2. Limit Service Mixing...")
    limit_service_mixing()

    print("\n✓ All GroupIsolationLimitSpec examples completed successfully!")
# example_end
