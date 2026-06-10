---
sidebar_position: 4
---

# Scopes and Partitions

Scopes and partitions are powerful concepts for grouping containers and objects. They enable hierarchical constraints and goals.

## Overview

- **Scopes**: Group containers for constraints/goals at different granularities
- **Partitions**: Group objects by shared properties

Together, they let you express complex placement rules like "each database should have shards in at least 2 datacenters" or "no rack should have more than 3 shards from the same table."

Examples from healthcare might be: "each patient group must have beds in at least 2 wards" or "no nurse should be assigned more than 5 patients from the same care unit."

## Scopes: Grouping Containers

**Scopes** organize containers into hierarchies or categories.

### Why Scopes?

Without scopes, constraints apply to all containers. With scopes, you can:

- Balance across racks, not just hosts
- Ensure datacenter-level diversity
- Apply capacity limits at multiple levels
- Model hierarchical infrastructure

### Defining Scopes

A scope maps each container to a scope item (group name).

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

#### Example: Rack Scope

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
# Map each host to its rack
host_to_rack = {
    "host0": "rack0",
    "host1": "rack0",
    "host2": "rack1",
    "host3": "rack1",
    "host4": "rack2",
    "host5": "rack2",
}

solver.add_scope("rack", host_to_rack)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Map each host to its rack
std::map<std::string, std::string> host_to_rack = {
    {"host0", "rack0"},
    {"host1", "rack0"},
    {"host2", "rack1"},
    {"host3", "rack1"},
    {"host4", "rack2"},
    {"host5", "rack2"}
};

solver.addScope("rack", host_to_rack);
```

</TabItem>
</Tabs>

Now you can apply constraints at the rack level:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
from rebalancer.specs import CapacitySpec, ConstraintSpec, Limit

# Limit CPU per rack (not just per host)
solver.add_constraint(
    ConstraintSpec(
        capacitySpec=CapacitySpec(
            name="rack-cpu-limit",
            scope="rack",  # Applies to racks
            dimension="cpu",
            # Max 50 CPU cores per rack
            limit=Limit(type="ABSOLUTE", globalLimit=50.0),
        )
    )
)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Limit CPU per rack (not just per host)
CapacitySpec rackCpuLimit;
rackCpuLimit.name() = "rack-cpu-limit";
rackCpuLimit.scope() = "rack";  // Applies to racks
rackCpuLimit.dimension() = "cpu";
rackCpuLimit.limit() = Limit();
rackCpuLimit.limit()->globalLimit() = 50.0;  // Max 50 CPU cores per rack
solver.addConstraint(rackCpuLimit);
```

</TabItem>
</Tabs>

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
# Define a scope
solver.add_scope(
    scope_name="rack",
    container_to_scope_item={
        "host0": "rack0",
        "host1": "rack0",
        "host2": "rack1",
        "host3": "rack1",
    },
)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
std::map<std::string, std::string> containerToRack = {
    {"host0", "rack0"},
    {"host1", "rack0"},
    {"host2", "rack1"},
    {"host3", "rack1"}
};
solver.addScope("rack", containerToRack);
```

</TabItem>
</Tabs>

### Built-In Scope: Container Level

Every problem implicitly has a container-level scope. When you write:

```python
solver.set_container_name("host")
```

Rebalancer creates a built-in scope where each container is its own scope item. You reference this as:

```python
from rebalancer.specs import CapacitySpec

CapacitySpec(
    name="host-cap",
    scope="host",  # Built-in container-level scope
    dimension="cpu",
)
```

### Multiple Scopes

You can define multiple scopes for different hierarchies:

```python
# Rack hierarchy
solver.add_scope("rack", host_to_rack)

# Datacenter hierarchy
solver.add_scope("datacenter", host_to_datacenter)

# Host type (e.g., GPU vs CPU)
solver.add_scope("host_type", host_to_type)
```

Each scope provides a different grouping:

```python
from rebalancer.specs import (
    BalanceSpec,
    CapacitySpec,
    ConstraintSpec,
    GoalSpec,
)

