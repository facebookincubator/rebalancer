---
sidebar_position: 5
---

# Multi-Tenant Fair Sharing

Allocate resources fairly across multiple tenants with different priorities and quotas.

## Problem Description

You have:
- **5 tenants** sharing **20 servers**
- **200 workloads** total across all tenants
- Each tenant has different **priorities** and **resource quotas**
- **Goal**: Fairly allocate resources while respecting quotas and isolation requirements

## Real-World Scenario

A shared compute platform serves 5 customers (tenants). Each tenant has purchased a quota of resources (CPU, memory) and has different service tiers (premium, standard, basic). You need to:

1. Ensure each tenant gets at least their guaranteed quota
2. Distribute excess capacity proportionally to tenant priority
3. Isolate tenants (no two tenants on same server for premium tier)
4. Balance load within each tenant's allocation

## Problem Modeling

### Rebalancer Terms

- **Objects**: 200 workload instances across all tenants
- **Containers**: 20 servers
- **Partitions**:
  - `tenant`: Groups workloads by tenant (5 groups)
- **Dimensions**:
  - `cpu_usage`: CPU consumed by each workload
  - `memory_usage`: Memory consumed by each workload
  - `tenant_cpu_quota`: Guaranteed CPU quota per tenant
  - `tenant_mem_quota`: Guaranteed memory quota per tenant
- **Constraints**:
  - Each tenant gets at least their quota
  - Premium tenants: max 1 per server (isolation)
  - Server capacity limits
- **Goals**:
  - Balance CPU within each tenant's servers
  - Balance memory within each tenant's servers
  - Minimize movement

## Complete Solution

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/multi_tenant/basic_multi_tenant.py start=solution_start end=solution_end
from rebalancer import ProblemSolver
from rebalancer.specs import (
    BalanceSpec,
    CapacitySpec,
    ConstraintSpec,
    GoalSpec,
    GroupCountSpec,
    Limit,
    LocalSearchSolverSpec,
    MinimizeMovementSpec,
    SolverSpec,
)


