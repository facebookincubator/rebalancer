#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Rack Affinity Variation

This variation demonstrates how to use rack-level affinity to prefer instances
on the same rack for low latency while maintaining host-level anti-affinity.
This is useful for colocating related services within the same rack for
performance while still spreading replicas across different hosts.
"""

from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    ConstraintSpecs,
    GoalSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    ColocateGroupsSpec,
    ColocateGroupsSpecBound,
    GroupCountSpec,
    GroupCountSpecBound,
    Limit,
    LimitType,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


def place_with_rack_affinity():
    """Place services with rack-level affinity for low latency."""
    solver = ProblemSolver(
        service_name="microservice-placer", service_scope="production"
    )

    solver.setObjectName("service_instance")
    solver.setContainerName("host")

    # Setup hosts and racks
    num_racks = 4
    hosts_per_rack = 5
    host_to_rack = {}
    assignment = {}

    for rack_id in range(num_racks):
        for host_id in range(hosts_per_rack):
            host_name = f"rack{rack_id}_host{host_id}"
            rack_name = f"rack{rack_id}"
            host_to_rack[host_name] = rack_name
            assignment[host_name] = []

    # Create service instances
    services = ["frontend", "backend", "cache"]
    instances_per_service = 20

    service_partition = {}
    for service_name in services:
        service_partition[service_name] = []
        for i in range(instances_per_service):
            instance_name = f"{service_name}_{i}"
            service_partition[service_name].append(instance_name)
            # Random initial assignment
            host_idx = hash(instance_name) % len(assignment)
            host_name = list(assignment.keys())[host_idx]
            assignment[host_name].append(instance_name)

    solver.setAssignment(assignment)
    solver.addPartition("service", service_partition)

    # variation_start
    # Define rack topology
    solver.addScope("rack", host_to_rack)

    # Prefer frontend and backend on same rack
    solver.addGoal(
        GoalSpecs(
            colocateGroupsSpec=ColocateGroupsSpec(
                name="frontend-backend-rack-locality",
                scope="rack",
                partitionName="service_type",  # Group by service type
                bound=ColocateGroupsSpecBound.MAX,
                limits=Limit(type=LimitType.ABSOLUTE, globalLimit=2),
            )
        ),
        weight=1.0,
    )
    # variation_end

    # Maintain host-level anti-affinity
    solver.addConstraint(
        ConstraintSpecs(
            groupCountSpec=GroupCountSpec(
                name="host-anti-affinity",
                scope="host",
                partitionName="service",
                bound=GroupCountSpecBound.MAX,
                limit=Limit(type=LimitType.ABSOLUTE, globalLimit=1),
            )
        )
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

    # Analyze rack colocation
    rack_assignment = {}
    for host, instances in solution.assignment.items():
        rack = host_to_rack[host]
        if rack not in rack_assignment:
            rack_assignment[rack] = []
        rack_assignment[rack].extend(instances)

    print("\nRack colocation analysis:")
    for rack, instances in rack_assignment.items():
        services_in_rack = {inst.split("_")[0] for inst in instances}
        print(f"  {rack}: {len(instances)} instances, services: {services_in_rack}")

    return solution


if __name__ == "__main__":
    place_with_rack_affinity()
