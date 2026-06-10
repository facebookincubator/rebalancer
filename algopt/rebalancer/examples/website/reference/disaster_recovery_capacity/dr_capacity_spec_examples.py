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
DisasterRecoveryCapacitySpec Reference Examples

This file demonstrates all the usage patterns for DisasterRecoveryCapacitySpec
shown in the reference documentation. Each function is a complete, runnable example.
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
    BalanceSpec,
    CapacitySpec,
    DisasterRecoveryCapacitySpec,
    GroupCountSpec,
    GroupCountSpecBound,
    Limit,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


def quick_example():
    """Quick example showing basic DisasterRecoveryCapacitySpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("database")
    solver.setContainerName("rack")

    # quick_example_start
    # Ensure capacity after single rack failure
    solver.addConstraint(
        ConstraintSpecs(
            disasterRecoveryCapacitySpec=DisasterRecoveryCapacitySpec(
                name="rack-failure-dr",
                scope="rack",
                dimension="cpu_usage",
                sharedDisasterGroups=[
                    {"rack1"},  # Scenario: rack1 fails
                    {"rack2"},  # Scenario: rack2 fails
                    {"rack3"},  # Scenario: rack3 fails
                ],
                primaryToSetOfSecondaryObjects={
                    "db_primary_0": ["db_replica_0_a", "db_replica_0_b"],
                    "db_primary_1": ["db_replica_1_a", "db_replica_1_b"],
                    "db_primary_2": ["db_replica_2_a", "db_replica_2_b"],
                },
            )
        )
    )
    # quick_example_end


def single_rack_failure():
    """Pattern 1: Single Rack Failure - ensure capacity after any single rack fails."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("database")
    solver.setContainerName("rack")

    # single_rack_failure_start
    # Database with primary-replica setup
    primaries = ["db_shard_0_primary", "db_shard_1_primary", "db_shard_2_primary"]
    replicas = {
        "db_shard_0_primary": ["db_shard_0_replica_a", "db_shard_0_replica_b"],
        "db_shard_1_primary": ["db_shard_1_replica_a", "db_shard_1_replica_b"],
        "db_shard_2_primary": ["db_shard_2_replica_a", "db_shard_2_replica_b"],
    }

    # Each rack as separate disaster group
    all_racks = ["rack1", "rack2", "rack3", "rack4"]

    solver.addConstraint(
        ConstraintSpecs(
            disasterRecoveryCapacitySpec=DisasterRecoveryCapacitySpec(
                name="single-rack-failure",
                scope="rack",
                dimension="cpu_usage",
                sharedDisasterGroups=[
                    {rack} for rack in all_racks
                ],  # Each rack fails independently
                primaryToSetOfSecondaryObjects=replicas,
            )
        )
    )
    # single_rack_failure_end


def datacenter_failure():
    """Pattern 2: Datacenter Failure - ensure capacity after entire datacenter fails."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("service")
    solver.setContainerName("rack")

    # datacenter_failure_start
    # Services with cross-datacenter replicas
    services = {
        "web_service_dc1": ["web_service_dc2", "web_service_dc3"],
        "api_service_dc1": ["api_service_dc2", "api_service_dc3"],
        "db_service_dc1": ["db_service_dc2", "db_service_dc3"],
    }

    # Datacenters
    datacenters = {
        "dc1": ["rack1", "rack2", "rack3"],
        "dc2": ["rack4", "rack5", "rack6"],
        "dc3": ["rack7", "rack8", "rack9"],
    }

    solver.addConstraint(
        ConstraintSpecs(
            disasterRecoveryCapacitySpec=DisasterRecoveryCapacitySpec(
                name="datacenter-failure",
                scope="rack",
                dimension="network_bandwidth",
                sharedDisasterGroups=[
                    set(datacenters["dc1"]),  # All racks in DC1 fail together
                    set(datacenters["dc2"]),  # All racks in DC2 fail together
                    set(datacenters["dc3"]),  # All racks in DC3 fail together
                ],
                primaryToSetOfSecondaryObjects=services,
            )
        )
    )
    # datacenter_failure_end


def availability_zone_failure():
    """Pattern 3: Availability Zone Failure - cloud provider AZ failure tolerance."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # availability_zone_failure_start
    # Objects per AZ
    az_hosts = {
        "us-east-1a": ["host_1a_1", "host_1a_2", "host_1a_3"],
        "us-east-1b": ["host_1b_1", "host_1b_2", "host_1b_3"],
        "us-east-1c": ["host_1c_1", "host_1c_2", "host_1c_3"],
    }

    # Primary to secondary mapping
    primary_to_secondary = {
        "shard_0_primary_1a": ["shard_0_replica_1b", "shard_0_replica_1c"],
        "shard_1_primary_1b": ["shard_1_replica_1a", "shard_1_replica_1c"],
        "shard_2_primary_1c": ["shard_2_replica_1a", "shard_2_replica_1b"],
    }

    solver.addConstraint(
        ConstraintSpecs(
            disasterRecoveryCapacitySpec=DisasterRecoveryCapacitySpec(
                name="az-failure-tolerance",
                scope="host",
                dimension="memory",
                sharedDisasterGroups=[
                    set(az_hosts["us-east-1a"]),  # AZ-a fails
                    set(az_hosts["us-east-1b"]),  # AZ-b fails
                    set(az_hosts["us-east-1c"]),  # AZ-c fails
                ],
                primaryToSetOfSecondaryObjects=primary_to_secondary,
            )
        )
    )
    # availability_zone_failure_end


