#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Multi-Tenant with Tenant Colocation

This variation demonstrates how to colocate tenant workloads for locality.
Standard tenants prefer colocation (fewer servers) which can improve
data locality, reduce network overhead, and simplify management.

Key Feature: ColocateGroupsSpec to encourage tenant workloads to share fewer servers
"""

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


def allocate_multi_tenant_with_colocation():
    solver = ProblemSolver(
        service_name="multi-tenant-allocator", service_scope="production"
    )

    solver.setObjectName("workload")
    solver.setContainerName("server")

    # Tenant configuration
    tenants = {
        "premium_tenant_a": {
            "tier": "premium",
            "cpu_quota": 100,
            "mem_quota": 500,
            "priority": 3,
            "workload_count": 40,
        },
        "premium_tenant_b": {
            "tier": "premium",
            "cpu_quota": 80,
            "mem_quota": 400,
            "priority": 3,
            "workload_count": 30,
        },
        "standard_tenant_c": {
            "tier": "standard",
            "cpu_quota": 60,
            "mem_quota": 300,
            "priority": 2,
            "workload_count": 50,
        },
        "standard_tenant_d": {
            "tier": "standard",
            "cpu_quota": 50,
            "mem_quota": 250,
            "priority": 2,
            "workload_count": 40,
        },
        "basic_tenant_e": {
            "tier": "basic",
            "cpu_quota": 30,
            "mem_quota": 150,
            "priority": 1,
            "workload_count": 40,
        },
    }

    # Initial assignment
    assignment = {f"server{i}": [] for i in range(20)}

    workload_id = 0
    for tenant, config in tenants.items():
        for i in range(config["workload_count"]):
            workload_name = f"{tenant}_workload_{i}"
            server_idx = workload_id % 5
            assignment[f"server{server_idx}"].append(workload_name)
            workload_id += 1

    solver.setAssignment(assignment)

    # Define tenant partition
    tenant_partition = {}
    for tenant, config in tenants.items():
        tenant_partition[tenant] = [
            f"{tenant}_workload_{i}" for i in range(config["workload_count"])
        ]
    solver.addPartition("tenant", tenant_partition)

    # Workload dimensions
    cpu_usage = {}
    memory_usage = {}
    for tenant, config in tenants.items():
        avg_cpu_per_workload = config["cpu_quota"] / config["workload_count"]
        avg_mem_per_workload = config["mem_quota"] / config["workload_count"]
        for i in range(config["workload_count"]):
            workload_name = f"{tenant}_workload_{i}"
            cpu_usage[workload_name] = avg_cpu_per_workload * (0.8 + 0.4 * (i % 5) / 5)
            memory_usage[workload_name] = avg_mem_per_workload * (
                0.8 + 0.4 * (i % 5) / 5
            )
    solver.addObjectDimension("cpu_usage", cpu_usage)
    solver.addObjectDimension("memory_usage", memory_usage)

    # Server capacities
    server_cpu_capacity = {f"server{i}": 25.0 for i in range(20)}
    server_mem_capacity = {f"server{i}": 128.0 for i in range(20)}
    solver.addContainerDimension("cpu_capacity", server_cpu_capacity)
    solver.addContainerDimension("memory_capacity", server_mem_capacity)

    # Basic constraints
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="server-cpu-capacity",
                scope="server",
                dimension="cpu_usage",
            )
        )
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="server-memory-capacity",
                scope="server",
                dimension="memory_usage",
            )
        )
    )

    # Premium tenant isolation
    premium_tenants = [
        tenant for tenant, config in tenants.items() if config["tier"] == "premium"
    ]

    if premium_tenants:
        solver.addConstraint(
            ConstraintSpecs(
                groupCountSpec=GroupCountSpec(
                    name="premium-tenant-isolation",
                    scope="server",
                    partitionName="tenant",
                    bound=GroupCountSpecBound.MAX,
                    limit=Limit(type=LimitType.ABSOLUTE, globalLimit=1),
                )
            )
        )

    # variation_start
    # Standard tenants prefer colocation (fewer servers)
    for tenant, config in tenants.items():
        if config["tier"] == "standard":
            solver.addGoal(
                GoalSpecs(
                    colocateGroupsSpec=ColocateGroupsSpec(
                        name=f"colocate-{tenant}",
                        scope="server",
                        partitionName="tenant",
                        bound=ColocateGroupsSpecBound.MAX,
                        limits=Limit(type=LimitType.ABSOLUTE, globalLimit=5),
                    )
                ),
                weight=0.3,
            )
    # variation_end

    # Goals
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-cpu",
                scope="server",
                dimension="cpu_usage",
                formula=BalanceSpecFormula.LEGACY,
                fixAverageToInitial=True,
            )
        ),
        weight=1.0,
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-memory",
                scope="server",
                dimension="memory_usage",
                formula=BalanceSpecFormula.LEGACY,
                fixAverageToInitial=True,
            )
        ),
        weight=1.0,
    )

    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-movement",
                scope="server",
                dimension="cpu_usage",
            )
        ),
        weight=0.2,
    )

    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=60000))
    )

    # Solve
    solution = solver.solve()

    # Analyze solution
    print(f"Solution found in {solution.profile.solveTime}ms")
    print(f"Objective value: {solution.objectiveValue}")
    print(f"Workloads moved: {solution.profile.moveCount}")

    # Calculate per-tenant server spread
    print("\nPer-tenant server spread (colocation analysis):")
    print(
        f"{'Tenant':<25} {'Tier':<10} {'Workloads':<12} {'Servers Used':<15} {'Avg per Server'}"
    )
    print("-" * 100)

    for tenant, config in tenants.items():
        tenant_workloads = tenant_partition[tenant]

        # Count servers with this tenant
        servers_with_tenant = set()
        for server, workloads in solution.assignment.items():
            if any(w in tenant_workloads for w in workloads):
                servers_with_tenant.add(server)

        avg_per_server = (
            len(tenant_workloads) / len(servers_with_tenant)
            if servers_with_tenant
            else 0
        )

        print(
            f"{tenant:<25} {config['tier']:<10} {len(tenant_workloads):<12} "
            f"{len(servers_with_tenant):<15} {avg_per_server:.1f}"
        )

    return solution


if __name__ == "__main__":
    allocate_multi_tenant_with_colocation()
