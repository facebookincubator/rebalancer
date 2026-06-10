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
MinimizeNthLargestSpec Reference Examples

This file demonstrates all usage patterns shown in the reference documentation.
Each function is a complete, runnable example.
"""

# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    MinimizeNthLargestUtilizationSpec,
)


def quick_example():
    """Quick example showing basic MinimizeNthLargestSpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # quick_example_start
    # Minimize the 2nd largest host utilization
    solver.addGoal(
        MinimizeNthLargestUtilizationSpec(
            name="minimize-2nd-highest",
            scope="host",
            dimension="cpu",
            n=1,  # 0-based: 0=max, 1=2nd largest, 2=3rd largest
        ),
        weight=1.0,
    )
    # quick_example_end


def percentile_95():
    """Minimize 95th percentile load."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # percentile_95_start
    # For 100 hosts, 95th percentile is the 5th largest value
    # n = ceil(100 * 0.05) - 1 = 4
    solver.addGoal(
        MinimizeNthLargestUtilizationSpec(
            name="minimize-p95",
            scope="host",
            dimension="memory",
            n=4,  # 5th largest (0-based index)
        ),
        weight=1.0,
    )
    # percentile_95_end


def cap_top_3():
    """Cap the top 3 hosts."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("server")

    # cap_top_3_start
    # Minimize the 3rd highest server load
    solver.addGoal(
        MinimizeNthLargestUtilizationSpec(
            name="cap-top-3-servers",
            scope="server",
            dimension="disk",
            n=2,  # 3rd largest (0-based)
        ),
        weight=1.0,
    )
    # cap_top_3_end


def with_target():
    """With target utilization to prevent under-utilization."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # with_target_start
    # Minimize 2nd largest, but don't go below 70% utilization
    solver.addGoal(
        MinimizeNthLargestUtilizationSpec(
            name="minimize-with-target",
            scope="host",
            dimension="cpu",
            n=1,
            targetUtilization=0.7,  # Don't minimize below 70%
        ),
        weight=1.0,
    )
    # with_target_end
