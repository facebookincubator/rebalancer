---
sidebar_position: 18
---

# GroupIsolationLimitSpec

**Type**: Constraint only

Limit the number of distinct partition groups present on each container.

## Overview

`GroupIsolationLimitSpec` limits how many different groups (from a partition) can coexist on each container. For example, limit each host to at most 3 different services, or ensure each rack has at least 2 different replica groups for diversity.

**Use this when**: You want to control group diversity per container - either limiting mixing (isolation) or ensuring mixing (diversity).

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/reference/groups/group_isolation_limit_spec_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/groups/group_isolation_limit_spec_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Unique identifier for this constraint |
| `scope` | string | Yes | - | Scope level (e.g., `"host"`, `"rack"`) |
| `partitionName` | string | Yes | - | Partition defining groups |
| `limit` | Limit | No | `{type: ABSOLUTE, globalLimit: 0}` | Max groups per container |
| `groupsAllowed` | int32 | No | 1 | Multiplier for limit (typically 1) |
| `filter` | Filter | No | none | Apply only to filtered containers |

## How It Works

For each container in scope:
```
number_of_groups_present ≤ limit × groupsAllowed
```

**Example**:
- Partition "service": `{service_a: [obj1, obj2], service_b: [obj3, obj4], service_c: [obj5]}`
- Host has: `[obj1, obj3]`
- Groups present: `{service_a, service_b}` → 2 groups
- If limit=2, groupsAllowed=1: 2 ≤ 2×1 = 2 ✓ (satisfied)

## Common Usage Patterns

### 1. Limit Service Mixing

Prevent too many services on same host:

```python
# Service partition
services = {
    "web": ["web_0", "web_1", "web_2"],
    "api": ["api_0", "api_1", "api_2"],
    "db": ["db_0", "db_1"],
    "cache": ["cache_0", "cache_1"],
}
solver.add_partition("service", services)

# Max 2 different services per host
solver.add_constraint(
        ConstraintSpec(
            groupIsolationLimitSpec=GroupIsolationLimitSpec(
        name="limit-service-mixing",
        scope="host",
        partitionName="service",
        limit=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=2,  # Max 2 services per host
        ),
    )
        )
)
```

**Result**: Each host has objects from at most 2 different services.

### 2. Ensure Minimum Diversity

Require minimum number of groups per rack (diversity):

```python
# Replica group partition
replica_groups = {
    "shard_0_replicas": ["shard_0_r0", "shard_0_r1", "shard_0_r2"],
    "shard_1_replicas": ["shard_1_r0", "shard_1_r1", "shard_1_r2"],
    # ... more shards
}
solver.add_partition("replica_group", replica_groups)

# Each rack must have at least 3 different shard groups (diversity)
# Note: This requires using goal with negative weight or specific formulation
# For minimum, typically use GroupCountSpec instead
```

**Note**: GroupIsolationLimitSpec is primarily for **maximum** limits. For **minimum** diversity, use [GroupCountSpec](group-count) with MIN bound.

### 3. Per-Host Custom Limits

Different limits for different hosts:

```python
# Premium hosts can handle more service mixing
host_limits = {
    "premium_host_1": 5,  # Can have 5 services
    "premium_host_2": 5,
    "standard_host_1": 2,  # Only 2 services
    "standard_host_2": 2,
}

solver.add_constraint(
        ConstraintSpec(
            groupIsolationLimitSpec=GroupIsolationLimitSpec(
        name="tiered-isolation",
        scope="host",
        partitionName="service",
        limit=Limit(
            type=LimitType.ABSOLUTE,
            scopeItemLimits=host_limits,
            globalLimit=3,  # Default for hosts not in scopeItemLimits
        ),
    )
        )
)
```

### 4. Tenant Isolation

Limit number of tenants per host:

```python
# Tenant partition
tenants = {
    "tenant_a": ["tenant_a_vm_0", "tenant_a_vm_1"],
    "tenant_b": ["tenant_b_vm_0", "tenant_b_vm_1"],
    "tenant_c": ["tenant_c_vm_0", "tenant_c_vm_1"],
}
solver.add_partition("tenant", tenants)

# Max 2 different tenants per host (isolation)
solver.add_constraint(
        ConstraintSpec(
            groupIsolationLimitSpec=GroupIsolationLimitSpec(
        name="tenant-isolation",
        scope="host",
        partitionName="tenant",
        limit=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=2,
        ),
    )
        )
)
```

### 5. Rack-Level Diversity Control

Limit application groups per rack:

```python
# Application partition
applications = {
    "app_prod": ["app_prod_0", "app_prod_1", "app_prod_2"],
    "app_staging": ["app_staging_0", "app_staging_1"],
    "app_dev": ["app_dev_0", "app_dev_1"],
}
solver.add_partition("application", applications)

# Each rack should have at most 2 application types
solver.add_constraint(
        ConstraintSpec(
            groupIsolationLimitSpec=GroupIsolationLimitSpec(
        name="rack-app-isolation",
        scope="rack",
        partitionName="application",
        limit=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=2,
        ),
    )
        )
)
```

### 6. Workload Type Segregation

Separate workload types to avoid interference:

```python
# Workload type partition
workload_types = {
    "latency_sensitive": ["web_0", "api_0", "realtime_0"],
    "throughput_oriented": ["batch_0", "analytics_0"],
    "background": ["backup_0", "maintenance_0"],
}
solver.add_partition("workload_type", workload_types)

# Don't mix more than 2 workload types per host
solver.add_constraint(
        ConstraintSpec(
            groupIsolationLimitSpec=GroupIsolationLimitSpec(
        name="workload-segregation",
        scope="host",
        partitionName="workload_type",
        limit=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=2,
        ),
    )
        )
)
```

### 7. Filtered Application

Apply limit only to specific hosts:

```python
# Only apply isolation limit to production hosts
production_hosts = ["prod_host_1", "prod_host_2", "prod_host_3"]

solver.add_constraint(
        ConstraintSpec(
            groupIsolationLimitSpec=GroupIsolationLimitSpec(
        name="prod-isolation",
        scope="host",
        partitionName="service",
        filter=Filter(items=production_hosts),
        limit=Limit(
            type=LimitType.ABSOLUTE,
            globalLimit=1,  # Prod hosts: only 1 service each (strict)
        ),
    )
        )
)
```

## groupsAllowed Parameter

The `groupsAllowed` parameter is a multiplier, typically left at default (1):

```
max_groups = limit × groupsAllowed
```

**Typical usage**: Leave as 1. Advanced use cases might adjust this for complex isolation patterns.

## Performance Considerations

- **Impact**: Minimal - simple group counting
- **Solver compatibility**: Works with both Local Search and Optimal solvers
- **Scaling**: O(number of containers × number of groups)

## Common Pitfalls

### 1. Limit Too Restrictive

**Problem**: Limit too low, problem becomes infeasible.

```python
# BAD: 10 services, 5 hosts, limit=1
# Impossible to fit 10 services with max 1 per host!
solver.add_constraint(
        ConstraintSpec(
            groupIsolationLimitSpec=GroupIsolationLimitSpec(
        partitionName="service",  # 10 different services
        limit=Limit(globalLimit=1),  # Only 1 service per host
    )
        )
)
# Need at least 10 hosts!
```

**Solution**: Ensure enough capacity for groups:

```python
# GOOD: Verify feasibility
num_groups = len(service_partition)
num_containers = len(hosts)
max_groups_per_container = 1

min_containers_needed = num_groups / max_groups_per_container

if num_containers < min_containers_needed:
    print(f"Warning: Need at least {min_containers_needed} containers!")
```

### 2. Partition Not Defined

**Problem**: Partition doesn't exist.

```python
# BAD: Forgot to define partition
solver.add_constraint(
    GroupIsolationLimitSpec(
        partition_name="service",  # Error: doesn't exist!
        limit=Limit(globalLimit=3),
    )
)
```

**Solution**: Define partition first:

```python
# GOOD: Define partition
solver.add_partition("service", service_groups)

solver.add_constraint(
        ConstraintSpec(
            groupIsolationLimitSpec=GroupIsolationLimitSpec(
        partitionName="service",
        limit=Limit(globalLimit=3),
    )
        )
)
```

### 3. Conflicting with ColocateGroupsSpec

**Problem**: Isolation limit conflicts with colocation goal.

```python
# BAD: Want to colocate all services, but limit=1 per host
solver.add_goal(
        GoalSpec(
            colocateGroupsSpec=ColocateGroupsSpec(
        partitionName="service",
        bound=ColocateGroupsSpecBound.MAX,
        limits=Limit(globalLimit=1),  # All services on 1 host
    )
        ),
    weight=10.0
)

solver.add_constraint(
        ConstraintSpec(
            groupIsolationLimitSpec=GroupIsolationLimitSpec(
        partitionName="service",
        limit=Limit(globalLimit=1),  # Max 1 service per host
    )
        )
)
# Conflicting: can't colocate all on 1 host AND have max 1 per host
```

**Solution**: Ensure limits are compatible with other goals.

### 4. Using for Minimum Diversity

**Problem**: Trying to enforce minimum groups (not maximum).

```python
# BAD: GroupIsolationLimitSpec is for MAXIMUM, not minimum
# Can't use it to require "at least 3 groups per rack"
```

