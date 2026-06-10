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
Movement Specs Reference Examples

This file demonstrates all the usage patterns for movement-related specs
(MinimizeMovementSpec, MovesInProgressSpec, AvoidMovingSpec) shown in the
reference documentation. Each function is a complete, runnable example.
"""

# example_start
# pyre-strict
import random
import time

from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    ConstraintSpecs,
    GoalSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    AvoidMovingSpec,
    BalanceSpec,
    BalanceSpecFormula,
    CapacitySpec,
    MinimizeMovementSpec,
    MoveInProgress,
    MovesInProgressSpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


# ============================================================================
# MinimizeMovementSpec Examples
# ============================================================================


def minimize_movement_quick_example():
    """Quick example showing basic MinimizeMovementSpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # minimize_movement_quick_example_start
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-moves",
                scope="host",
                dimension="data_size",
            )
        ),
        weight=0.1,  # Lower priority than balance
    )
    # minimize_movement_quick_example_end


def minimize_data_movement():
    """Minimize moving large objects while rebalancing."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # minimize_data_movement_start
    # Prefer moving small objects over large ones
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-data-movement",
                scope="host",
                dimension="data_size_gb",
            )
        ),
        weight=0.2,
    )
    # minimize_data_movement_end


def stability_with_allowance():
    """Allow some movement, but penalize beyond a threshold."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # stability_with_allowance_start
    # Allow up to 10GB of movement without penalty
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-movement",
                scope="host",
                dimension="data_size_gb",
                allowance=10.0,  # Free movement budget
            )
        ),
        weight=0.5,
    )
    # stability_with_allowance_end


def minimize_move_count():
    """Minimize number of objects moved (use count dimension)."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    all_objects = ["shard0", "shard1", "shard2"]

    # minimize_move_count_start
    # Add a count dimension first
    solver.addObjectDimension("count", {obj: 1.0 for obj in all_objects})

    # Minimize number of moves
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-move-count",
                scope="host",
                dimension="count",
            )
        ),
        weight=0.1,
    )
    # minimize_move_count_end


def hierarchical_movement_minimization():
    """Prefer moving within racks over across racks."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # Add rack scope
    host_to_rack = {
        "host0": "rack0",
        "host1": "rack0",
        "host2": "rack1",
        "host3": "rack1",
    }
    solver.addScope("rack", host_to_rack)

    # hierarchical_movement_start
    # Higher penalty for cross-rack movement
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-rack-movement",
                scope="rack",
                dimension="data_size",
            )
        ),
        weight=0.3,
    )

    # Lower penalty for intra-rack movement
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-host-movement",
                scope="host",
                dimension="data_size",
            )
        ),
        weight=0.1,
    )
    # hierarchical_movement_end


def disable_magic_scaling():
    """Use raw movement values without heuristic scaling."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # disable_magic_scaling_start
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-movement",
                scope="host",
                dimension="data_size",
                magicScaling=False,  # Use exact dimension values
            )
        ),
        weight=0.2,
    )
    # disable_magic_scaling_end


def minimize_movement_with_balance():
    """Combining MinimizeMovementSpec with BalanceSpec."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # minimize_with_balance_start
    # Primary: Achieve balance
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-cpu",
                scope="host",
                dimension="cpu",
                formula=BalanceSpecFormula.LEGACY,
            )
        ),
        weight=1.0,
    )

    # Secondary: Minimize disruption
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-moves",
                scope="host",
                dimension="cpu",
            )
        ),
        weight=0.2,
    )
    # minimize_with_balance_end


