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
AvoidAssignmentsSpec Reference Examples

This file demonstrates all the usage patterns for AvoidAssignmentsSpec shown in the
reference documentation. Each function is a complete, runnable example.
"""

# example_start
# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    AvoidAssignment,
    AvoidAssignmentsSpec,
)


def quick_example():
    """Quick example showing basic AvoidAssignmentsSpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("workload")
    solver.setContainerName("host")

    # quick_example_start
    # Prevent specific GPU workloads from CPU-only hosts
    solver.addConstraint(
        AvoidAssignmentsSpec(
            name="gpu-workload-restrictions",
            scope="host",
            assignments=[
                AvoidAssignment(
                    object="gpu_workload_1",
                    scopeItems=["cpu_only_host_1", "cpu_only_host_2"],
                ),
                AvoidAssignment(
                    object="gpu_workload_2",
                    scopeItems=["cpu_only_host_1", "cpu_only_host_2"],
                ),
            ],
        )
    )
    # quick_example_end


if __name__ == "__main__":
    print("Running all AvoidAssignmentsSpec examples...\n")

    print("1. Quick Example...")
    quick_example()

    print("\n✓ All AvoidAssignmentsSpec examples completed successfully!")
# example_end