def allocate_multi_tenant_resources():
    # ``rebalancer.specs`` exposes TypedDict shims over the Thrift JSON wire
    # format the binding accepts; the constructors just produce dicts at
    # runtime, so editors can autocomplete fields and warn on typos.
    solver = ProblemSolver(
        service_name="multi-tenant-allocator",
        service_scope="production",
    )

    solver.set_object_name("workload")
    solver.set_container_name("server")

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

    solver.set_assignment(assignment)

    # Define tenant partition (group workloads by tenant)
    tenant_partition = {}
    for tenant, config in tenants.items():
        tenant_partition[tenant] = [
            f"{tenant}_workload_{i}"
            for i in range(config["workload_count"])
        ]
    solver.add_partition("tenant", tenant_partition)

    # Workload CPU usage (varies per tenant tier)
    cpu_usage = {}
    for tenant, config in tenants.items():
        avg_cpu_per_workload = config["cpu_quota"] / config["workload_count"]
        for i in range(config["workload_count"]):
            workload_name = f"{tenant}_workload_{i}"
            # Add some variance
            cpu_usage[workload_name] = avg_cpu_per_workload * (0.8 + 0.4 * (i % 5) / 5)
    solver.add_object_dimension("cpu_usage", cpu_usage)

    # Workload memory usage
    memory_usage = {}
    for tenant, config in tenants.items():
        avg_mem_per_workload = config["mem_quota"] / config["workload_count"]
        for i in range(config["workload_count"]):
            workload_name = f"{tenant}_workload_{i}"
            memory_usage[workload_name] = avg_mem_per_workload * (0.8 + 0.4 * (i % 5) / 5)
    solver.add_object_dimension("memory_usage", memory_usage)

    # Server capacities (uniform for simplicity)
    server_cpu_capacity = {f"server{i}": 25.0 for i in range(20)}  # 25 cores each
    server_mem_capacity = {f"server{i}": 128.0 for i in range(20)}  # 128 GB each
    solver.add_container_dimension("cpu_capacity", server_cpu_capacity)
    solver.add_container_dimension("memory_capacity", server_mem_capacity)

    # CONSTRAINT 1: Server CPU capacity
    solver.add_constraint(
        ConstraintSpec(
            capacitySpec=CapacitySpec(
                name="server-cpu-capacity",
                scope="server",
                dimension="cpu_usage",
            )
        )
    )

    # CONSTRAINT 2: Server memory capacity
    solver.add_constraint(
        ConstraintSpec(
            capacitySpec=CapacitySpec(
                name="server-memory-capacity",
                scope="server",
                dimension="memory_usage",
            )
        )
    )

    # CONSTRAINT 3: Premium tenant isolation (max 1 premium tenant per server)
    premium_tenants = [
        tenant for tenant, config in tenants.items()
        if config["tier"] == "premium"
    ]

    if premium_tenants:
        solver.add_constraint(
            ConstraintSpec(
                groupCountSpec=GroupCountSpec(
                    name="premium-tenant-isolation",
                    scope="server",
                    partitionName="tenant",
                    bound="MAX",
                    limit=Limit(type="ABSOLUTE", globalLimit=1),
                )
            )
        )

    # GOAL 1: Balance CPU within each tenant's allocation
    solver.add_goal(
        GoalSpec(
            balanceSpec=BalanceSpec(
                name="balance-cpu",
                scope="server",
                dimension="cpu_usage",
                formula="LEGACY",
                fixAverageToInitial=True,
            )
        ),
        weight=1.0,
    )

    # GOAL 2: Balance memory within each tenant's allocation
    solver.add_goal(
        GoalSpec(
            balanceSpec=BalanceSpec(
                name="balance-memory",
                scope="server",
                dimension="memory_usage",
                formula="LEGACY",
                fixAverageToInitial=True,
            )
        ),
        weight=1.0,
    )

    # GOAL 3: Minimize workload movement
    solver.add_goal(
        GoalSpec(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-movement",
                scope="server",
                dimension="cpu_usage",
            )
        ),
        weight=0.2,
    )

    # Use Local Search solver (60-second time budget).
    solver.add_solver(
        SolverSpec(
            localSearchSolverSpec=LocalSearchSolverSpec(solveTime=60)
        )
    )

    # Solve
    solution = solver.solve()

    # Analyze solution (rebalancer.specs.AssignmentSolution shape).
    print(f"Solve time: {solution['problemProfile']['solveSec']:.1f}s")
    print(f"Objective value: {solution['finalObjective']['value']}")
    move_count = sum(len(s.get("moves", [])) for s in solution.get("movesSummary", []))
    print(f"Workloads moved: {move_count}")

    # Build server -> workloads map from object -> container assignment
    by_server: dict[str, list[str]] = {}
    for workload, server in solution["assignment"].items():
        by_server.setdefault(server, []).append(workload)

    # Calculate per-tenant allocation
    print("\nPer-tenant resource allocation:")
    print(f"{'Tenant':<25} {'Tier':<10} {'Quota CPU':<12} {'Actual CPU':<12} {'Quota Mem':<12} {'Actual Mem':<12} {'Servers'}")
    print("-" * 110)

    for tenant, config in tenants.items():
        tenant_workloads = tenant_partition[tenant]
        tenant_cpu = sum(cpu_usage[w] for w in tenant_workloads)
        tenant_mem = sum(memory_usage[w] for w in tenant_workloads)

        # Count servers with this tenant
        servers_with_tenant = set()
        for server, workloads in by_server.items():
            if any(w in tenant_workloads for w in workloads):
                servers_with_tenant.add(server)

        quota_cpu = config["cpu_quota"]
        quota_mem = config["mem_quota"]

        print(f"{tenant:<25} {config['tier']:<10} {quota_cpu:<12.1f} {tenant_cpu:<12.1f} "
              f"{quota_mem:<12.1f} {tenant_mem:<12.1f} {len(servers_with_tenant)}")

    # Verify premium isolation
    print("\nPremium tenant isolation verification:")
    verify_premium_isolation(by_server, tenant_partition, tenants)

    return solution


def verify_premium_isolation(by_server, tenant_partition, tenants):
    """Verify no two premium tenants share a server."""
    premium_tenants = [
        tenant for tenant, config in tenants.items()
        if config["tier"] == "premium"
    ]

    violations = 0
    for server, workloads in by_server.items():
        # Count how many premium tenants on this server
        tenants_on_server = set()
        for tenant in premium_tenants:
            if any(w in tenant_partition[tenant] for w in workloads):
                tenants_on_server.add(tenant)

        if len(tenants_on_server) > 1:
            violations += 1
            print(f"  ❌ {server} has {len(tenants_on_server)} premium tenants: {tenants_on_server}")

    if violations == 0:
        print(f"  ✅ All servers respect premium isolation (≤1 premium tenant per server)")