def minimize_movement_with_capacity():
    """Combining MinimizeMovementSpec with CapacitySpec."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # minimize_with_capacity_start
    # Constraint: Don't exceed capacity
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="cpu-capacity",
                scope="host",
                dimension="cpu",
            )
        )
    )

    # Goal: Minimize moves while respecting capacity
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-moves",
                scope="host",
                dimension="cpu",
            )
        ),
        weight=1.0,
    )
    # minimize_with_capacity_end


# ============================================================================
# MovesInProgressSpec Examples
# ============================================================================


def moves_in_progress_quick_example():
    """Quick example showing basic MovesInProgressSpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # moves_in_progress_quick_example_start
    # Shard migrations currently happening
    ongoing_moves = [
        MoveInProgress(
            object="shard_42",
            destination="host5",  # Moving to host5
        ),
        MoveInProgress(
            object="shard_17",
            destination="host3",
        ),
    ]

    solver.addConstraint(
        ConstraintSpecs(
            movesInProgressSpec=MovesInProgressSpec(
                name="ongoing-migrations",
                scope="host",
                moves=ongoing_moves,
            )
        )
    )
    # moves_in_progress_quick_example_end


def basic_production_rebalancing():
    """Track ongoing moves between rebalancing rounds."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    def get_ongoing_moves_from_db():
        """Mock function to get ongoing moves."""
        return [
            {"object": "shard_1", "destination": "host2"},
            {"object": "shard_3", "destination": "host4"},
        ]

    # basic_production_rebalancing_start
    # Track moves initiated in previous round
    ongoing_moves = get_ongoing_moves_from_db()

    moves_in_progress = [
        MoveInProgress(object=move["object"], destination=move["destination"])
        for move in ongoing_moves
    ]

    solver.addConstraint(
        ConstraintSpecs(
            movesInProgressSpec=MovesInProgressSpec(
                name="ongoing-moves",
                scope="host",
                moves=moves_in_progress,
            )
        )
    )

    solution = solver.solve()

    # New solution won't touch objects currently being moved
    # basic_production_rebalancing_end


def multi_round_migration():
    """Track moves across multiple rebalancing rounds."""

    def apply_moves_async(solution):
        """Mock function to apply moves."""
        return []

    # multi_round_migration_start
    def multi_round_rebalancing(rounds=5):
        """Rebalance over multiple rounds, tracking ongoing moves."""
        ongoing_moves = []

        for round_num in range(rounds):
            solver = ProblemSolver(service_name="example", service_scope="test")
            solver.setObjectName("shard")
            solver.setContainerName("host")

            # Mark moves from previous round as in-progress
            if ongoing_moves:
                solver.addConstraint(
                    ConstraintSpecs(
                        movesInProgressSpec=MovesInProgressSpec(
                            name="ongoing-moves",
                            scope="host",
                            moves=ongoing_moves,
                        )
                    )
                )

            solution = solver.solve()

            # Apply moves
            ongoing_moves = apply_moves_async(solution)

            # ongoing_moves now contains moves that haven't finished yet
            print(f"Round {round_num + 1}: {len(ongoing_moves)} moves in progress")

            # Next round will respect these ongoing moves

    # multi_round_migration_end


def separate_migration_and_rebalancing():
    """Use for systems with background migration separate from rebalancing."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # separate_migration_start
    # Background process is moving shards for maintenance
    background_migrations = [
        MoveInProgress(object="shard_10", destination="new_host"),
        MoveInProgress(object="shard_20", destination="new_host"),
    ]

    # Rebalancer respects background migrations
    solver.addConstraint(
        ConstraintSpecs(
            movesInProgressSpec=MovesInProgressSpec(
                name="background-migrations",
                scope="host",
                moves=background_migrations,
            )
        )
    )

    # Rebalancing won't interfere with background migrations
    # separate_migration_end


def resumable_rebalancing():
    """Resume rebalancing after interruption."""

    class CheckpointDB:
        """Mock checkpoint database."""

        def get_unfinished_moves(self):
            return [
                {"object": "shard_1", "dest": "host2"},
                {"object": "shard_2", "dest": "host3"},
            ]

    # resumable_rebalancing_start
    def resume_rebalancing(checkpoint_db):
        """Resume rebalancing from last checkpoint."""
        solver = ProblemSolver(service_name="example", service_scope="test")
        solver.setObjectName("shard")
        solver.setContainerName("host")

        # Load moves that were in progress when we stopped
        unfinished_moves = checkpoint_db.get_unfinished_moves()

        moves_in_progress = [
            MoveInProgress(object=move["object"], destination=move["dest"])
            for move in unfinished_moves
        ]

        solver.addConstraint(
            ConstraintSpecs(
                movesInProgressSpec=MovesInProgressSpec(
                    name="resume-moves",
                    scope="host",
                    moves=moves_in_progress,
                )
            )
        )

        # Continue from where we left off
        solution = solver.solve()

    # resumable_rebalancing_end


