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
Correct Partition Definition for Colocation

This example demonstrates the correct way to define partitions for service
pairs versus common mistakes.

Use this when: You need to understand how to properly structure partitions
for colocation goals.
"""

# pyre-strict


def define_partitions_correctly():
    """Examples of correct vs incorrect partition definitions."""
    # variation_start
    # BAD: All instances in one group
    bad_partition = {
        "all_services": list(all_instances)  # Won't colocate pairs!
    }

    # GOOD: Each pair is a separate group
    good_partition = {
        f"pair_{i}": [instance_a, instance_b]
        for i, (instance_a, instance_b) in enumerate(pairs)
    }
    # variation_end


if __name__ == "__main__":
    # This is a snippet - would be integrated into a full solution
    # See basic_colocation.py for the complete example
    print("Partition definition variation")
    print("Each service pair must be a separate partition group")
