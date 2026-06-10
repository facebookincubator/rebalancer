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
ScopeAffinitiesSpec Reference Examples

This file demonstrates all the usage patterns for ScopeAffinitiesSpec shown in the
reference documentation. Each function is a complete, runnable example.
"""

# example_start
# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import ScopeAffinitiesSpec


def quick_example():
    """Quick example showing basic ScopeAffinitiesSpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("workload")
    solver.setContainerName("host")

    # quick_example_start
    # High-priority workloads prefer premium hosts
    # Affinity scores for containers
    host_affinities = {
        "premium_host_1": 10.0,  # High affinity
        "premium_host_2": 10.0,
        "standard_host_1": 1.0,  # Low affinity
        "standard_host_2": 1.0,
    }

    solver.addGoal(
        ScopeAffinitiesSpec(
            name="priority-affinity",
            scope="host",
            dimension="priority",  # Object dimension
            affinities=host_affinities,
        ),
        weight=2.0,
    )
    # quick_example_end


if __name__ == "__main__":
    print("Running all ScopeAffinitiesSpec examples...\n")

    print("1. Quick Example...")
    quick_example()

    print("\n✓ All ScopeAffinitiesSpec examples completed successfully!")
# example_end