def prioritized_migration():
    """Allow high-priority moves to complete before new rebalancing."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    def get_critical_moves_in_progress():
        """Mock function to get critical moves."""
        return [
            MoveInProgress(object="critical_shard_1", destination="host5"),
        ]

    # prioritized_migration_start
    # High-priority moves from previous critical rebalancing
    critical_moves = get_critical_moves_in_progress()

    solver.addConstraint(
        ConstraintSpecs(
            movesInProgressSpec=MovesInProgressSpec(
                name="critical-moves",
                scope="host",
                moves=critical_moves,
            )
        )
    )

    # New rebalancing won't interfere with critical moves
    # Can plan additional moves for other objects
    # prioritized_migration_end


def rack_level_moves():
    """Track moves at rack level."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("service")
    solver.setContainerName("rack")

    # rack_level_moves_start
    # Moving entire services across racks
    rack_moves = [
        MoveInProgress(
            object="service_frontend",
            destination="rack2",
        ),
        MoveInProgress(
            object="service_backend",
            destination="rack3",
        ),
    ]

    solver.addConstraint(
        ConstraintSpecs(
            movesInProgressSpec=MovesInProgressSpec(
                name="rack-migrations",
                scope="rack",
                moves=rack_moves,
            )
        )
    )
    # rack_level_moves_end


def integration_with_move_tracking():
    """Integrate with production move tracking."""

    # move_tracker_class_start
    class MoveTracker:
        """Track moves in production."""

        def __init__(self):
            self.moves_db = {}

        def start_move(self, object_id, destination):
            """Record move start."""
            self.moves_db[object_id] = {
                "destination": destination,
                "start_time": time.time(),
                "status": "in_progress",
            }

        def complete_move(self, object_id):
            """Record move completion."""
            if object_id in self.moves_db:
                self.moves_db[object_id]["status"] = "completed"

        def get_in_progress_moves(self):
            """Get all in-progress moves for MovesInProgressSpec."""
            return [
                MoveInProgress(object=obj, destination=info["destination"])
                for obj, info in self.moves_db.items()
                if info["status"] == "in_progress"
            ]

    # move_tracker_class_end

    def enumerate_moves(solution):
        """Mock function to enumerate moves."""
        return []

    def initiate_migration(obj, new_host):
        """Mock function to initiate migration."""
        pass

    # move_tracker_usage_start
    # Usage
    tracker = MoveTracker()
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # Apply solution and track moves
    solution = solver.solve()
    for obj, (_old_host, new_host) in enumerate_moves(solution):
        tracker.start_move(obj, new_host)
        initiate_migration(obj, new_host)

    # Later: next rebalancing round
    ongoing = tracker.get_in_progress_moves()
    solver.addConstraint(
        ConstraintSpecs(
            movesInProgressSpec=MovesInProgressSpec(
                name="tracked-moves",
                scope="host",
                moves=ongoing,
            )
        )
    )
    # move_tracker_usage_end


