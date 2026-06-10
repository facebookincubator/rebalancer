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
GroupMoveLimitSpec Reference Examples

This file demonstrates all the usage patterns for GroupMoveLimitSpec shown in the
reference documentation. Each function is a complete, runnable example.
"""

# example_start
# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    ConstraintSpecs,
    GoalSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    AvoidMovingSpec,
    Filter,
    GroupMoveLimitSpec,
    Limit,
    LimitType,
    MinimizeMovementSpec,
)


def quick_example():
    """Quick example showing basic GroupMoveLimitSpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # quick_example_start
    # Limit moves per service
    solver.addConstraint(
        ConstraintSpecs(
            groupMoveLimitSpec=GroupMoveLimitSpec(
                name="limit-moves-per-service",
                partitionName="service",
                limit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=5.0,  # Max 5 objects move per service
                ),
            )
        )
    )
    # quick_example_end


def uniform_move_limit():
    """Uniform move limit per service."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("instance")
    solver.setContainerName("host")

    # uniform_move_limit_start
    # Define service partition
    service_partition = {
        "web_frontend": ["frontend_0", "frontend_1", "frontend_2"],
        "api_backend": ["api_0", "api_1", "api_2", "api_3"],
        "database": ["db_shard_0", "db_shard_1"],
    }
    solver.addPartition("service", service_partition)

    # Limit: Max 3 objects move per service
    solver.addConstraint(
        ConstraintSpecs(
            groupMoveLimitSpec=GroupMoveLimitSpec(
                name="uniform-move-limit",
                partitionName="service",
                limit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=3.0,
                ),
            )
        )
    )
    # uniform_move_limit_end


def per_service_custom_limits():
    """Different limits for different services."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("instance")
    solver.setContainerName("host")

    # per_service_custom_limits_start
    # Critical services get lower limits
    service_limits = {
        "critical_database": 1.0,  # Only 1 shard at a time
        "important_api": 3.0,  # Max 3 instances
        "batch_processing": 10.0,  # Can move more
    }

    solver.addConstraint(
        ConstraintSpecs(
            groupMoveLimitSpec=GroupMoveLimitSpec(
                name="per-service-limits",
                partitionName="service",
                limit=Limit(
                    type=LimitType.ABSOLUTE,
                    groupLimits=service_limits,
                    globalLimit=5.0,  # Default for services not in groupLimits
                ),
            )
        )
    )
    # per_service_custom_limits_end


def weighted_move_limit_by_size():
    """Limit based on data volume moved, not object count."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # weighted_move_limit_by_size_start
    # Use data_size dimension instead of count
    solver.addConstraint(
        ConstraintSpecs(
            groupMoveLimitSpec=GroupMoveLimitSpec(
                name="data-volume-limit",
                partitionName="database",
                dimension="data_size",  # GB
                limit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=1000.0,  # Max 1TB moved per database
                ),
            )
        )
    )
    # weighted_move_limit_by_size_end


def directional_move_limits():
    """Only count moves FROM certain hosts (e.g., draining)."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("instance")
    solver.setContainerName("host")

    # directional_move_limits_start
    # Only limit moves FROM hosts being drained
    draining_hosts = ["old_host_1", "old_host_2"]

    solver.addConstraint(
        ConstraintSpecs(
            groupMoveLimitSpec=GroupMoveLimitSpec(
                name="drain-move-limit",
                partitionName="service",
                sourceScopeItemsAffectingLimitFilter=Filter(
                    items=draining_hosts  # Only count moves from these hosts
                ),
                limit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=5.0,
                ),
            )
        )
    )
    # directional_move_limits_end


def multi_round_gradual_rebalancing():
    """Gradually increase move limit over rounds."""

    # multi_round_gradual_rebalancing_start
    def multi_round_with_increasing_limit(rounds=5):
        """Gradually increase move limit over rounds."""
        for round_num in range(rounds):
            solver = ProblemSolver(service_name="example", service_scope="test")

            # Increase limit each round: 5, 10, 15, 20, 25
            limit = 5.0 * (round_num + 1)

            solver.addConstraint(
                ConstraintSpecs(
                    groupMoveLimitSpec=GroupMoveLimitSpec(
                        name="gradual-move-limit",
                        partitionName="service",
                        limit=Limit(
                            type=LimitType.ABSOLUTE,
                            globalLimit=limit,
                        ),
                    )
                )
            )

            solution = solver.solve()
            # apply_moves(solution)
            # print(f"Round {round_num + 1}: Limit {limit}, Moved {count_moves(solution)}")

    # multi_round_gradual_rebalancing_end


