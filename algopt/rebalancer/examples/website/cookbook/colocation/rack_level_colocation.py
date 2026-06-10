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
Rack-Level Colocation

This variation demonstrates how to colocate services at the rack level instead
of the host level, providing a balance between performance and fault tolerance.

Use this when: Host-level colocation is too strict, but you still want services
in the same rack for reduced network latency.
"""

# pyre-strict
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    ColocateGroupsSpec,
    ColocateGroupsSpecBound,
    Limit,
    LimitType,
)


def add_rack_level_colocation(solver):
    """Colocate services at rack level instead of host level."""
    # variation_start
    # Colocate service pairs at rack level (lower latency within rack)
    solver.addGoal(
        ColocateGroupsSpec(
            name="colocate-pairs-rack-level",
            scope="rack",
            partitionName="service_pair",
            bound=ColocateGroupsSpecBound.MAX,
            limits=Limit(type=LimitType.ABSOLUTE, globalLimit=1),
        ),
        weight=5.0,
    )

    # But spread each service across racks (fault tolerance)
    solver.addGoal(
        ColocateGroupsSpec(
            name="spread-services-rack-level",
            scope="rack",
            partitionName="service_type",
            bound=ColocateGroupsSpecBound.MIN,
            limits=Limit(type=LimitType.ABSOLUTE, globalLimit=3),
        ),
        weight=3.0,
    )
    # variation_end


if __name__ == "__main__":
    # This is a snippet - would be integrated into a full solution
    # See basic_colocation.py for the complete example
    print("Rack-level colocation variation")
    print("Colocate at rack level for balance of performance and fault tolerance")