def moves_with_avoid_moving():
    """Combine MovesInProgressSpec with AvoidMovingSpec."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    ongoing_moves = [
        MoveInProgress(object="shard_1", destination="host2"),
    ]
    recently_moved_objects = ["shard_5", "shard_8"]

    # moves_with_avoid_moving_start
    # Objects being moved right now
    solver.addConstraint(
        ConstraintSpecs(
            movesInProgressSpec=MovesInProgressSpec(
                name="ongoing",
                scope="host",
                moves=ongoing_moves,
            )
        )
    )

    # Objects recently moved (don't move again)
    solver.addConstraint(
        ConstraintSpecs(
            avoidMovingSpec=AvoidMovingSpec(
                name="recently-moved",
                objects=recently_moved_objects,
            )
        )
    )
    # moves_with_avoid_moving_end


def moves_with_minimize_movement():
    """Soft movement limit with in-progress tracking."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    ongoing_moves = [
        MoveInProgress(object="shard_1", destination="host2"),
    ]

    # moves_with_minimize_movement_start
    # Hard constraint: Don't touch ongoing moves
    solver.addConstraint(
        ConstraintSpecs(
            movesInProgressSpec=MovesInProgressSpec(
                name="ongoing",
                scope="host",
                moves=ongoing_moves,
            )
        )
    )

    # Soft goal: Minimize new moves
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-new-moves",
                scope="host",
                dimension="data_size",
            )
        ),
        weight=2.0,
    )
    # moves_with_minimize_movement_end


def verify_move_tracking_example():
    """Verify move tracking is accurate."""

    # verify_move_tracking_start
    def verify_move_tracking(ongoing_moves, actual_migrations):
        """Verify MovesInProgressSpec matches reality.

        Args:
            ongoing_moves: List of MoveInProgress objects in spec
            actual_migrations: Actual migrations from production system

        Returns:
            True if tracking is accurate
        """
        spec_moves = {(m.object, m.destination) for m in ongoing_moves}
        actual_moves = {(m["object"], m["destination"]) for m in actual_migrations}

        # Find discrepancies
        in_spec_not_actual = spec_moves - actual_moves  # Stale
        in_actual_not_spec = actual_moves - spec_moves  # Missing

        if not in_spec_not_actual and not in_actual_not_spec:
            print(f"✅ Move tracking accurate: {len(spec_moves)} moves")
            return True
        else:
            print(f"❌ Move tracking discrepancies:")
            if in_spec_not_actual:
                print(
                    f"   Stale (in spec but not in progress): {len(in_spec_not_actual)}"
                )
                for obj, dest in list(in_spec_not_actual)[:5]:
                    print(f"     - {obj} → {dest}")
            if in_actual_not_spec:
                print(
                    f"   Missing (in progress but not in spec): {len(in_actual_not_spec)}"
                )
                for obj, dest in list(in_actual_not_spec)[:5]:
                    print(f"     - {obj} → {dest}")
            return False

    # verify_move_tracking_end


def move_lifecycle_manager():
    """Complete example of tracking move lifecycle."""

    # move_lifecycle_manager_start
    class MoveLifecycleManager:
        """Manage complete move lifecycle with rebalancer integration."""

        def __init__(self):
            self.moves = {}  # object -> move info

        def get_current_host(self, obj):
            """Mock function to get current host."""
            return "host0"

        def trigger_migration(self, obj, destination):
            """Mock function to trigger migration."""
            pass

        def plan_moves(self, solution):
            """Extract moves from solution."""
            planned = []
            for new_host, objects in solution.assignment.items():
                for obj in objects:
                    old_host = self.get_current_host(obj)
                    if old_host != new_host:
                        planned.append(
                            {
                                "object": obj,
                                "source": old_host,
                                "destination": new_host,
                                "state": "planned",
                            }
                        )
            return planned

        def start_moves(self, planned_moves):
            """Start executing planned moves."""
            for move in planned_moves:
                self.moves[move["object"]] = {
                    "destination": move["destination"],
                    "state": "in_progress",
                    "start_time": time.time(),
                }
                # Trigger actual migration
                self.trigger_migration(move["object"], move["destination"])

        def get_moves_for_spec(self):
            """Get MovesInProgress list for next rebalancing."""
            return [
                MoveInProgress(
                    object=obj,
                    destination=info["destination"],
                )
                for obj, info in self.moves.items()
                if info["state"] == "in_progress"
            ]

        def mark_complete(self, object_id):
            """Mark move as complete."""
            if object_id in self.moves:
                self.moves[object_id]["state"] = "completed"
                self.moves[object_id]["end_time"] = time.time()

        def cleanup_completed(self):
            """Remove completed moves (periodic cleanup)."""
            self.moves = {
                obj: info
                for obj, info in self.moves.items()
                if info["state"] != "completed"
            }

    # move_lifecycle_manager_end

    # move_lifecycle_usage_start
    # Usage:
    manager = MoveLifecycleManager()
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # Round 1: Plan and start moves
    solution = solver.solve()
    planned = manager.plan_moves(solution)
    manager.start_moves(planned)

    # Round 2: Respect ongoing moves
    ongoing_moves = manager.get_moves_for_spec()
    solver.addConstraint(
        ConstraintSpecs(
            movesInProgressSpec=MovesInProgressSpec(
                name="ongoing",
                scope="host",
                moves=ongoing_moves,
            )
        )
    )

    solution2 = solver.solve()
    # This won't interfere with moves from round 1
    # move_lifecycle_usage_end


