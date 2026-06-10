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
Basic Service Colocation for Microservices

This example demonstrates how to colocate related microservices together
for performance benefits while maintaining fault tolerance through diversity.

Problem: 60 service instances (20 frontend, 20 API, 20 cache) need to be
deployed across 20 hosts. Related services benefit from colocation (low latency)
but each service needs diversity across hosts for fault tolerance.

Goal: Maximize colocation of service pairs while spreading each service type
across multiple hosts.
"""

# solution_start
# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    ConstraintSpecs,
    GoalSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    BalanceSpec,
    BalanceSpecFormula,
    CapacitySpec,
    ColocateGroupsSpec,
    ColocateGroupsSpecBound,
    GroupCountSpec,
    GroupCountSpecBound,
    Limit,
    LimitType,
    MinimizeMovementSpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


def colocate_microservices():
    """Colocate related microservices for performance."""
    solver = ProblemSolver(
        service_name="microservice-colocation", service_scope="production"
    )

    solver.setObjectName("instance")
    solver.setContainerName("host")

    # Service configuration
    services = {
        "frontend": {"count": 20, "cpu": 2.0, "memory": 4.0},
        "api": {"count": 20, "cpu": 4.0, "memory": 8.0},
        "cache": {"count": 20, "cpu": 1.0, "memory": 16.0},
    }

    # Initial spread assignment (all services evenly distributed)
    assignment = {f"host{i}": [] for i in range(20)}

    instance_id = 0
    for service_name, config in services.items():
        for i in range(config["count"]):
            instance_name = f"{service_name}_instance_{i}"
            # Initially spread across all hosts
            host_idx = instance_id % 20
            assignment[f"host{host_idx}"].append(instance_name)
            instance_id += 1

    solver.setAssignment(assignment)

    # Define service_type partition (group by service)
    service_type_partition = {}
    for service_name, config in services.items():
        service_type_partition[service_name] = [
            f"{service_name}_instance_{i}" for i in range(config["count"])
        ]
    solver.addPartition("service_type", service_type_partition)

    # Define colocation pairs partition
    # Each group represents instances that should be colocated
    service_pair_partition = {}

    # Frontend-API pairs (instance 0 with instance 0, etc.)
    for i in range(20):
        pair_name = f"frontend_api_pair_{i}"
        service_pair_partition[pair_name] = [
            f"frontend_instance_{i}",
            f"api_instance_{i}",
        ]

    # API-Cache pairs
    for i in range(20):
        pair_name = f"api_cache_pair_{i}"
        service_pair_partition[pair_name] = [
            f"api_instance_{i}",
            f"cache_instance_{i}",
        ]

    solver.addPartition("service_pair", service_pair_partition)

    # Instance resource usage
    cpu_usage = {}
    memory_usage = {}
    for service_name, config in services.items():
        for i in range(config["count"]):
            instance_name = f"{service_name}_instance_{i}"
            cpu_usage[instance_name] = config["cpu"]
            memory_usage[instance_name] = config["memory"]

    solver.addObjectDimension("cpu_usage", cpu_usage)
    solver.addObjectDimension("memory_usage", memory_usage)

    # Host capacities (uniform)
    host_cpu_capacity = {f"host{i}": 32.0 for i in range(20)}  # 32 cores
    host_memory_capacity = {f"host{i}": 128.0 for i in range(20)}  # 128 GB
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

    # CONSTRAINT 3: Max 5 instances per service per host (diversity)
    solver.addConstraint(
        ConstraintSpecs(
            groupCountSpec=GroupCountSpec(
                name="max-instances-per-service-per-host",
                scope="host",
                partitionName="service_type",
                bound=GroupCountSpecBound.MAX,
                limit=Limit(type=LimitType.ABSOLUTE, globalLimit=5),
            )
        )
    )

    # GOAL 1: Colocate service pairs (MAX = keep pairs on same host)
    solver.addGoal(
        GoalSpecs(
            colocateGroupsSpec=ColocateGroupsSpec(
                name="colocate-service-pairs",
                scope="host",
                partitionName="service_pair",
                bound=ColocateGroupsSpecBound.MAX,
                limits=Limit(type=LimitType.ABSOLUTE, globalLimit=1),
            )
        ),
        weight=5.0,  # High weight - colocation is important for performance
    )

    # GOAL 2: Spread each service across hosts (MIN = spread out)
    solver.addGoal(
        GoalSpecs(
            colocateGroupsSpec=ColocateGroupsSpec(
                name="spread-services",
                scope="host",
                partitionName="service_type",
                bound=ColocateGroupsSpecBound.MIN,
                limits=Limit(type=LimitType.ABSOLUTE, globalLimit=10),
            )
        ),
        weight=3.0,  # Medium weight - diversity for fault tolerance
    )

    # GOAL 3: Balance CPU across hosts
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

    # GOAL 4: Balance memory across hosts
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

    # GOAL 5: Minimize movement
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-movement",
                scope="host",
                dimension="cpu_usage",
            )
        ),
        weight=0.2,
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

    # Analyze colocation results
    print("\nColocation analysis:")
    analyze_colocation(solution.assignment, service_pair_partition)

    # Analyze service spread
    print("\nService spread analysis:")
    analyze_service_spread(solution.assignment, service_type_partition)

    return solution


def analyze_colocation(assignment, service_pair_partition):
    """Analyze how well service pairs are colocated."""
    colocated_pairs = 0
    split_pairs = 0

    for instances in service_pair_partition.values():
        # Find which hosts have these instances
        hosts_with_pair = set()
        for host, host_instances in assignment.items():
            if any(instance in host_instances for instance in instances):
                hosts_with_pair.add(host)

        if len(hosts_with_pair) == 1:
            colocated_pairs += 1
        else:
            split_pairs += 1

    total_pairs = len(service_pair_partition)
    colocation_pct = (colocated_pairs / total_pairs) * 100

    print(f"  Colocated pairs: {colocated_pairs}/{total_pairs} ({colocation_pct:.1f}%)")
    print(f"  Split pairs: {split_pairs}/{total_pairs}")


def analyze_service_spread(assignment, service_type_partition):
    """Analyze service diversity across hosts."""
    for service_name, instances in service_type_partition.items():
        # Count hosts with this service
        hosts_with_service = set()
        for host, host_instances in assignment.items():
            if any(instance in instances for instance in host_instances):
                hosts_with_service.add(host)

        instance_count = len(instances)
        host_count = len(hosts_with_service)

        print(f"  {service_name}: {instance_count} instances on {host_count} hosts")


if __name__ == "__main__":
    colocate_microservices()
# solution_end
