---
sidebar_position: 5
---

# Groups

Groups are collections of related objects with special semantics. They extend the concept of partitions with additional features for movement coordination and relationship modeling.

## Groups vs. Partitions

**Partitions** (covered in [Scopes and Partitions](scopes-and-partitions)) simply categorize objects. **Groups** add additional semantics:

| Aspect | Partitions | Groups |
|--------|------------|--------|
| **Basic purpose** | Categorize objects | Relate objects with special behavior |
| **Created by** | `addPartition()` | `addPartition()` + special specs |
| **Examples** | Database, tenant, priority | Move groups, replica sets, affinity groups |
| **Special features** | None | Coordinated movement, colocation, diversity |

In practice, groups are often implemented using partitions, but with specialized specs that understand group semantics.

## Types of Groups

### 1. Move Groups

**Move groups** contain objects that must move together as a unit.

#### Use Case

- Database primary + replicas must move together
- Service components that depend on each other
- Related tasks that should migrate together

#### API

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
# Define which objects belong to move groups
solver.add_partition("move_group", {
    "group_A": ["obj1", "obj2", "obj3"],  # These move together
    "group_B": ["obj4", "obj5"]           # These move together
})

# Enforce move group semantics
solver.add_constraint(
    {
        "moveGroupSpec": {
            "name": "coordinated-moves",
            "partitionName": "move_group",
        }
    }
)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Define which objects belong to move groups
std::map<std::string, std::vector<std::string>> move_groups = {
    {"group_A", {"obj1", "obj2", "obj3"}},
    {"group_B", {"obj4", "obj5"}}
};
solver.addPartition("move_group", move_groups);

// Enforce move group semantics
MoveGroupSpec moveGroupSpec;
moveGroupSpec.name() = "coordinated-moves";
moveGroupSpec.partitionName() = "move_group";
solver.addConstraint(moveGroupSpec);
```

</TabItem>
</Tabs>

#### Behavior

If `obj1` moves, then `obj2` and `obj3` must also move to the same destination. The entire group moves atomically.

### 2. Colocation Groups

**Colocation groups** contain objects that should be placed together on the same container.

#### Use Case

- Microservices that communicate frequently
- Cache and application on same host
- Data locality requirements

#### API

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
# Define colocation groups
solver.add_partition("colocation_group", {
    "service_A": ["api_server", "cache", "worker"],
    "service_B": ["db_primary", "db_replica"]
})

# Goal: minimize split groups (prefer colocation)
solver.add_goal(
    {
        "colocateGroupsSpec": {
            "name": "keep-services-together",
            "scope": "host",
            "partitionName": "colocation_group",
        }
    },
    weight=1.0,
)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Define colocation groups
std::map<std::string, std::vector<std::string>> colocation_groups = {
    {"service_A", {"api_server", "cache", "worker"}},
    {"service_B", {"db_primary", "db_replica"}}
};
solver.addPartition("colocation_group", colocation_groups);

// Goal: minimize split groups (prefer colocation)
ColocateGroupsSpec colocateSpec;
colocateSpec.name() = "keep-services-together";
colocateSpec.scope() = "host";
colocateSpec.partition() = "colocation_group";
solver.addGoal(colocateSpec, 1.0);
```

</TabItem>
</Tabs>

#### Behavior

Rebalancer tries to place all objects from the same group on the same container. This is a soft goal - can be violated if necessary.

