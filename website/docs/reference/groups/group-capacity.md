---
sidebar_position: 15
---

# GroupCapacitySpec

**Type**: Constraint or Goal

Limit the total utilization of partition groups across all containers, with support for contribution partitions and various utilization types.

## Overview

`GroupCapacitySpec` constrains how much of a partition group can exist across all containers in a scope. Unlike [CapacitySpec](../capacity) which limits per-container capacity, GroupCapacitySpec limits the **total** across all containers for each group.

**Key features**:
- Supports **contribution partitions** (higher-resolution sub-groups that contribute to main group)
- Multiple **utilization types** (LINEAR, STEP, STEP_MOD_K for bundling)
- **Temporal control** (DURING, AFTER, DURING_AND_AFTER)
- **Bound types** (MAX, MIN, EXACT)

**Use this when**: You need to limit total deployment of each group across your infrastructure, especially with hierarchical partitions or bundling requirements.

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/reference/groups/group_capacity_spec_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/groups/group_capacity_spec_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Unique identifier for this constraint |
| `scope` | string | Yes | - | Scope level (e.g., `"host"`, `"rack"`) |
| `partitionName` | string | Yes | - | Main partition defining groups |
| `contributionPartition` | string | No | partitionName | Higher-resolution partition for contributions |
| `definition` | GroupCapacitySpecDefinition | No | DURING_AND_AFTER | When to apply constraint |
| `bound` | GroupCapacitySpecBound | No | MAX | Type of bound (MAX/MIN/EXACT) |
| `limit` | Limit | No | `{type: ABSOLUTE, globalLimit: 0}` | Limits per group in partitionName |
| `contribution` | Limit | No | `{type: ABSOLUTE, globalLimit: 0}` | Contribution weights |
| `filter` | Filter | No | none | Apply only to filtered containers |
| `utilType` | GroupCapacitySpecUtilType | No | LINEAR | Utilization calculation type |
| `bundleConfig` | Limit | No | none | Bundle configuration (for STEP_MOD_K) |

### GroupCapacitySpecDefinition Enum

| Value | Description | When Applied |
|-------|-------------|--------------|
| `DURING` | Only during moves | Counts objects being moved |
| `DURING_AND_AFTER` | During and after moves | Counts all objects (default) |
| `AFTER` | Only after moves | Counts objects after move completes |

### GroupCapacitySpecBound Enum

| Value | Description | Formula |
|-------|-------------|---------|
| `MAX` | Maximum limit | `Util(G) <= limit` |
| `MIN` | Minimum limit | `Util(G) >= limit` |
| `EXACT` | Exact match | `Util(G) == limit` |

### GroupCapacitySpecUtilType Enum

| Value | Description | Formula |
|-------|-------------|---------|
| `LINEAR` | Use utilization as-is | `Util(G_i, SI)` |
| `STEP` | Presence (0 or 1) | `STEP(x) = 1 if x > 0 else 0` |
| `STEP_MOD_K` | Bundle-aware | `STEP_MOD_K(x, k) = 0 if x % k == 0 else 1` |

## How It Works

For each group `G` in `partitionName`:

```
Util(G) = sum_{G_i in C_G} sum_{SI in S} UTIL_FUNC(G_i, SI) * weight(G_i, SI)
```

Where:
- `C_G`: Contributing groups (from contributionPartition) that belong to G
- `SI`: Scope items (e.g., hosts)
- `UTIL_FUNC`: Depends on utilType (LINEAR, STEP, or STEP_MOD_K)
- `weight(G_i, SI)`: From contribution.scopeItemLimits

**Constraint**: `Util(G) <= limit(G)` (for MAX bound)

**Example**:
```
Main partition "service": {service_a: [...], service_b: [...]}
Contribution partition "instance": {
  service_a_instance_0: [obj1, obj2],
  service_a_instance_1: [obj3, obj4],
  service_b_instance_0: [obj5, obj6],
}

Util(service_a) = Util(service_a_instance_0) + Util(service_a_instance_1)
                = (presence on host1 + presence on host2 + ...) + (...)
```

## Common Usage Patterns

### 1. Basic Service Capacity Limits

Limit total deployment of each service:

```python
# Service partition
services = {
    "frontend": ["frontend_0", "frontend_1", "frontend_2"],
    "backend": ["backend_0", "backend_1", "backend_2", "backend_3"],
    "database": ["db_0", "db_1"],
}
solver.add_partition("service", services)

# Limit total deployment (max instances across all hosts)
solver.add_constraint(
        ConstraintSpec(
            groupCapacitySpec=GroupCapacitySpec(
        name="service-deployment-limits",
        scope="host",
        partitionName="service",
        bound=GroupCapacitySpecBound.MAX,
        limit=Limit(
            type=LimitType.ABSOLUTE,
            groupLimits={
                "frontend": 10,  # Max 10 instances total
                "backend": 15,   # Max 15 instances total
                "database": 5,   # Max 5 instances total
            },
        ),
        contribution=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=1.0,  # Each instance counts as 1
        ),
        utilType=GroupCapacitySpecUtilType.LINEAR,
    )
        )
)
```

### 2. Hierarchical Partitions (Contribution Partition)

Service with shards, limit total service size:

```python
# Main partition: service
services = {
    "service_a": ["shard_a_0", "shard_a_1", "shard_a_2", "..."],
    "service_b": ["shard_b_0", "shard_b_1", "..."],
}
solver.add_partition("service", services)

# Contributing partition: shards (higher resolution)
shards = {
    "shard_a_0": ["shard_a_0_replica_0", "shard_a_0_replica_1"],
    "shard_a_1": ["shard_a_1_replica_0", "shard_a_1_replica_1"],
    # ... more shards
}
solver.add_partition("shard", shards)

# Limit total service capacity based on shard contributions
solver.add_constraint(
        ConstraintSpec(
            groupCapacitySpec=GroupCapacitySpec(
        name="service-capacity-via-shards",
        scope="host",
        partitionName="service",  # Limit service
        contributionPartition="shard",  # Based on shard contributions
        bound=GroupCapacitySpecBound.MAX,
        limit=Limit(
            type=LimitType.ABSOLUTE,
            groupLimits={
                "service_a": 100.0,  # Max 100 GB
                "service_b": 200.0,  # Max 200 GB
            },
        ),
        contribution=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=10.0,  # Each shard counts as 10 GB
        ),
    )
        )
)
```

### 3. Presence-Based Limits (STEP)

Limit number of hosts each service appears on:

```python
# Count presence (not quantity)
solver.add_constraint(
        ConstraintSpec(
            groupCapacitySpec=GroupCapacitySpec(
        name="limit-service-spread",
        scope="host",
        partitionName="service",
        bound=GroupCapacitySpecBound.MAX,
        limit=Limit(
            type=LimitType.ABSOLUTE,
            groupLimits={
                "frontend": 5,  # Max 5 hosts
                "backend": 10,  # Max 10 hosts
            },
        ),
        contribution=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=1.0,
        ),
        utilType=GroupCapacitySpecUtilType.STEP,  # 0 or 1 per host
    )
        )
)

# With STEP, each host counts as 1 if service present (regardless of instances)
```

### 4. Bundle Configuration (STEP_MOD_K)

Incentivize forming bundles of specific sizes:

```python
# Shards should deploy in bundles of 3
solver.add_constraint(
        ConstraintSpec(
            groupCapacitySpec=GroupCapacitySpec(
        name="bundle-shards",
        scope="host",
        partitionName="shard_group",
        bound=GroupCapacitySpecBound.MAX,
        limit=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=100,
        ),
        contribution=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=1.0,
        ),
        utilType=GroupCapacitySpecUtilType.STEP_MOD_K,
        bundleConfig=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=3,  # Bundle size = 3
        ),
    )
        )
)

# Utilization:
#   0 shards on host: count = 0
#   3 shards on host: count = 0 (perfect bundle!)
#   6 shards on host: count = 0 (two bundles)
#   1, 2, 4, 5 shards: count = 1 (incomplete bundle)
```

**Effect**: Solver prefers deploying in multiples of bundle size (e.g., 0, 3, 6, 9...).

### 5. Minimum Deployment Requirements

Ensure minimum total deployment:

```python
# Each service must have at least N instances deployed
solver.add_constraint(
        ConstraintSpec(
            groupCapacitySpec=GroupCapacitySpec(
        name="min-service-deployment",
        scope="host",
        partitionName="service",
        bound=GroupCapacitySpecBound.MIN,  # Minimum
        limit=Limit(
            type=LimitType.ABSOLUTE,
            groupLimits={
                "frontend": 3,  # At least 3 instances
                "backend": 5,   # At least 5 instances
            },
        ),
        contribution=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=1.0,
        ),
    )
        )
)
```

### 6. Exact Deployment Size

Enforce exact deployment size:

```python
# Services must deploy exactly N instances (no more, no less)
solver.add_constraint(
        ConstraintSpec(
            groupCapacitySpec=GroupCapacitySpec(
        name="exact-deployment-size",
        scope="host",
        partitionName="service",
        bound=GroupCapacitySpecBound.EXACT,  # Exact match
        limit=Limit(
            type=LimitType.ABSOLUTE,
            groupLimits={
                "frontend": 6,  # Exactly 6 instances
                "backend": 12,  # Exactly 12 instances
            },
        ),
        contribution=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=1.0,
        ),
    )
        )
)
```

### 7. Weighted Contributions

Different weights for different scope items:

```python
# Premium hosts contribute more to capacity
solver.add_constraint(
        ConstraintSpec(
            groupCapacitySpec=GroupCapacitySpec(
        name="weighted-capacity",
        scope="host",
        partitionName="service",
        bound=GroupCapacitySpecBound.MAX,
        limit=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=100.0,  # Max 100 weighted units
        ),
        contribution=Limit(
            type=LimitType.ABSOLUTE,
            scopeItemLimits={
                "premium_host_1": 2.0,  # Premium hosts count 2x
                "premium_host_2": 2.0,
                "budget_host_1": 1.0,   # Budget hosts count 1x
                "budget_host_2": 1.0,
            },
            globalLimit=1.0,  # Default weight
        ),
    )
        )
)

# Service on premium_host counts more toward total limit
```

### 8. Filtered Application

Apply limits only to specific scope items:

```python
# Limit service deployment only on production hosts
production_hosts = ["prod_host_1", "prod_host_2", "prod_host_3"]

solver.add_constraint(
        ConstraintSpec(
            groupCapacitySpec=GroupCapacitySpec(
        name="prod-service-limits",
        scope="host",
        partitionName="service",
        filter=Filter(items=production_hosts),  # Only prod hosts
        bound=GroupCapacitySpecBound.MAX,
        limit=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=50,
        ),
        contribution=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=1.0,
        ),
    )
        )
)
```

## Definition Types Explained

### DURING vs AFTER

| Definition | Counts | Use Case |
|------------|--------|----------|
| **DURING** | Objects being moved | Limit in-flight migrations |
| **AFTER** | Objects after move completes | Limit final state only |
| **DURING_AND_AFTER** | All objects | Limit both (default, safest) |

**Example**: Limit total frontend instances to 10

```python
# DURING_AND_AFTER (default): Both current + moved count toward limit
# - Currently deployed: 8 instances
# - Moving in 3 more instances
# - Total counted: 11 (would violate limit=10)

# AFTER only: Only final state counts
# - After moves complete: 10 instances
# - Allowed (even though temporarily 11 during migration)

# DURING only: Only in-flight counts
# - Moving 3 instances
# - Limit applies to these 3 only
```

## Performance Considerations

- **Impact**: Moderate to High - depends on partition size and utilization type
- **Solver compatibility**: Works with both Local Search and Optimal solvers
- **Scaling**:
  - LINEAR: O(groups × scope items × contribution groups)
  - STEP/STEP_MOD_K: Adds complexity due to non-linearity
- **Bundle config**: STEP_MOD_K significantly increases solver complexity

## Common Pitfalls

### 1. Contribution Partition Not Hierarchical

**Problem**: contributionPartition groups don't cleanly belong to partitionName groups.

```python
# BAD: Contribution groups span multiple main groups
services = {
    "service_a": ["instance_0", "instance_1"],
    "service_b": ["instance_2", "instance_3"],
}

# instance_shared belongs to BOTH service_a and service_b (invalid!)
contribution_groups = {
    "group_0": ["instance_0", "instance_shared"],
    "group_1": ["instance_1"],
    "group_2": ["instance_2", "instance_shared"],
}
# Error: instance_shared in multiple main groups
```

**Solution**: Ensure each contribution group belongs to exactly one main group:

```python
# GOOD: Clean hierarchy
services = {
    "service_a": ["instance_0", "instance_1", "instance_2"],
    "service_b": ["instance_3", "instance_4"],
}

contribution_groups = {
    "service_a_shard_0": ["instance_0"],
    "service_a_shard_1": ["instance_1", "instance_2"],
    "service_b_shard_0": ["instance_3", "instance_4"],
}
```

### 2. Contribution Weights Not Configured

**Problem**: Forgot to set contribution weights.

```python
# BAD: contribution limit has globalLimit=0 (default)
solver.add_constraint(
        ConstraintSpec(
            groupCapacitySpec=GroupCapacitySpec(
        limit=Limit(globalLimit=100),
        contribution=Limit(),  # Default: globalLimit=0!
    )
        )
)
# All contributions are 0, constraint does nothing
```

**Solution**: Set contribution weights:

