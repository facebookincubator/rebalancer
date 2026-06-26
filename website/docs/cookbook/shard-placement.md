---
sidebar_position: 2
---

# Database Shard Placement

Place database shards across servers with capacity constraints and balance goals.

## Problem Description

You have:
- **50 database shards** of varying sizes
- **10 servers** with different storage capacities
- Shards have different **storage** and **IOPS** requirements
- **Goal**: Place shards to balance load while respecting capacity limits

## Real-World Scenario

A distributed database has 50 shards storing user data. Shards vary in size (10GB to 500GB) and query load (100 to 5000 IOPS). Servers have different capacities (2TB to 8TB storage, max 50K IOPS). You need to redistribute shards to:
1. Not exceed any server's storage or IOPS capacity
2. Balance storage usage across servers
3. Balance IOPS across servers
4. Minimize data movement during rebalancing

## Problem Modeling

### Rebalancer Terms

- **Objects**: The 50 database shards
- **Containers**: The 10 servers
- **Dimensions**:
  - `storage_gb` (object): Storage used by each shard
  - `iops` (object): IOPS consumed by each shard
  - `storage_capacity_gb` (container): Max storage per server
  - `iops_capacity` (container): Max IOPS per server
- **Constraints**:
  - CapacitySpec for storage
  - CapacitySpec for IOPS
- **Goals**:
  - Balance storage across servers
  - Balance IOPS across servers
  - Minimize data movement

## Complete Solution

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/shard_placement/basic_shard_placement.py start=solution_start end=solution_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/shard_placement/basic_shard_placement.cpp start=solution_start end=solution_end
```

</TabItem>
</Tabs>

## Explanation

### Why These Choices?

1. **CapacitySpec for both dimensions**: Hard constraints prevent overloading servers
2. **BalanceSpec with LEGACY formula**: Balances both storage and IOPS evenly
3. **fixAverageToInitial=True**: Don't require adding capacity during rebalancing
4. **MinimizeMovementSpec with low weight**: Prefer stability but allow moves for balance
5. **Weight by storage_gb**: Large shard moves are more expensive than small ones
6. **LocalSearchSolver**: Handles 50 shards easily, finds good solutions quickly

### What Happens?

1. Solver identifies capacity violations (server0-2 may be over capacity)
2. Moves shards from overloaded servers to empty ones
3. Balances both storage and IOPS across all servers
4. Minimizes movement by preferring small shard moves over large ones
5. Respects capacity limits at all times

## Variations

### Add Rack Diversity

Ensure shards spread across racks for fault tolerance:

```python file=algopt/rebalancer/examples/website/cookbook/shard_placement/with_rack_diversity.py start=variation_start end=variation_end
```

### Pin Critical Shards

Prevent moving shards with active operations:

```python file=algopt/rebalancer/examples/website/cookbook/shard_placement/pin_critical_shards.py start=variation_start end=variation_end
```

### Limit Move Count

Only move a few shards per round:

```python file=algopt/rebalancer/examples/website/cookbook/shard_placement/limit_move_count.py start=variation_start end=variation_end
```

### Reserve Emergency Capacity

Keep some capacity free for emergency growth:

```python file=algopt/rebalancer/examples/website/cookbook/shard_placement/reserve_emergency_capacity.py start=variation_start end=variation_end
```

### Use Optimal Solver for Small Problems

For &lt;20 shards, get provably optimal solution:

```python file=algopt/rebalancer/examples/website/cookbook/shard_placement/use_optimal_solver.py start=variation_start end=variation_end
```

## Performance Notes

- **LocalSearchSolver**: Handles 100+ shards easily, &lt;30s solve time
- **OptimalSolver**: Use for &lt;30 shards for provably best solution
- **Memory**: Each shard ~100 bytes, servers ~50 bytes
- **Scaling**: Problem scales linearly with number of shards

## Common Issues

### Capacity Violations

**Problem**: Initial assignment violates capacity.

**Solution**: Ensure current assignment is feasible, or use soft capacity goals instead of constraints.

### Conflicting Goals

**Problem**: Can't balance both storage and IOPS simultaneously.

**Solution**: Adjust weights or use tuple optimization to prioritize one dimension.

### Too Much Movement

**Problem**: Solution moves most shards.

**Solution**: Increase MinimizeMovementSpec weight, or use AvoidMovingSpec to pin shards.

## Next Steps

- Handle replica groups with diversity: [Disaster Recovery Placement](disaster-recovery)
- Minimize shard movement during rebalancing: [MinimizeMovementSpec](../reference/movement/minimize-movement)
- Balance multiple objectives with tuples: [Multi-Objective Optimization](multi-objective)

## Related Goals and Constraints

- [CapacitySpec](../reference/capacity) - Enforce capacity limits
- [BalanceSpec](../reference/balance) - Balance resource utilization
- [MinimizeMovementSpec](../reference/movement/minimize-movement) - Minimize data movement
- [AvoidMovingSpec](../reference/movement/avoid-moving) - Pin specific shards
