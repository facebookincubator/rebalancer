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
GroupCapacitySpec Reference Examples

This file demonstrates all the usage patterns for GroupCapacitySpec shown in the
reference documentation. Each function is a complete, runnable example.
"""

# example_start
# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    GroupCapacitySpec,
    GroupCapacitySpecBound,
    Limit,
    LimitType,
)


def quick_example():
    """Quick example showing basic GroupCapacitySpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # quick_example_start
    # Limit each service to max 100 GB total across all hosts
    solver.addConstraint(
        GroupCapacitySpec(
            name="service-total-capacity",
            scope="host",
            partitionName="service",
            bound=GroupCapacitySpecBound.MAX,
            limit=Limit(
                type=LimitType.ABSOLUTE,
                groupLimits={
                    "web_service": 100.0,  # Max 100 GB total
                    "api_service": 200.0,  # Max 200 GB total
                    "db_service": 500.0,  # Max 500 GB total
                },
            ),
            contribution=Limit(
                type=LimitType.ABSOLUTE,
                globalLimit=1.0,  # Each object counts as 1 unit
            ),
        )
    )
    # quick_example_end


if __name__ == "__main__":
    print("Running all GroupCapacitySpec examples...\n")

    print("1. Quick Example...")
    quick_example()

    print("\n✓ All GroupCapacitySpec examples completed successfully!")
# example_end
