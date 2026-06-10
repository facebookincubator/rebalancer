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
NonAcceptingSpec Reference Examples

This file demonstrates all the usage patterns for NonAcceptingSpec shown in the
reference documentation. Each function is a complete, runnable example.
"""

# example_start
# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import ConstraintSpecs
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import NonAcceptingSpec


def quick_example():
    """Quick example showing basic NonAcceptingSpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # quick_example_start
    # Prevent new objects on hosts under maintenance
    solver.addConstraint(
        ConstraintSpecs(
            nonAcceptingSpec=NonAcceptingSpec(
                name="block-new-on-maintenance",
                scope="host",
                items=["host5", "host8", "host12"],
            )
        )
    )
    # quick_example_end


def gradual_host_draining():
    """Prevent new placements as first step in draining."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # gradual_host_draining_start
    # Step 1: Block new placements
    hosts_to_drain = ["host1", "host2"]
    solver.addConstraint(
        ConstraintSpecs(
            nonAcceptingSpec=NonAcceptingSpec(
                name="block-new-placements",
                scope="host",
                items=hosts_to_drain,
            )
        )
    )

    # Step 2 (later): Actively drain existing objects
    # solver.addConstraint(ToFreeSpec(containers=hosts_to_drain))
    # gradual_host_draining_end


if __name__ == "__main__":
    print("Running all NonAcceptingSpec examples...\n")

    print("1. Quick Example...")
    quick_example()

    print("\n2. Gradual Host Draining...")
    gradual_host_draining()

    print("\n✓ All NonAcceptingSpec examples completed successfully!")
# example_end