def multi_replica_failover():
    """Pattern 4: Multi-Replica Failover - multiple replica promotion levels."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("database")
    solver.setContainerName("rack")

    # multi_replica_failover_start
    # Primary → replica1 (first backup) → replica2 (second backup)
    multi_level_replicas = {
        "db_primary_0": ["db_replica_0_level1", "db_replica_0_level2"],
        "db_primary_1": ["db_replica_1_level1", "db_replica_1_level2"],
        "db_primary_2": ["db_replica_2_level1", "db_replica_2_level2"],
    }

    solver.addConstraint(
        ConstraintSpecs(
            disasterRecoveryCapacitySpec=DisasterRecoveryCapacitySpec(
                name="multi-level-failover",
                scope="rack",
                dimension="storage",
                sharedDisasterGroups=[{"rack1"}, {"rack2"}, {"rack3"}],
                primaryToSetOfSecondaryObjects=multi_level_replicas,
            )
        )
    )

    # When primary fails, level1 replica activates
    # If level1 also fails, level2 replica activates
    # multi_replica_failover_end


def partial_failure_groups():
    """Pattern 5: Partial Failure Groups - some scope items share disaster fate."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("database")
    solver.setContainerName("rack")

    # Assume replica_mapping is defined elsewhere
    replica_mapping = {
        "db_primary_0": ["db_replica_0_a", "db_replica_0_b"],
        "db_primary_1": ["db_replica_1_a", "db_replica_1_b"],
    }

    # partial_failure_groups_start
    # Racks on same power circuit fail together
    power_circuit_groups = [
        {"rack1", "rack2"},  # Circuit A
        {"rack3", "rack4"},  # Circuit B
        {"rack5", "rack6"},  # Circuit C
    ]

    solver.addConstraint(
        ConstraintSpecs(
            disasterRecoveryCapacitySpec=DisasterRecoveryCapacitySpec(
                name="power-circuit-failure",
                scope="rack",
                dimension="cpu_usage",
                sharedDisasterGroups=power_circuit_groups,
                primaryToSetOfSecondaryObjects=replica_mapping,
            )
        )
    )
    # partial_failure_groups_end


