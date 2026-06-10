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
ThrottlingSpec Reference Examples

This file demonstrates all the usage patterns for ThrottlingSpec shown in the
reference documentation. Each function is a complete, runnable example.
"""

# example_start
# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    ConstraintSpecs,
    GoalSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    Filter,
    Limit,
    LimitType,
    MinimizeMovementSpec,
    ThrottlingSpec,
    ThrottlingSpecDefinition,
    ToFreeSpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


def quick_example():
    """Quick example showing basic ThrottlingSpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # quick_example_start
    # Limit data moving OUT of hosts being drained (max 500 GB)
    draining_hosts = ["old_host_1", "old_host_2"]

    solver.addConstraint(
        ConstraintSpecs(
            throttlingSpec=ThrottlingSpec(
                name="throttle-drain",
                scope="host",
                dimension="data_size",
                definition=ThrottlingSpecDefinition.OUT,  # Count moves OUT
                limit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=500.0,  # Max 500 GB total
                ),
                filter=Filter(items=draining_hosts),
            )
        )
    )
    # quick_example_end


def throttle_host_draining():
    """Throttle host draining - limit data leaving hosts being drained."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # throttle_drain_start
    # Hosts being decommissioned
    draining_hosts = ["old_host_1", "old_host_2", "old_host_3"]

    # Limit outbound data movement (network bandwidth protection)
    solver.addConstraint(
        ConstraintSpecs(
            throttlingSpec=ThrottlingSpec(
                name="drain-throttle",
                scope="host",
                dimension="data_size",  # GB
                definition=ThrottlingSpecDefinition.OUT,  # Leaving hosts
                limit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=1000.0,  # Max 1TB total across all draining hosts
                ),
                filter=Filter(items=draining_hosts),
            )
        )
    )
    # throttle_drain_end


def limit_rack_network_usage():
    """Limit rack network usage - prevent saturating rack network."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # limit_rack_network_start
    # Rack with limited network capacity
    bandwidth_limited_racks = ["rack5", "rack8"]

    # Limit total data movement through these racks
    solver.addConstraint(
        ConstraintSpecs(
            throttlingSpec=ThrottlingSpec(
                name="rack-bandwidth-limit",
                scope="rack",
                dimension="data_size",
                definition=ThrottlingSpecDefinition.ANY,  # In or out
                limit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=2000.0,  # Max 2TB moved through rack
                ),
                filter=Filter(items=bandwidth_limited_racks),
            )
        )
    )
    # limit_rack_network_end


def per_host_throttling():
    """Per-host throttling - different limits for different hosts."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # per_host_throttling_start
    # Per-host bandwidth limits
    host_bandwidth_limits = {
        "slow_network_host_1": 100.0,  # 100 GB
        "slow_network_host_2": 100.0,
        "fast_network_host_1": 500.0,  # 500 GB
    }

    solver.addConstraint(
        ConstraintSpecs(
            throttlingSpec=ThrottlingSpec(
                name="per-host-throttle",
                scope="host",
                dimension="data_size",
                definition=ThrottlingSpecDefinition.ANY,
                limit=Limit(
                    type=LimitType.ABSOLUTE,
                    scopeItemLimits=host_bandwidth_limits,
                    globalLimit=200.0,  # Default for other hosts
                ),
            )
        )
    )
    # per_host_throttling_end


def gradual_migration():
    """Gradual migration - limit incoming data to new datacenter."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # gradual_migration_start
    # New datacenter hosts
    new_dc_hosts = ["new_dc_host_1", "new_dc_host_2", "new_dc_host_3"]

    # Limit incoming data (gradual migration)
    solver.addConstraint(
        ConstraintSpecs(
            throttlingSpec=ThrottlingSpec(
                name="gradual-migration",
                scope="host",
                dimension="data_size",
                definition=ThrottlingSpecDefinition.IN,  # Coming in
                limit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=500.0,  # Max 500 GB migrated in this round
                ),
                filter=Filter(items=new_dc_hosts),
            )
        )
    )
    # gradual_migration_end


