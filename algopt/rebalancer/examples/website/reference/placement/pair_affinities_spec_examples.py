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
PairAffinitiesSpec Reference Examples

This file demonstrates all the usage patterns for PairAffinitiesSpec shown in the
reference documentation. Each function is a complete, runnable example.
"""

# example_start
# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    ObjectPair,
    PairAffinitiesSpec,
    PairAffinity,
)


def quick_example():
    """Quick example showing basic PairAffinitiesSpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("service")
    solver.setContainerName("host")

    # quick_example_start
    # Encourage frontend-backend pairs to colocate
    affinities = [
        PairAffinity(
            pair=ObjectPair(
                object1="frontend_0",
                object2="backend_0",
            ),
            affinity=10.0,  # Strong affinity
        ),
        PairAffinity(
            pair=ObjectPair(
                object1="frontend_1",
                object2="backend_1",
            ),
            affinity=10.0,
        ),
    ]

    solver.addGoal(
        PairAffinitiesSpec(
            name="frontend-backend-affinity",
            scope="host",
            affinities=affinities,
            limit=1.0,  # Default normalization
        ),
        weight=3.0,
    )
    # quick_example_end


if __name__ == "__main__":
    print("Running all PairAffinitiesSpec examples...\n")

    print("1. Quick Example...")
    quick_example()

    print("\n✓ All PairAffinitiesSpec examples completed successfully!")
# example_end