if __name__ == "__main__":
    allocate_multi_tenant_resources()
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/multi_tenant/basic_multi_tenant.cpp start=solution_start end=solution_end
#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/SolverSpecs_types.h"
#include <folly/executors/ThreadPoolExecutor.h>
#include <fmt/format.h>
#include <iostream>
#include <map>
#include <vector>

using namespace facebook::rebalancer::interface;

struct TenantConfig {
    std::string tier;
    int cpu_quota;
    int mem_quota;
    int priority;
    int workload_count;
};

int main() {
    auto executor = std::make_shared<folly::ThreadPoolExecutor>(4);
    ProblemSolver solver(executor, "multi-tenant-allocator", "production");

    solver.setObjectName("workload");
    solver.setContainerName("server");

    // Tenant configuration
    std::map<std::string, TenantConfig> tenants = {
        {"premium_tenant_a", {"premium", 100, 500, 3, 40}},
        {"premium_tenant_b", {"premium", 80, 400, 3, 30}},
        {"standard_tenant_c", {"standard", 60, 300, 2, 50}},
        {"standard_tenant_d", {"standard", 50, 250, 2, 40}},
        {"basic_tenant_e", {"basic", 30, 150, 1, 40}},
    };

    // Initial imbalanced assignment
    std::map<std::string, std::vector<std::string>> assignment;
    for (int i = 0; i < 20; i++) {
        assignment[fmt::format("server{}", i)] = {};
    }

    // Create workloads and assign initially (imbalanced)
    int workload_id = 0;
    for (const auto& [tenant, config] : tenants) {
        for (int i = 0; i < config.workload_count; i++) {
            std::string workload_name = fmt::format("{}_workload_{}", tenant, i);
            int server_idx = workload_id % 5;  // Concentrate on first 5 servers
            assignment[fmt::format("server{}", server_idx)].push_back(workload_name);
            workload_id++;
        }
    }
    solver.setAssignment(assignment);

    // Define tenant partition
    std::map<std::string, std::vector<std::string>> tenant_partition;
    for (const auto& [tenant, config] : tenants) {
        for (int i = 0; i < config.workload_count; i++) {
            tenant_partition[tenant].push_back(
                fmt::format("{}_workload_{}", tenant, i)
            );
        }
    }
    solver.addPartition("tenant", tenant_partition);

    // Workload CPU and memory usage
    std::map<std::string, double> cpu_usage, memory_usage;
    for (const auto& [tenant, config] : tenants) {
        double avg_cpu = static_cast<double>(config.cpu_quota) / config.workload_count;
        double avg_mem = static_cast<double>(config.mem_quota) / config.workload_count;

        for (int i = 0; i < config.workload_count; i++) {
            std::string workload_name = fmt::format("{}_workload_{}", tenant, i);
            cpu_usage[workload_name] = avg_cpu * (0.8 + 0.4 * (i % 5) / 5.0);
            memory_usage[workload_name] = avg_mem * (0.8 + 0.4 * (i % 5) / 5.0);
        }
    }
    solver.addObjectDimension("cpu_usage", cpu_usage);
    solver.addObjectDimension("memory_usage", memory_usage);

    // Server capacities
    std::map<std::string, double> server_cpu_capacity, server_mem_capacity;
    for (int i = 0; i < 20; i++) {
        server_cpu_capacity[fmt::format("server{}", i)] = 25.0;
        server_mem_capacity[fmt::format("server{}", i)] = 128.0;
    }
    solver.addContainerDimension("cpu_capacity", server_cpu_capacity);
    solver.addContainerDimension("memory_capacity", server_mem_capacity);

    // CONSTRAINT 1: Server CPU capacity
    CapacitySpec cpuCap;
    cpuCap.name() = "server-cpu-capacity";
    cpuCap.scope() = "server";
    cpuCap.dimension() = "cpu_usage";
    solver.addConstraint(cpuCap);

    // CONSTRAINT 2: Server memory capacity
    CapacitySpec memCap;
    memCap.name() = "server-memory-capacity";
    memCap.scope() = "server";
    memCap.dimension() = "memory_usage";
    solver.addConstraint(memCap);

    // CONSTRAINT 3: Premium tenant isolation
    std::vector<std::string> premium_tenants;
    for (const auto& [tenant, config] : tenants) {
        if (config.tier == "premium") {
            premium_tenants.push_back(tenant);
        }
    }

    if (!premium_tenants.empty()) {
        GroupCountSpec premiumIsolation;
        premiumIsolation.name() = "premium-tenant-isolation";
        premiumIsolation.scope() = "server";
        premiumIsolation.partitionName() = "tenant";
        premiumIsolation.bound() = GroupCountSpecBound::MAX;
        premiumIsolation.limit() = Limit();
        premiumIsolation.limit()->type() = LimitType::ABSOLUTE;
        premiumIsolation.limit()->globalLimit() = 1;
        solver.addConstraint(premiumIsolation);
    }

    // GOAL 1: Balance CPU
    BalanceSpec balanceCpu;
    balanceCpu.name() = "balance-cpu";
    balanceCpu.scope() = "server";
    balanceCpu.dimension() = "cpu_usage";
    balanceCpu.formula() = BalanceSpecFormula::LEGACY;
    balanceCpu.fixAverageToInitial() = true;
    solver.addGoal(balanceCpu, 1.0);

    // GOAL 2: Balance memory
    BalanceSpec balanceMem;
    balanceMem.name() = "balance-memory";
    balanceMem.scope() = "server";
    balanceMem.dimension() = "memory_usage";
    balanceMem.formula() = BalanceSpecFormula::LEGACY;
    balanceMem.fixAverageToInitial() = true;
    solver.addGoal(balanceMem, 1.0);

    // GOAL 3: Minimize movement
    MinimizeMovementSpec minMove;
    minMove.name() = "minimize-movement";
    minMove.scope() = "server";
    minMove.dimension() = "cpu_usage";
    solver.addGoal(minMove, 0.2);

    // Use Local Search solver
    LocalSearchSolverSpec localSearch;
    localSearch.timeLimitMs() = 60000;
    solver.addSolver(localSearch);

    // Solve
    auto solution = solver.solve();

    // Print results
    std::cout << "Solution found in " << solution.profile().solveTime() << "ms\n";
    std::cout << "Objective value: " << solution.objectiveValue() << "\n";
    std::cout << "Workloads moved: " << solution.profile().moveCount() << "\n";

    // Print per-tenant allocation
    std::cout << "\nPer-tenant resource allocation:\n";
    std::cout << fmt::format("{:<25} {:<10} {:<12} {:<12} {:<12} {:<12} {}\n",
        "Tenant", "Tier", "Quota CPU", "Actual CPU", "Quota Mem", "Actual Mem", "Servers");
    std::cout << std::string(110, '-') << "\n";

    for (const auto& [tenant, config] : tenants) {
        const auto& tenant_workloads = tenant_partition[tenant];
        double tenant_cpu = 0.0, tenant_mem = 0.0;

        for (const auto& w : tenant_workloads) {
            tenant_cpu += cpu_usage[w];
            tenant_mem += memory_usage[w];
        }

        // Count servers with this tenant
        std::set<std::string> servers_with_tenant;
        for (const auto& [server, workloads] : solution.assignment()) {
            for (const auto& w : workloads) {
                if (std::find(tenant_workloads.begin(), tenant_workloads.end(), w) != tenant_workloads.end()) {
                    servers_with_tenant.insert(server);
                    break;
                }
            }
        }

        std::cout << fmt::format("{:<25} {:<10} {:<12.1f} {:<12.1f} {:<12.1f} {:<12.1f} {}\n",
            tenant, config.tier, static_cast<double>(config.cpu_quota), tenant_cpu,
            static_cast<double>(config.mem_quota), tenant_mem, servers_with_tenant.size());
    }

    return 0;
}
```

</TabItem>
</Tabs>

## Explanation

### Why These Choices?

1. **Partition by tenant**: Enables tenant-level constraints and tracking
2. **GroupCountSpec for isolation**: Ensures premium tenants don't share servers
3. **CapacitySpec**: Prevents server overload
4. **BalanceSpec**: Distributes load evenly within each tenant's allocation
5. **MinimizeMovementSpec**: Reduces churn during rebalancing

### What Happens?

1. Solver identifies imbalanced initial state (all on first 5 servers)
2. Redistributes workloads across all 20 servers
3. Ensures premium tenants are isolated (max 1 per server)
4. Balances CPU and memory within each tenant's allocation
5. Respects all capacity constraints

## Variations

### Tenant-Specific Quotas (Reserved Capacity)

Reserve specific servers for high-priority tenants:

```python file=algopt/rebalancer/examples/website/cookbook/multi_tenant/reserved_capacity.py start=variation_start end=variation_end
# Reserve servers 0-5 for premium tenants
premium_workloads = []
for tenant, config in tenants.items():
    if config["tier"] == "premium":
        premium_workloads.extend(tenant_partition[tenant])

