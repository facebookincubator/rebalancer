---
sidebar_position: 8
---

# Gradual Migration and Staged Rollouts

Migrate workloads incrementally over multiple rounds to minimize risk and disruption.

## Problem Description

You have:
- **200 workloads** on **old datacenter** infrastructure
- **New datacenter** with better hardware
- **Risk constraints**: Can't move everything at once
- **Goal**: Migrate gradually while maintaining availability and balance

## Real-World Scenario

A company is migrating from an aging datacenter (DC-OLD with 30 hosts) to a new datacenter (DC-NEW with 30 hosts). Moving all 200 workloads at once is too risky. Instead, you need to:

1. Migrate in **5 rounds** over several weeks
2. Maintain **balance** in both datacenters during migration
3. Keep **critical workloads** stable until late rounds
4. Ensure **capacity** never exceeded
5. Minimize **disruption** within each round

## Problem Modeling

### Rebalancer Terms

- **Objects**: 200 workload instances
- **Containers**: 60 hosts (30 in DC-OLD, 30 in DC-NEW)
- **Scopes**:
  - `host`: Individual hosts
  - `datacenter`: DC-OLD vs DC-NEW
- **Strategy**: Multi-round solving with increasing migration targets
- **Goals per round**:
  - Move N% of workloads to DC-NEW
  - Balance load within each datacenter
  - Minimize movement of already-migrated workloads

## Complete Solution

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/gradual_migration/basic_gradual_migration.py start=solution_start end=solution_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/gradual_migration/basic_gradual_migration.cpp start=solution_start end=solution_end
```

</TabItem>
</Tabs>

## Explanation

### Why These Choices?

1. **Multi-round approach**: Reduces risk by spreading migration over time
2. **AvoidMovingSpec**: Pins already-migrated workloads (avoid ping-pong)
3. **NonAcceptingSpec**: Gradually freezes old DC in later rounds
4. **Critical workload protection**: Moves low-risk workloads first
5. **Increasing movement weight**: More stability in later rounds
6. **Balance per DC**: Keeps both datacenters balanced during migration

### What Happens?

1. **Round 1**: Move 20% of workloads to DC-NEW, pin critical workloads
2. **Round 2**: Move another 20%, pin already-migrated
3. **Round 3**: Move another 20%, freeze old DC, higher movement penalty
4. **Round 4**: Move another 20%, migrate some critical workloads
5. **Round 5**: Move final 20%, complete migration

## Variations

### Percentage-Based Targets

Set explicit percentage targets per round:

```python file=algopt/rebalancer/examples/website/cookbook/gradual_migration/percentage_based_targets.py start=variation_start end=variation_end
```

### Canary Migration

Migrate small canary group first, then proceed:

```python file=algopt/rebalancer/examples/website/cookbook/gradual_migration/canary_migration.py start=variation_start end=variation_end
```

### Rollback Capability

Maintain ability to rollback in early rounds:

```python file=algopt/rebalancer/examples/website/cookbook/gradual_migration/rollback_capability.py start=variation_start end=variation_end
```

### Workload Prioritization

Migrate by priority tiers:

```python file=algopt/rebalancer/examples/website/cookbook/gradual_migration/workload_prioritization.py start=variation_start end=variation_end
```

### Time-Based Gates

Add approval gates between rounds:

```python file=algopt/rebalancer/examples/website/cookbook/gradual_migration/time_based_gates.py start=variation_start end=variation_end
```

## Best Practices

### 1. Start Small

```python
# First round: Only 5-10% of workloads
# Validate before proceeding
```

### 2. Pin Already-Migrated

```python
from rebalancer.specs import AvoidMovingSpec, ConstraintSpec

# Avoid ping-pong between DCs
solver.add_constraint(
    ConstraintSpec(
        avoidMovingSpec=AvoidMovingSpec(
            name="pin-migrated", objects=already_migrated
        )
    )
)
```

### 3. Monitor Between Rounds

```python
# Check metrics after each round
- Error rates
- Latency
- Resource utilization
- User complaints
```

### 4. Maintain Rollback

```python
# Keep old DC functional until late rounds
# Don't decommission hosts too early
```

### 5. Gradual Freeze

```python
# Round 1-2: Both DCs accepting
# Round 3-4: Old DC non-accepting
# Round 5: Old DC draining
```

## Performance Notes

- **Multi-round overhead**: Each round is independent solve (~30s each)
- **Total time**: 5 rounds × 30s = ~2.5 minutes of solve time
- **Real-world**: Spread rounds over days/weeks for validation
- **Scaling**: Works for 1000+ workloads with linear scaling

## Common Issues

### Too Aggressive

**Problem**: Moving too many workloads per round causes instability.

**Solution**: Reduce percentage per round, add more rounds.

### Ping-Pong Movement

**Problem**: Workloads moving back and forth between DCs.

**Solution**: Pin already-migrated workloads with AvoidMovingSpec.

### Critical Workload Risk

**Problem**: Critical workloads moved too early.

**Solution**: Pin critical workloads in early rounds, migrate in final round.

### Insufficient Capacity

**Problem**: New DC can't handle all workloads.

**Solution**: Verify capacity before starting:

```python
total_cpu_needed = sum(workload_cpu.values())
new_dc_capacity = 30 * 32.0  # 30 hosts × 32 cores

assert total_cpu_needed <= new_dc_capacity, "Insufficient capacity!"
```

## Verification Checklist

After each round:
- ✅ No workload moved backward (to old DC)
- ✅ Target percentage achieved
- ✅ No capacity violations
- ✅ Critical workloads stable
- ✅ Balance maintained in both DCs
- ✅ Error rates normal

## Next Steps

- Throttle movement rate: [Throttled Rebalancing](throttled-rebalancing)
- Handle colocation during migration: [Colocation Patterns](colocation)
- Maintain diversity: [Disaster Recovery](disaster-recovery)

## Related Goals and Constraints

- [AvoidMovingSpec](../reference/movement/avoid-moving) - Pin migrated workloads
- [NonAcceptingSpec](../reference/placement/non-accepting) - Freeze old datacenter
- [MinimizeMovementSpec](../reference/movement/minimize-movement) - Reduce churn
- [BalanceSpec](../reference/balance-optimize/balance) - Maintain balance during migration