For hard colocation (must be together):

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
solver.add_constraint(
    {
        "colocateGroupsSpec": {
            "name": "must-colocate",
            "scope": "host",
            "partitionName": "colocation_group",
        }
    }
)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
ColocateGroupsSpec mustColocate;
mustColocate.name() = "must-colocate";
mustColocate.scope() = "host";
mustColocate.partition() = "colocation_group";
solver.addConstraint(mustColocate);
```

</TabItem>
</Tabs>

### 3. Diversity/Anti-Affinity Groups

**Diversity groups** contain objects that should be spread apart (opposite of colocation).

#### Use Case

- Replicas of same data (fault tolerance)
- Avoid single point of failure
- Load distribution across failure domains

#### API

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
# Define replica groups
solver.add_partition("replica_group", {
    "data_X": ["replica_X_0", "replica_X_1", "replica_X_2"],
    "data_Y": ["replica_Y_0", "replica_Y_1"]
})

# Constraint: Max 1 replica per rack
solver.add_constraint(
    {
        "groupCountSpec": {
            "name": "replica-diversity",
            "scope": "rack",
            "partitionName": "replica_group",
            "limit": {"type": "ABSOLUTE", "globalLimit": 1.0},
        }
    }
)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Define replica groups
std::map<std::string, std::vector<std::string>> replica_groups = {
    {"data_X", {"replica_X_0", "replica_X_1", "replica_X_2"}},
    {"data_Y", {"replica_Y_0", "replica_Y_1"}}
};
solver.addPartition("replica_group", replica_groups);

// Constraint: Max 1 replica per rack
GroupCountSpec replicaDiversity;
replicaDiversity.name() = "replica-diversity";
replicaDiversity.scope() = "rack";
replicaDiversity.partition() = "replica_group";
replicaDiversity.limit() = Limit();
replicaDiversity.limit()->globalLimit() = 1.0;
solver.addConstraint(replicaDiversity);
```

</TabItem>
</Tabs>

#### Behavior

No more than 1 object from each group on the same rack. Ensures replicas are spread across failure domains.

## Group-Based Dimensions

Some dimensions are defined at the group level rather than per-object.

### Dynamic Dimensions by Partition

```python
# All shards in same database have same priority
solver.add_dynamic_object_dimension_by_partition(
    dimension_name="priority",
    scope="host",
    partition_name="database",
    scope_item_to_group_to_value={
        "host_west": {"db_users": 10.0, "db_orders": 5.0},
        "host_east": {"db_users": 8.0, "db_orders": 6.0},
    },
)
```

**Use case**: When all members of a group share a property.

## Group Constraints and Goals

### Limit Group Presence

```python
# Max 3 shards from same database per host
solver.add_constraint(
    {
        "groupCountSpec": {
            "name": "max-3-per-db",
            "scope": "host",
            "partitionName": "database",
            "limit": {"type": "ABSOLUTE", "globalLimit": 3.0},
        }
    }
)
```

### Capacity with Group Presence

Special capacity constraint where groups have minimum weight even if small:

```python
# Each database group contributes at least 10 units of presence weight
# to its host, even if only 1 shard lives there.
solver.add_constraint(
    {
        "capacityWithGroupPresenceSpec": {
            "name": "group-aware-capacity",
            "scope": "host",
            "dimension": "load",
            "partition": "database",
            "groupToPresenceWeight": {
                "type": "ABSOLUTE",
                "globalLimit": 10.0,
            },
        }
    }
)
```

**Use case**: Groups have overhead beyond their object count (e.g., each database has metadata overhead).

### Group Move Limits

```python
# Limit how many objects from each group can move
solver.add_constraint(
    {
        "groupMoveLimitSpec": {
            "name": "limit-moves-per-db",
            "partitionName": "database",
            # Max 5 shards per database can move
            "limit": {"type": "ABSOLUTE", "globalLimit": 5.0},
        }
    }
)
```