def protect_production_hosts():
    """Protect production hosts - limit churn on production hosts."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # protect_prod_start
    # Critical production hosts
    prod_hosts = ["prod_host_1", "prod_host_2"]

    # Limit any movement (stability)
    solver.addConstraint(
        ConstraintSpecs(
            throttlingSpec=ThrottlingSpec(
                name="prod-stability",
                scope="host",
                dimension="data_size",
                definition=ThrottlingSpecDefinition.ANY,
                limit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=50.0,  # Very small limit - minimize disruption
                ),
                filter=Filter(items=prod_hosts),
            )
        )
    )
    # protect_prod_end


def network_aware_throttling():
    """Network-aware throttling - throttle based on network traffic instead of data size."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # network_aware_start
    # Dimension: estimated network transfer time
    all_objects = ["obj0", "obj1", "obj2"]
    data_size = {"obj0": 100.0, "obj1": 200.0, "obj2": 300.0}
    network_bandwidth = {"obj0": 10.0, "obj1": 10.0, "obj2": 10.0}
    hosts_to_drain = ["drain_host_1"]

    network_transfer_time = {
        obj: data_size[obj] / network_bandwidth[obj] for obj in all_objects
    }
    solver.addObjectDimension("transfer_time", network_transfer_time)

    # Limit total transfer time (network hours)
    solver.addConstraint(
        ConstraintSpecs(
            throttlingSpec=ThrottlingSpec(
                name="network-time-limit",
                scope="host",
                dimension="transfer_time",  # Hours
                definition=ThrottlingSpecDefinition.OUT,
                limit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=10.0,  # Max 10 hours of network transfer
                ),
                filter=Filter(items=hosts_to_drain),
            )
        )
    )
    # network_aware_end


def object_count_throttling():
    """Object count throttling - limit number of objects moved (not data size)."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # object_count_throttling_start
    # Use count dimension
    all_objects = ["obj0", "obj1", "obj2", "obj3"]
    sensitive_hosts = ["sensitive_host_1"]

    object_count = {obj: 1.0 for obj in all_objects}
    solver.addObjectDimension("count", object_count)

    # Limit number of objects moved OUT
    solver.addConstraint(
        ConstraintSpecs(
            throttlingSpec=ThrottlingSpec(
                name="object-count-limit",
                scope="host",
                dimension="count",
                definition=ThrottlingSpecDefinition.OUT,
                limit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=20.0,  # Max 20 objects move out
                ),
                filter=Filter(items=sensitive_hosts),
            )
        )
    )
    # object_count_throttling_end


def pitfall_limit_too_restrictive_bad():
    """PITFALL: Limit too restrictive - need to drain 1TB but limit=10GB."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # pitfall_restrictive_bad_start
    hosts_with_1TB = ["host_with_1TB"]

    # BAD: Need to drain 1TB but limit=10GB
    solver.addConstraint(
        ConstraintSpecs(
            throttlingSpec=ThrottlingSpec(
                name="too-restrictive",
                scope="host",
                dimension="data_size",
                definition=ThrottlingSpecDefinition.OUT,
                limit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=10.0,  # Only 10GB!
                ),
                filter=Filter(items=hosts_with_1TB),
            )
        )
    )
    # Will take 100 rounds to drain!
    # pitfall_restrictive_bad_end


def pitfall_limit_too_restrictive_good():
    """SOLUTION: Calculate reasonable limit based on network capacity."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # pitfall_restrictive_good_start
    hosts_to_drain = ["host_to_drain"]

    # GOOD: Calculate reasonable limit
    network_bandwidth_gbps = 10.0  # 10 Gbps
    migration_window_hours = 1.0
    max_transfer_gb = network_bandwidth_gbps * migration_window_hours * 3600 / 8

    solver.addConstraint(
        ConstraintSpecs(
            throttlingSpec=ThrottlingSpec(
                name="reasonable-limit",
                scope="host",
                dimension="data_size",
                definition=ThrottlingSpecDefinition.OUT,
                limit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=max_transfer_gb,  # ~4500 GB
                ),
                filter=Filter(items=hosts_to_drain),
            )
        )
    )
    # pitfall_restrictive_good_end


def pitfall_wrong_definition_bad():
    """PITFALL: Using wrong direction for intended throttling."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # pitfall_wrong_definition_bad_start
    hosts_to_drain = ["host_to_drain"]

    # BAD: Want to limit draining (OUT), but using IN
    solver.addConstraint(
        ConstraintSpecs(
            throttlingSpec=ThrottlingSpec(
                name="wrong-direction",
                scope="host",
                dimension="data_size",
                definition=ThrottlingSpecDefinition.IN,  # Wrong!
                limit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=500.0,
                ),
                filter=Filter(items=hosts_to_drain),
            )
        )
    )
    # This limits moves INTO draining hosts, not OUT
    # pitfall_wrong_definition_bad_end


def pitfall_wrong_definition_good():
    """SOLUTION: Use correct definition for draining."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # pitfall_wrong_definition_good_start
    hosts_to_drain = ["host_to_drain"]

    # GOOD: OUT for draining
    solver.addConstraint(
        ConstraintSpecs(
            throttlingSpec=ThrottlingSpec(
                name="correct-direction",
                scope="host",
                dimension="data_size",
                definition=ThrottlingSpecDefinition.OUT,  # Correct
                limit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=500.0,
                ),
                filter=Filter(items=hosts_to_drain),
            )
        )
    )
    # pitfall_wrong_definition_good_end


def pitfall_filter_not_specified_bad():
    """PITFALL: No filter means throttling applies to ALL containers."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # pitfall_no_filter_bad_start
    # BAD: Throttles all hosts to 100GB total
    solver.addConstraint(
        ConstraintSpecs(
            throttlingSpec=ThrottlingSpec(
                name="no-filter",
                scope="host",
                dimension="data_size",
                definition=ThrottlingSpecDefinition.ANY,
                limit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=100.0,
                ),
                # No filter!
            )
        )
    )
    # Only 100GB can move across entire cluster!
    # pitfall_no_filter_bad_end


