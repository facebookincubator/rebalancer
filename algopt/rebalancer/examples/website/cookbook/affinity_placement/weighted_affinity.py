#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Weighted Affinity Scores Variation

This variation demonstrates how to use different affinity strengths for
different relationships. Strong affinities are used for critical colocation
requirements, medium affinities for preferred placements, and weak affinities
for nice-to-have optimizations. This allows fine-grained control over placement
priorities.
"""

from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    ConstraintSpecs,
    GoalSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    AssignmentAffinitiesSpec,
    AssignmentAffinity,
    CapacitySpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


def place_with_weighted_affinities():
    """Place services with varying affinity strengths."""
    solver = ProblemSolver(
        service_name="microservice-placer", service_scope="production"
    )

    solver.setObjectName("service_instance")
    solver.setContainerName("host")

    # Setup hosts
    hosts = [f"host_{i}" for i in range(10)]
    assignment = {host: [] for host in hosts}

    # Create service instances
    services = {
        "api_gateway": 5,
        "cache": 10,
        "logger": 15,
    }

    all_instances = []
    for service_name, count in services.items():
        for i in range(count):
            instance_name = f"{service_name}_{i}"
            all_instances.append(instance_name)
            # Random initial assignment
            host_idx = len(all_instances) % len(hosts)
            assignment[hosts[host_idx]].append(instance_name)

    solver.setAssignment(assignment)

    # Add dimensions
    resource_usage = {inst: 4.0 for inst in all_instances}
    solver.addObjectDimension("cpu_usage", resource_usage)

    host_cpu_capacity = {host: 64.0 for host in hosts}
    solver.addContainerDimension("cpu_capacity", host_cpu_capacity)

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="cpu-capacity",
                scope="host",
                dimension="cpu_usage",
            )
        )
    )

    # variation_start
    # Strong affinity (must colocate)
    critical_affinities = [
        AssignmentAffinity(
            object="api_gateway_0", scopeItemName="host_0", affinity=100.0
        )
    ]

    # Medium affinity (prefer colocate)
    moderate_affinities = [
        AssignmentAffinity(object="cache_0", scopeItemName="host_0", affinity=5.0),
        AssignmentAffinity(object="cache_1", scopeItemName="host_0", affinity=5.0),
    ]

    # Weak affinity (nice to have)
    weak_affinities = [
        AssignmentAffinity(object="logger_0", scopeItemName="host_0", affinity=1.0),
        AssignmentAffinity(object="logger_1", scopeItemName="host_0", affinity=1.0),
    ]
    # variation_end

    # Add affinity goals with different weights
    if critical_affinities:
        solver.addGoal(
            GoalSpecs(
                assignmentAffinitiesSpec=AssignmentAffinitiesSpec(
                    name="critical-affinity",
                    scope="host",
                    affinities=critical_affinities,
                )
            ),
            weight=10.0,  # Highest priority
        )

    if moderate_affinities:
        solver.addGoal(
            GoalSpecs(
                assignmentAffinitiesSpec=AssignmentAffinitiesSpec(
                    name="moderate-affinity",
                    scope="host",
                    affinities=moderate_affinities,
                )
            ),
            weight=3.0,  # Medium priority
        )

    if weak_affinities:
        solver.addGoal(
            GoalSpecs(
                assignmentAffinitiesSpec=AssignmentAffinitiesSpec(
                    name="weak-affinity",
                    scope="host",
                    affinities=weak_affinities,
                )
            ),
            weight=1.0,  # Low priority
        )

    # Use Local Search solver
    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=30000))
    )

    # Solve
    solution = solver.solve()

    print(f"Solution found in {solution.profile.solveTime}ms")
    print(f"Objective value: {solution.objectiveValue}")
    print(f"Instances moved: {solution.profile.moveCount}")

    # Analyze affinity satisfaction
    print("\nAffinity satisfaction:")
    print(
        f"  Critical: api_gateway_0 on host_0? {_check_affinity(solution.assignment, 'api_gateway_0', 'host_0')}"
    )
    print(
        f"  Moderate: cache_0 on host_0? {_check_affinity(solution.assignment, 'cache_0', 'host_0')}"
    )
    print(
        f"  Weak: logger_0 on host_0? {_check_affinity(solution.assignment, 'logger_0', 'host_0')}"
    )

    return solution


def _check_affinity(assignment, instance, host):
    """Check if instance is assigned to host."""
    return instance in assignment.get(host, [])


if __name__ == "__main__":
    place_with_weighted_affinities()
