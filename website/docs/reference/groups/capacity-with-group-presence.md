---
sidebar_position: 14
---

# CapacityWithGroupPresenceSpec

**Type**: Both (Constraint or Goal)

Capacity constraints with minimum utilization overhead when groups are present on scope items.

## Overview

`CapacityWithGroupPresenceSpec` extends capacity limiting with a **minimum presence weight**: when a group is present on a scope item (even with small utilization), it counts toward capacity by at least a specified minimum amount. This models overhead costs like setup, metadata, connections, or reservations that don't scale linearly with actual utilization.

**Key features**:
- **Minimum presence weight**: Groups contribute at least a base amount when present
- **Rounding options**: Optional ceiling rounding of group contributions
- **Multipliers**: Apply multiplicative factors to contributions (with iterative rounding)
- **Aggregation scopes**: Aggregate utilization at different granularities
- **Usage intents**: Per-scope-item or per-(group, scope-item) limits

**Use this when**: Groups have fixed overhead costs or minimum resource requirements regardless of actual size.

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/reference/groups/capacity_with_group_presence_spec_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/groups/capacity_with_group_presence_spec_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Unique identifier for this constraint |
| `scope` | string | Yes | - | Scope level (e.g., `"host"`, `"rack"`) |
| `partition` | string | Yes | - | Partition defining groups |
| `dimension` | string | Yes | - | Object dimension to constrain |
| `groupToPresenceWeight` | Limit | No | `{type: ABSOLUTE}` | Minimum utilization when group present |
| `scopeItemToLimit` | Limit | Yes | - | Capacity limits per scope item |
| `bound` | CapacityWithGroupPresenceBound | No | MAX | Type of bound (MAX/MIN) |
| `scopeItemFilter` | Filter | No | all | Apply only to filtered scope items |
| `roundUpGroupUtilOnScopeItem` | bool | No | true | Round up group contributions |
| `multiplierList` | list\<Limit\> | No | [] | Multiplicative factors for contributions |
| `aggregationScope` | string | No | scope | Scope at which to aggregate utilization |
| `groupToExtraAdditivePenalty` | Limit | No | `{globalLimit: 0}` | Extra penalty for local search |
| `groupFilter` | Filter | No | all | Apply only to filtered groups |
| `intent` | CapacityWithGroupPresenceUsageIntent | No | PER_SCOPE_ITEM | Limit granularity |
| `aggregationPartition` | string | No | partition | Partition for contribution aggregation |

### CapacityWithGroupPresenceBound Enum

| Value | Description | Formula |
|-------|-------------|---------|
| `MAX` | Maximum capacity | `Util(S) <= limit(S)` |
| `MIN` | Minimum capacity | `Util(S) >= limit(S)` |

### CapacityWithGroupPresenceUsageIntent Enum

| Value | Description | Limit Scope |
|-------|-------------|-------------|
| `PER_SCOPE_ITEM` | Limits per scope item | `limit(host1)`, `limit(host2)` |
| `PER_GROUP_AND_SCOPE_ITEM` | Limits per (group, scope item) | `limit(service_a, host1)` |

## How It Works

For each group G and scope item S, the contribution is computed as:

### Formula (1): Basic Contribution

```
G's contribution to S = max(
    groupToPresenceWeight(G, S)  if G has non-zero utilization in S,
    actual utilization of G in S
)
```

### Formula (2): Rounded Contribution (if roundUpGroupUtilOnScopeItem = true)

```
G's rounded-up contribution to S = ceil(G's contribution to S)
```

### Formula (3): With Single Multiplier

```
G's weighted contribution to S = {
    ceil(G's rounded-up contribution to S * multiplier)  if rounding enabled,
    G's contribution to S * multiplier                   if rounding disabled
}
```

### Formula (4): With Multiple Multipliers

```
G's weighted contribution to S = {
    ceil(ceil(G's contribution to S * m1) * m2)  if rounding enabled,
    G's contribution to S * m1 * m2              if rounding disabled
}
```

**Final constraint**: `sum_G (G's weighted contribution to S) <= limit(S)`

## Common Usage Patterns

### 1. Service Overhead Costs

Model fixed overhead per service:

