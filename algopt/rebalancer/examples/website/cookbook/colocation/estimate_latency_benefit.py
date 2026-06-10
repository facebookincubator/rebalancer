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
Estimate Latency Benefit from Colocation

This example demonstrates how to estimate the latency improvement achieved
through service colocation.

Use this when: You want to quantify the performance benefit of colocation
to justify the optimization effort.
"""

# pyre-strict


# variation_start
def estimate_latency_benefit(
    initial_assignment,
    final_assignment,
    service_pairs,
    intra_host_latency_ms=0.1,
    inter_host_latency_ms=2.0,
):
    """Estimate latency improvement from colocation.

    Args:
        initial_assignment: Assignment before optimization
        final_assignment: Assignment after optimization
        service_pairs: List of (instance_a, instance_b) tuples
        intra_host_latency_ms: Latency within same host (default 0.1ms)
        inter_host_latency_ms: Latency across hosts (default 2.0ms)

    Returns:
        Dict with latency metrics
    """

    def calculate_avg_latency(assignment):
        total_latency = 0
        for instance_a, instance_b in service_pairs:
            # Find hosts
            host_a = None
            host_b = None
            for host, instances in assignment.items():
                if instance_a in instances:
                    host_a = host
                if instance_b in instances:
                    host_b = host

            if host_a == host_b:
                total_latency += intra_host_latency_ms
            else:
                total_latency += inter_host_latency_ms

        return total_latency / len(service_pairs)

    initial_latency = calculate_avg_latency(initial_assignment)
    final_latency = calculate_avg_latency(final_assignment)
    improvement_pct = ((initial_latency - final_latency) / initial_latency) * 100

    print(f"Latency analysis:")
    print(f"  Initial avg latency: {initial_latency:.2f}ms")
    print(f"  Final avg latency: {final_latency:.2f}ms")
    print(f"  Improvement: {improvement_pct:.1f}%")

    return {
        "initial_latency_ms": initial_latency,
        "final_latency_ms": final_latency,
        "improvement_pct": improvement_pct,
    }


# variation_end


if __name__ == "__main__":
    # This is a snippet - would be integrated into a full solution
    # See basic_colocation.py for the complete example
    print("Latency benefit estimation")
    print("Calculate performance improvement from colocation")
