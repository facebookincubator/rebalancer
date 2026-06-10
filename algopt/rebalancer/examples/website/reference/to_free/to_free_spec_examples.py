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
ToFreeSpec Reference Examples

This file demonstrates all the usage patterns for ToFreeSpec shown in the
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
    CapacitySpec,
    NonAcceptingSpec,
    Priority,
    ToFreeSpec,
    ToFreeSpecFormula,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


def quick_example():
    """Quick example showing basic ToFreeSpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # quick_example_start
    # Drain all objects from hosts under maintenance
    solver.addConstraint(
        ConstraintSpecs(
            toFreeSpec=ToFreeSpec(
                name="drain-maintenance-hosts",
                containers=["host3", "host7", "host12"],
            )
        )
    )
    # quick_example_end


def drain_hosts_for_maintenance():
    """Remove all objects from hosts under maintenance."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # drain_maintenance_start
    # Hosts scheduled for maintenance
    maintenance_hosts = ["host5", "host8", "host15"]

    solver.addConstraint(
        ConstraintSpecs(
            toFreeSpec=ToFreeSpec(
                name="maintenance-drain",
                containers=maintenance_hosts,
            )
        )
    )
    # drain_maintenance_end


def decommission_old_hardware():
    """Empty old hosts before removing from cluster."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # decommission_old_hardware_start
    # Old generation hosts to retire
    old_hosts = ["old_host_1", "old_host_2", "old_host_3"]

    solver.addConstraint(
        ConstraintSpecs(
            toFreeSpec=ToFreeSpec(
                name="decommission-old-hardware",
                containers=old_hosts,
            )
        )
    )
    # decommission_old_hardware_end


def drain_specific_resource_type():
    """Reduce specific dimension to zero (not all objects)."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # drain_specific_resource_start
    # Remove all storage from hosts, but keep CPU workloads
    solver.addGoal(
        GoalSpecs(
            toFreeSpec=ToFreeSpec(
                name="drain-storage",
                containers=["host1", "host2"],
                dimension="storage_gb",  # Only storage, not all objects
            )
        ),
        weight=1.0,
    )
    # drain_specific_resource_end


def gradual_draining_as_goal():
    """Soft goal to drain, but don't fail if impossible."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    overloaded_hosts = ["host1", "host2", "host3"]

    # gradual_draining_start
    # Try to drain, but don't fail if capacity insufficient elsewhere
    solver.addGoal(
        GoalSpecs(
            toFreeSpec=ToFreeSpec(
                name="prefer-empty-hosts",
                containers=overloaded_hosts,
                formula=ToFreeSpecFormula.MINIMIZE_OCCUPIED_CONTAINERS,
            )
        ),
        weight=0.5,
    )
    # gradual_draining_end


def drain_prioritization():
    """Use formula to control draining strategy."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    candidate_hosts = ["host1", "host2", "host3", "host4"]

    # drain_prioritization_start
    # Fully drain SOME hosts (don't spread evenly)
    solver.addConstraint(
        ConstraintSpecs(
            toFreeSpec=ToFreeSpec(
                name="focused-drain",
                containers=candidate_hosts,
                formula=ToFreeSpecFormula.MINIMIZE_OCCUPIED_CONTAINERS,
            )
        )
    )
    # drain_prioritization_end


def drain_by_priority():
    """Drain high-priority containers first."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    critical_maintenance = ["host1", "host2"]
    optional_maintenance = ["host3", "host4", "host5"]

    # drain_by_priority_start
    # Tuple 0: Drain critical maintenance hosts
    solver.addGoal(
        GoalSpecs(
            toFreeSpec=ToFreeSpec(
                name="drain-critical",
                containers=critical_maintenance,
            )
        ),
        weight=1.0,
        priority=Priority(tuple=0),
    )

    # Tuple 1: Drain nice-to-have hosts
    solver.addGoal(
        GoalSpecs(
            toFreeSpec=ToFreeSpec(
                name="drain-optional",
                containers=optional_maintenance,
            )
        ),
        weight=1.0,
        priority=Priority(tuple=1),
    )
    # drain_by_priority_end


def constraint_usage():
    """Use ToFreeSpec as a hard constraint."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    decommission_hosts = ["host1", "host2"]

    # constraint_usage_start
    # Hard requirement: These hosts MUST be empty
    solver.addConstraint(
        ConstraintSpecs(
            toFreeSpec=ToFreeSpec(
                name="must-drain",
                containers=decommission_hosts,
            )
        )
    )
    # constraint_usage_end


