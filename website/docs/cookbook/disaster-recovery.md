---
sidebar_position: 3
---

# Disaster Recovery Placement

Place replicated data with diversity constraints to ensure fault tolerance.

## Problem Description

You have:
- **100 database shards**, each with **3 replicas** (300 objects total)
- **30 servers** across **6 racks** in **2 datacenters**
- **Goal**: Place replicas to survive rack and datacenter failures

## Real-World Scenario

A critical database stores user data across 100 shards. Each shard has 3 replicas for durability. The infrastructure spans 2 datacenters (DC1 and DC2), each with 3 racks, each rack with 5 servers. You need to:

1. Ensure no two replicas of the same shard on the same rack (survive rack failure)
2. Ensure each shard has at least 1 replica in each datacenter (survive DC failure)
3. Balance load across servers
4. Respect server capacity limits

## Problem Modeling

### Rebalancer Terms

- **Objects**: 300 replica instances (100 shards × 3 replicas)
- **Containers**: 30 servers
- **Partitions**:
  - `shard`: Groups replicas by shard (100 groups, 3 objects each)
- **Scopes**:
  - `server`: Individual servers (30 items)
  - `rack`: Rack grouping (6 racks)
  - `datacenter`: DC grouping (2 DCs)
- **Constraints**:
  - Max 1 replica per shard per rack
  - Each shard in at least 2 datacenters
  - Server capacity limits
- **Goals**:
  - Balance replica count across servers
  - Balance storage across servers

## Complete Solution

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/disaster_recovery/basic_disaster_recovery.py start=solution_start end=solution_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/disaster_recovery/basic_disaster_recovery.cpp start=solution_start end=solution_end
```

</TabItem>
</Tabs>

## Explanation

### Why These Choices?

1. **GroupCountSpec MAX for rack diversity**: Hard constraint prevents rack failure from losing data
2. **GroupCountSpec MIN for DC diversity**: Ensures datacenter failure doesn't lose access to shards
3. **DURING definition for rack**: Maintains diversity even during migration (critical!)
4. **CapacitySpec**: Prevents overloading servers
5. **BalanceSpec for both count and storage**: Even distribution across all servers
6. **LocalSearchSolver**: Handles 300 objects efficiently

### What Happens?

1. Solver identifies diversity violations in initial assignment
2. Redistributes replicas to satisfy rack and DC diversity
3. Balances load evenly across all 30 servers
4. Respects capacity limits
5. Ensures no rack/DC failure loses data

## Variations

### Stricter DC Diversity

Require ALL replicas in different DCs:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/disaster_recovery/stricter_dc_diversity.py start=variation_start end=variation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/disaster_recovery/stricter_dc_diversity.cpp start=variation_start end=variation_end
```

</TabItem>
</Tabs>

### Prefer Keeping Replicas Nearby

Balance at rack level to keep replicas close:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/disaster_recovery/prefer_rack_locality.py start=variation_start end=variation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/disaster_recovery/prefer_rack_locality.cpp start=variation_start end=variation_end
```

</TabItem>
</Tabs>

### Server-Level Diversity

Also ensure max 1 replica per shard per server:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/disaster_recovery/server_level_diversity.py start=variation_start end=variation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/disaster_recovery/server_level_diversity.cpp start=variation_start end=variation_end
```

</TabItem>
</Tabs>

### Proportional DC Placement

Place replicas proportionally to DC capacity:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/disaster_recovery/proportional_dc_placement.py start=variation_start end=variation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/disaster_recovery/proportional_dc_placement.cpp start=variation_start end=variation_end
```

</TabItem>
</Tabs>


## Performance Notes

- **LocalSearchSolver**: Handles 1000+ replicas easily, &lt;60s solve time
- **OptimalSolver**: Use for &lt;100 replicas for provably optimal placement
- **Memory**: Each replica ~100 bytes, servers ~50 bytes
- **Scaling**: Diversity constraints add moderate complexity

## Common Issues

### Impossible DC Diversity

**Problem**: Not enough datacenters for MIN constraint.

**Solution**: Ensure MIN limit ≤ number of datacenters:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
# Can't require 3 DCs when only 2 exist
# Use MIN=2 or add more DCs
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Can't require 3 DCs when only 2 exist
// Use MIN=2 or add more DCs
```

</TabItem>
</Tabs>

### Rack Imbalance

**Problem**: All replicas fit on one rack set, leaving others empty.

**Solution**: Add rack-level balance goal:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/disaster_recovery/rack_balance.py start=variation_start end=variation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/disaster_recovery/rack_balance.cpp start=variation_start end=variation_end
```

</TabItem>
</Tabs>


### Capacity Violations

**Problem**: Diversity constraints force capacity violations.

**Solution**: Add more capacity or relax diversity constraints to goals instead of constraints.

## Next Steps

- Handle multi-tenant scenarios: [Multi-Tenant Allocation](multi-tenant)
- Minimize disruption during migration: [MinimizeMovementSpec](../reference/movement/minimize-movement)
- Colocate related objects: [Colocation Patterns](colocation)

## Related Goals and Constraints

- [GroupCountSpec](../reference/groups/group-count) - Control group presence per scope item
- [ColocateGroupsSpec](../reference/groups/colocate-groups) - Control group spread
- [BalanceSpec](../reference/balance-optimize/balance) - Balance resource utilization
- [CapacitySpec](../reference/capacity) - Enforce capacity limits