```python
# Each service has 5 GB metadata/connection overhead
services = {
    "web_service": ["web_0", "web_1", "web_2"],
    "api_service": ["api_0", "api_1", "api_2"],
    "cache_service": ["cache_0", "cache_1"],
}
solver.add_partition("service", services)

solver.add_constraint(
        ConstraintSpec(
            capacityWithGroupPresenceSpec=CapacityWithGroupPresenceSpec(
        name="memory-with-service-overhead",
        scope="host",
        partition="service",
        dimension="memory",
        groupToPresenceWeight=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=5.0,  # 5 GB overhead per service
        ),
        scopeItemToLimit=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=128.0,  # 128 GB per host
        ),
    )
        )
)

# Example:
# Host has web_service (2 GB actual) + api_service (3 GB actual)
# Contribution = max(5, 2) + max(5, 3) = 5 + 5 = 10 GB
# Even though actual usage is only 2+3=5 GB, overhead makes it 10 GB
```

### 2. Per-Service Custom Overhead

Different overhead per service:

```python
# Premium services have higher overhead
solver.add_constraint(
        ConstraintSpec(
            capacityWithGroupPresenceSpec=CapacityWithGroupPresenceSpec(
        name="variable-service-overhead",
        scope="host",
        partition="service",
        dimension="memory",
        groupToPresenceWeight=Limit(
            type=LimitType.ABSOLUTE,
            groupLimits={
                "web_service": 10.0,     # 10 GB overhead
                "api_service": 15.0,     # 15 GB overhead
                "cache_service": 5.0,    # 5 GB overhead
            },
            globalLimit=8.0,  # Default: 8 GB
        ),
        scopeItemToLimit=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=128.0,
        ),
    )
        )
)
```

### 3. Connection Pool Overhead

Model connection pool reserves:

```python
# Databases reserve connections per client service
solver.add_constraint(
        ConstraintSpec(
            capacityWithGroupPresenceSpec=CapacityWithGroupPresenceSpec(
        name="db-connection-reserve",
        scope="db_host",
        partition="client_service",
        dimension="connection_count",
        groupToPresenceWeight=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=50.0,  # Reserve 50 connections per service
        ),
        scopeItemToLimit=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=1000.0,  # Max 1000 connections per DB
        ),
    )
        )
)
```

### 4. Rounding for Integer Resources

Round up group contributions to whole units:

```python
# Each service rounds up to whole CPU cores
solver.add_constraint(
        ConstraintSpec(
            capacityWithGroupPresenceSpec=CapacityWithGroupPresenceSpec(
        name="cpu-rounded",
        scope="host",
        partition="service",
        dimension="cpu_cores",
        groupToPresenceWeight=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=1.0,  # Min 1 core when present
        ),
        scopeItemToLimit=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=64.0,  # 64 cores per host
        ),
        roundUpGroupUtilOnScopeItem=True,  # Round up to integer cores
    )
        )
)

# Example:
# Service uses 2.3 cores → rounds up to 3 cores
# Service uses 0.1 cores → max(1, 0.1) = 1 core (presence weight)
```

### 5. Multipliers for Replica Overhead

Apply multipliers for replication:

```python
# Primary + replica replication overhead (2x multiplier)
solver.add_constraint(
        ConstraintSpec(
            capacityWithGroupPresenceSpec=CapacityWithGroupPresenceSpec(
        name="replicated-capacity",
        scope="rack",
        partition="shard",
        dimension="storage",
        groupToPresenceWeight=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=100.0,  # 100 GB min per shard
        ),
        scopeItemToLimit=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=5000.0,  # 5 TB per rack
        ),
        multiplierList=[
            Limit(
                type=LimitType.ABSOLUTE,
                globalLimit=2.0,  # 2x for replication
            ),
        ],
    )
        )
)

# Example:
# Shard uses 500 GB → max(100, 500) = 500 GB
# With 2x multiplier: ceil(500 * 2) = 1000 GB counted
```

### 6. Multi-Level Multipliers

Cascading multipliers (e.g., replication + compression):