def goal_usage():
    """Use ToFreeSpec as a soft goal."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    candidate_hosts = ["host1", "host2", "host3"]

    # goal_usage_start
    # Nice to have: Try to drain these hosts
    solver.addGoal(
        GoalSpecs(
            toFreeSpec=ToFreeSpec(
                name="prefer-drain",
                containers=candidate_hosts,
            )
        ),
        weight=0.5,
    )
    # goal_usage_end


def combining_with_capacity_spec():
    """Ensure receiving hosts don't exceed capacity."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    maintenance_hosts = ["host1", "host2"]

    # combining_capacity_start
    solver.addConstraint(
        ConstraintSpecs(
            toFreeSpec=ToFreeSpec(
                name="drain-maintenance",
                containers=maintenance_hosts,
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
    # combining_capacity_end


def combining_with_non_accepting_spec():
    """Prevent new placements AND drain existing."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    maintenance_hosts = ["host1", "host2"]

    # combining_non_accepting_start
    # Block new assignments
    solver.addConstraint(
        ConstraintSpecs(
            nonAcceptingSpec=NonAcceptingSpec(
                name="block-maintenance-hosts",
                scope="host",
                items=maintenance_hosts,
            )
        )
    )

    # Drain existing objects
    solver.addConstraint(
        ConstraintSpecs(
            toFreeSpec=ToFreeSpec(
                name="drain-maintenance-hosts",
                containers=maintenance_hosts,
            )
        )
    )
    # combining_non_accepting_end


def verify_drained(solution, containers_to_drain):
    """Verify containers were successfully drained."""
    # verification_example_start
    for container in containers_to_drain:
        objects = solution.assignment.get(container, [])
        if objects:
            print(f"❌ {container} still has {len(objects)} objects: {objects}")
            return False
        else:
            print(f"✅ {container} successfully drained")
    return True
    # verification_example_end


def verification_example():
    """Verify draining succeeded."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    maintenance_hosts = ["host1", "host2"]

    # verification_usage_start
    # After solving
    solution = solver.solve()
    if verify_drained(solution, maintenance_hosts):
        print("All maintenance hosts successfully drained!")
    else:
        print("Warning: Some hosts not fully drained")
    # verification_usage_end


def incremental_drain(hosts_to_drain, rounds=3):
    """Drain hosts over multiple rounds."""
    # incremental_drain_start
    hosts_per_round = len(hosts_to_drain) // rounds

    for round_num in range(rounds):
        # Drain subset this round
        drain_this_round = hosts_to_drain[
            round_num * hosts_per_round : (round_num + 1) * hosts_per_round
        ]

        solver = ProblemSolver(service_name="example", service_scope="test")
        solver.addConstraint(
            ConstraintSpecs(
                toFreeSpec=ToFreeSpec(
                    name=f"drain-round-{round_num}",
                    containers=drain_this_round,
                )
            )
        )

        solution = solver.solve()
        # apply_moves(solution)

        print(f"Round {round_num + 1}: Drained {len(drain_this_round)} hosts")
    # incremental_drain_end


def pitfall_wrong_dimension():
    """Common pitfall: Draining wrong dimension."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    storage_hosts = ["host1", "host2"]

    # pitfall_wrong_dimension_good_start
    # GOOD: Only drain storage dimension
    solver.addConstraint(
        ConstraintSpecs(
            toFreeSpec=ToFreeSpec(
                containers=storage_hosts,
                dimension="storage_gb",
            )
        )
    )
    # pitfall_wrong_dimension_good_end


def complete_example():
    """
    Complete runnable example demonstrating ToFreeSpec.

    This example shows how to use ToFreeSpec to drain hosts under maintenance
    while ensuring capacity constraints are respected on receiving hosts.
    """
    # Create solver
    solver = ProblemSolver(service_name="tofree-example", service_scope="production")

    solver.setObjectName("task")
    solver.setContainerName("host")

    # Initial assignment with tasks on maintenance hosts
    assignment = {
        "host0": ["task0", "task1"],
        "host1": ["task2", "task3"],
        "host2": ["task4", "task5"],  # Under maintenance
        "host3": ["task6", "task7"],  # Under maintenance
        "host4": ["task8", "task9"],
    }
    solver.setAssignment(assignment)

    # Task resources
    cpu_usage = {f"task{i}": 2.0 for i in range(10)}
    memory_usage = {f"task{i}": 4.0 for i in range(10)}

    solver.addObjectDimension("cpu", cpu_usage)
    solver.addObjectDimension("memory", memory_usage)

    # Host capacities
    host_cpu_capacity = {f"host{i}": 10.0 for i in range(5)}
    host_memory_capacity = {f"host{i}": 20.0 for i in range(5)}

    solver.addContainerDimension("cpu_capacity", host_cpu_capacity)
    solver.addContainerDimension("memory_capacity", host_memory_capacity)

    # Hosts to drain
    maintenance_hosts = ["host2", "host3"]

    # Add capacity constraints
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="cpu-capacity", scope="host", dimension="cpu"
            )
        )
    )
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity", scope="host", dimension="memory"
            )
        )
    )

    # Add ToFreeSpec to drain maintenance hosts
    solver.addConstraint(
        ConstraintSpecs(
            toFreeSpec=ToFreeSpec(
                name="drain-maintenance",
                containers=maintenance_hosts,
            )
        )
    )

    # Block new assignments to maintenance hosts
    solver.addConstraint(
        ConstraintSpecs(
            nonAcceptingSpec=NonAcceptingSpec(
                name="block-maintenance",
                scope="host",
                items=maintenance_hosts,
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
    print(f"Tasks moved: {solution.profile.moveCount}")

    # Verify draining
    print("\nVerifying draining:")
    verify_drained(solution, maintenance_hosts)

    return solution


if __name__ == "__main__":
    print("Running all ToFree examples...")

    print("\n1. Quick Example...")
    quick_example()

    print("\n2. Drain Hosts For Maintenance...")
    drain_hosts_for_maintenance()

    print("\n3. Decommission Old Hardware...")
    decommission_old_hardware()

    print("\n4. Drain Specific Resource Type...")
    drain_specific_resource_type()

    print("\n5. Gradual Draining As Goal...")
    gradual_draining_as_goal()

    print("\n6. Drain Prioritization...")
    drain_prioritization()

    print("\n7. Drain By Priority...")
    drain_by_priority()

    print("\n8. Constraint Usage...")
    constraint_usage()

    print("\n9. Goal Usage...")
    goal_usage()

    print("\n10. Combining With Capacity Spec...")
    combining_with_capacity_spec()

    print("\n11. Combining With Non Accepting Spec...")
    combining_with_non_accepting_spec()

    print("\n12. Verification Example...")
    verification_example()

    print("\n13. Incremental Drain...")
    incremental_drain()

    print("\n14. Pitfall Wrong Dimension...")
    pitfall_wrong_dimension()

    print("\n15. Complete Example...")
    complete_example()

    print("\n✓ All ToFree examples completed successfully!")
# example_end
