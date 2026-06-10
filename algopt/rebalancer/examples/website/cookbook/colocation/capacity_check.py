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
Capacity Check for Colocation Feasibility

This example demonstrates how to verify that host capacity is sufficient for
colocating services before attempting optimization.

Use this when: You want to ensure colocation is physically possible given
resource constraints.
"""

# pyre-strict


def check_colocation_capacity(cpu_usage, host_cpu_capacity, service_pairs):
    """Check if colocation is feasible given capacity constraints."""
    # variation_start
    # Check if colocation is feasible
    max_colocated_cpu = max(
        cpu_usage[instance_a] + cpu_usage[instance_b]
        for pair in service_pairs
        for instance_a, instance_b in pair
    )

    assert max_colocated_cpu <= min(host_cpu_capacity.values()), (
        "Host capacity insufficient for colocation!"
    )
    # variation_end


if __name__ == "__main__":
    # This is a snippet - would be integrated into a full solution
    # See basic_colocation.py for the complete example
    print("Capacity check variation")
    print("Verify host capacity is sufficient for colocation")