other_workloads = []
for tenant, config in tenants.items():
    if config["tier"] != "premium":
        other_workloads.extend(tenant_partition[tenant])

# Block non-premium from premium servers
solver.add_constraint(
    ConstraintSpec(
        avoidAssignmentsSpec=AvoidAssignmentsSpec(
            name="reserve-premium-servers",
            scope="server",
            assignments=[
                AvoidAssignment(
                    object=w,
                    scopeItems=[f"server{i}" for i in range(6)],
                )
                for w in other_workloads
            ],
        )
    )
)
```

### Fair Share with Excess Capacity

Distribute excess capacity proportionally to tenant priority:

```python file=algopt/rebalancer/examples/website/cookbook/multi_tenant/priority_based_excess.py start=variation_start end=variation_end
# After guaranteeing quotas, distribute excess by priority
# Use weighted balance based on tenant priority

for tenant, config in tenants.items():
    priority_weight = config["priority"]

    # Higher priority tenants get more weight in balance goal
    solver.add_goal(
        GoalSpec(
            balanceSpec=BalanceSpec(
                name=f"balance-{tenant}-cpu",
                scope="server",
                dimension="cpu_usage",
                formula="LEGACY",
            )
        ),
        weight=priority_weight,  # Higher priority = more weight
    )
```

### Tenant Colocation (Opposite of Isolation)

Colocate tenant workloads for locality:

```python file=algopt/rebalancer/examples/website/cookbook/multi_tenant/tenant_colocation.py start=variation_start end=variation_end
# Standard tenants prefer colocation (fewer servers)
for tenant, config in tenants.items():
    if config["tier"] == "standard":
        solver.add_goal(
            GoalSpec(
                colocateGroupsSpec=ColocateGroupsSpec(
                    name=f"colocate-{tenant}",
                    scope="server",
                    partitionName="tenant",
                    bound="MAX",
                    limits=Limit(type="ABSOLUTE", globalLimit=5),
                )
            ),
            weight=0.3,
        )