# Limit per datacenter
solver.add_constraint(
    ConstraintSpec(
        capacitySpec=CapacitySpec(
            name="dc-cap", scope="datacenter", dimension="cpu"
        )
    )
)

# Balance across racks
solver.add_goal(
    GoalSpec(
        balanceSpec=BalanceSpec(
            name="balance-rack-mem", scope="rack", dimension="memory"
        )
    ),
    weight=1.0,
)

# Avoid assignments to specific host types
# (using host_type scope)
```

### Hierarchical Scopes

Scopes can form hierarchies:

```
Datacenter
  └─ Rack
      └─ Host (container)
```

```python
# Define each level
solver.add_scope("datacenter", {
    "host0": "dc1", "host1": "dc1",
    "host2": "dc2", "host3": "dc2",
})

solver.add_scope("rack", {
    "host0": "rack1", "host1": "rack2",
    "host2": "rack3", "host3": "rack4",
})

# Note: Each scope item name must be unique within that scope
# Rebalancer doesn't enforce hierarchy - you define each level independently
```

## Partitions: Grouping Objects

**Partitions** group objects by shared properties.

### Why Partitions?

Without partitions, you can only constrain/optimize individual objects. With partitions:

- Limit how many objects from same group per container
- Keep objects from same group together (colocation)
- Keep objects from same group apart (diversity)
- Apply group-level constraints

### Defining Partitions

A partition maps each object to a group name.

#### Example: Database Partitions

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
# Group shards by database (the binding wants {group: [objects]} form)
shards_by_database = {
    "db_users": ["shard0", "shard1", "shard2"],
    "db_orders": ["shard3", "shard4"],
    "db_products": ["shard5"],
}

solver.add_partition("database", shards_by_database)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Group shards by database
std::map<std::string, std::vector<std::string>> shard_groups = {
    {"db_users", {"shard0", "shard1", "shard2"}},
    {"db_orders", {"shard3", "shard4"}},
    {"db_products", {"shard5"}}
};

solver.addPartition("database", shard_groups);
```

</TabItem>
</Tabs>

Now you can constrain at the database level:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
from rebalancer.specs import ConstraintSpec, GroupCountSpec, Limit

# Max 2 shards from same database per host
solver.add_constraint(
    ConstraintSpec(
        groupCountSpec=GroupCountSpec(
            name="database-diversity",
            scope="host",
            partitionName="database",
            limit=Limit(type="ABSOLUTE", globalLimit=2.0),
        )
    )
)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Max 2 shards from same database per host
GroupCountSpec dbDiversity;
dbDiversity.name() = "database-diversity";
dbDiversity.scope() = "host";
dbDiversity.partition() = "database";
dbDiversity.limit() = Limit();
dbDiversity.limit()->globalLimit() = 2.0;
solver.addConstraint(dbDiversity);
```

</TabItem>
</Tabs>

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
# Define a partition using a group-to-objects mapping
solver.add_partition(
    partition_name="database",
    group_to_objects={
        "db_users": ["shard0", "shard1", "shard2"],
        "db_orders": ["shard3", "shard4"],
        "db_products": ["shard5"],
    },
)

# If you have an object-to-group mapping, convert it first:
object_to_group = {
    "shard0": "db_users",
    "shard1": "db_users",
    "shard2": "db_users",
    "shard3": "db_orders",
    "shard4": "db_orders",
    "shard5": "db_products",
}

# Convert to group-to-objects:
from collections import defaultdict

group_to_objects: dict[str, list[str]] = defaultdict(list)
for obj, group in object_to_group.items():
    group_to_objects[group].append(obj)

solver.add_partition("database", dict(group_to_objects))
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
std::map<std::string, std::vector<std::string>> groupToObjects = {
    {"db_users", {"shard0", "shard1", "shard2"}},
    {"db_orders", {"shard3", "shard4"}},
    {"db_products", {"shard5"}}
};
solver.addPartition("database", groupToObjects);
```

</TabItem>
</Tabs>

### Multiple Partitions

