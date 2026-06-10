---
sidebar_position: 6
---

# Bin Packing and Consolidation

Pack objects efficiently to minimize the number of containers used.

## Problem Description

You have:

- **100 virtual machines** of varying sizes
- **30 physical hosts** with different capacities
- **Goal**: Consolidate VMs onto fewest hosts possible to save costs

## Real-World Scenario

A cloud provider has 100 VMs running across 30 hosts. Many hosts are lightly loaded (&lt;30% utilization). To reduce operational costs and improve efficiency, you want to:

1. Pack VMs onto as few hosts as possible
2. Free up hosts for decommissioning or maintenance
3. Respect capacity constraints
4. Maintain reasonable balance (don't overload packed hosts)

This is the classic **bin packing** problem - fitting items into the minimum number of bins.

## Problem Modeling

### Rebalancer Terms

- **Objects**: 100 virtual machines
- **Containers**: 30 physical hosts
- **Dimensions**:
  - `cpu_usage`: CPU consumed by each VM
  - `memory_usage`: Memory consumed by each VM
  - `cpu_capacity`: Max CPU per host
  - `memory_capacity`: Max memory per host
- **Goal**: Minimize number of hosts used
- **Constraints**: Capacity limits

## Complete Solution

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/bin_packing/basic_bin_packing.py start=solution_start end=solution_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/bin_packing/basic_bin_packing.cpp start=solution_start end=solution_end
```

</TabItem>
</Tabs>

## Explanation

### Why These Choices?

1. **MinimizeContainersSpec**: Core goal - reduce number of used hosts
2. **High weight (10.0)**: Consolidation is primary objective
3. **BalanceSpec secondary**: Prevents overloading packed hosts
4. **CapacitySpec**: Hard constraints prevent capacity violations
5. **LocalSearchSolver**: Efficient for bin packing problems

### What Happens?

1. Solver identifies thinly spread initial state (VMs across all 30 hosts)
2. Consolidates VMs onto fewer hosts by packing efficiently
3. Balances load among used hosts to avoid overloading
4. Respects capacity constraints
5. Frees up hosts for decommissioning

## Variations

### Explicitly Drain Target Hosts

Combine with ToFreeSpec to target specific hosts for freeing:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
from rebalancer.specs import (
    ConstraintSpec,
    GoalSpec,
    MinimizeContainersSpec,
    ToFreeSpec,
)

# Identify target hosts to free (oldest hardware)
target_free_hosts = ["host25", "host26", "host27", "host28", "host29"]

# Explicitly drain these hosts
solver.add_constraint(
    ConstraintSpec(
        toFreeSpec=ToFreeSpec(
            name="drain-old-hosts",
            containers=target_free_hosts,
        )
    )
)

# Still minimize total hosts used
solver.add_goal(
    GoalSpec(
        minimizeContainersSpec=MinimizeContainersSpec(
            name="minimize-hosts",
            scope="host",
            dimension="count",
        )
    ),
    weight=5.0,
)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Identify target hosts to free (oldest hardware)
std::vector<std::string> target_free_hosts = {
    "host25", "host26", "host27", "host28", "host29"
};

// Explicitly drain these hosts
ToFreeSpec drainHosts;
drainHosts.name() = "drain-old-hosts";
drainHosts.containers() = target_free_hosts;
solver.addConstraint(drainHosts);

// Still minimize total hosts used
MinimizeContainersSpec minimizeHosts;
minimizeHosts.name() = "minimize-hosts";
minimizeHosts.scope() = "host";
minimizeHosts.dimension() = "count";
solver.addGoal(minimizeHosts, 5.0);
```

</TabItem>
</Tabs>

### Weighted Host Costs

Prefer freeing expensive or problematic hosts:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
from rebalancer.specs import GoalSpec, MinimizeContainersSpec

# Higher cost = prefer to free this host
host_costs = {
    "old_host_1": 10.0,  # Expensive, prefer to free
    "old_host_2": 10.0,
    "standard_host": 1.0,  # Normal cost
}

solver.add_goal(
    GoalSpec(
        minimizeContainersSpec=MinimizeContainersSpec(
            name="minimize-hosts",
            scope="host",
            dimension="count",
            containerCosts=host_costs,  # Prefer freeing expensive hosts
        )
    ),
    weight=10.0,
)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Higher cost = prefer to free this host
std::map<std::string, double> host_costs = {
    {"old_host_1", 10.0},  // Expensive, prefer to free
    {"old_host_2", 10.0},
    {"standard_host", 1.0},  // Normal cost
};

MinimizeContainersSpec minimizeHosts;
minimizeHosts.name() = "minimize-hosts";
minimizeHosts.scope() = "host";
minimizeHosts.dimension() = "count";
minimizeHosts.containerCosts() = host_costs;  // Prefer freeing expensive hosts
solver.addGoal(minimizeHosts, 10.0);
```

</TabItem>
</Tabs>

### Maximum Free Limit

Limit how many hosts can be freed (safety):

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
from rebalancer.specs import GoalSpec, MinimizeContainersSpec

# Free at most 10 hosts
solver.add_goal(
    GoalSpec(
        minimizeContainersSpec=MinimizeContainersSpec(
            name="minimize-hosts",
            scope="host",
            dimension="count",
            maxFreeLimit=10,  # Safety limit
        )
    ),
    weight=10.0,
)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Free at most 10 hosts
MinimizeContainersSpec minimizeHosts;
minimizeHosts.name() = "minimize-hosts";
minimizeHosts.scope() = "host";
minimizeHosts.dimension() = "count";
minimizeHosts.maxFreeLimit() = 10;  // Safety limit
solver.addGoal(minimizeHosts, 10.0);
```

</TabItem>
</Tabs>

### Gradual Consolidation

Consolidate in multiple rounds:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
from rebalancer import ProblemSolver
from rebalancer.specs import GoalSpec, MinimizeContainersSpec


def gradual_consolidation(total_rounds=5):
    """Consolidate gradually over multiple rounds."""
    for round_num in range(total_rounds):
        solver = ProblemSolver(
            service_name="vm-consolidator", service_scope="production"
        )

        # Increase consolidation pressure each round
        weight = 2.0 * (round_num + 1)  # 2.0, 4.0, 6.0, ...

        solver.add_goal(
            GoalSpec(
                minimizeContainersSpec=MinimizeContainersSpec(
                    name="minimize-hosts",
                    scope="host",
                    dimension="count",
                )
            ),
            weight=weight,  # Increasing pressure
        )

        solution = solver.solve()
        apply_moves(solution)

        print(f"Round {round_num + 1}: Freed {count_freed_hosts(solution)} hosts")
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
void gradual_consolidation(int total_rounds = 5) {
    // Consolidate gradually over multiple rounds
    for (int round_num = 0; round_num < total_rounds; round_num++) {
        auto executor = std::make_shared<folly::ThreadPoolExecutor>(4);
        ProblemSolver solver(executor, "vm-consolidator", "production");
        // ... setup solver ...

        // Increase consolidation pressure each round
        double weight = 2.0 * (round_num + 1);  // 2.0, 4.0, 6.0, ...

        MinimizeContainersSpec minimizeHosts;
        minimizeHosts.name() = "minimize-hosts";
        minimizeHosts.scope() = "host";
        minimizeHosts.dimension() = "count";
        solver.addGoal(minimizeHosts, weight);  // Increasing pressure

        auto solution = solver.solve();
        apply_moves(solution);

        std::cout << "Round " << (round_num + 1) << ": Freed "
                  << count_freed_hosts(solution) << " hosts\n";
    }
}
```

</TabItem>
</Tabs>

### Keep Minimum Headroom

Reserve capacity for future growth:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
from rebalancer.specs import BalanceSpec, GoalSpec

# Ensure used hosts stay under 80% utilization
solver.add_goal(
    GoalSpec(
        balanceSpec=BalanceSpec(
            name="reserve-headroom",
            scope="host",
            dimension="cpu_usage",
            upperBound=0.8,  # Max 80% utilization
            boundType="RELATIVE_UTIL",
        )
    ),
    weight=2.0,
)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Ensure used hosts stay under 80% utilization
BalanceSpec reserveHeadroom;
reserveHeadroom.name() = "reserve-headroom";
reserveHeadroom.scope() = "host";
reserveHeadroom.dimension() = "cpu_usage";
reserveHeadroom.upperBound() = 0.8;  // Max 80% utilization
reserveHeadroom.boundType() = BalanceSpecBoundType::RELATIVE_UTIL;
solver.addGoal(reserveHeadroom, 2.0);
```

</TabItem>
</Tabs>

## Performance Notes

- **MinimizeContainersSpec**: Moderate overhead
- **LocalSearchSolver**: Handles 100+ VMs efficiently, &lt;60s solve time
- **OptimalSolver**: Can get true minimum for &lt;50 VMs
- **Scaling**: Problem complexity increases with VM count and diversity

## Common Issues

### Overloaded Packed Hosts

**Problem**: Consolidation creates overloaded hosts.

**Solution**: Increase balance goal weight or add upper bound:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
from rebalancer.specs import BalanceSpec, GoalSpec

# Prevent overloading
solver.add_goal(
    GoalSpec(
        balanceSpec=BalanceSpec(
            name="balance-cpu",
            scope="host",
            dimension="cpu_usage",
            upperBound=0.85,  # Max 85% util
            boundType="RELATIVE_UTIL",
        )
    ),
    weight=3.0,  # Higher weight
)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Prevent overloading
BalanceSpec balanceCpu;
balanceCpu.name() = "balance-cpu";
balanceCpu.scope() = "host";
balanceCpu.dimension() = "cpu_usage";
balanceCpu.upperBound() = 0.85;  // Max 85% util
balanceCpu.boundType() = BalanceSpecBoundType::RELATIVE_UTIL;
solver.addGoal(balanceCpu, 3.0);  // Higher weight
```

</TabItem>
</Tabs>

### Can't Free Enough Hosts

**Problem**: Capacity constraints prevent freeing target number of hosts.

**Solution**: Verify total capacity sufficient:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
# Check if consolidation is feasible
total_vm_cpu = sum(vm_cpu.values())
total_vm_mem = sum(vm_memory.values())

target_hosts = 10  # Want to use only 10 hosts

target_capacity_cpu = target_hosts * 64.0
target_capacity_mem = target_hosts * 256.0

if total_vm_cpu > target_capacity_cpu or total_vm_mem > target_capacity_mem:
    print("Warning: Target host count infeasible!")
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Check if consolidation is feasible
double total_vm_cpu = 0.0;
double total_vm_mem = 0.0;
for (const auto& [vm, cpu] : vm_cpu) {
    total_vm_cpu += cpu;
}
for (const auto& [vm, mem] : vm_memory) {
    total_vm_mem += mem;
}

int target_hosts = 10;  // Want to use only 10 hosts

double target_capacity_cpu = target_hosts * 64.0;
double target_capacity_mem = target_hosts * 256.0;

if (total_vm_cpu > target_capacity_cpu || total_vm_mem > target_capacity_mem) {
    std::cout << "Warning: Target host count infeasible!\n";
}
```

</TabItem>
</Tabs>

### Weight Balance Wrong

**Problem**: Too much consolidation weight causes poor balance, or vice versa.

**Solution**: Tune weight ratio:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
from rebalancer.specs import (
    BalanceSpec,
    GoalSpec,
    MinimizeContainersSpec,
)

minimize = GoalSpec(
    minimizeContainersSpec=MinimizeContainersSpec(
        name="minimize-hosts", scope="host", dimension="count"
    )
)
balance = GoalSpec(
    balanceSpec=BalanceSpec(
        name="balance-cpu", scope="host", dimension="cpu_usage"
    )
)

# For aggressive consolidation
solver.add_goal(minimize, weight=20.0)
solver.add_goal(balance, weight=1.0)

# For conservative consolidation (better balance)
solver.add_goal(minimize, weight=5.0)
solver.add_goal(balance, weight=2.0)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// For aggressive consolidation
solver.addGoal(minimizeContainers, 20.0);
solver.addGoal(balanceSpec, 1.0);

// For conservative consolidation (better balance)
solver.addGoal(minimizeContainers, 5.0);
solver.addGoal(balanceSpec, 2.0);
```

</TabItem>
</Tabs>

## Verification Example

Verify consolidation achieved target:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
def verify_consolidation(solution, target_host_count):
    """Verify consolidation achieved target."""
    # solution["assignment"] maps object -> container; flip it to count
    # how many distinct containers actually have any object on them.
    hosts_used = len(set(solution["assignment"].values()))

    if hosts_used <= target_host_count:
        print(f"Success: Using {hosts_used} hosts (target: <= {target_host_count})")
        return True
    else:
        print(f"Using {hosts_used} hosts (target: <= {target_host_count})")
        print(f"   Missed by {hosts_used - target_host_count} hosts")
        return False
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
bool verify_consolidation(const Solution& solution, int target_host_count) {
    // Verify consolidation achieved target
    int hosts_used = 0;
    for (const auto& [host, vms] : solution.assignment()) {
        if (!vms.empty()) {
            hosts_used++;
        }
    }

    if (hosts_used <= target_host_count) {
        std::cout << "✅ Success: Using " << hosts_used
                  << " hosts (target: ≤" << target_host_count << ")\n";
        return true;
    } else {
        std::cout << "❌ Using " << hosts_used
                  << " hosts (target: ≤" << target_host_count << ")\n";
        std::cout << "   Missed by " << (hosts_used - target_host_count)
                  << " hosts\n";
        return false;
    }
}
```

</TabItem>
</Tabs>

## Cost Savings Calculation

Estimate savings from consolidation:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
def calculate_savings(initial_hosts, final_hosts, cost_per_host_per_month):
    """Calculate cost savings from consolidation."""
    hosts_freed = initial_hosts - final_hosts
    monthly_savings = hosts_freed * cost_per_host_per_month
    annual_savings = monthly_savings * 12

    print(f"Cost savings analysis:")
    print(f"  Hosts freed: {hosts_freed}")
    print(f"  Monthly savings: ${monthly_savings:,.2f}")
    print(f"  Annual savings: ${annual_savings:,.2f}")

    return annual_savings

# Example
calculate_savings(
    initial_hosts=30,
    final_hosts=15,
    cost_per_host_per_month=500  # $500/month per host
)
# Output: $90,000/year savings
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
double calculate_savings(int initial_hosts, int final_hosts,
                        double cost_per_host_per_month) {
    // Calculate cost savings from consolidation
    int hosts_freed = initial_hosts - final_hosts;
    double monthly_savings = hosts_freed * cost_per_host_per_month;
    double annual_savings = monthly_savings * 12;

    std::cout << "Cost savings analysis:\n";
    std::cout << "  Hosts freed: " << hosts_freed << "\n";
    std::cout << fmt::format("  Monthly savings: ${:,.2f}\n", monthly_savings);
    std::cout << fmt::format("  Annual savings: ${:,.2f}\n", annual_savings);

    return annual_savings;
}

// Example
calculate_savings(
    30,   // initial_hosts
    15,   // final_hosts
    500   // $500/month per host
);
// Output: $90,000/year savings
```

</TabItem>
</Tabs>

## Next Steps

- Handle multi-tenant bin packing: [Multi-Tenant Allocation](multi-tenant)
- Add disaster recovery constraints: [Disaster Recovery](disaster-recovery)
- Minimize disruption during consolidation: [MinimizeMovementSpec](../reference/movement/minimize-movement)

## Related Goals and Constraints

- [MinimizeContainersSpec](../reference/balance-optimize/minimize-containers) - Minimize containers used
- [ToFreeSpec](../reference/to-free) - Explicitly drain hosts
- [BalanceSpec](../reference/balance-optimize/balance) - Balance among packed hosts
- [CapacitySpec](../reference/capacity) - Capacity constraints