```

### Per-Tenant Balance Goals

Balance separately for each tenant:

```python
# Create tenant-specific scopes
for tenant in tenants:
    # Get servers with this tenant's workloads
    # Then add tenant-specific balance goals
    pass  # Implementation depends on scope creation
```

## Performance Notes

- **LocalSearchSolver**: Handles 200+ workloads easily, &lt;60s solve time
- **Partitions**: Minimal overhead for 5 tenants
- **GroupCountSpec**: Adds moderate complexity
- **Scaling**: Linear with number of tenants and workloads

## Common Issues

### Premium Isolation Conflicts with Capacity

**Problem**: Too many premium tenants for isolation constraint.

**Solution**: Ensure enough servers for 1 premium tenant each:

```python
# If 3 premium tenants, need at least 3 servers
# with sufficient capacity for each tenant's workloads
```

### Quotas Exceed Total Capacity

**Problem**: Sum of quotas > total cluster capacity.

**Solution**: Verify quotas are feasible:

```python
total_cpu_quota = sum(config["cpu_quota"] for config in tenants.values())
total_cpu_capacity = sum(server_cpu_capacity.values())

assert total_cpu_quota <= total_cpu_capacity, "Quotas exceed capacity!"
```

### Unfair Allocation

**Problem**: Excess capacity not distributed fairly.

**Solution**: Use priority weights or separate quota goals.

## Next Steps

- Handle bin packing for efficiency: [Bin Packing](bin-packing)
- Add affinity for tenant-specific hardware: [Affinity Placement](affinity-placement)
- Disaster recovery for tenant workloads: [Disaster Recovery](disaster-recovery)

## Related Goals and Constraints

- [GroupCountSpec](../reference/groups/group-count) - Tenant isolation constraints
- [BalanceSpec](../reference/balance-optimize/balance) - Balance within tenant allocations
- [CapacitySpec](../reference/capacity) - Server capacity limits
- [ColocateGroupsSpec](../reference/groups/colocate-groups) - Tenant colocation patterns