def no_shared_disaster_groups():
    """Pattern 6: No Shared Disaster Groups - each scope item fails independently."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("database")
    solver.setContainerName("host")

    # no_shared_disaster_groups_start
    # If sharedDisasterGroups not specified, each scope item is independent
    solver.addConstraint(
        ConstraintSpecs(
            disasterRecoveryCapacitySpec=DisasterRecoveryCapacitySpec(
                name="independent-failures",
                scope="host",
                dimension="memory",
                # sharedDisasterGroups omitted → each host fails alone
                primaryToSetOfSecondaryObjects={
                    "primary_0": ["replica_0_a", "replica_0_b"],
                    "primary_1": ["replica_1_a", "replica_1_b"],
                },
            )
        )
    )

    # Tests capacity for: host1 fails, host2 fails, host3 fails (separately)
    # no_shared_disaster_groups_end


def cross_dimension_dr():
    """Pattern 7: Cross-Dimension DR - different dimensions for different failure modes."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("database")
    solver.setContainerName("rack")

    # Assume these are defined elsewhere
    cpu_replicas = {
        "db_primary_0": ["db_replica_0_a", "db_replica_0_b"],
        "db_primary_1": ["db_replica_1_a", "db_replica_1_b"],
    }
    network_replicas = {
        "db_primary_0": ["db_replica_0_a", "db_replica_0_b"],
        "db_primary_1": ["db_replica_1_a", "db_replica_1_b"],
    }

    # cross_dimension_dr_start
    # CPU capacity for rack failure
    solver.addConstraint(
        ConstraintSpecs(
            disasterRecoveryCapacitySpec=DisasterRecoveryCapacitySpec(
                name="rack-failure-cpu",
                scope="rack",
                dimension="cpu_usage",
                sharedDisasterGroups=[{"rack1"}, {"rack2"}],
                primaryToSetOfSecondaryObjects=cpu_replicas,
            )
        )
    )

    # Network capacity for same failure
    solver.addConstraint(
        ConstraintSpecs(
            disasterRecoveryCapacitySpec=DisasterRecoveryCapacitySpec(
                name="rack-failure-network",
                scope="rack",
                dimension="network_bandwidth",
                sharedDisasterGroups=[{"rack1"}, {"rack2"}],
                primaryToSetOfSecondaryObjects=network_replicas,
            )
        )
    )
    # cross_dimension_dr_end


def asymmetric_replica_distribution():
    """Pattern 8: Asymmetric Replica Distribution - different numbers of replicas per primary."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("database")
    solver.setContainerName("rack")

    # asymmetric_replica_distribution_start
    # Critical services have more replicas
    asymmetric_replicas = {
        "critical_db_primary": [
            "critical_db_replica_1",
            "critical_db_replica_2",
            "critical_db_replica_3",  # 3 replicas
        ],
        "non_critical_db_primary": [
            "non_critical_db_replica_1",  # Only 1 replica
        ],
    }

    solver.addConstraint(
        ConstraintSpecs(
            disasterRecoveryCapacitySpec=DisasterRecoveryCapacitySpec(
                name="asymmetric-dr",
                scope="rack",
                dimension="storage",
                sharedDisasterGroups=[{"rack1"}, {"rack2"}],
                primaryToSetOfSecondaryObjects=asymmetric_replicas,
            )
        )
    )
    # asymmetric_replica_distribution_end


def pitfall_insufficient_capacity_bad():
    """Pitfall 1 BAD: Insufficient capacity for failover."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("database")
    solver.setContainerName("rack")

    # pitfall_insufficient_capacity_bad_start
    # BAD: 3 racks, each at 90% capacity
    # If one rack fails, remaining 2 racks must absorb 90% extra load
    # New load per rack: 90% (existing) + 45% (failover) = 135% (exceeds capacity!)
    # pitfall_insufficient_capacity_bad_end


def pitfall_insufficient_capacity_good():
    """Pitfall 1 GOOD: Ensure N+1 or N+2 capacity."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("database")
    solver.setContainerName("rack")

    # Assume replicas is defined elsewhere
    replicas = {
        "db_primary_0": ["db_replica_0_a", "db_replica_0_b"],
        "db_primary_1": ["db_replica_1_a", "db_replica_1_b"],
    }

    # pitfall_insufficient_capacity_good_start
    # GOOD: Size for N-1 racks (if you have N racks)
    # 3 racks: each at max 50% capacity
    # If one fails: 2 racks × 50% = 100% (can handle failover)

    # Or use DR as soft goal with lower weight
    solver.addGoal(
        GoalSpecs(
            disasterRecoveryCapacitySpec=DisasterRecoveryCapacitySpec(
                name="best-effort-dr",
                scope="rack",
                dimension="cpu_usage",
                sharedDisasterGroups=[{"rack1"}, {"rack2"}, {"rack3"}],
                primaryToSetOfSecondaryObjects=replicas,
            )
        ),
        weight=0.5,  # Soft goal - do best effort
    )
    # pitfall_insufficient_capacity_good_end


def pitfall_missing_mapping_bad():
    """Pitfall 2 BAD: Primary objects without secondaries."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("database")
    solver.setContainerName("rack")

    # pitfall_missing_mapping_bad_start
    # BAD: primary_2 has no secondaries
    primaryToSetOfSecondaryObjects = {
        "primary_0": ["replica_0"],
        "primary_1": ["replica_1"],
        # primary_2 missing!
    }
    # If primary_2 fails, no failover!
    # pitfall_missing_mapping_bad_end


