#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Zone Anti-Affinity Variation

This variation demonstrates how to spread replicas across availability zones
for high availability and fault tolerance. This ensures that if one zone fails,
the application remains available with replicas in other zones.
"""

from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    ConstraintSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    GroupCountSpec,
    GroupCountSpecBound,
    Limit,
    LimitType,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


def place_with_zone_anti_affinity():
    """Spread replicas across availability zones."""
    solver = ProblemSolver(
        service_name="microservice-placer", service_scope="production"
    )

    solver.setObjectName("service_instance")
    solver.setContainerName("host")

    # Setup hosts across zones
    zones = ["us-east-1a", "us-east-1b", "us-east-1c"]
    hosts_per_zone = 5
    host_to_zone = {}
    assignment = {}

    for zone in zones:
        for host_id in range(hosts_per_zone):
            host_name = f"{zone}_host_{host_id}"
            host_to_zone[host_name] = zone
            assignment[host_name] = []

    # Create replica groups
    num_replicas = 3
    instances_per_replica = 5
    replica_to_instances = {}

    for replica_id in range(num_replicas):
        replica_name = f"replica_{replica_id}"
        replica_to_instances[replica_name] = []
        for instance_id in range(instances_per_replica):
            instance_name = f"app_{replica_id}_{instance_id}"
            replica_to_instances[replica_name].append(instance_name)
            # Random initial assignment
            host_idx = (replica_id * instances_per_replica + instance_id) % len(
                assignment
            )
            host_name = list(assignment.keys())[host_idx]
            assignment[host_name].append(instance_name)

    solver.setAssignment(assignment)

    # variation_start
    # Define zone partition
    solver.addPartition("replica_group", replica_to_instances)

    # Max 1 replica per zone
    solver.addConstraint(
        ConstraintSpecs(
            groupCountSpec=GroupCountSpec(
                name="zone-anti-affinity",
                scope="availability_zone",
                partitionName="replica_group",
                bound=GroupCountSpecBound.MAX,
                limit=Limit(type=LimitType.ABSOLUTE, globalLimit=1),
            )
        )
    )
    # variation_end

    # Define zone scope
    solver.addScope("availability_zone", host_to_zone)

    # Use Local Search solver
    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=30000))
    )

    # Solve
    solution = solver.solve()

    print(f"Solution found in {solution.profile.solveTime}ms")
    print(f"Objective value: {solution.objectiveValue}")
    print(f"Instances moved: {solution.profile.moveCount}")

    # Analyze zone distribution
    zone_assignment = {}
    for host, instances in solution.assignment.items():
        zone = host_to_zone[host]
        if zone not in zone_assignment:
            zone_assignment[zone] = []
        zone_assignment[zone].extend(instances)

    print("\nZone distribution:")
    for zone, instances in zone_assignment.items():
        replicas_in_zone = set()
        for inst in instances:
            replica_id = inst.split("_")[1]
            replicas_in_zone.add(f"replica_{replica_id}")
        print(f"  {zone}: {len(instances)} instances, replicas: {replicas_in_zone}")

    # Verify anti-affinity
    print("\nAnti-affinity verification:")
    violations = 0
    for zone, instances in zone_assignment.items():
        replica_count = {}
        for inst in instances:
            replica_id = inst.split("_")[1]
            replica_name = f"replica_{replica_id}"
            replica_count[replica_name] = replica_count.get(replica_name, 0) + 1

        for replica_name, count in replica_count.items():
            if count > 1:
                violations += 1
                print(f"  ❌ {zone} has {count} instances of {replica_name}")

    if violations == 0:
        print(f"  ✅ All zones respect anti-affinity (≤1 instance per replica)")

    return solution


if __name__ == "__main__":
    place_with_zone_anti_affinity()