def pitfall_filter_not_specified_good():
    """SOLUTION: Specify filter for target hosts."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # pitfall_no_filter_good_start
    target_hosts = ["target_host_1", "target_host_2"]

    # GOOD: Only throttle specific hosts
    solver.addConstraint(
        ConstraintSpecs(
            throttlingSpec=ThrottlingSpec(
                name="with-filter",
                scope="host",
                dimension="data_size",
                definition=ThrottlingSpecDefinition.ANY,
                limit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=100.0,
                ),
                filter=Filter(items=target_hosts),  # Specified
            )
        )
    )
    # pitfall_no_filter_good_end


def pitfall_dimension_not_defined_bad():
    """PITFALL: Dimension doesn't exist."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # pitfall_no_dimension_bad_start
    # BAD: Dimension "bandwidth" not added
    solver.addConstraint(
        ConstraintSpecs(
            throttlingSpec=ThrottlingSpec(
                name="missing-dimension",
                scope="host",
                dimension="bandwidth",  # Doesn't exist!
                limit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=1000.0,
                ),
            )
        )
    )
    # pitfall_no_dimension_bad_end


def pitfall_dimension_not_defined_good():
    """SOLUTION: Define dimension first."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # pitfall_no_dimension_good_start
    # GOOD: Define dimension
    data_sizes = {"shard0": 100.0, "shard1": 200.0}
    solver.addObjectDimension("data_size", data_sizes)

    solver.addConstraint(
        ConstraintSpecs(
            throttlingSpec=ThrottlingSpec(
                name="with-dimension",
                scope="host",
                dimension="data_size",
                limit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=1000.0,
                ),
            )
        )
    )
    # pitfall_no_dimension_good_end


def combining_with_minimize_movement():
    """Combining with MinimizeMovementSpec - soft + hard movement limits."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # combining_minimize_movement_start
    # Soft: Prefer minimal movement
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="prefer-minimal",
                dimension="data_size",
            )
        ),
        weight=2.0,
    )

    # Hard: Never exceed bandwidth limit
    solver.addConstraint(
        ConstraintSpecs(
            throttlingSpec=ThrottlingSpec(
                name="bandwidth-limit",
                scope="host",
                dimension="data_size",
                definition=ThrottlingSpecDefinition.ANY,
                limit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=1000.0,
                ),
            )
        )
    )
    # combining_minimize_movement_end


