#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Negative Affinity (Avoid Assignments) Variation

This variation demonstrates how to use AvoidAssignmentsSpec to enforce hard
incompatibility constraints. This is useful when certain workloads CANNOT run
on specific hosts due to hardware requirements or security restrictions.
Unlike soft affinity, this creates a hard constraint that cannot be violated.
"""

from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    ConstraintSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    AvoidAssignment,
    AvoidAssignmentsSpec,
    CapacitySpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


def place_with_negative_affinity():
    """Place services with hard incompatibility constraints."""
    solver = ProblemSolver(
        service_name="microservice-placer", service_scope="production"
    )

    solver.setObjectName("service_instance")
    solver.setContainerName("host")

    # Host hardware profiles
    host_profiles = {
        "gpu_host_0": {"gpu": True, "memory": 128},
        "gpu_host_1": {"gpu": True, "memory": 128},
        "standard_host_0": {"gpu": False, "memory": 64},
        "standard_host_1": {"gpu": False, "memory": 64},
        "standard_host_2": {"gpu": False, "memory": 64},
        "standard_host_3": {"gpu": False, "memory": 64},
    }

    # Initial assignment
    assignment = {host: [] for host in host_profiles.keys()}

    # Create GPU instances
    gpu_instances = [f"ml_training_{i}" for i in range(10)]
    cpu_instances = [f"web_server_{i}" for i in range(20)]

    # Random initial assignment (may violate requirements)
    all_hosts = list(host_profiles.keys())
    for i, inst in enumerate(gpu_instances + cpu_instances):
        host_idx = i % len(all_hosts)
        assignment[all_hosts[host_idx]].append(inst)

    solver.setAssignment(assignment)

    # Add dimensions
    resource_usage = {}
    for inst in gpu_instances:
        resource_usage[inst] = 8.0
    for inst in cpu_instances:
        resource_usage[inst] = 2.0

    solver.addObjectDimension("cpu_usage", resource_usage)

    host_cpu_capacity = {host: 64.0 for host in host_profiles.keys()}
    solver.addContainerDimension("cpu_capacity", host_cpu_capacity)

    # CPU capacity constraint
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
    # GPU workloads CANNOT run on non-GPU hosts (hard constraint)
    non_gpu_hosts = [
        host for host, profile in host_profiles.items() if not profile["gpu"]
    ]

    solver.addConstraint(
        ConstraintSpecs(
            avoidAssignmentsSpec=AvoidAssignmentsSpec(
                name="gpu-hardware-requirement",
                scope="host",
                assignments=[
                    AvoidAssignment(object=inst, scopeItems=non_gpu_hosts)
                    for inst in gpu_instances
                ],
            )
        )
    )
    # variation_end

    # Use Local Search solver
    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=30000))
    )

    # Solve
    solution = solver.solve()

    print(f"Solution found in {solution.profile.solveTime}ms")
    print(f"Objective value: {solution.objectiveValue}")
    print(f"Instances moved: {solution.profile.moveCount}")

    # Verify GPU placement
    print("\nGPU placement verification:")
    for host, instances in solution.assignment.items():
        is_gpu_host = host_profiles[host]["gpu"]
        gpu_count = sum(1 for inst in instances if inst in gpu_instances)
        if gpu_count > 0:
            if is_gpu_host:
                print(f"  ✅ {host}: {gpu_count} GPU instances (valid)")
            else:
                print(f"  ❌ {host}: {gpu_count} GPU instances (VIOLATION!)")

    return solution


if __name__ == "__main__":
    place_with_negative_affinity()
