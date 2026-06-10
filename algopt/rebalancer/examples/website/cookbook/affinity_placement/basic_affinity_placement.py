#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Affinity and Anti-Affinity Placement Example

This example demonstrates how to place microservices with hardware and service
affinities. It shows:
- Hardware affinity (GPU workloads on GPU hosts, high-memory workloads on high-memory hosts)
- Service affinity (frontend near backend for low latency)
- Anti-affinity (replicas of same service on different hosts)
- Load balancing across hosts
"""

# solution_start
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    ConstraintSpecs,
    GoalSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    AssignmentAffinitiesSpec,
    AssignmentAffinity,
    BalanceSpec,
    BalanceSpecFormula,
    CapacitySpec,
    GroupCountSpec,
    GroupCountSpecBound,
    Limit,
    LimitType,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


def place_with_affinities():
    """Place microservices with hardware and service affinities."""
    solver = ProblemSolver(
        service_name="microservice-placer", service_scope="production"
    )

    solver.setObjectName("service_instance")
    solver.setContainerName("host")

    # Host hardware profiles
    host_profiles = {
        # GPU hosts (4 hosts)
        "gpu_host_0": {"gpu": True, "memory": 128},
        "gpu_host_1": {"gpu": True, "memory": 128},
        "gpu_host_2": {"gpu": True, "memory": 128},
        "gpu_host_3": {"gpu": True, "memory": 128},
        # High-memory hosts (6 hosts)
        "highmem_host_0": {"gpu": False, "memory": 512},
        "highmem_host_1": {"gpu": False, "memory": 512},
        "highmem_host_2": {"gpu": False, "memory": 512},
        "highmem_host_3": {"gpu": False, "memory": 512},
        "highmem_host_4": {"gpu": False, "memory": 512},
        "highmem_host_5": {"gpu": False, "memory": 512},
        # Standard hosts (10 hosts)
        **{f"standard_host_{i}": {"gpu": False, "memory": 64} for i in range(10)},
    }

    # Service instance configuration
    services = {
        "ml_training": {
            "count": 10,
            "requires_gpu": True,
            "cpu": 8.0,
            "memory": 32.0,
        },
        "data_processing": {
            "count": 15,
            "requires_highmem": True,
            "cpu": 4.0,
            "memory": 128.0,
        },
        "frontend": {
            "count": 25,
            "near_service": "backend",  # Should be near backend
            "cpu": 2.0,
            "memory": 4.0,
        },
        "backend": {
            "count": 25,
            "cpu": 4.0,
            "memory": 8.0,
        },
        "cache": {
            "count": 25,
            "cpu": 1.0,
            "memory": 16.0,
        },
    }

    # Initial random assignment (ignores affinities)
    assignment = {host: [] for host in host_profiles.keys()}

    instance_id = 0
    for service_name, config in services.items():
        for i in range(config["count"]):
            instance_name = f"{service_name}_{i}"
            # Random assignment
            host_idx = instance_id % len(host_profiles)
            host_name = list(host_profiles.keys())[host_idx]
            assignment[host_name].append(instance_name)
            instance_id += 1

    solver.setAssignment(assignment)

    # Define service partition (for anti-affinity)
    service_partition = {}
    for service_name, config in services.items():
        service_partition[service_name] = [
            f"{service_name}_{i}" for i in range(config["count"])
        ]
    solver.addPartition("service", service_partition)

    # Instance CPU and memory usage
    cpu_usage = {}
    memory_usage = {}
    for service_name, config in services.items():
        for i in range(config["count"]):
            instance_name = f"{service_name}_{i}"
            cpu_usage[instance_name] = config["cpu"]
            memory_usage[instance_name] = config["memory"]

    solver.addObjectDimension("cpu_usage", cpu_usage)
    solver.addObjectDimension("memory_usage", memory_usage)

    # Host capacities
    host_cpu_capacity = {host: 64.0 for host in host_profiles.keys()}
    host_memory_capacity = {
        host: profile["memory"] for host, profile in host_profiles.items()
    }

    solver.addContainerDimension("cpu_capacity", host_cpu_capacity)
    solver.addContainerDimension("memory_capacity", host_memory_capacity)

    # CONSTRAINT 1: CPU capacity
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="cpu-capacity",
                scope="host",
                dimension="cpu_usage",
            )
        )
    )

    # CONSTRAINT 2: Memory capacity
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="host",
                dimension="memory_usage",
            )
        )
    )

    # CONSTRAINT 3: Anti-affinity for replicas (max 1 instance per service per host)
    solver.addConstraint(
        ConstraintSpecs(
            groupCountSpec=GroupCountSpec(
                name="service-anti-affinity",
                scope="host",
                partitionName="service",
                bound=GroupCountSpecBound.MAX,
                limit=Limit(type=LimitType.ABSOLUTE, globalLimit=1),
            )
        )
    )

    # GOAL 1: Hardware affinity - GPU workloads prefer GPU hosts
    gpu_hosts = [host for host, profile in host_profiles.items() if profile["gpu"]]
    gpu_instances = [
        f"{service_name}_{i}"
        for service_name, config in services.items()
        if config.get("requires_gpu")
        for i in range(config["count"])
    ]

    gpu_affinities = [
        AssignmentAffinity(
            objectName=instance,
            scopeItemName=host,
            affinity=10.0,  # Strong preference
        )
        for instance in gpu_instances
        for host in gpu_hosts
    ]

    if gpu_affinities:
        solver.addGoal(
            GoalSpecs(
                assignmentAffinitiesSpec=AssignmentAffinitiesSpec(
                    name="gpu-hardware-affinity",
                    scope="host",
                    affinities=gpu_affinities,
                )
            ),
            weight=5.0,  # High priority
        )

    # GOAL 2: High-memory affinity
    highmem_hosts = [
        host for host, profile in host_profiles.items() if profile["memory"] >= 512
    ]
    highmem_instances = [
        f"{service_name}_{i}"
        for service_name, config in services.items()
        if config.get("requires_highmem")
        for i in range(config["count"])
    ]

    highmem_affinities = [
        AssignmentAffinity(
            objectName=instance,
            scopeItemName=host,
            affinity=8.0,
        )
        for instance in highmem_instances
        for host in highmem_hosts
    ]

    if highmem_affinities:
        solver.addGoal(
            GoalSpecs(
                assignmentAffinitiesSpec=AssignmentAffinitiesSpec(
                    name="highmem-hardware-affinity",
                    scope="host",
                    affinities=highmem_affinities,
                )
            ),
            weight=4.0,
        )

    # GOAL 3: Service affinity - frontend near backend (same host)
    frontend_backend_affinities = []
    frontend_instances = [f"frontend_{i}" for i in range(services["frontend"]["count"])]
    backend_instances = [f"backend_{i}" for i in range(services["backend"]["count"])]

    # Get hosts with backend instances and create affinity for frontend
    for host, instances in assignment.items():
        has_backend = any(inst in backend_instances for inst in instances)
        if has_backend:
            for frontend_inst in frontend_instances:
                frontend_backend_affinities.append(
                    AssignmentAffinity(
                        objectName=frontend_inst,
                        scopeItemName=host,
                        affinity=5.0,  # Medium preference
                    )
                )

    if frontend_backend_affinities:
        solver.addGoal(
            GoalSpecs(
                assignmentAffinitiesSpec=AssignmentAffinitiesSpec(
                    name="frontend-backend-affinity",
                    scope="host",
                    affinities=frontend_backend_affinities,
                )
            ),
            weight=2.0,
        )

    # GOAL 4: Balance CPU across hosts
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-cpu",
                scope="host",
                dimension="cpu_usage",
                formula=BalanceSpecFormula.LEGACY,
                fixAverageToInitial=True,
            )
        ),
        weight=1.0,
    )

    # GOAL 5: Balance memory across hosts
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-memory",
                scope="host",
                dimension="memory_usage",
                formula=BalanceSpecFormula.LEGACY,
                fixAverageToInitial=True,
            )
        ),
        weight=1.0,
    )

    # Use Local Search solver
    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=60000))
    )

    # Solve
    solution = solver.solve()

    # Analyze solution
    print(f"Solution found in {solution.profile.solveTime}ms")
    print(f"Objective value: {solution.objectiveValue}")
    print(f"Instances moved: {solution.profile.moveCount}")

    # Verify affinity satisfaction
    print("\nHardware affinity verification:")
    verify_hardware_affinity(solution.assignment, services, host_profiles)

    print("\nService anti-affinity verification:")
    verify_anti_affinity(solution.assignment, service_partition)

    print("\nFrontend-backend colocation:")
    verify_service_colocation(
        solution.assignment, frontend_instances, backend_instances
    )

    return solution


def verify_hardware_affinity(assignment, services, host_profiles):
    """Verify GPU and high-memory affinities."""
    # Check GPU placement
    gpu_instances = [
        f"{service_name}_{i}"
        for service_name, config in services.items()
        if config.get("requires_gpu")
        for i in range(config["count"])
    ]

    gpu_on_gpu_hosts = 0
    gpu_on_other_hosts = 0

    for host, instances in assignment.items():
        is_gpu_host = host_profiles[host]["gpu"]
        gpu_count = sum(1 for inst in instances if inst in gpu_instances)

        if is_gpu_host:
            gpu_on_gpu_hosts += gpu_count
        else:
            gpu_on_other_hosts += gpu_count

    print(f"  GPU instances on GPU hosts: {gpu_on_gpu_hosts}/{len(gpu_instances)}")
    if gpu_on_other_hosts > 0:
        print(f"  ❌ {gpu_on_other_hosts} GPU instances on non-GPU hosts")
    else:
        print(f"  ✅ All GPU instances on GPU hosts")


def verify_anti_affinity(assignment, service_partition):
    """Verify no host has >1 instance of same service."""
    violations = 0

    for host, instances in assignment.items():
        for service_name, service_instances in service_partition.items():
            count = sum(1 for inst in instances if inst in service_instances)
            if count > 1:
                violations += 1
                print(f"  ❌ {host} has {count} instances of {service_name}")

    if violations == 0:
        print(f"  ✅ All hosts respect anti-affinity (≤1 instance per service)")


def verify_service_colocation(assignment, frontend_instances, backend_instances):
    """Verify frontend and backend colocation."""
    colocated_count = 0

    for instances in assignment.values():
        has_frontend = any(inst in frontend_instances for inst in instances)
        has_backend = any(inst in backend_instances for inst in instances)

        if has_frontend and has_backend:
            frontend_count = sum(1 for inst in instances if inst in frontend_instances)
            backend_count = sum(1 for inst in instances if inst in backend_instances)
            colocated_count += min(frontend_count, backend_count)

    print(f"  Colocated frontend-backend pairs: {colocated_count}")


if __name__ == "__main__":
    place_with_affinities()
# solution_end