def pitfall_missing_mapping_good():
    """Pitfall 2 GOOD: Ensure all primaries have secondaries."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("database")
    solver.setContainerName("rack")

    # pitfall_missing_mapping_good_start
    # GOOD: All primaries mapped
    primaryToSetOfSecondaryObjects = {
        "primary_0": ["replica_0"],
        "primary_1": ["replica_1"],
        "primary_2": ["replica_2"],  # Added
    }
    # pitfall_missing_mapping_good_end


def pitfall_same_disaster_group_bad():
    """Pitfall 3 BAD: Primary and secondary in same failure domain."""
    # pitfall_same_disaster_group_bad_start
    # BAD: Primary and replica on same rack
    # rack1: [primary_0, replica_0]  # Both on rack1
    # If rack1 fails, BOTH primary AND replica fail! (no DR)
    # pitfall_same_disaster_group_bad_end
    pass


def pitfall_same_disaster_group_good():
    """Pitfall 3 GOOD: Use anti-affinity to spread primaries and secondaries."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("database")
    solver.setContainerName("rack")

    # pitfall_same_disaster_group_good_start
    # Ensure primary and replicas are on different racks
    solver.addConstraint(
        ConstraintSpecs(
            groupCountSpec=GroupCountSpec(
                name="spread-replicas",
                scope="rack",
                partitionName="replica_set",  # Group primary+replicas
                bound=GroupCountSpecBound.MAX,
                limit=Limit(globalLimit=1),  # Max 1 per rack
            )
        )
    )
    # pitfall_same_disaster_group_good_end


def pitfall_disaster_groups_confusion_bad():
    """Pitfall 4 BAD: Misunderstanding how disaster groups work."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("database")
    solver.setContainerName("rack")

    # pitfall_disaster_groups_confusion_bad_start
    # CONFUSION: Thinking this tests "any 2 racks fail"
    sharedDisasterGroups = [
        {"rack1"},
        {"rack2"},
    ]
    # Actually tests: "rack1 fails" OR "rack2 fails" (separately)
    # Does NOT test "rack1 AND rack2 both fail together"
    # pitfall_disaster_groups_confusion_bad_end


def pitfall_disaster_groups_confusion_good():
    """Pitfall 4 GOOD: Disaster groups are OR scenarios, not AND."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("database")
    solver.setContainerName("rack")

    # pitfall_disaster_groups_confusion_good_start
    # To test "rack1 AND rack2 fail together"
    sharedDisasterGroups = [
        {"rack1", "rack2"},  # Both fail in this scenario
    ]
    # pitfall_disaster_groups_confusion_good_end


def pitfall_dimension_mismatch_bad():
    """Pitfall 5 BAD: Dimension doesn't exist or isn't relevant."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("database")
    solver.setContainerName("rack")

    # Assume replicas is defined elsewhere
    replicas = {
        "db_primary_0": ["db_replica_0_a", "db_replica_0_b"],
    }

    # pitfall_dimension_mismatch_bad_start
    # BAD: Using dimension that doesn't represent failover load
    solver.addConstraint(
        ConstraintSpecs(
            disasterRecoveryCapacitySpec=DisasterRecoveryCapacitySpec(
                name="bad-dimension-dr",
                scope="rack",
                dimension="object_count",  # Not meaningful for DR!
                sharedDisasterGroups=[{"rack1"}],
                primaryToSetOfSecondaryObjects=replicas,
            )
        )
    )
    # pitfall_dimension_mismatch_bad_end


def pitfall_dimension_mismatch_good():
    """Pitfall 5 GOOD: Use resource dimensions (CPU, memory, network)."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("database")
    solver.setContainerName("rack")

    # Assume replicas is defined elsewhere
    replicas = {
        "db_primary_0": ["db_replica_0_a", "db_replica_0_b"],
    }

    # pitfall_dimension_mismatch_good_start
    # GOOD: Use actual resource dimensions
    solver.addConstraint(
        ConstraintSpecs(
            disasterRecoveryCapacitySpec=DisasterRecoveryCapacitySpec(
                name="good-dimension-dr",
                scope="rack",
                dimension="cpu_usage",  # Actual resource
                sharedDisasterGroups=[{"rack1"}],
                primaryToSetOfSecondaryObjects=replicas,
            )
        )
    )
    # pitfall_dimension_mismatch_good_end