**Use case**: Limit disruption per group (e.g., don't move more than 5 shards from any one database).

## Affinity Groups

Groups that have preferences (not hard requirements) for placement.

### Group Assignment Affinities

```python
# Prefer placing certain databases on certain racks
# Note: Using "shard_count" assumes set_object_name("shard") was called
solver.add_goal(
    {
        "groupAssignmentAffinitiesSpec": {
            "name": "db-affinity",
            "scope": "rack",
            "partition": "database",
            "dimension": "shard_count",
            "affinities": [
                {
                    "group": "db_users",
                    "scopeItem": "rack_ssd",
                    # 80% of db_users shards on SSD rack
                    "targetDimensionValue": 0.8,
                    "affinity": 10.0,  # Strength
                }
            ],
        }
    },
    weight=1.0,
)
```

**Use case**: Soft placement preferences (e.g., prefer SSD racks for high-throughput databases).

### Pair Affinities

Pairs of objects that prefer to be together:

```python
solver.add_goal(
    {
        "pairAffinitiesSpec": {
            "name": "pair-affinity",
            "scope": "host",
            "affinities": [
                {
                    "pair": {"object1": "obj1", "object2": "obj2"},
                    "affinity": 10.0,  # Strength of preference
                },
                {
                    "pair": {"object1": "obj3", "object2": "obj4"},
                    "affinity": 5.0,
                },
            ],
        }
    },
    weight=1.0,
)
```

**Use case**: Objects that communicate frequently should be colocated to reduce network latency.

## Common Patterns

### Pattern 1: Replica Placement

```python
# Partition: replica groups
solver.add_partition("replica_group", {
    "data_1": ["replica_1a", "replica_1b", "replica_1c"],
    "data_2": ["replica_2a", "replica_2b", "replica_2c"],
})

# Scope: failure domains (racks)
solver.add_scope("rack", host_to_rack)

# Constraint: 1 replica per rack per data item
solver.add_constraint(
    {
        "groupCountSpec": {
            "name": "one-replica-per-rack",
            "scope": "rack",
            "partitionName": "replica_group",
            "limit": {"type": "ABSOLUTE", "globalLimit": 1.0},
        }
    }
)

# Goal: balance replicas across racks
# Note: Using "replica_count" assumes set_object_name("replica") was called
solver.add_goal(
    {
        "balanceSpec": {
            "name": "balance-replicas",
            "scope": "rack",
            "dimension": "replica_count",
        }
    },
    weight=1.0,
)
```

### Pattern 2: Service Colocation with Affinity

```python
# Partition: service components
solver.add_partition("service", {
    "service_A": ["api_A", "cache_A", "worker_A"],
    "service_B": ["api_B", "cache_B", "worker_B"],
})

# Soft goal: colocate service components
solver.add_goal(
    {
        "colocateGroupsSpec": {
            "name": "colocate-services",
            "scope": "host",
            "partitionName": "service",
        }
    },
    weight=5.0,
)

# But also balance across hosts
solver.add_goal(
    {
        "balanceSpec": {
            "name": "balance-cpu",
            "scope": "host",
            "dimension": "cpu",
        }
    },
    weight=1.0,
)

# Colocation is 5x more important than balance
```

### Pattern 3: Gradual Migration with Move Limits

```python
# Partition: by database
solver.add_partition("database", shard_to_db)

# Limit moves per database (gradual migration)
solver.add_constraint(
    {
        "groupMoveLimitSpec": {
            "name": "limit-per-db",
            "partitionName": "database",
            # Max 10 shards per DB
            "limit": {"type": "ABSOLUTE", "globalLimit": 10.0},
        }
    }
)

# Also limit total moves
solver.add_goal(
    {"minimizeMovementSpec": {"name": "minimize-total-moves"}},
    weight=1.0,
)
```

## Group Naming Conventions

### Best Practices

1. **Descriptive names**: `database`, `replica_group`, `service`
2. **Group IDs should be meaningful**: `db_users`, `replica_data_X`, `service_payment`
3. **Consistent prefixes**: `db_*`, `svc_*`, `tenant_*`
4. **Hierarchical when appropriate**: `region_us_west`, `region_eu_central`

## Next Steps

- See [GroupCountSpec](../reference/groups/group-count) for diversity constraints
- See [ColocateGroupsSpec](../reference/groups/colocate-groups) for colocation goals
- See [GroupMoveLimitSpec](../reference/groups/group-move-limit) for move limits
- Check [Disaster Recovery cookbook](../cookbook/disaster-recovery) for replica placement example

## Related Concepts

- [Scopes and Partitions](scopes-and-partitions) - Foundation for groups
- [Dimensions](dimensions) - Can be group-based
- [GroupCountSpec](../reference/groups/group-count) - Diversity constraints
- [ColocateGroupsSpec](../reference/groups/colocate-groups) - Colocation
- [MoveGroupSpec](../reference/groups/move-group) - Coordinated movement
