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
Dynamic Pair Definition Based on Traffic

This variation demonstrates how to dynamically create service pairs based on
observed runtime communication patterns rather than static definitions.

Use this when: Service communication patterns change over time and you want
to optimize based on actual traffic data.
"""

# pyre-strict
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    ColocateGroupsSpec,
    ColocateGroupsSpecBound,
    Limit,
    LimitType,
)


# variation_start
def create_pairs_from_traffic(traffic_matrix):
    """Create service pairs based on observed traffic patterns.

    Args:
        traffic_matrix: Dict[tuple[str, str], float] - (service_a, service_b) -> requests/sec

    Returns:
        Partition mapping for high-traffic pairs
    """
    pairs = {}
    pair_id = 0

    # Find instance pairs with high traffic between them
    for (instance_a, instance_b), traffic in traffic_matrix.items():
        if traffic > TRAFFIC_THRESHOLD:  # High traffic
            pair_name = f"dynamic_pair_{pair_id}"
            pairs[pair_name] = [instance_a, instance_b]
            pair_id += 1

    return pairs


# Use observed traffic patterns
traffic_data = get_traffic_metrics()
dynamic_pairs = create_pairs_from_traffic(traffic_data)
solver.addPartition("dynamic_pairs", dynamic_pairs)

solver.addGoal(
    ColocateGroupsSpec(
        name="colocate-high-traffic",
        scope="host",
        partitionName="dynamic_pairs",
        bound=ColocateGroupsSpecBound.MAX,
        limits=Limit(type=LimitType.ABSOLUTE, globalLimit=1),
    ),
    weight=6.0,
)
# variation_end


if __name__ == "__main__":
    # This is a snippet - would be integrated into a full solution
    # See basic_colocation.py for the complete example
    print("Dynamic pair definition variation")
    print("Create service pairs based on observed traffic patterns")
