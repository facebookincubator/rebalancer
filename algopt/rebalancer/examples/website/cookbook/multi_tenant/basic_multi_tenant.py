#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Multi-Tenant Fair Sharing Example

This example demonstrates how to allocate resources fairly across multiple
tenants with different priorities and quotas. It shows:
- Partitioning workloads by tenant
- Premium tenant isolation (max 1 per server)
- CPU and memory capacity constraints
- Balanced resource allocation within tenant allocations
- Minimizing workload movement during rebalancing

Problem: 5 tenants, 20 servers, 200 workloads total
Goal: Fair allocation while respecting quotas and isolation
"""

# solution_start
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
    GroupCountSpec,
    GroupCountSpecBound,
    Limit,
    LimitType,
    MinimizeMovementSpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


def allocate_multi_tenant_resources():
    solver = ProblemSolver(
        service_name="multi-tenant-allocator", service_scope="production"
    )

    solver.setObjectName("workload")
    solver.setContainerName("server")

    # Tenant configuration
    tenants = {
        "premium_tenant_a": {
            "tier": "premium",
            "cpu_quota": 100,  # cores
            "mem_quota": 500,  # GB
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

    # Initial imbalanced assignment
    assignment = {f"server{i}": [] for i in range(20)}

    # Create workloads and assign initially (imbalanced)
    workload_id = 0
    for tenant, config in tenants.items():
        for i in range(config["workload_count"]):
            workload_name = f"{tenant}_workload_{i}"
            # Concentrate on first few servers (bad)
            server_idx = workload_id % 5
            assignment[f"server{server_idx}"].append(workload_name)
            workload_id += 1

    solver.setAssignment(assignment)

    # Define tenant partition (group workloads by tenant)
    tenant_partition = {}
    for tenant, config in tenants.items():
        tenant_partition[tenant] = [
            f"{tenant}_workload_{i}" for i in range(config["workload_count"])
        ]
    solver.addPartition("tenant", tenant_partition)

    # Workload CPU usage (varies per tenant tier)
    cpu_usage = {}
    for tenant, config in tenants.items():
        avg_cpu_per_workload = config["cpu_quota"] / config["workload_count"]
        for i in range(config["workload_count"]):
            workload_name = f"{tenant}_workload_{i}"
            # Add some variance
            cpu_usage[workload_name] = avg_cpu_per_workload * (0.8 + 0.4 * (i % 5) / 5)
    solver.addObjectDimension("cpu_usage", cpu_usage)

    # Workload memory usage
    memory_usage = {}
    for tenant, config in tenants.items():
        avg_mem_per_workload = config["mem_quota"] / config["workload_count"]
        for i in range(config["workload_count"]):
            workload_name = f"{tenant}_workload_{i}"
            memory_usage[workload_name] = avg_mem_per_workload * (
                0.8 + 0.4 * (i % 5) / 5
            )
    solver.addObjectDimension("memory_usage", memory_usage)

    # Server capacities (uniform for simplicity)
    server_cpu_capacity = {f"server{i}": 25.0 for i in range(20)}  # 25 cores each
    server_mem_capacity = {f"server{i}": 128.0 for i in range(20)}  # 128 GB each
    solver.addContainerDimension("cpu_capacity", server_cpu_capacity)
    solver.addContainerDimension("memory_capacity", server_mem_capacity)

    # CONSTRAINT 1: Server CPU capacity
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="server-cpu-capacity",
                scope="server",
                dimension="cpu_usage",
            )
        )
    )

    # CONSTRAINT 2: Server memory capacity
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="server-memory-capacity",
                scope="server",
                dimension="memory_usage",
            )
        )
    )

    # CONSTRAINT 3: Premium tenant isolation (max 1 premium tenant per server)
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

    # GOAL 1: Balance CPU within each tenant's allocation
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

    # GOAL 2: Balance memory within each tenant's allocation
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

    # GOAL 3: Minimize workload movement
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

    # Use Local Search solver
    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=60000))
    )

    # Solve
    solution = solver.solve()

    # Analyze solution
    print(f"Solution found in {solution.profile.solveTime}ms")
    print(f"Objective value: {solution.objectiveValue}")
    print(f"Workloads moved: {solution.profile.moveCount}")

    # Calculate per-tenant allocation
    print("\nPer-tenant resource allocation:")
    print(
        f"{'Tenant':<25} {'Tier':<10} {'Quota CPU':<12} {'Actual CPU':<12} {'Quota Mem':<12} {'Actual Mem':<12} {'Servers'}"
    )
    print("-" * 110)

    for tenant, config in tenants.items():
        tenant_workloads = tenant_partition[tenant]
        tenant_cpu = sum(cpu_usage[w] for w in tenant_workloads)
        tenant_mem = sum(memory_usage[w] for w in tenant_workloads)

        # Count servers with this tenant
        servers_with_tenant = set()
        for server, workloads in solution.assignment.items():
            if any(w in tenant_workloads for w in workloads):
                servers_with_tenant.add(server)

        quota_cpu = config["cpu_quota"]
        quota_mem = config["mem_quota"]

        print(
            f"{tenant:<25} {config['tier']:<10} {quota_cpu:<12.1f} {tenant_cpu:<12.1f} "
            f"{quota_mem:<12.1f} {tenant_mem:<12.1f} {len(servers_with_tenant)}"
        )

    # Verify premium isolation
    print("\nPremium tenant isolation verification:")
    verify_premium_isolation(solution.assignment, tenant_partition, tenants)

    return solution


def verify_premium_isolation(assignment, tenant_partition, tenants):
    """Verify no two premium tenants share a server."""
    premium_tenants = [
        tenant for tenant, config in tenants.items() if config["tier"] == "premium"
    ]

    violations = 0
    for server, workloads in assignment.items():
        # Count how many premium tenants on this server
        tenants_on_server = set()
        for tenant in premium_tenants:
            if any(w in tenant_partition[tenant] for w in workloads):
                tenants_on_server.add(tenant)

        if len(tenants_on_server) > 1:
            violations += 1
            print(
                f"  X {server} has {len(tenants_on_server)} premium tenants: {tenants_on_server}"
            )

    if violations == 0:
        print(
            f"  OK All servers respect premium isolation (<=1 premium tenant per server)"
        )


if __name__ == "__main__":
    allocate_multi_tenant_resources()
# solution_end
