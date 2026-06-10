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
Tuning Goal Weights for Conflicting Goals

This example shows how to tune the weight ratio between colocation and diversity
goals to achieve the desired balance.

Use this when: You need to prioritize either performance (colocation) or
fault tolerance (diversity) based on your specific requirements.
"""

# pyre-strict
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import ColocateGroupsSpec


def tune_conflicting_goals(solver):
    """Examples of tuning weight ratios for different priorities."""
    # variation_start
    # Prioritize colocation (accept less diversity)
    solver.addGoal(ColocateGroupsSpec(...), weight=10.0)  # High
    solver.addGoal(ColocateGroupsSpec(MIN, ...), weight=1.0)  # Low

    # Prioritize diversity (accept less colocation)
    solver.addGoal(ColocateGroupsSpec(...), weight=2.0)  # Low
    solver.addGoal(ColocateGroupsSpec(MIN, ...), weight=8.0)  # High
    # variation_end


if __name__ == "__main__":
    # This is a snippet - would be integrated into a full solution
    # See basic_colocation.py for the complete example
    print("Conflicting goals tuning variation")
    print("Adjust weights to prioritize colocation vs diversity")
