---
sidebar_position: 1
---

# Goals & Constraints Reference

This section provides comprehensive API reference for all goals and constraints available in Rebalancer.

## Overview

Rebalancer provides 40+ built-in specifications (specs) for defining goals and constraints. Each spec page includes:

- API signatures for C++ and Python
- Parameter descriptions
- Usage examples
- Performance notes
- Related specs

## Quick Reference Table

| Name | Type | Description |
|------|------|-------------|
| [BalanceSpec](balance) | Goal | Balance utilization across scope items |
| [CapacitySpec](capacity) | Both | Limit utilization within scope items |
| [MinimizeMovementSpec](movement/minimize-movement) | Goal | Minimize number of moved objects |
| [UtilIncreaseCostSpec](util-increase-cost) | Goal | Prefer moves to under-utilized containers |
| [GroupCountSpec](groups/group-count) | Both | Limit objects from same group per scope item |
| [GroupCapacitySpec](groups/group-capacity) | Both | Limit total utilization of partition groups |
| [AvoidMovingSpec](movement/avoid-moving) | Constraint | Prevent specific objects from moving |
| [AvoidAssignmentsSpec](placement/avoid-assignments) | Constraint | Prevent specific object-container assignments |
| [AssignmentAffinitiesSpec](placement/assignment-affinities) | Goal | Prefer specific assignments |
| [ColocateGroupsSpec](groups/colocate-groups) | Both | Place objects from same group together |
| [CapacityWithGroupPresenceSpec](groups/capacity-with-group-presence) | Both | Capacity with minimum group presence weight |
| [DisasterRecoveryCapacitySpec](disaster-recovery-capacity) | Both | Ensure DR capacity after failures |
| [GroupMoveLimitSpec](groups/group-move-limit) | Constraint | Limit moves per group |
| [GroupIsolationLimitSpec](groups/group-isolation-limit) | Constraint | Limit distinct groups per container |
| [GroupDiversitySpec](groups/group-diversity) | Both | Dimension-weighted group diversity per scope item |
| [AggregatedGroupSpec](groups/aggregated-group) | Both | Aggregate group utilization with flexible functions |
| [MinimizeContainersSpec](balance-optimize/minimize-containers) | Goal | Minimize number of used containers |
| [MinimizeSquaresSpec](balance-optimize/minimize-squares) | Goal | Minimize sum of squares of utilization |
| [MinimizeNthLargestSpec](balance-optimize/minimize-nth-largest) | Goal | Minimize Nth largest utilization value |
| [MaximizeAllocationSpec](balance-optimize/maximize-allocation) | Goal | Maximize utilization on scope items |
| [MovesInProgressSpec](movement/moves-in-progress) | Constraint | Account for ongoing moves |
| [MoveGroupSpec](groups/move-group) | Constraint | Control which partition groups can move |
| [NonAcceptingSpec](placement/non-accepting) | Constraint | Mark containers as non-accepting |
| [PairAffinitiesSpec](placement/pair-affinities) | Goal | Encourage pairs of objects to colocate |
| [ScopeAffinitiesSpec](placement/scope-affinities) | Goal | Affinity between objects and containers |
| [GroupAssignmentAffinitiesSpec](placement/group-assignment-affinities) | Goal | Group-level affinity preferences |
| [ExclusiveScopeItemsSpec](placement/exclusive-scope-items) | Both | Mutually exclusive scope items |
| [ThrottlingSpec](throttling) | Constraint | Limit volume of objects moved |
| [ToFreeSpec](to-free) | Both | Free specific containers |
| [WorkingSetSpec](misc/working-set) | Goal | Optimize working set latency |

## Filter by Type

### Goals Only
Specs that can only be used as goals (soft objectives):

- [MinimizeMovementSpec](movement/minimize-movement)
- [UtilIncreaseCostSpec](util-increase-cost)
- [AssignmentAffinitiesSpec](placement/assignment-affinities)
- [GroupAssignmentAffinitiesSpec](placement/group-assignment-affinities)
- [MinimizeContainersSpec](balance-optimize/minimize-containers)
- [MinimizeSquaresSpec](balance-optimize/minimize-squares)
- [MinimizeNthLargestSpec](balance-optimize/minimize-nth-largest)
- [MaximizeAllocationSpec](balance-optimize/maximize-allocation)
- [PairAffinitiesSpec](placement/pair-affinities)
- [ScopeAffinitiesSpec](placement/scope-affinities)
- [WorkingSetSpec](misc/working-set)

### Constraints Only
Specs that can only be used as constraints (hard requirements):

- [AvoidMovingSpec](movement/avoid-moving)
- [AvoidAssignmentsSpec](placement/avoid-assignments)
- [NonAcceptingSpec](placement/non-accepting)
- [MovesInProgressSpec](movement/moves-in-progress)
- [MoveGroupSpec](groups/move-group)
- [GroupMoveLimitSpec](groups/group-move-limit)
- [GroupIsolationLimitSpec](groups/group-isolation-limit)
- [ThrottlingSpec](throttling)

### Both Goal and Constraint
Specs that can be used as either:

- [BalanceSpec](balance)
- [CapacitySpec](capacity)
- [GroupCountSpec](groups/group-count)
- [GroupCapacitySpec](groups/group-capacity)
- [GroupDiversitySpec](groups/group-diversity)
- [AggregatedGroupSpec](groups/aggregated-group)
- [ColocateGroupsSpec](groups/colocate-groups)
- [CapacityWithGroupPresenceSpec](groups/capacity-with-group-presence)
- [DisasterRecoveryCapacitySpec](disaster-recovery-capacity)
- [ExclusiveScopeItemsSpec](placement/exclusive-scope-items)
- [ToFreeSpec](to-free)

## Filter by Use Case

### Load Balancing
- [BalanceSpec](balance) - Balance resources
- [BalanceRatioSpec](balance) - Balance ratios of dimensions
- [UtilIncreaseCostSpec](util-increase-cost) - Prefer less-utilized containers

### Capacity Management
- [CapacitySpec](capacity) - Basic capacity limits
- [CapacityWithGroupPresenceSpec](groups/capacity-with-group-presence) - Capacity with group minimums
- [GroupCapacitySpec](groups/group-capacity) - Per-group capacity
- [DisasterRecoveryCapacitySpec](disaster-recovery-capacity) - DR capacity

### Placement Rules
- [AvoidAssignmentsSpec](placement/avoid-assignments) - Forbidden assignments
- [GroupCountSpec](groups/group-count) - Limit group members per container
- [ColocateGroupsSpec](groups/colocate-groups) - Keep group together
- [GroupIsolationLimitSpec](groups/group-isolation-limit) - Separate groups

### Affinity & Preferences
- [AssignmentAffinitiesSpec](placement/assignment-affinities) - Object-container preferences
- [PairAffinitiesSpec](placement/pair-affinities) - Pair colocation preferences
- [ScopeAffinitiesSpec](placement/scope-affinities) - Scope-level preferences

### Movement Control
- [MinimizeMovementSpec](movement/minimize-movement) - Reduce moves
- [AvoidMovingSpec](movement/avoid-moving) - Pin specific objects
- [GroupMoveLimitSpec](groups/group-move-limit) - Limit moves per group
- [ThrottlingSpec](throttling) - Rate-limit moves

### Optimization
- [MinimizeContainersSpec](balance-optimize/minimize-containers) - Bin packing
- [MaximizeAllocationSpec](balance-optimize/maximize-allocation) - Maximize usage
- [MinimizeSquaresSpec](balance-optimize/minimize-squares) - Minimize variance
- [MinimizeNthLargestSpec](balance-optimize/minimize-nth-largest) - Minimize peak

## Common Patterns

### Pattern: Balance with Capacity

Most common pattern - balance load while respecting capacity:

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python
from rebalancer.specs import (
    BalanceSpec,
    CapacitySpec,
    ConstraintSpec,
    GoalSpec,
)

# Constraint: don't exceed capacity
solver.add_constraint(
    ConstraintSpec(
        capacitySpec=CapacitySpec(name="capacity", scope="host", dimension="cpu")
    )
)

# Goal: balance across hosts
solver.add_goal(
    GoalSpec(
        balanceSpec=BalanceSpec(name="balance", scope="host", dimension="cpu")
    ),
    weight=1.0,
)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Constraint: don't exceed capacity
CapacitySpec capacitySpec;
capacitySpec.name() = "capacity";
capacitySpec.scope() = "host";
capacitySpec.dimension() = "cpu";
solver.addConstraint(capacitySpec);

// Goal: balance across hosts
BalanceSpec balanceSpec;
balanceSpec.name() = "balance";
balanceSpec.scope() = "host";
balanceSpec.dimension() = "cpu";
solver.addGoal(balanceSpec, 1.0);
```

</TabItem>
</Tabs>

### Pattern: Minimize Movement While Rebalancing

Balance load but prefer minimal disruption:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python
from rebalancer.specs import (
    BalanceSpec,
    GoalSpec,
    MinimizeMovementSpec,
)

solver.add_goal(
    GoalSpec(
        balanceSpec=BalanceSpec(name="balance", scope="host", dimension="cpu")
    ),
    weight=10.0,
)
solver.add_goal(
    GoalSpec(
        minimizeMovementSpec=MinimizeMovementSpec(
            name="minimize-movement", scope="host", dimension="cpu"
        )
    ),
    weight=1.0,
)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
solver.addGoal(balanceSpec, 10.0);
solver.addGoal(minimizeMovementSpec, 1.0);
```

</TabItem>
</Tabs>

### Pattern: Disaster Recovery

Ensure group diversity across failure domains:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python
from rebalancer.specs import ConstraintSpec, GroupCountSpec, Limit