def combining_with_to_free():
    """Combining with ToFreeSpec - throttled draining."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # combining_to_free_start
    hosts_to_drain = ["drain_host_1", "drain_host_2"]

    # Goal: Drain these hosts
    solver.addConstraint(
        ConstraintSpecs(
            toFreeSpec=ToFreeSpec(
                name="drain-hosts",
                scope="host",
                containers=hosts_to_drain,
            )
        )
    )

    # Constraint: But don't exceed bandwidth limit
    solver.addConstraint(
        ConstraintSpecs(
            throttlingSpec=ThrottlingSpec(
                name="throttle-drain-rate",
                scope="host",
                dimension="data_size",
                definition=ThrottlingSpecDefinition.OUT,
                limit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=500.0,
                ),
                filter=Filter(items=hosts_to_drain),
            )
        )
    )
    # combining_to_free_end


def verify_throttling(
    initial_assignment,
    final_assignment,
    dimension_values,
    filtered_containers,
    definition,
    max_limit,
):
    """Verify throttling limit was respected.

    Args:
        initial_assignment: Initial assignment
        final_assignment: Final assignment
        dimension_values: Object dimension values
        filtered_containers: Filtered containers
        definition: ThrottlingSpecDefinition (IN/OUT/ANY)
        max_limit: Maximum allowed movement volume

    Returns:
        True if limit respected
    """
    # verification_start
    total_volume = 0.0

    for container in filtered_containers:
        initial_objects = set(initial_assignment.get(container, []))
        final_objects = set(final_assignment.get(container, []))

        if (
            definition == ThrottlingSpecDefinition.OUT
            or definition == ThrottlingSpecDefinition.ANY
        ):
            # Objects that left
            left = initial_objects - final_objects
            volume_out = sum(dimension_values.get(obj, 0.0) for obj in left)
            total_volume += volume_out

        if (
            definition == ThrottlingSpecDefinition.IN
            or definition == ThrottlingSpecDefinition.ANY
        ):
            # Objects that arrived
            arrived = final_objects - initial_objects
            volume_in = sum(dimension_values.get(obj, 0.0) for obj in arrived)
            total_volume += volume_in

    if total_volume <= max_limit:
        print(f"✅ Throttling respected: {total_volume:.1f} ≤ {max_limit}")
        return True
    else:
        print(f"❌ Throttling violated: {total_volume:.1f} > {max_limit}")
        print(f"   Excess: {total_volume - max_limit:.1f}")
        return False
    # verification_end


def complete_example():
    """
    Complete runnable example demonstrating ThrottlingSpec.

    This example shows how to use ThrottlingSpec to throttle data movement
    while draining hosts, protecting network bandwidth.
    """
    # Create solver
    solver = ProblemSolver(
        service_name="throttling-example", service_scope="production"
    )

    solver.setObjectName("shard")
    solver.setContainerName("host")

    # Initial assignment with hosts to drain
    assignment = {
        "host0": ["shard0", "shard1", "shard2"],
        "host1": ["shard3", "shard4"],
        "host2": [],
        "host3": [],
        "drain_host": ["shard5", "shard6", "shard7", "shard8"],
    }
    solver.setAssignment(assignment)

    # Shard data sizes (GB)
    data_sizes = {
        "shard0": 100.0,
        "shard1": 150.0,
        "shard2": 200.0,
        "shard3": 120.0,
        "shard4": 180.0,
        "shard5": 300.0,
        "shard6": 250.0,
        "shard7": 200.0,
        "shard8": 150.0,
    }
    solver.addObjectDimension("data_size", data_sizes)

    # Host capacities (GB)
    host_capacities = {
        "host0": 1000.0,
        "host1": 1000.0,
        "host2": 1000.0,
        "host3": 1000.0,
        "drain_host": 1000.0,
    }
    solver.addContainerDimension("capacity", host_capacities)

    # Drain the drain_host
    solver.addConstraint(
        ConstraintSpecs(
            toFreeSpec=ToFreeSpec(
                name="drain-host",
                scope="host",
                containers=["drain_host"],
            )
        )
    )

    # Throttle data movement OUT of drain_host (max 500 GB)
    solver.addConstraint(
        ConstraintSpecs(
            throttlingSpec=ThrottlingSpec(
                name="throttle-drain",
                scope="host",
                dimension="data_size",
                definition=ThrottlingSpecDefinition.OUT,
                limit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=500.0,  # Max 500 GB in this round
                ),
                filter=Filter(items=["drain_host"]),
            )
        )
    )

    # Solve
    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=10000))
    )
    solution = solver.solve()

    # Print results
    print(f"Solution found in {solution.profile.solveTime}ms")
    print(f"Objective value: {solution.objectiveValue}")
    print(f"Shards moved: {solution.profile.moveCount}")

    return solution


if __name__ == "__main__":
    print("Running all Throttling examples...")

    print("\n1. Quick Example...")
    quick_example()

    print("\n2. Throttle Host Draining...")
    throttle_host_draining()

    print("\n3. Limit Rack Network Usage...")
    limit_rack_network_usage()

    print("\n4. Per Host Throttling...")
    per_host_throttling()

    print("\n5. Gradual Migration...")
    gradual_migration()

    print("\n6. Protect Production Hosts...")
    protect_production_hosts()

    print("\n7. Network Aware Throttling...")
    network_aware_throttling()

    print("\n8. Object Count Throttling...")
    object_count_throttling()

    print("\n9. Pitfall Limit Too Restrictive Bad...")
    pitfall_limit_too_restrictive_bad()

    print("\n10. Pitfall Limit Too Restrictive Good...")
    pitfall_limit_too_restrictive_good()

    print("\n11. Pitfall Wrong Definition Bad...")
    pitfall_wrong_definition_bad()

    print("\n12. Pitfall Wrong Definition Good...")
    pitfall_wrong_definition_good()

    print("\n13. Pitfall Filter Not Specified Bad...")
    pitfall_filter_not_specified_bad()

    print("\n14. Pitfall Filter Not Specified Good...")
    pitfall_filter_not_specified_good()

    print("\n15. Pitfall Dimension Not Defined Bad...")
    pitfall_dimension_not_defined_bad()

    print("\n16. Pitfall Dimension Not Defined Good...")
    pitfall_dimension_not_defined_good()

    print("\n17. Combining With Minimize Movement...")
    combining_with_minimize_movement()

    print("\n18. Combining With To Free...")
    combining_with_to_free()

    print("\n19. Complete Example...")
    complete_example()

    print("\n✓ All Throttling examples completed successfully!")
# example_end
