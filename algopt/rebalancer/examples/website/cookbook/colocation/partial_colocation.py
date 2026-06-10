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
Partial Colocation - Subset of Services

This variation demonstrates how to colocate only a subset of service pairs
(e.g., high-traffic pairs) while leaving others unconstrained.

Use this when: Only some service pairs have strong colocation requirements
based on traffic patterns or business logic.
"""

# pyre-strict
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    ColocateGroupsSpec,
    ColocateGroupsSpecBound,
    Limit,
    LimitType,
)


def add_partial_colocation(solver):
    """Colocate only high-traffic pairs."""
    # variation_start
    # Colocate only high-traffic pairs (instances 0-9)
    high_traffic_pairs = {}
    for i in range(10):  # Only first 10 pairs
        pair_name = f"frontend_api_pair_{i}"
        high_traffic_pairs[pair_name] = [
            f"frontend_instance_{i}",
            f"api_instance_{i}",
        ]

    solver.addPartition("high_traffic_pairs", high_traffic_pairs)

    solver.addGoal(
        ColocateGroupsSpec(
            name="colocate-high-traffic",
            scope="host",
            partitionName="high_traffic_pairs",
            bound=ColocateGroupsSpecBound.MAX,
            limits=Limit(type=LimitType.ABSOLUTE, globalLimit=1),
        ),
        weight=10.0,  # Very high weight - these MUST colocate
    )
    # variation_end


if __name__ == "__main__":
    # This is a snippet - would be integrated into a full solution
    # See basic_colocation.py for the complete example
    print("Partial colocation variation")
    print("Colocate only high-traffic service pairs (subset)")