# ============================================================================
# AvoidMovingSpec Examples
# ============================================================================


def avoid_moving_quick_example():
    """Quick example showing basic AvoidMovingSpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # avoid_moving_quick_example_start
    # Pin critical database shards
    solver.addConstraint(
        ConstraintSpecs(
            avoidMovingSpec=AvoidMovingSpec(
                name="pin-critical-shards",
                objects=["shard_0", "shard_1", "shard_5"],
            )
        )
    )
    # avoid_moving_quick_example_end


def pin_critical_objects():
    """Prevent moving objects with strict SLAs."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("instance")
    solver.setContainerName("host")

    # pin_critical_objects_start
    critical_objects = ["primary_db_shard_0", "cache_master", "coordinator_instance"]

    solver.addConstraint(
        ConstraintSpecs(
            avoidMovingSpec=AvoidMovingSpec(
                name="pin-critical",
                objects=critical_objects,
            )
        )
    )
    # pin_critical_objects_end


def pin_objects_with_ongoing_operations():
    """Prevent moving objects currently being migrated or under maintenance."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    def get_objects_being_migrated():
        """Mock function to get objects being migrated."""
        return ["shard_10", "shard_15"]

    # pin_ongoing_operations_start
    # Objects with active migrations
    in_flight_migrations = get_objects_being_migrated()

    solver.addConstraint(
        ConstraintSpecs(
            avoidMovingSpec=AvoidMovingSpec(
                name="pin-in-flight",
                objects=in_flight_migrations,
            )
        )
    )
    # pin_ongoing_operations_end


def pin_recently_moved_objects():
    """Avoid moving objects that were recently relocated."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    def get_recently_moved_objects(hours):
        """Mock function to get recently moved objects."""
        return ["shard_5", "shard_12"]

    # pin_recently_moved_start
    # Get objects moved in the last 24 hours
    recently_moved = get_recently_moved_objects(hours=24)

    solver.addConstraint(
        ConstraintSpecs(
            avoidMovingSpec=AvoidMovingSpec(
                name="avoid-recent-moves",
                objects=recently_moved,
            )
        )
    )
    # pin_recently_moved_end


def gradual_rebalancing():
    """Incrementally rebalance by pinning most objects."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    current_assignment = {
        "host0": ["shard0", "shard1"],
        "host1": ["shard2", "shard3"],
    }
    all_objects = [obj for objs in current_assignment.values() for obj in objs]

    # gradual_rebalancing_start
    # Only allow 10% of objects to move per round
    objects_to_pin = random.sample(all_objects, int(0.9 * len(all_objects)))

    solver.addConstraint(
        ConstraintSpecs(
            avoidMovingSpec=AvoidMovingSpec(
                name="pin-most-objects",
                objects=objects_to_pin,
            )
        )
    )
    # gradual_rebalancing_end


def pin_by_tag():
    """Pin objects matching certain criteria."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    all_objects = ["shard0", "shard1", "shard2"]
    object_metadata = {
        "shard0": {"type": "stateful"},
        "shard1": {"type": "stateless"},
        "shard2": {"type": "stateful"},
    }

    # pin_by_tag_start
    # Pin all objects with tag "stateful"
    stateful_objects = [
        obj for obj in all_objects if object_metadata[obj].get("type") == "stateful"
    ]

    solver.addConstraint(
        ConstraintSpecs(
            avoidMovingSpec=AvoidMovingSpec(
                name="pin-stateful",
                objects=stateful_objects,
            )
        )
    )
    # pin_by_tag_end