```python
# 2x for replication, then 0.5x for compression
solver.add_constraint(
        ConstraintSpec(
            capacityWithGroupPresenceSpec=CapacityWithGroupPresenceSpec(
        name="multi-multiplier",
        scope="host",
        partition="service",
        dimension="storage",
        groupToPresenceWeight=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=50.0,
        ),
        scopeItemToLimit=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=1000.0,
        ),
        multiplierList=[
            Limit(globalLimit=2.0),   # Replication
            Limit(globalLimit=0.5),   # Compression
        ],
    )
        )
)

# Example:
# Service uses 400 GB
# After m1: ceil(400 * 2) = 800 GB
# After m2: ceil(800 * 0.5) = 400 GB
```

### 7. Aggregation Scope

Aggregate utilization at different scope level:

```python
# Limit per rack, but aggregate at host level
solver.add_constraint(
        ConstraintSpec(
            capacityWithGroupPresenceSpec=CapacityWithGroupPresenceSpec(
        name="rack-limit-host-aggregation",
        scope="rack",
        partition="service",
        dimension="memory",
        aggregationScope="host",  # Aggregate at host level first
        groupToPresenceWeight=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=10.0,  # 10 GB overhead per service per host
        ),
        scopeItemToLimit=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=500.0,  # 500 GB per rack
        ),
    )
        )
)

# Util(rack) = sum_{host in rack} sum_{service} max(10, service_util_on_host)
```

### 8. Per-Group-and-Scope-Item Limits

Different limits per (group, scope item) combination:

```python
# Different service limits on different hosts
solver.add_constraint(
        ConstraintSpec(
            capacityWithGroupPresenceSpec=CapacityWithGroupPresenceSpec(
        name="per-service-per-host-limit",
        scope="host",
        partition="service",
        dimension="cpu",
        intent=CapacityWithGroupPresenceUsageIntent.PER_GROUP_AND_SCOPE_ITEM,
        groupToPresenceWeight=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=1.0,
        ),
        scopeItemToLimit=Limit(
            type=LimitType.ABSOLUTE,
            # Limits per (service, host)
            perGroupAndScopeItemLimits={
                ("web_service", "host1"): 20.0,
                ("web_service", "host2"): 30.0,
                ("api_service", "host1"): 40.0,
            },
            globalLimit=50.0,  # Default
        ),
    )
        )
)
```

## Rounding Behavior

### With roundUpGroupUtilOnScopeItem = true

```
Service uses 2.3 GB, presence weight = 5 GB
→ max(5, 2.3) = 5 GB
→ ceil(5) = 5 GB

Service uses 7.2 GB, presence weight = 5 GB
→ max(5, 7.2) = 7.2 GB
→ ceil(7.2) = 8 GB
```

### With roundUpGroupUtilOnScopeItem = false

```
Service uses 2.3 GB, presence weight = 5 GB
→ max(5, 2.3) = 5 GB (no rounding)

Service uses 7.2 GB, presence weight = 5 GB
→ max(5, 7.2) = 7.2 GB (no rounding)
```

## Performance Considerations

- **Impact**: Moderate to High - complex formula with rounding and multipliers
- **Solver compatibility**: Works with both Local Search and Optimal solvers
- **Scaling**: O(groups × scope items)
- **Rounding**: Adds non-linearity, may slow optimal solver
- **Multipliers**: Each multiplier adds overhead

## Common Pitfalls

### 1. Presence Weight Too High

**Problem**: Presence weight higher than capacity, making problem infeasible.

```python
# BAD: Presence weight 200 GB, but capacity only 100 GB
solver.add_constraint(
        ConstraintSpec(
            capacityWithGroupPresenceSpec=CapacityWithGroupPresenceSpec(
        groupToPresenceWeight=Limit(globalLimit=200.0),  # Too high!
        scopeItemToLimit=Limit(globalLimit=100.0),
    )
        )
)
# Can't place even one service on any host!
```

**Solution**: Ensure presence weight < capacity:

```python
# GOOD: Presence weight fits within capacity
solver.add_constraint(
        ConstraintSpec(
            capacityWithGroupPresenceSpec=CapacityWithGroupPresenceSpec(
        groupToPresenceWeight=Limit(globalLimit=10.0),  # Reasonable
        scopeItemToLimit=Limit(globalLimit=100.0),
    )
        )
)
```

### 2. Partition Not Defined

**Problem**: Partition doesn't exist.