**Solution**: Use GroupCountSpec for minimum diversity:

```python
# GOOD: Use GroupCountSpec for minimum
solver.add_constraint(
        ConstraintSpec(
            groupCountSpec=GroupCountSpec(
        name="min-diversity",
        scope="rack",
        partitionName="replica_group",
        bound=GroupCountSpecBound.MIN,
        limit=Limit(globalLimit=3),  # At least 3 groups per rack
    )
        )
)
```

## Combining with Other Specs

### With BalanceSpec

Limit mixing while balancing load:

```python
# Limit service mixing
solver.add_constraint(
        ConstraintSpec(
            groupIsolationLimitSpec=GroupIsolationLimitSpec(
        name="isolation",
        scope="host",
        partitionName="service",
        limit=Limit(globalLimit=2),
    )
        )
)

# Balance load within isolation constraints
solver.add_goal(
        GoalSpec(
            balanceSpec=BalanceSpec(
        name="balance",
        scope="host",
        dimension="cpu_usage",
    )
        ),
    weight=1.0
)
```

### With GroupCountSpec

Combine for complex diversity patterns:

```python
# Max 3 services per host (isolation)
solver.add_constraint(
        ConstraintSpec(
            groupIsolationLimitSpec=GroupIsolationLimitSpec(
        name="max-mixing",
        scope="host",
        partitionName="service",
        limit=Limit(globalLimit=3),
    )
        )
)

# But any service present must have at least 2 instances per host (redundancy)
solver.add_constraint(
        ConstraintSpec(
            groupCountSpec=GroupCountSpec(
        name="min-instances",
        scope="host",
        partitionName="service",
        bound=GroupCountSpecBound.MIN,
        limit=Limit(globalLimit=2),
    )
        )
)
```

## Verification Example

Verify group isolation limits respected:

```python
def verify_group_isolation(solution, partition, max_groups_per_container):
    """Verify group isolation limits.

    Args:
        solution: Solver solution
        partition: Partition dict (group -> objects)
        max_groups_per_container: Maximum allowed groups per container

    Returns:
        True if limits respected
    """
    violations = []

    # Invert partition to map objects to groups
    object_to_group = {}
    for group, objects in partition.items():
        for obj in objects:
            object_to_group[obj] = group

    for container, objects in solution["assignment"].items():
        # Find unique groups on this container
        groups_present = set()
        for obj in objects:
            if obj in object_to_group:
                groups_present.add(object_to_group[obj])

        if len(groups_present) > max_groups_per_container:
            violations.append({
                'container': container,
                'groups_present': len(groups_present),
                'groups': groups_present,
                'limit': max_groups_per_container,
            })

    if not violations:
        print(f"✅ All containers within group limit (≤{max_groups_per_container} groups)")
        return True
    else:
        print(f"❌ {len(violations)} containers exceed group limit:")
        for v in violations[:5]:
            print(f"   {v['container']}: {v['groups_present']} groups "
                  f"(limit: {v['limit']}) - {v['groups']}")
        return False
```

## Group Diversity Analysis

Analyze group distribution:

```python
def analyze_group_diversity(solution, partition):
    """Analyze how groups are distributed across containers.

    Args:
        solution: Solver solution
        partition: Partition dict
    """
    # Invert partition
    object_to_group = {}
    for group, objects in partition.items():
        for obj in objects:
            object_to_group[obj] = group

    # Count groups per container
    groups_per_container = {}
    for container, objects in solution["assignment"].items():
        groups = set(object_to_group.get(obj) for obj in objects if obj in object_to_group)
        groups.discard(None)
        groups_per_container[container] = len(groups)

    # Statistics
    counts = list(groups_per_container.values())
    if counts:
        print("Group diversity per container:")
        print(f"  Min: {min(counts)} groups")
        print(f"  Median: {sorted(counts)[len(counts)//2]} groups")
        print(f"  Max: {max(counts)} groups")
        print(f"  Average: {sum(counts)/len(counts):.1f} groups")

        # Distribution
        from collections import Counter
        distribution = Counter(counts)
        print("\nDistribution:")
        for num_groups in sorted(distribution.keys()):
            container_count = distribution[num_groups]
            print(f"  {num_groups} groups: {container_count} containers")
```

## Related Specs

- [GroupCountSpec](group-count) - Control group presence (min/max instances per group)
- [ColocateGroupsSpec](colocate-groups) - Colocate or spread groups (soft goal)
- [GroupCapacitySpec](group-capacity) - Group-based capacity constraints

## Source Code

- Thrift definition: `interface/thrift/ProblemSpecs.thrift:737` (GroupIsolationLimitSpec)
- Implementation: `solver/constraints/GroupIsolationLimit.cpp`
