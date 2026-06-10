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
Weighted Colocation Priority

This variation demonstrates how to assign different importance levels to
different colocation pairs based on their business impact.

Use this when: Some service pairs have higher performance requirements than
others (e.g., user-facing vs internal services).
"""

# pyre-strict
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    ColocateGroupsSpec,
    ColocateGroupsSpecBound,
    Limit,
    LimitType,
)


def add_weighted_colocation_priority(solver):
    """Add colocation goals with different priorities."""
    # variation_start
    # High-priority pairs (frontend-API) - stronger colocation
    solver.addGoal(
        ColocateGroupsSpec(
            name="colocate-frontend-api",
            scope="host",
            partitionName="frontend_api_pairs",
            bound=ColocateGroupsSpecBound.MAX,
            limits=Limit(type=LimitType.ABSOLUTE, globalLimit=1),
        ),
        weight=8.0,  # Critical for user-facing latency
    )

    # Medium-priority pairs (API-cache) - weaker colocation
    solver.addGoal(
        ColocateGroupsSpec(
            name="colocate-api-cache",
            scope="host",
            partitionName="api_cache_pairs",
            bound=ColocateGroupsSpecBound.MAX,
            limits=Limit(type=LimitType.ABSOLUTE, globalLimit=1),
        ),
        weight=3.0,  # Nice to have but not critical
    )
    # variation_end


if __name__ == "__main__":
    # This is a snippet - would be integrated into a full solution
    # See basic_colocation.py for the complete example
    print("Weighted colocation priority variation")
    print("Assign different weights based on business importance")
