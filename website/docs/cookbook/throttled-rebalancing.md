---
sidebar_position: 10
---

# Throttled Rebalancing

Limit the rate and volume of object movement during rebalancing to avoid overwhelming production systems.

## Problem Description

You have:
- **500 shards** that need rebalancing
- **Production system** that can't handle moving all shards at once
- **Network bandwidth** and **system load** constraints
- **Goal**: Gradually rebalance without disrupting service

## Real-World Scenario

A database cluster has 500 shards distributed across 50 hosts. The distribution is imbalanced (some hosts at 90% capacity, others at 30%). You need to rebalance, but:

1. **Moving data consumes bandwidth** - moving all 500 shards would saturate the network
2. **Moves impact performance** - each shard migration causes CPU/IO load
3. **Risk of cascading failures** - too many concurrent moves could destabilize the cluster
4. **Production constraints** - can only move shards during low-traffic periods

Solution: Rebalance gradually over multiple rounds with strict move limits.

## Problem Modeling

### Rebalancer Terms

- **Objects**: 500 database shards
- **Containers**: 50 hosts
- **Dimensions**:
  - `data_size`: Size of each shard in GB
  - `query_rate`: Queries per second for each shard
  - `host_capacity`: Max storage per host
- **Constraints**:
  - Maximum moves per round (throttle)
  - Capacity limits
  - Pin recently moved shards (stability)
- **Goals**:
  - Balance load across hosts
  - Minimize total movement
  - Prefer small shard moves (less disruptive)

## Complete Solution

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/throttled_rebalancing/basic_throttled_rebalancing.py start=solution_start end=solution_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/throttled_rebalancing/basic_throttled_rebalancing.cpp start=solution_start end=solution_end
```

</TabItem>
</Tabs>

## Explanation

### Why These Choices?

1. **AvoidMovingSpec for recently moved**: Prevents ping-pong (moving same shard back and forth)
2. **MinimizeMovementSpec with allowance**: Allows limited moves without penalty, hard throttle beyond allowance
3. **Multi-round approach**: Distributes moves over time to avoid overwhelming system
4. **Dimension-based movement cost**: Moving large shards costs more than small shards (implicit throttling)
5. **Balance as goals not constraints**: Allows partial progress each round

### What Happens?

1. **Round 1**: Solver identifies worst imbalances, moves up to 50 shards
2. **Round 2**: Previously moved shards pinned, solver finds next-best moves
3. **Rounds 3-10**: Gradual convergence toward balanced state
4. **Each round**: Respects move limit, improves balance incrementally
5. **Termination**: Stops when no more beneficial moves within constraints

## Variations

### Time-Based Throttling

Limit moves per time period instead of per round:

```python file=algopt/rebalancer/examples/website/cookbook/throttled_rebalancing/time_based_throttling.py start=variation_start end=variation_end
```

### Bandwidth-Aware Throttling

Limit moves based on network bandwidth:

```python file=algopt/rebalancer/examples/website/cookbook/throttled_rebalancing/bandwidth_aware_throttling.py start=variation_start end=variation_end
```

### Priority-Based Movement

Move critical shards first, then others:

```python file=algopt/rebalancer/examples/website/cookbook/throttled_rebalancing/priority_based_movement.py start=variation_start end=variation_end
```

### Gradual Constraint Relaxation

Start with strict movement limits, gradually relax:

```python file=algopt/rebalancer/examples/website/cookbook/throttled_rebalancing/gradual_constraint_relaxation.py start=variation_start end=variation_end
```

### Production-Hours Aware

Only rebalance during low-traffic periods:

```python file=algopt/rebalancer/examples/website/cookbook/throttled_rebalancing/production_hours_aware.py start=variation_start end=variation_end
```

### Move Velocity Tracking

Track and limit move velocity over time:

```python file=algopt/rebalancer/examples/website/cookbook/throttled_rebalancing/move_velocity_tracking.py start=variation_start end=variation_end
```

## Performance Notes

- **AvoidMovingSpec**: Minimal overhead, O(pinned object count)
- **MinimizeMovementSpec**: Low overhead, helps solver converge faster
- **Multi-round solving**: Each round is independent, can be parallelized with analysis
- **Scaling**: Linear with move limit, not total object count

## Common Issues

### Moves Exceed Limit

**Problem**: Solver moves more shards than max_moves parameter.

**Solution**: MinimizeMovementSpec is a soft goal, not a hard constraint. Use tighter allowance:

```python
from rebalancer.specs import GoalSpec, MinimizeMovementSpec

# Stricter throttling
solver.add_goal(
    GoalSpec(
        minimizeMovementSpec=MinimizeMovementSpec(
            name="strict-limit",
            scope="host",
            dimension="data_size",
            allowance=max_moves * 20.0,  # Reduced allowance
        )
    ),
    weight=5.0,  # Higher weight
)
```

### Ping-Pong Behavior

**Problem**: Same shards move back and forth across rounds.

**Solution**: Always pin previously moved shards:

```python
from rebalancer.specs import AvoidMovingSpec, ConstraintSpec

# Track ALL previously moved shards across all rounds
all_moved_shards = set()

for round_num in range(TOTAL_ROUNDS):
    solver.add_constraint(
        ConstraintSpec(
            avoidMovingSpec=AvoidMovingSpec(
                name="pin-all-previously-moved",
                objects=list(all_moved_shards),
            )
        )
    )

    # ... solve and update all_moved_shards
```

### Slow Convergence

**Problem**: Too many rounds needed to reach balance.

**Solution**: Increase move allowance or reduce balance weight:

```python
from rebalancer.specs import GoalSpec, MinimizeMovementSpec

# Allow more aggressive moves
solver.add_goal(
    GoalSpec(
        minimizeMovementSpec=MinimizeMovementSpec(
            name="loose-throttle",
            scope="host",
            dimension="data_size",
            allowance=max_moves * 100.0,  # Higher allowance
        )
    ),
    weight=0.5,  # Lower weight (less resistance to movement)
)
```

### Capacity Violations

**Problem**: Throttling prevents fixing capacity violations.

**Solution**: Use CapacitySpec as constraint, prioritize fixing violations:

```python
from rebalancer.specs import CapacitySpec, ConstraintSpec

# Capacity is hard constraint
solver.add_constraint(
    ConstraintSpec(
        capacitySpec=CapacitySpec(
            name="capacity", scope="host", dimension="data_size"
        )
    )
)

# Allow more moves to fix violations
if has_capacity_violations(current_assignment):
    max_moves_this_round = max_moves * 2  # Temporarily increase
```

## Verification Example

Verify move limits are respected:

```python file=algopt/rebalancer/examples/website/cookbook/throttled_rebalancing/verification_example.py start=variation_start end=variation_end
```

## Progress Tracking

Track rebalancing progress over rounds:

```python file=algopt/rebalancer/examples/website/cookbook/throttled_rebalancing/progress_tracking.py start=variation_start end=variation_end
```

## Next Steps

- Gradual datacenter migration: [Gradual Migration](gradual-migration)
- Full movement control: [MinimizeMovementSpec](../reference/movement/minimize-movement)
- Handle production constraints: [Affinity Placement](affinity-placement)

## Related Goals and Constraints

- [MinimizeMovementSpec](../reference/movement/minimize-movement) - Control movement volume
- [AvoidMovingSpec](../reference/movement/avoid-moving) - Pin specific objects
- [BalanceSpec](../reference/balance-optimize/balance) - Balance goals driving movement
- [CapacitySpec](../reference/capacity) - Capacity constraints