def combining_with_capacity_spec():
    """Combining: Normal capacity + DR capacity."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("database")
    solver.setContainerName("rack")

    # Assume replicas is defined elsewhere
    replicas = {
        "db_primary_0": ["db_replica_0_a", "db_replica_0_b"],
        "db_primary_1": ["db_replica_1_a", "db_replica_1_b"],
    }

    # combining_with_capacity_spec_start
    # Hard: Normal capacity limits
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="normal-capacity",
                scope="rack",
                dimension="cpu_usage",
            )
        )
    )

    # Hard: DR capacity (must handle failover)
    solver.addConstraint(
        ConstraintSpecs(
            disasterRecoveryCapacitySpec=DisasterRecoveryCapacitySpec(
                name="dr-capacity",
                scope="rack",
                dimension="cpu_usage",
                sharedDisasterGroups=[{"rack1"}, {"rack2"}, {"rack3"}],
                primaryToSetOfSecondaryObjects=replicas,
            )
        )
    )
    # combining_with_capacity_spec_end


def combining_with_group_count_spec():
    """Combining: Spread replicas + DR capacity."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("database")
    solver.setContainerName("rack")

    # Assume replicas is defined elsewhere
    replicas = {
        "db_primary_0": ["db_replica_0_a", "db_replica_0_b"],
        "db_primary_1": ["db_replica_1_a", "db_replica_1_b"],
    }

    # combining_with_group_count_spec_start
    # Ensure replicas spread across racks
    solver.addConstraint(
        ConstraintSpecs(
            groupCountSpec=GroupCountSpec(
                name="replica-spread",
                scope="rack",
                partitionName="shard",
                bound=GroupCountSpecBound.MAX,
                limit=Limit(globalLimit=1),
            )
        )
    )

    # Ensure DR capacity
    solver.addConstraint(
        ConstraintSpecs(
            disasterRecoveryCapacitySpec=DisasterRecoveryCapacitySpec(
                name="dr-capacity",
                scope="rack",
                dimension="memory",
                sharedDisasterGroups=[{"rack1"}, {"rack2"}],
                primaryToSetOfSecondaryObjects=replicas,
            )
        )
    )
    # combining_with_group_count_spec_end