```python
# BAD: Partition not defined
solver.add_constraint(
    CapacityWithGroupPresenceSpec(
        partition="service",  # Error: doesn't exist!
    )
)
```

**Solution**: Define partition first:

```python
# GOOD: Define partition
solver.add_partition("service", service_groups)

solver.add_constraint(
        ConstraintSpec(
            capacityWithGroupPresenceSpec=CapacityWithGroupPresenceSpec(
        partition="service",
    )
        )
)
```

### 3. Multipliers Not Understood

**Problem**: Confused about multiplier application.

```python
# CONFUSION: What does multiplier=2 mean?
# Does it double capacity or double utilization?
```

**Answer**: Multiplier multiplies **group contribution** (not capacity limit):

```python
# Multiplier=2 means each service counts 2x toward capacity
# Useful for: replication overhead, safety margin, etc.
solver.add_constraint(
        ConstraintSpec(
            capacityWithGroupPresenceSpec=CapacityWithGroupPresenceSpec(
        multiplierList=[Limit(globalLimit=2.0)],  # 2x contribution
        scopeItemToLimit=Limit(globalLimit=100.0),  # Still 100 GB limit
    )
        )
)
# Service using 40 GB → counts as 80 GB toward limit
```

### 4. Intent Confusion

**Problem**: Not understanding PER_SCOPE_ITEM vs PER_GROUP_AND_SCOPE_ITEM.

```python
# PER_SCOPE_ITEM (default): One limit per scope item
#   limit(host1) = 100 GB total for ALL services
#   limit(host2) = 100 GB total for ALL services

# PER_GROUP_AND_SCOPE_ITEM: Separate limit per (service, host)
#   limit(service_a, host1) = 50 GB
#   limit(service_b, host1) = 50 GB
#   → Host1 can have 100 GB total (50+50)
```

**Use PER_SCOPE_ITEM** for overall capacity.
**Use PER_GROUP_AND_SCOPE_ITEM** for per-group quotas.

### 5. Rounding Causing Unexpected Behavior

**Problem**: Rounding up makes problem infeasible.

```python
# BAD: Rounding up with tight capacity
# 10 services each using 9.5 GB → ceil(9.5) = 10 GB each = 100 GB total
# But capacity is 100 GB, barely fits
# Add one more small service (0.1 GB) → ceil(0.1) = 1 GB
# Now total = 101 GB > 100 GB capacity (infeasible!)
```

**Solution**: Account for rounding overhead:

```python
# GOOD: Add margin for rounding
num_services = 10
capacity = 100.0
margin = num_services * 1.0  # Up to 1 GB rounding per service

solver.add_constraint(
        ConstraintSpec(
            capacityWithGroupPresenceSpec=CapacityWithGroupPresenceSpec(
        scopeItemToLimit=Limit(globalLimit=capacity + margin),
    )
        )
)
```

## Combining with Other Specs

### With BalanceSpec

Capacity with overhead + balance:

```python
# Hard: Capacity with overhead
solver.add_constraint(
        ConstraintSpec(
            capacityWithGroupPresenceSpec=CapacityWithGroupPresenceSpec(
        name="capacity-overhead",
        scope="host",
        partition="service",
        dimension="memory",
        groupToPresenceWeight=Limit(globalLimit=10.0),
        scopeItemToLimit=Limit(globalLimit=128.0),
    )
        )
)

# Soft: Balance load
solver.add_goal(
        GoalSpec(
            balanceSpec=BalanceSpec(
        name="balance-memory",
        scope="host",
        dimension="memory",
    )
        ),
    weight=1.0
)
```

### With GroupCountSpec

Limit overhead sources + capacity:

```python
# Limit number of services per host (to limit overhead)
solver.add_constraint(
        ConstraintSpec(
            groupCountSpec=GroupCountSpec(
        name="max-services-per-host",
        scope="host",
        partitionName="service",
        bound=GroupCountSpecBound.MAX,
        limit=Limit(globalLimit=5),  # Max 5 services
    )
        )
)

# Capacity with per-service overhead
solver.add_constraint(
        ConstraintSpec(
            capacityWithGroupPresenceSpec=CapacityWithGroupPresenceSpec(
        name="capacity-with-overhead",
        partition="service",
        groupToPresenceWeight=Limit(globalLimit=10.0),  # 10 GB per service
        scopeItemToLimit=Limit(globalLimit=100.0),
    )
        )
)
```

