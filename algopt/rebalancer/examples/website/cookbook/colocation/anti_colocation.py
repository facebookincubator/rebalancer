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
Anti-Colocation - Keeping Services Apart

This variation demonstrates how to prevent conflicting services from sharing
hosts to avoid resource contention.

Use this when: Services compete for the same resources (CPU, I/O, network)
and should be separated to prevent performance degradation.
"""

# pyre-strict
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    ColocateGroupsSpec,
    ColocateGroupsSpecBound,
    Limit,
    LimitType,
)


def add_anti_colocation(solver):
    """Prevent conflicting services from sharing hosts."""
    # variation_start
    # Define conflicting service pairs (resource contention)
    conflict_partition = {
        "conflict_0": ["heavy_cpu_service_0", "heavy_cpu_service_1"],
        "conflict_1": ["heavy_io_service_0", "heavy_io_service_1"],
    }
    solver.addPartition("conflicts", conflict_partition)

    # Spread conflicting services (MIN = keep apart)
    solver.addGoal(
        ColocateGroupsSpec(
            name="separate-conflicts",
            scope="host",
            partitionName="conflicts",
            bound=ColocateGroupsSpecBound.MIN,
            limits=Limit(type=LimitType.ABSOLUTE, globalLimit=2),
        ),
        weight=7.0,  # High weight - avoid resource contention
    )
    # variation_end


if __name__ == "__main__":
    # This is a snippet - would be integrated into a full solution
    # See basic_colocation.py for the complete example
    print("Anti-colocation variation")
    print("Prevent conflicting services from sharing hosts")