solver.add_constraint(
    ConstraintSpec(
        groupCountSpec=GroupCountSpec(
            name="rack-diversity",
            scope="rack",
            partitionName="database",
            # Max 1 shard per database per rack
            limit=Limit(type="ABSOLUTE", globalLimit=1.0),
        )
    )
)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
GroupCountSpec spec;
spec.name() = "rack-diversity";
spec.scope() = "rack";
spec.partition() = "database";
Limit limit;
limit.globalLimit() = 1.0;
spec.limit() = limit;
solver.addConstraint(spec);
```

</TabItem>
</Tabs>

## API Common Patterns

### Scopes and Dimensions

Most specs require `scope` and `dimension` parameters:

- **scope**: Which level to apply the spec (`"host"`, `"rack"`, `"datacenter"`)
- **dimension**: Which resource to constrain/optimize (`"cpu"`, `"memory"`, `"disk"`)

### Limits

Many specs use `Limit` objects:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python
from rebalancer.specs import Limit

# Absolute limit: max 100 units
Limit(type="ABSOLUTE", globalLimit=100.0)

# Per-scope-item limits
Limit(type="ABSOLUTE", scopeItemLimits={"host1": 50.0, "host2": 75.0})

# Relative limit: max 80% of capacity
Limit(type="RELATIVE", globalLimit=0.8)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Absolute limit: max 100 units
Limit limit1;
limit1.globalLimit() = 100.0;

// Per-scope-item limits
Limit limit2;
limit2.scopeItemLimits() = {{"host1", 50.0}, {"host2", 75.0}};

// Relative limit: max 80% of capacity
Limit limit3;
limit3.globalLimit() = 0.8;
limit3.limitType() = LimitType::RELATIVE;
```

</TabItem>
</Tabs>

### Filters

Many specs support filters to apply constraints or goals only to a subset of objects or containers:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python
from rebalancer.specs import (
    BalanceSpec,
    CapacitySpec,
    Filter,
    Limit,
)

# Example: Apply capacity constraint only to specific hosts
capacity_spec = CapacitySpec(
    name="capacity-critical-hosts",
    scope="host",
    dimension="memory",
    limit=Limit(type="ABSOLUTE", globalLimit=1000.0),
    filter=Filter(
        itemsWhitelist=["host1", "host2", "host3"],  # Only these hosts
        type="SCOPE_ITEM",  # Filter by scope items (default)
    ),
)

# Or filter by blacklist (exclude certain hosts)
capacity_spec_blacklist = CapacitySpec(
    name="capacity-most-hosts",
    scope="host",
    dimension="cpu",
    limit=Limit(type="ABSOLUTE", globalLimit=80.0),
    filter=Filter(
        itemsBlacklist=["maintenance_host1", "draining_host2"],  # Exclude these
    ),
)

# Filter by group (requires partition)
balance_spec = BalanceSpec(
    name="balance-critical-groups",
    scope="rack",
    dimension="disk",
    filter=Filter(
        itemsWhitelist=["critical_group", "prod_group"],
        type="GROUP",  # Filter by group names
    ),
)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
#include <rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h>

using namespace facebook::rebalancer::interface;

// Example: Apply capacity constraint only to specific hosts
CapacitySpec capacitySpec;
capacitySpec.name() = "capacity-critical-hosts";
capacitySpec.scope() = "host";
capacitySpec.dimension() = "memory";

Limit limit;
limit.globalLimit() = 1000.0;
capacitySpec.limit() = limit;

Filter filter;
filter.itemsWhitelist() = {"host1", "host2", "host3"};  // Only these hosts
filter.type() = FilterType::SCOPE_ITEM;  // Filter by scope items (default)
capacitySpec.filter() = filter;

// Or filter by blacklist (exclude certain hosts)
CapacitySpec capacitySpecBlacklist;
capacitySpecBlacklist.name() = "capacity-most-hosts";
capacitySpecBlacklist.scope() = "host";
capacitySpecBlacklist.dimension() = "cpu";

Limit limitBlacklist;
limitBlacklist.globalLimit() = 80.0;
capacitySpecBlacklist.limit() = limitBlacklist;

Filter filterBlacklist;
filterBlacklist.itemsBlacklist() = {"maintenance_host1", "draining_host2"};  // Exclude these
capacitySpecBlacklist.filter() = filterBlacklist;

// Filter by group (requires partition)
BalanceSpec balanceSpec;
balanceSpec.name() = "balance-critical-groups";
balanceSpec.scope() = "rack";
balanceSpec.dimension() = "disk";

Filter groupFilter;
groupFilter.itemsWhitelist() = {"critical_group", "prod_group"};
groupFilter.type() = FilterType::GROUP;  // Filter by group names
balanceSpec.filter() = groupFilter;
```

</TabItem>
</Tabs>

**Filter Fields:**
- `itemsWhitelist` (optional): List of items to include (all others excluded)
- `itemsBlacklist` (optional): List of items to exclude (all others included)
- `type` (default: SCOPE_ITEM): What to filter - SCOPE_ITEM (containers) or GROUP (partition groups)

**Note**: Whitelist and blacklist are mutually exclusive - use one or the other, not both.

## Next Steps

- Choose a spec from the table above to see detailed documentation
- Review [Core Concepts](../core-concepts/overview) to understand how specs work
- See [Cookbook Examples](../cookbook/) for real-world usage patterns

## Contributing

Found an error or want to improve a spec page? See [Contributing Guide](https://github.com/facebookincubator/rebalancer/blob/main/CONTRIBUTING.md).