def combining_with_balance_spec():
    """Combining: Balance load + ensure DR capacity."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("database")
    solver.setContainerName("rack")

    # Assume replicas is defined elsewhere
    replicas = {
        "db_primary_0": ["db_replica_0_a", "db_replica_0_b"],
        "db_primary_1": ["db_replica_1_a", "db_replica_1_b"],
    }

    # combining_with_balance_spec_start
    # Soft: Balance load across racks
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-racks",
                scope="rack",
                dimension="cpu_usage",
            )
        ),
        weight=1.0,
    )

    # Hard: Ensure DR capacity
    solver.addConstraint(
        ConstraintSpecs(
            disasterRecoveryCapacitySpec=DisasterRecoveryCapacitySpec(
                name="rack-failure-dr",
                scope="rack",
                dimension="cpu_usage",
                sharedDisasterGroups=[{"rack1"}, {"rack2"}],
                primaryToSetOfSecondaryObjects=replicas,
            )
        )
    )
    # combining_with_balance_spec_end


def verify_dr_capacity(
    solution,
    disaster_groups,
    primary_to_secondary,
    dimension_values,
    capacity_values,
):
    """Verify DR capacity for all disaster scenarios.

    Args:
        solution: Solver solution
        disaster_groups: List of sets of failing scope items
        primary_to_secondary: Primary -> secondaries mapping
        dimension_values: Object dimension values
        capacity_values: Scope item capacity values

    Returns:
        True if all DR scenarios have sufficient capacity
    """
    # verification_function_start
    violations = []

    for disaster_group in disaster_groups:
        # Find surviving scope items
        all_scope_items = set(solution.assignment.keys())
        surviving_items = all_scope_items - disaster_group

        # Find primaries on failed scope items
        failed_primaries = set()
        for failed_item in disaster_group:
            for obj in solution.assignment.get(failed_item, []):
                if obj in primary_to_secondary:
                    failed_primaries.add(obj)

        # Calculate failover load per surviving item
        for survivor in surviving_items:
            # Existing load
            existing_objects = solution.assignment.get(survivor, [])
            existing_load = sum(
                dimension_values.get(obj, 0) for obj in existing_objects
            )

            # Failover load (activated secondaries)
            failover_load = 0
            for failed_primary in failed_primaries:
                secondaries = primary_to_secondary[failed_primary]
                for secondary in secondaries:
                    if secondary in existing_objects:
                        # This secondary is on this survivor
                        failover_load += dimension_values.get(failed_primary, 0)

            total_load = existing_load + failover_load
            capacity = capacity_values.get(survivor, float("inf"))

            if total_load > capacity:
                violations.append(
                    {
                        "disaster_group": disaster_group,
                        "survivor": survivor,
                        "existing_load": existing_load,
                        "failover_load": failover_load,
                        "total_load": total_load,
                        "capacity": capacity,
                        "excess": total_load - capacity,
                    }
                )

    if not violations:
        print(f"All DR scenarios have sufficient capacity")
        return True
    else:
        print(f"{len(violations)} DR capacity violations:")
        for v in violations[:5]:
            print(f"   Disaster: {v['disaster_group']}")
            print(f"   Survivor: {v['survivor']}")
            print(
                f"   Load: {v['total_load']:.1f} (existing={v['existing_load']:.1f}, "
                f"failover={v['failover_load']:.1f}) > capacity={v['capacity']:.1f}"
            )
        return False
    # verification_function_end


def analyze_dr_coverage(primary_to_secondary, solution):
    """Analyze DR coverage.

    Args:
        primary_to_secondary: Primary -> secondaries mapping
        solution: Solver solution
    """
    # coverage_analysis_function_start
    # Build reverse mapping: scope item -> objects
    scope_to_objects = solution.assignment

    # Analyze coverage
    total_primaries = len(primary_to_secondary)
    primaries_with_replicas = 0
    primaries_without_replicas = 0

    for primary, secondaries in primary_to_secondary.items():
        if secondaries:
            primaries_with_replicas += 1
        else:
            primaries_without_replicas += 1

    # Replica distribution
    replica_counts = [len(secondaries) for secondaries in primary_to_secondary.values()]
    avg_replicas = sum(replica_counts) / len(replica_counts) if replica_counts else 0

    print(f"DR Coverage Analysis:")
    print(f"  Total primaries: {total_primaries}")
    print(
        f"  Primaries with replicas: {primaries_with_replicas} ({primaries_with_replicas / total_primaries * 100:.1f}%)"
    )
    print(f"  Primaries without replicas: {primaries_without_replicas}")
    print(f"  Average replicas per primary: {avg_replicas:.1f}")
    print(f"  Min replicas: {min(replica_counts) if replica_counts else 0}")
    print(f"  Max replicas: {max(replica_counts) if replica_counts else 0}")
    # coverage_analysis_function_end


def complete_example():
    """
    Complete runnable example demonstrating DisasterRecoveryCapacitySpec.

    This example shows how to use DisasterRecoveryCapacitySpec to ensure
    capacity for database failover after rack failures.
    """
    # Create solver
    solver = ProblemSolver(
        service_name="dr-capacity-example", service_scope="production"
    )

    solver.setObjectName("database")
    solver.setContainerName("rack")

    # Define racks
    racks = ["rack0", "rack1", "rack2", "rack3"]

    # Initial assignment - primaries and replicas distributed across racks
    assignment = {
        "rack0": ["db_primary_0", "db_replica_1_a"],
        "rack1": ["db_primary_1", "db_replica_0_a"],
        "rack2": ["db_replica_0_b", "db_replica_1_b"],
        "rack3": ["db_primary_2", "db_replica_2_a"],
    }
    solver.setAssignment(assignment)

    # Database resource usage (primaries have usage, replicas are standby)
    cpu_usage = {
        # Primaries
        "db_primary_0": 10.0,
        "db_primary_1": 12.0,
        "db_primary_2": 8.0,
        # Replicas (standby, no CPU until failover)
        "db_replica_0_a": 0.0,
        "db_replica_0_b": 0.0,
        "db_replica_1_a": 0.0,
        "db_replica_1_b": 0.0,
        "db_replica_2_a": 0.0,
    }

    memory_usage = {
        # Primaries
        "db_primary_0": 16.0,
        "db_primary_1": 20.0,
        "db_primary_2": 12.0,
        # Replicas
        "db_replica_0_a": 0.0,
        "db_replica_0_b": 0.0,
        "db_replica_1_a": 0.0,
        "db_replica_1_b": 0.0,
        "db_replica_2_a": 0.0,
    }

    solver.addObjectDimension("cpu", cpu_usage)
    solver.addObjectDimension("memory", memory_usage)

    # Rack capacities (sized for N+1 redundancy)
    rack_cpu_capacity = {rack: 50.0 for rack in racks}
    rack_memory_capacity = {rack: 80.0 for rack in racks}

    solver.addContainerDimension("cpu_capacity", rack_cpu_capacity)
    solver.addContainerDimension("memory_capacity", rack_memory_capacity)

    # Primary to secondary mapping
    primary_to_secondary = {
        "db_primary_0": ["db_replica_0_a", "db_replica_0_b"],
        "db_primary_1": ["db_replica_1_a", "db_replica_1_b"],
        "db_primary_2": ["db_replica_2_a"],
    }

    # Add normal capacity constraints
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="cpu-capacity", scope="rack", dimension="cpu"
            )
        )
    )
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity", scope="rack", dimension="memory"
            )
        )
    )

    # Add DR capacity constraints for single rack failure
    solver.addConstraint(
        ConstraintSpecs(
            disasterRecoveryCapacitySpec=DisasterRecoveryCapacitySpec(
                name="rack-failure-dr-cpu",
                scope="rack",
                dimension="cpu",
                sharedDisasterGroups=[{rack} for rack in racks],
                primaryToSetOfSecondaryObjects=primary_to_secondary,
            )
        )
    )

    solver.addConstraint(
        ConstraintSpecs(
            disasterRecoveryCapacitySpec=DisasterRecoveryCapacitySpec(
                name="rack-failure-dr-memory",
                scope="rack",
                dimension="memory",
                sharedDisasterGroups=[{rack} for rack in racks],
                primaryToSetOfSecondaryObjects=primary_to_secondary,
            )
        )
    )

    # Add balance goal
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-cpu",
                scope="rack",
                dimension="cpu",
            )
        ),
        weight=1.0,
    )

    # Solve
    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=10000))
    )
    solution = solver.solve()

    # Print results
    print(f"Solution found in {solution.profile.solveTime}ms")
    print(f"Objective value: {solution.objectiveValue}")
    print(f"Moves: {solution.profile.moveCount}")

    return solution


if __name__ == "__main__":
    print("Running all DrCapacity examples...")

    print("\n1. Quick Example...")
    quick_example()

    print("\n2. Single Rack Failure...")
    single_rack_failure()

    print("\n3. Datacenter Failure...")
    datacenter_failure()

    print("\n4. Availability Zone Failure...")
    availability_zone_failure()

    print("\n5. Multi Replica Failover...")
    multi_replica_failover()

    print("\n6. Partial Failure Groups...")
    partial_failure_groups()

    print("\n7. No Shared Disaster Groups...")
    no_shared_disaster_groups()

    print("\n8. Cross Dimension Dr...")
    cross_dimension_dr()

    print("\n9. Asymmetric Replica Distribution...")
    asymmetric_replica_distribution()

    print("\n10. Pitfall Insufficient Capacity Bad...")
    pitfall_insufficient_capacity_bad()

    print("\n11. Pitfall Insufficient Capacity Good...")
    pitfall_insufficient_capacity_good()

    print("\n12. Pitfall Missing Mapping Bad...")
    pitfall_missing_mapping_bad()

    print("\n13. Pitfall Missing Mapping Good...")
    pitfall_missing_mapping_good()

    print("\n14. Pitfall Same Disaster Group Bad...")
    pitfall_same_disaster_group_bad()

    print("\n15. Pitfall Same Disaster Group Good...")
    pitfall_same_disaster_group_good()

    print("\n16. Pitfall Disaster Groups Confusion Bad...")
    pitfall_disaster_groups_confusion_bad()

    print("\n17. Pitfall Disaster Groups Confusion Good...")
    pitfall_disaster_groups_confusion_good()

    print("\n18. Pitfall Dimension Mismatch Bad...")
    pitfall_dimension_mismatch_bad()

    print("\n19. Pitfall Dimension Mismatch Good...")
    pitfall_dimension_mismatch_good()

    print("\n20. Combining With Capacity Spec...")
    combining_with_capacity_spec()

    print("\n21. Combining With Group Count Spec...")
    combining_with_group_count_spec()

    print("\n22. Combining With Balance Spec...")
    combining_with_balance_spec()

    print("\n23. Complete Example...")
    complete_example()

    print("\n✓ All DrCapacity examples completed successfully!")
# example_end