def avoid_moving_with_capacity():
    """Combining AvoidMovingSpec with CapacitySpec."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    critical_objects = ["shard_0", "shard_1"]

    # avoid_moving_with_capacity_start
    # Ensure pinned objects don't violate capacity
    # (You may need to verify this beforehand)

    solver.addConstraint(
        ConstraintSpecs(
            avoidMovingSpec=AvoidMovingSpec(
                name="pin-critical",
                objects=critical_objects,
            )
        )
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="capacity",
                scope="host",
                dimension="memory",
            )
        )
    )
    # avoid_moving_with_capacity_end


def avoid_moving_with_avoid_assignments():
    """Combining AvoidMovingSpec with AvoidAssignmentsSpec (if available)."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # avoid_moving_with_avoid_assignments_start
    # Pin some objects, block others from certain hosts

    solver.addConstraint(
        ConstraintSpecs(
            avoidMovingSpec=AvoidMovingSpec(
                name="pin-on-current-hosts",
                objects=["obj_1", "obj_2"],
            )
        )
    )

    # Note: AvoidAssignmentsSpec would be imported and used here
    # This is a placeholder showing the pattern
    # avoid_moving_with_avoid_assignments_end


def gradual_migration_example():
    """Gradually migrate objects using rounds."""

    def apply_moves(solution):
        """Mock function to apply moves."""
        pass

    # gradual_migration_start
    def gradual_migration(objects, rounds=5):
        """Migrate objects over multiple rounds."""
        objects_per_round = len(objects) // rounds

        for round_num in range(rounds):
            # Pin all except current round's objects
            allow_moving = objects[
                round_num * objects_per_round : (round_num + 1) * objects_per_round
            ]
            pin_objects = [obj for obj in objects if obj not in allow_moving]

            solver = ProblemSolver(service_name="example", service_scope="test")
            solver.setObjectName("shard")
            solver.setContainerName("host")

            solver.addConstraint(
                ConstraintSpecs(
                    avoidMovingSpec=AvoidMovingSpec(
                        name=f"pin-round-{round_num}",
                        objects=pin_objects,
                    )
                )
            )

            solution = solver.solve()
            apply_moves(solution)

            print(f"Round {round_num + 1}: Moved {len(allow_moving)} objects")

    # gradual_migration_end