def replica_group_move_limits():
    """Limit moves per replica group for safety."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("replica")
    solver.setContainerName("host")

    # replica_group_move_limits_start
    # Partition by replica group
    replica_groups = {
        "shard_0_replicas": [
            "shard_0_replica_0",
            "shard_0_replica_1",
            "shard_0_replica_2",
        ],
        "shard_1_replicas": [
            "shard_1_replica_0",
            "shard_1_replica_1",
            "shard_1_replica_2",
        ],
    }
    solver.addPartition("replica_group", replica_groups)

    # Don't move more than 1 replica per shard at a time
    solver.addConstraint(
        ConstraintSpecs(
            groupMoveLimitSpec=GroupMoveLimitSpec(
                name="replica-safety",
                partitionName="replica_group",
                limit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=1.0,  # Max 1 replica move per shard
                ),
            )
        )
    )
    # replica_group_move_limits_end


def combined_source_destination_filters():
    """Count moves only between specific hosts."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("instance")
    solver.setContainerName("host")

    # combined_source_destination_filters_start
    # Only count moves FROM old datacenter TO new datacenter
    old_dc_hosts = ["old_dc_host_1", "old_dc_host_2"]
    new_dc_hosts = ["new_dc_host_1", "new_dc_host_2"]

    solver.addConstraint(
        ConstraintSpecs(
            groupMoveLimitSpec=GroupMoveLimitSpec(
                name="dc-migration-limit",
                partitionName="service",
                sourceScopeItemsAffectingLimitFilter=Filter(items=old_dc_hosts),
                destinationScopeItemsAffectingLimitFilter=Filter(items=new_dc_hosts),
                limit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=10.0,
                ),
            )
        )
    )
    # combined_source_destination_filters_end


def pitfall_restrictive_bad():
    """Pitfall: Limit too restrictive (BAD example)."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("instance")
    solver.setContainerName("host")

    # pitfall_restrictive_bad_start
    # BAD: Service has 100 imbalanced objects, but limit=1
    solver.addConstraint(
        ConstraintSpecs(
            groupMoveLimitSpec=GroupMoveLimitSpec(
                name="too-restrictive",
                partitionName="large_service",
                limit=Limit(globalLimit=1.0),  # Too restrictive!
            )
        )
    )
    # Rebalancing will take 100 rounds!
    # pitfall_restrictive_bad_end


def pitfall_restrictive_good():
    """Pitfall: Limit too restrictive (GOOD example)."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("instance")
    solver.setContainerName("host")

    # pitfall_restrictive_good_start
    # GOOD: Allow 10-20% of objects to move per round
    service_size = 100
    reasonable_limit = service_size * 0.15  # 15 objects

    solver.addConstraint(
        ConstraintSpecs(
            groupMoveLimitSpec=GroupMoveLimitSpec(
                name="reasonable-limit",
                partitionName="large_service",
                limit=Limit(globalLimit=reasonable_limit),
            )
        )
    )
    # pitfall_restrictive_good_end


def pitfall_forgot_partition_bad():
    """Pitfall: Partition not defined (BAD example)."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("instance")
    solver.setContainerName("host")

    # pitfall_forgot_partition_bad_start
    # BAD: Partition "service" not added
    solver.addConstraint(
        ConstraintSpecs(
            groupMoveLimitSpec=GroupMoveLimitSpec(
                name="forgot-partition",
                partitionName="service",  # Error: partition doesn't exist!
                limit=Limit(globalLimit=5.0),
            )
        )
    )
    # pitfall_forgot_partition_bad_end


def pitfall_forgot_partition_good():
    """Pitfall: Partition not defined (GOOD example)."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("instance")
    solver.setContainerName("host")

    # pitfall_forgot_partition_good_start
    # GOOD: Define partition first
    service_partition = {"service_a": ["obj1", "obj2"], "service_b": ["obj3"]}
    solver.addPartition("service", service_partition)

    solver.addConstraint(
        ConstraintSpecs(
            groupMoveLimitSpec=GroupMoveLimitSpec(
                name="with-partition",
                partitionName="service",
                limit=Limit(globalLimit=5.0),
            )
        )
    )
    # pitfall_forgot_partition_good_end


def pitfall_wrong_dimension_bad():
    """Pitfall: Dimension doesn't exist (BAD example)."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("instance")
    solver.setContainerName("host")

    # pitfall_wrong_dimension_bad_start
    # BAD: Using dimension "priority" but objects don't have it
    solver.addConstraint(
        ConstraintSpecs(
            groupMoveLimitSpec=GroupMoveLimitSpec(
                name="wrong-dimension",
                partitionName="service",
                dimension="priority",  # Doesn't exist!
                limit=Limit(globalLimit=10.0),
            )
        )
    )
    # pitfall_wrong_dimension_bad_end


def pitfall_wrong_dimension_good():
    """Pitfall: Dimension doesn't exist (GOOD example)."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # pitfall_wrong_dimension_good_start
    # GOOD: Use existing dimension or default
    data_sizes = {"shard_0": 100.0, "shard_1": 200.0}
    solver.addObjectDimension("data_size", data_sizes)

    solver.addConstraint(
        ConstraintSpecs(
            groupMoveLimitSpec=GroupMoveLimitSpec(
                name="valid-dimension",
                partitionName="service",
                dimension="data_size",  # Exists!
                limit=Limit(globalLimit=1000.0),
            )
        )
    )
    # pitfall_wrong_dimension_good_end


