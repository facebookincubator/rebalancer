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
Adjusting Weight Balance

This example shows how to increase colocation weight when services aren't
colocating as expected.

Use this when: The solver isn't achieving enough colocation due to competing
goals with higher weights.
"""

# pyre-strict
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import ColocateGroupsSpec


def adjust_weight_balance(solver):
    """Increase colocation weight if not happening."""
    # variation_start
    # If colocation not happening, increase weight
    solver.addGoal(
        ColocateGroupsSpec(...),
        weight=10.0,  # Increase from 5.0
    )
    # variation_end


if __name__ == "__main__":
    # This is a snippet - would be integrated into a full solution
    # See basic_colocation.py for the complete example
    print("Weight balance variation")
    print("Increase colocation weight to achieve better results")