def complete_movement_example():
    """
    Complete runnable example demonstrating movement specs together.

    This example shows how to use all three movement specs together:
    - MinimizeMovementSpec to prefer stability
    - MovesInProgressSpec to track ongoing moves
    - AvoidMovingSpec to pin critical objects
    """
    # Create solver
    solver = ProblemSolver(service_name="movement-example", service_scope="production")

    solver.setObjectName("shard")
    solver.setContainerName("host")

    # Initial assignment
    assignment = {
        "host0": ["shard0", "shard1", "shard2"],
        "host1": ["shard3", "shard4"],
        "host2": ["shard5"],
        "host3": ["shard6", "shard7", "shard8"],
    }
    solver.setAssignment(assignment)

    # Shard data sizes
    data_sizes = {
        "shard0": 10.0,
        "shard1": 15.0,
        "shard2": 20.0,
        "shard3": 12.0,
        "shard4": 18.0,
        "shard5": 25.0,
        "shard6": 8.0,
        "shard7": 22.0,
        "shard8": 14.0,
    }
    solver.addObjectDimension("data_size_gb", data_sizes)

    # Host capacities
    host_capacity = {
        "host0": 100.0,
        "host1": 100.0,
        "host2": 100.0,
        "host3": 100.0,
    }
    solver.addContainerDimension("capacity_gb", host_capacity)

    # Constraint: Respect capacity
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="capacity",
                scope="host",
                dimension="data_size_gb",
            )
        )
    )

    # Constraint: Track moves in progress from previous round
    ongoing_moves = [
        MoveInProgress(object="shard5", destination="host0"),
    ]
    solver.addConstraint(
        ConstraintSpecs(
            movesInProgressSpec=MovesInProgressSpec(
                name="ongoing-migrations",
                scope="host",
                moves=ongoing_moves,
            )
        )
    )

    # Constraint: Pin critical shards
    critical_shards = ["shard0", "shard1"]
    solver.addConstraint(
        ConstraintSpecs(
            avoidMovingSpec=AvoidMovingSpec(
                name="pin-critical",
                objects=critical_shards,
            )
        )
    )

    # Goal: Achieve balance
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-data",
                scope="host",
                dimension="data_size_gb",
                formula=BalanceSpecFormula.LEGACY,
            )
        ),
        weight=1.0,
    )

    # Goal: Minimize movement (secondary priority)
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-moves",
                scope="host",
                dimension="data_size_gb",
            )
        ),
        weight=0.3,
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
    print(f"Critical shards pinned: {critical_shards}")
    print(f"Moves in progress respected: {len(ongoing_moves)}")

    return solution


if __name__ == "__main__":
    print("Running all Movement examples...")

    print("\n1. Minimize Movement Quick Example...")
    minimize_movement_quick_example()

    print("\n2. Minimize Data Movement...")
    minimize_data_movement()

    print("\n3. Stability With Allowance...")
    stability_with_allowance()

    print("\n4. Minimize Move Count...")
    minimize_move_count()

    print("\n5. Hierarchical Movement Minimization...")
    hierarchical_movement_minimization()

    print("\n6. Disable Magic Scaling...")
    disable_magic_scaling()

    print("\n7. Minimize Movement With Balance...")
    minimize_movement_with_balance()

    print("\n8. Minimize Movement With Capacity...")
    minimize_movement_with_capacity()

    print("\n9. Moves In Progress Quick Example...")
    moves_in_progress_quick_example()

    print("\n10. Basic Production Rebalancing...")
    basic_production_rebalancing()

    print("\n11. Multi Round Migration...")
    multi_round_migration()

    print("\n12. Separate Migration And Rebalancing...")
    separate_migration_and_rebalancing()

    print("\n13. Resumable Rebalancing...")
    resumable_rebalancing()

    print("\n14. Prioritized Migration...")
    prioritized_migration()

    print("\n15. Rack Level Moves...")
    rack_level_moves()

    print("\n16. Integration With Move Tracking...")
    integration_with_move_tracking()

    print("\n17. Moves With Avoid Moving...")
    moves_with_avoid_moving()

    print("\n18. Moves With Minimize Movement...")
    moves_with_minimize_movement()

    print("\n19. Move Lifecycle Manager...")
    move_lifecycle_manager()

    print("\n20. Avoid Moving Quick Example...")
    avoid_moving_quick_example()

    print("\n21. Pin Critical Objects...")
    pin_critical_objects()

    print("\n22. Pin Objects With Ongoing Operations...")
    pin_objects_with_ongoing_operations()

    print("\n23. Pin Recently Moved Objects...")
    pin_recently_moved_objects()

    print("\n24. Gradual Rebalancing...")
    gradual_rebalancing()

    print("\n25. Pin By Tag...")
    pin_by_tag()

    print("\n26. Avoid Moving With Capacity...")
    avoid_moving_with_capacity()

    print("\n27. Avoid Moving With Avoid Assignments...")
    avoid_moving_with_avoid_assignments()

    print("\n28. Gradual Migration Example...")
    gradual_migration_example()

    print("\n29. Complete Movement Example...")
    complete_movement_example()

    print("\n✓ All Movement examples completed successfully!")
# example_end