Objects can belong to multiple partitions simultaneously:

```python
# Partition by database
solver.add_partition("database", shards_by_db)

# Partition by tenant
solver.add_partition("tenant", shards_by_tenant)

# Partition by priority
solver.add_partition("priority", shards_by_priority)
```

Each partition enables different constraints:

```python
from rebalancer.specs import (
    ColocateGroupsSpec,
    ConstraintSpec,
    GoalSpec,
    GroupCountSpec,
    Limit,
)

# Diversity by database
solver.add_constraint(
    ConstraintSpec(
        groupCountSpec=GroupCountSpec(
            name="db-diversity",
            scope="host",
            partitionName="database",
            limit=Limit(type="ABSOLUTE", globalLimit=1.0),
        )
    )
)

# Colocation by tenant (keep tenant shards together)
solver.add_goal(
    GoalSpec(
        colocateGroupsSpec=ColocateGroupsSpec(
            name="tenant-colocation",
            scope="host",
            partitionName="tenant",
        )
    ),
    weight=1.0,
)

# Limit high-priority tasks per host
solver.add_constraint(
    ConstraintSpec(
        groupCountSpec=GroupCountSpec(
            name="priority-limit",
            scope="host",
            partitionName="priority",
            limit=Limit(type="ABSOLUTE", globalLimit=2.0),
        )
    )
)
```

## Combining Scopes and Partitions

The real power comes from using both together.

### Example: Datacenter Diversity

**Goal**: Each database should have shards in at least 2 datacenters.

```python
from rebalancer.specs import ConstraintSpec, GroupCountSpec, Limit

# Scope: group hosts by datacenter
solver.add_scope("datacenter", host_to_dc)

# Partition: group shards by database
solver.add_partition("database", shards_by_db)

# Constraint: Each database should span >= 2 DCs.
# This means: max (total_shards - 1) shards per DC. If a database has 3
# shards, max 2 per DC forces it to span multiple DCs.
solver.add_constraint(
    ConstraintSpec(
        groupCountSpec=GroupCountSpec(
            name="dc-diversity",
            scope="datacenter",
            partitionName="database",
            # Max 2 shards from same DB per DC
            limit=Limit(type="ABSOLUTE", globalLimit=2.0),
        )
    )
)
```

### Example: Rack-Aware Balancing

**Goal**: Balance CPU across racks, not just hosts.

```python
from rebalancer.specs import BalanceSpec, GoalSpec

# Scope: group hosts by rack
solver.add_scope("rack", host_to_rack)

# Goal: balance CPU across racks
solver.add_goal(
    GoalSpec(
        balanceSpec=BalanceSpec(
            name="balance-racks", scope="rack", dimension="cpu"
        )
    ),
    weight=1.0,
)
```

Rebalancer will aggregate CPU usage from all hosts in each rack, then balance across racks.

## Scope Dimensions

You can add dimensions directly to scope items:

```python
from rebalancer.specs import CapacitySpec, ConstraintSpec

# Define rack scope
solver.add_scope("rack", host_to_rack)

# Add dimensions to racks (not hosts)
solver.add_scope_dimension(
    dimension_name="rack_power_budget",
    scope_name="rack",
    scope_item_to_value={
        "rack0": 5000.0,  # 5kW budget
        "rack1": 7000.0,
        "rack2": 5000.0,
    },
)

# Now you can use this in constraints
solver.add_constraint(
    ConstraintSpec(
        capacitySpec=CapacitySpec(
            name="power-budget",
            scope="rack",
            dimension="power_usage",  # Object dimension
            # Compared against rack_power_budget
        )
    )
)
```

**Use case**: Capacity limits that apply to scope items, not individual containers.

## Common Patterns

### Pattern 1: Datacenter Hierarchy

