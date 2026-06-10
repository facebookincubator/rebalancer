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
Verify Colocation Results

This example demonstrates how to verify that the solution achieved the target
colocation percentage.

Use this when: You want to validate that the optimization met your colocation
requirements before deploying changes.
"""

# pyre-strict


# variation_start
def verify_colocation(solution, service_pair_partition, target_pct=80):
    """Verify colocation percentage meets target.

    Args:
        solution: Solver solution
        service_pair_partition: Partition defining service pairs
        target_pct: Target colocation percentage (default 80%)

    Returns:
        True if target met
    """
    colocated_pairs = 0

    for instances in service_pair_partition.values():
        # Find hosts with these instances
        hosts_with_instances = set()
        for host, host_instances in solution.assignment.items():
            if any(instance in host_instances for instance in instances):
                hosts_with_instances.add(host)

        # Colocated if all instances on 1 host
        if len(hosts_with_instances) == 1:
            colocated_pairs += 1

    total_pairs = len(service_pair_partition)
    colocation_pct = (colocated_pairs / total_pairs) * 100

    if colocation_pct >= target_pct:
        print(
            f"✅ Colocation target met: {colocation_pct:.1f}% (target: {target_pct}%)"
        )
        return True
    else:
        print(
            f"❌ Colocation below target: {colocation_pct:.1f}% (target: {target_pct}%)"
        )
        return False


# variation_end


if __name__ == "__main__":
    # This is a snippet - would be integrated into a full solution
    # See basic_colocation.py for the complete example
    print("Verification example")
    print("Check if colocation percentage meets target requirements")