def combining_minimize_movement():
    """Soft limit (goal) + hard limit (constraint)."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # combining_minimize_movement_start
    # Soft: Prefer minimal movement
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="prefer-minimal-moves",
                scope="host",
                dimension="data_size",
            )
        ),
        weight=1.0,
    )

    # Hard: Never exceed per-service limit
    solver.addConstraint(
        ConstraintSpecs(
            groupMoveLimitSpec=GroupMoveLimitSpec(
                name="enforce-move-limit",
                partitionName="service",
                dimension="data_size",
                limit=Limit(globalLimit=500.0),  # Hard limit
            )
        )
    )
    # combining_minimize_movement_end


def combining_avoid_moving():
    """Pin + limit moves for others."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("instance")
    solver.setContainerName("host")

    # combining_avoid_moving_start
    # Pin critical objects (don't move at all)
    critical_objects = ["critical_db_0", "critical_api_0"]
    solver.addConstraint(
        ConstraintSpecs(
            avoidMovingSpec=AvoidMovingSpec(
                name="pin-critical",
                objects=critical_objects,
            )
        )
    )

    # Limit moves for non-critical
    solver.addConstraint(
        ConstraintSpecs(
            groupMoveLimitSpec=GroupMoveLimitSpec(
                name="limit-non-critical",
                partitionName="service",
                limit=Limit(globalLimit=10.0),
            )
        )
    )
    # combining_avoid_moving_end


def verification_example():
    """Verify move limits respected."""

    # verification_start
    def verify_group_move_limits(
        initial_assignment,
        final_assignment,
        partition,
        max_moves_per_group,
    ):
        """Verify group move limits were respected.

        Args:
            initial_assignment: Initial assignment
            final_assignment: Final assignment
            partition: Partition dict (group -> objects)
            max_moves_per_group: Maximum allowed moves per group

        Returns:
            True if all groups within limit
        """
        violations = []

        for group, objects in partition.items():
            # Count moves for this group
            moves = 0
            for obj in objects:
                initial_host = find_host(obj, initial_assignment)
                final_host = find_host(obj, final_assignment)
                if initial_host != final_host:
                    moves += 1

            if moves > max_moves_per_group:
                violations.append(
                    {
                        "group": group,
                        "moves": moves,
                        "limit": max_moves_per_group,
                    }
                )

        if not violations:
            print(f"✅ All groups within move limit (≤{max_moves_per_group} moves)")
            return True
        else:
            print(f"❌ {len(violations)} groups exceeded move limit:")
            for v in violations[:5]:
                print(f"   {v['group']}: {v['moves']} moves (limit: {v['limit']})")
            return False

    def find_host(obj, assignment):
        """Find which host has object."""
        for host, objects in assignment.items():
            if obj in objects:
                return host
        return None

    # verification_end


def move_count_analysis():
    """Analyze moves per group."""

    # move_count_analysis_start
    def analyze_moves_per_group(initial, final, partition):
        """Analyze how many moves occurred per group.

        Args:
            initial: Initial assignment
            final: Final assignment
            partition: Partition dict

        Returns:
            Dict of group -> move count
        """
        move_counts = {}

        for group, objects in partition.items():
            moves = 0
            for obj in objects:
                initial_host = find_host(obj, initial)
                final_host = find_host(obj, final)
                if initial_host != final_host:
                    moves += 1
            move_counts[group] = moves

        print("Moves per group:")
        for group in sorted(move_counts.keys()):
            print(f"  {group}: {move_counts[group]} moves")

        return move_counts

    def find_host(obj, assignment):
        """Find which host has object."""
        for host, objects in assignment.items():
            if obj in objects:
                return host
        return None

    # move_count_analysis_end


if __name__ == "__main__":
    print("Running all GroupMoveLimitSpec examples...\n")

    print("1. Quick Example...")
    quick_example()

    print("2. Uniform Move Limit...")
    uniform_move_limit()

    print("3. Per-Service Custom Limits...")
    per_service_custom_limits()

    print("4. Weighted Move Limit by Size...")
    weighted_move_limit_by_size()

    print("5. Directional Move Limits...")
    directional_move_limits()

    print("6. Multi-Round Gradual Rebalancing...")
    multi_round_gradual_rebalancing()

    print("7. Replica Group Move Limits...")
    replica_group_move_limits()

    print("8. Combined Source/Destination Filters...")
    combined_source_destination_filters()

    print("\n✓ All GroupMoveLimitSpec examples completed successfully!")
# example_end