## Verification Example

Verify capacity with presence weights:

```python
def verify_capacity_with_presence(
    solution,
    partition,
    presence_weights,
    capacity_limits,
    round_up=True,
):
    """Verify capacity with group presence constraints.

    Args:
        solution: Solver solution
        partition: Partition dict (group -> objects)
        presence_weights: Presence weight per group (dict)
        capacity_limits: Capacity per scope item (dict)
        round_up: Whether to round up contributions

    Returns:
        True if all limits respected
    """
    violations = []

    # Invert partition
    obj_to_group = {}
    for group, objects in partition.items():
        for obj in objects:
            obj_to_group[obj] = group

    for scope_item, objects in solution["assignment"].items():
        # Calculate contribution per group
        group_contributions = {}
        for obj in objects:
            group = obj_to_group.get(obj)
            if group:
                # Simplified: assume uniform object size
                actual_util = 1.0
                group_contributions[group] = \
                    group_contributions.get(group, 0) + actual_util

        # Apply presence weight and rounding
        total_util = 0.0
        for group, actual_util in group_contributions.items():
            presence_weight = presence_weights.get(group, 0)
            contribution = max(presence_weight, actual_util)

            if round_up:
                contribution = ceil(contribution)

            total_util += contribution

        # Check limit
        limit = capacity_limits.get(scope_item, float('inf'))
        if total_util > limit:
            violations.append({
                'scope_item': scope_item,
                'utilization': total_util,
                'limit': limit,
                'excess': total_util - limit,
            })

    if not violations:
        print(f"✅ All capacity limits respected (with presence weights)")
        return True
    else:
        print(f"❌ {len(violations)} scope items exceed capacity:")
        for v in violations[:5]:
            print(f"   {v['scope_item']}: {v['utilization']:.1f} > "
                  f"{v['limit']:.1f} (excess: {v['excess']:.1f})")
        return False

from math import ceil
```

## Overhead Analysis

Analyze overhead impact:

```python
def analyze_overhead_impact(solution, partition, presence_weights):
    """Analyze how much overhead presence weights add.

    Args:
        solution: Solver solution
        partition: Partition dict
        presence_weights: Presence weight per group
    """
    # Invert partition
    obj_to_group = {}
    for group, objects in partition.items():
        for obj in objects:
            obj_to_group[obj] = group

    total_actual = 0
    total_with_overhead = 0

    for scope_item, objects in solution["assignment"].items():
        group_contributions = {}
        for obj in objects:
            group = obj_to_group.get(obj)
            if group:
                actual_util = 1.0  # Simplified
                group_contributions[group] = \
                    group_contributions.get(group, 0) + actual_util

        scope_actual = sum(group_contributions.values())
        scope_with_overhead = 0

        for group, actual_util in group_contributions.items():
            presence_weight = presence_weights.get(group, 0)
            contribution = max(presence_weight, actual_util)
            scope_with_overhead += contribution

        total_actual += scope_actual
        total_with_overhead += scope_with_overhead

    overhead = total_with_overhead - total_actual
    overhead_pct = (overhead / total_with_overhead * 100) if total_with_overhead > 0 else 0

    print(f"Overhead analysis:")
    print(f"  Actual utilization: {total_actual:.1f}")
    print(f"  With overhead: {total_with_overhead:.1f}")
    print(f"  Overhead: {overhead:.1f} ({overhead_pct:.1f}%)")
```

## Related Specs

- [CapacitySpec](../capacity) - Basic capacity limits (without presence weights)
- [GroupCapacitySpec](group-capacity) - Group-based capacity (different model)
- [BalanceSpec](../balance) - Balance load

## Source Code

- Thrift definition: `interface/thrift/ProblemSpecs.thrift:1178` (CapacityWithGroupPresenceBound enum)
- Thrift definition: `interface/thrift/ProblemSpecs.thrift:1183` (CapacityWithGroupPresenceUsageIntent enum)
- Thrift definition: `interface/thrift/ProblemSpecs.thrift:1224` (CapacityWithGroupPresenceSpec struct)
- Implementation: `solver/constraints/CapacityWithGroupPresence.cpp`