```python
# GOOD: Specify contribution
solver.add_constraint(
        ConstraintSpec(
            groupCapacitySpec=GroupCapacitySpec(
        limit=Limit(globalLimit=100),
        contribution=Limit(globalLimit=1.0),  # Each object counts as 1
    )
        )
)
```

### 3. STEP_MOD_K Without bundleConfig

**Problem**: Using STEP_MOD_K but not specifying bundle size.

```python
# BAD: STEP_MOD_K requires bundleConfig
solver.add_constraint(
        ConstraintSpec(
            groupCapacitySpec=GroupCapacitySpec(
        utilType=GroupCapacitySpecUtilType.STEP_MOD_K,
        # Missing bundleConfig!
    )
        )
)
```

**Solution**: Provide bundleConfig:

```python
# GOOD: Specify bundle size
solver.add_constraint(
        ConstraintSpec(
            groupCapacitySpec=GroupCapacitySpec(
        utilType=GroupCapacitySpecUtilType.STEP_MOD_K,
        bundleConfig=Limit(globalLimit=3),  # Bundle size = 3
    )
        )
)
```

### 4. Limits Too Restrictive

**Problem**: Limits too tight, problem becomes infeasible.

```python
# BAD: Have 100 instances, limit total to 10
solver.add_constraint(
        ConstraintSpec(
            groupCapacitySpec=GroupCapacitySpec(
        partitionName="service",
        limit=Limit(globalLimit=10),  # Way too low!
    )
        )
)
# Can't fit 100 instances with limit=10
```

**Solution**: Calculate realistic limits:

```python
# GOOD: Check feasibility
total_instances = len(all_objects_in_service)
min_limit = total_instances  # At minimum

solver.add_constraint(
        ConstraintSpec(
            groupCapacitySpec=GroupCapacitySpec(
        partitionName="service",
        limit=Limit(globalLimit=total_instances),
    )
        )
)
```

### 5. Partition Not Defined

**Problem**: Partition doesn't exist.

```python
# BAD: Forgot to define partition
solver.add_constraint(
    GroupCapacitySpec(
        partition_name="service",  # Error: doesn't exist!
    )
)
```

**Solution**: Define partitions first:

```python
# GOOD: Define partitions
solver.add_partition("service", service_groups)
solver.add_partition("shard", shard_groups)

solver.add_constraint(
        ConstraintSpec(
            groupCapacitySpec=GroupCapacitySpec(
        partitionName="service",
        contributionPartition="shard",
    )
        )
)
```

## Combining with Other Specs

### With CapacitySpec

Limit both per-container and total:

```python
# Hard: Per-host capacity
solver.add_constraint(
        ConstraintSpec(
            capacitySpec=CapacitySpec(
        name="per-host-capacity",
        scope="host",
        dimension="cpu_usage",
    )
        )
)

# Hard: Total service capacity
solver.add_constraint(
        ConstraintSpec(
            groupCapacitySpec=GroupCapacitySpec(
        name="total-service-capacity",
        scope="host",
        partitionName="service",
        limit=Limit(globalLimit=500),
    )
        )
)
```

### With GroupCountSpec

Combine total capacity with per-container limits:

```python
# Limit total service instances to 20
solver.add_constraint(
        ConstraintSpec(
            groupCapacitySpec=GroupCapacitySpec(
        name="total-instances",
        scope="host",
        partitionName="service",
        limit=Limit(globalLimit=20),
        utilType=GroupCapacitySpecUtilType.LINEAR,
    )
        )
)

# And max 3 instances per host
solver.add_constraint(
        ConstraintSpec(
            groupCountSpec=GroupCountSpec(
        name="max-per-host",
        scope="host",
        partitionName="service",
        bound=GroupCountSpecBound.MAX,
        limit=Limit(globalLimit=3),
    )
        )
)
```

### With BalanceSpec

Balance within group capacity limits:

```python
# Hard: Group capacity limits
solver.add_constraint(
        ConstraintSpec(
            groupCapacitySpec=GroupCapacitySpec(
        name="service-limits",
        partitionName="service",
        limit=Limit(globalLimit=100),
    )
        )
)

# Soft: Balance load (within limits)
solver.add_goal(
        GoalSpec(
            balanceSpec=BalanceSpec(
        name="balance-load",
        scope="host",
        dimension="cpu_usage",
    )
        ),
    weight=1.0
)
```

## Verification Example

Verify group capacity limits:

```python
def verify_group_capacity(
    solution,
    partition,
    contribution_partition,
    contribution_weights,
    group_limits,
    util_type="LINEAR",
):
    """Verify group capacity constraints.

    Args:
        solution: Solver solution
        partition: Main partition dict
        contribution_partition: Contributing partition dict
        contribution_weights: Weight per scope item (dict)
        group_limits: Limit per group (dict)
        util_type: LINEAR, STEP, or STEP_MOD_K

    Returns:
        True if all limits respected
    """
    violations = []

    # Build object -> contribution group mapping
    obj_to_contrib_group = {}
    for cgroup, objs in contribution_partition.items():
        for obj in objs:
            obj_to_contrib_group[obj] = cgroup

    # Build contribution group -> main group mapping
    contrib_to_main = {}
    for main_group, contrib_groups in partition.items():
        for cgroup in contrib_groups:
            contrib_to_main[cgroup] = main_group

    # Calculate utilization per main group
    for main_group, contrib_groups in partition.items():
        total_util = 0.0

        # Sum contributions from each contributing group
        for cgroup in contrib_groups:
            contrib_objs = contribution_partition.get(cgroup, [])

            # Sum across scope items
            for scope_item, objects in solution["assignment"].items():
                weight = contribution_weights.get(scope_item, 1.0)

                # Count objects from this contribution group on this scope item
                count = sum(1 for obj in objects if obj in contrib_objs)

                if util_type == "LINEAR":
                    util_value = count
                elif util_type == "STEP":
                    util_value = 1 if count > 0 else 0
                elif util_type == "STEP_MOD_K":
                    bundle_size = 3  # Example
                    util_value = 0 if count % bundle_size == 0 else 1
                else:
                    util_value = count

                total_util += util_value * weight

        # Check limit
        limit = group_limits.get(main_group, float('inf'))
        if total_util > limit:
            violations.append({
                'group': main_group,
                'utilization': total_util,
                'limit': limit,
                'excess': total_util - limit,
            })

    if not violations:
        print(f"✅ All group capacity limits respected")
        return True
    else:
        print(f"❌ {len(violations)} groups exceed capacity:")
        for v in violations[:5]:
            print(f"   {v['group']}: {v['utilization']:.1f} > {v['limit']:.1f} "
                  f"(excess: {v['excess']:.1f})")
        return False
```

## Bundle Analysis

Analyze bundling effectiveness:

```python
def analyze_bundling(solution, partition, bundle_size):
    """Analyze how well objects form bundles.

    Args:
        solution: Solver solution
        partition: Partition dict
        bundle_size: Target bundle size

    Returns:
        Bundling statistics
    """
    # Invert partition
    obj_to_group = {}
    for group, objects in partition.items():
        for obj in objects:
            obj_to_group[obj] = group

    # Count group instances per scope item
    group_counts = {}
    for scope_item, objects in solution["assignment"].items():
        counts = {}
        for obj in objects:
            group = obj_to_group.get(obj)
            if group:
                counts[group] = counts.get(group, 0) + 1

        for group, count in counts.items():
            if group not in group_counts:
                group_counts[group] = []
            group_counts[group].append(count)

    # Analyze bundling
    perfect_bundles = 0
    incomplete_bundles = 0

    for group, counts in group_counts.items():
        for count in counts:
            if count % bundle_size == 0:
                perfect_bundles += 1
            else:
                incomplete_bundles += 1

    total = perfect_bundles + incomplete_bundles
    print(f"Bundling analysis (bundle size = {bundle_size}):")
    print(f"  Perfect bundles: {perfect_bundles} ({perfect_bundles/total*100:.1f}%)")
    print(f"  Incomplete bundles: {incomplete_bundles} ({incomplete_bundles/total*100:.1f}%)")
    print(f"  Total scope items: {total}")
```

## Related Specs

- [CapacitySpec](../capacity) - Per-container capacity limits
- [GroupCountSpec](group-count) - Control group presence per container
- [DisasterRecoveryCapacitySpec](../disaster-recovery-capacity) - DR-aware capacity
- [CapacityWithMinGroupPresenceSpec](capacity-with-group-presence) - Capacity with group requirements

## Source Code

- Thrift definition: `interface/thrift/ProblemSpecs.thrift:620` (GroupCapacitySpecDefinition enum)
- Thrift definition: `interface/thrift/ProblemSpecs.thrift:626` (GroupCapacitySpecBound enum)
- Thrift definition: `interface/thrift/ProblemSpecs.thrift:632` (GroupCapacitySpecUtilType enum)
- Thrift definition: `interface/thrift/ProblemSpecs.thrift:663` (GroupCapacitySpec struct)
- Implementation: `solver/constraints/GroupCapacity.cpp`