```python
from rebalancer.specs import BalanceSpec, GoalSpec

# Three levels: DC > Rack > Host
solver.add_scope("datacenter", {
    "host0": "us-west", "host1": "us-west",
    "host2": "us-east", "host3": "us-east",
})

solver.add_scope("rack", {
    "host0": "rack-w1", "host1": "rack-w2",
    "host2": "rack-e1", "host3": "rack-e2",
})

# Balance at DC level (coarse)
solver.add_goal(
    GoalSpec(
        balanceSpec=BalanceSpec(name="dc-cpu", scope="datacenter", dimension="cpu")
    ),
    weight=0.5,
)

# Balance at rack level (finer)
solver.add_goal(
    GoalSpec(
        balanceSpec=BalanceSpec(name="rack-cpu", scope="rack", dimension="cpu")
    ),
    weight=0.8,
)

# Balance at host level (finest)
solver.add_goal(
    GoalSpec(
        balanceSpec=BalanceSpec(name="host-cpu", scope="host", dimension="cpu")
    ),
    weight=1.0,
)
```

### Pattern 2: Failure Domain Diversity

```python
from rebalancer.specs import ConstraintSpec, GroupCountSpec, Limit

# Partition: replicas of same data
solver.add_partition("replica_group", {
    "data_A": ["replica_A_0", "replica_A_1", "replica_A_2"],
    "data_B": ["replica_B_0", "replica_B_1"],
})

# Scope: failure domains (racks)
solver.add_scope("rack", host_to_rack)

# Constraint: No two replicas of same data on same rack
solver.add_constraint(
    ConstraintSpec(
        groupCountSpec=GroupCountSpec(
            name="replica-diversity",
            scope="rack",
            partitionName="replica_group",
            # Max 1 replica per rack
            limit=Limit(type="ABSOLUTE", globalLimit=1.0),
        )
    )
)
```

### Pattern 3: Colocation by Service

```python
from rebalancer.specs import ColocateGroupsSpec, GoalSpec

# Partition: group tasks by service
solver.add_partition("service", tasks_by_service)

# Goal: Keep tasks from same service together
solver.add_goal(
    GoalSpec(
        colocateGroupsSpec=ColocateGroupsSpec(
            name="service-colocation",
            scope="host",
            partitionName="service",
        )
    ),
    weight=1.0,
)
```

### Pattern 4: Multi-Level Capacity

```python
from rebalancer.specs import CapacitySpec, ConstraintSpec

# Capacity at host level
solver.add_constraint(
    ConstraintSpec(
        capacitySpec=CapacitySpec(name="host-cpu", scope="host", dimension="cpu")
    )
)

# Capacity at rack level
solver.add_constraint(
    ConstraintSpec(
        capacitySpec=CapacitySpec(name="rack-cpu", scope="rack", dimension="cpu")
    )
)

# Capacity at datacenter level
solver.add_constraint(
    ConstraintSpec(
        capacitySpec=CapacitySpec(name="dc-cpu", scope="datacenter", dimension="cpu")
    )
)
```

Each level enforces its own limit. An object's contribution is counted at all levels.

## Scope vs. Partition Summary

| Aspect | Scopes | Partitions |
|--------|--------|------------|
| **Groups** | Containers | Objects |
| **Purpose** | Apply constraints/goals at different granularities | Constrain/optimize related objects |
| **Example** | Racks, datacenters, host types | Databases, tenants, replica groups |
| **Use in** | Almost all specs (scope parameter) | Group-related specs (partition parameter) |
| **Multiple** | Yes - multiple independent scopes | Yes - multiple independent partitions |

## Next Steps

- Learn about [Groups](groups) for advanced object relationships
- See [GroupCountSpec](../reference/groups/group-count) for diversity constraints
- See [ColocateGroupsSpec](../reference/groups/colocate-groups) for colocation
- Check [Disaster Recovery cookbook](../cookbook/disaster-recovery) for practical examples

## Related Concepts

- [Objects and Containers](objects-and-containers) - What scopes and partitions organize
- [Dimensions](dimensions) - Aggregated via scopes
- [Groups](groups) - Related to partitions
- [GroupCountSpec](../reference/groups/group-count) - Uses partitions
- [BalanceSpec](../reference/balance-optimize/balance) - Uses scopes
