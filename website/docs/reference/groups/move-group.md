---
sidebar_position: 17
---

# MoveGroupSpec

**Type**: Constraint only

Move all objects in the same partition group together to the same container.

## Overview

`MoveGroupSpec` ensures that when any object in a partition group moves, all objects in that group move together to the same destination container. This creates "atomic" group moves - either the entire group moves or nothing moves.

**Use this when**: You have tightly coupled objects that must stay together (e.g., all replicas of a shard, coordinated service instances, database partitions).

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/reference/groups/move_group_spec_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/groups/move_group_spec_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Unique identifier for this constraint |
| `partitionName` | string | Yes | - | Partition defining the groups |

## How It Works

For each group in the partition:
- If **any** object in the group moves, **all** objects in the group must move to the **same** destination container
- If **no** objects in the group move, all stay where they are

**Example**:
```
Partition "shard": {
  "shard_0": ["replica_0_a", "replica_0_b", "replica_0_c"],
  "shard_1": ["replica_1_a", "replica_1_b", "replica_1_c"],
}

Initial:
  host1: [replica_0_a, replica_0_b]
  host2: [replica_0_c]
  host3: [replica_1_a, replica_1_b, replica_1_c]

With MoveGroupSpec(partitionName="shard"):
- If replica_0_a moves, replica_0_b and replica_0_c MUST move to same destination
- shard_1 can stay on host3 (all together already)
```

## Common Usage Patterns

### 1. Replica Set Cohesion

Keep database replicas together:

```python
# Partition by replica set
replica_sets = {
    "db_shard_0": ["shard_0_primary", "shard_0_replica_1", "shard_0_replica_2"],
    "db_shard_1": ["shard_1_primary", "shard_1_replica_1", "shard_1_replica_2"],
    "db_shard_2": ["shard_2_primary", "shard_2_replica_1", "shard_2_replica_2"],
}
solver.add_partition("replica_set", replica_sets)

# Move replicas together (for migration, not anti-affinity!)
solver.add_constraint(
        ConstraintSpec(
            moveGroupSpec=MoveGroupSpec(
        name="move-replicas-together",
        partitionName="replica_set",
    )
        )
)
```

**Use case**: Migrating entire replica sets between datacenters atomically.

### 2. Service Instance Cohesion

Move all instances of a service together:

```python
# Partition by service
services = {
    "frontend": ["frontend_0", "frontend_1", "frontend_2"],
    "backend": ["backend_0", "backend_1", "backend_2", "backend_3"],
    "cache": ["cache_0", "cache_1"],
}
solver.add_partition("service", services)

# If rebalancing any service, move all its instances together
solver.add_constraint(
        ConstraintSpec(
            moveGroupSpec=MoveGroupSpec(
        name="service-cohesion",
        partitionName="service",
    )
        )
)
```

**Use case**: Migrating services wholesale to new infrastructure.

### 3. Batch Job Cohesion

Keep distributed batch job tasks together:

```python
# Partition by job ID
batch_jobs = {
    "job_123": ["task_123_0", "task_123_1", "task_123_2", "task_123_3"],
    "job_456": ["task_456_0", "task_456_1"],
}
solver.add_partition("job", batch_jobs)

# Move job tasks together (e.g., for network locality)
solver.add_constraint(
        ConstraintSpec(
            moveGroupSpec=MoveGroupSpec(
        name="job-task-cohesion",
        partitionName="job",
    )
        )
)
```

### 4. Gradual Service Migration

Migrate services one at a time:

```python
# With MoveGroupSpec, solver will either:
# - Move entire service to new datacenter, OR
# - Leave entire service in old datacenter
#
# Won't split services across datacenters during migration

solver.add_constraint(
        ConstraintSpec(
            moveGroupSpec=MoveGroupSpec(
        name="atomic-service-migration",
        partitionName="service",
    )
        )
)

# Combine with ToFreeSpec to drain old datacenter
old_dc_hosts = ["old_dc_host_1", "old_dc_host_2"]
solver.add_constraint(
        ConstraintSpec(
            toFreeSpec=ToFreeSpec(
        name="drain-old-dc",
        containers=old_dc_hosts,
    )
        )
)

# Result: Services migrate one-by-one (all instances together) from old to new DC
```

### 5. Multi-Tier Application

Keep application tiers together:

```python
# Partition by application instance
applications = {
    "app_instance_0": ["web_0", "app_0", "db_0"],  # Web + app + db for instance 0
    "app_instance_1": ["web_1", "app_1", "db_1"],
}
solver.add_partition("app_instance", applications)

# Move application tiers together
solver.add_constraint(
        ConstraintSpec(
            moveGroupSpec=MoveGroupSpec(
        name="tier-cohesion",
        partitionName="app_instance",
    )
        )
)
```

### 6. Stateful Sets

Kubernetes stateful set-like behavior:

```python
# Partition by stateful set
stateful_sets = {
    "zookeeper": ["zk_0", "zk_1", "zk_2"],
    "kafka": ["kafka_0", "kafka_1", "kafka_2", "kafka_3"],
}
solver.add_partition("stateful_set", stateful_sets)

solver.add_constraint(
        ConstraintSpec(
            moveGroupSpec=MoveGroupSpec(
        name="stateful-set-cohesion",
        partitionName="stateful_set",
    )
        )
)
```

### 7. Workload Bundling

Bundle related workloads for network efficiency:

```python
# Partition by workload bundle
bundles = {
    "ml_training_bundle_0": ["trainer_0", "parameter_server_0", "evaluator_0"],
    "ml_training_bundle_1": ["trainer_1", "parameter_server_1", "evaluator_1"],
}
solver.add_partition("bundle", bundles)

solver.add_constraint(
        ConstraintSpec(
            moveGroupSpec=MoveGroupSpec(
        name="bundle-cohesion",
        partitionName="bundle",
    )
        )
)
```

## Important Notes

### Not for Anti-Affinity!

**Common mistake**: Using MoveGroupSpec to keep replicas on same host for *normal operation*.

```python
# WRONG: Don't use MoveGroupSpec for anti-affinity
# This moves replicas TOGETHER, not APART!
solver.add_constraint(
        ConstraintSpec(
            moveGroupSpec=MoveGroupSpec(partitionName="replica_set")
        )  # BAD for fault tolerance!
)
```

**For anti-affinity** (spreading replicas), use [GroupCountSpec](group-count):

```python
# CORRECT: Spread replicas across hosts
solver.add_constraint(
        ConstraintSpec(
            groupCountSpec=GroupCountSpec(
        name="replica-anti-affinity",
        scope="host",
        partitionName="replica_set",
        bound=GroupCountSpecBound.MAX,
        limit=Limit(globalLimit=1),  # Max 1 replica per host
    )
        )
)
```

**MoveGroupSpec is for**: Migration scenarios where you want to move groups atomically, not day-to-day replica placement.

## Performance Considerations

- **Impact**: Moderate - constrains move decisions significantly
- **Solver compatibility**: Works with both Local Search and Optimal solvers
- **Scaling**: O(number of groups)
- **Warning**: Can make problems harder to solve (reduces solution space)

## Common Pitfalls

### 1. Conflicting with Anti-Affinity

**Problem**: MoveGroupSpec wants objects together, but other constraints want them apart.

```python
# BAD: Conflicting constraints
solver.add_constraint(
        ConstraintSpec(
            moveGroupSpec=MoveGroupSpec(partitionName="replica_set")
        )  # Want together
)

solver.add_constraint(
        ConstraintSpec(
            groupCountSpec=GroupCountSpec(
        partitionName="replica_set",
        bound=GroupCountSpecBound.MAX,
        limit=Limit(globalLimit=1),  # Want apart!
    )
        )
)
# Can't satisfy both!
```

**Solution**: Only use MoveGroupSpec when you truly want atomic moves:

```python
# GOOD: Use MoveGroupSpec only for migration scenarios
# During migration: move replicas together
solver.add_constraint(
        ConstraintSpec(
            moveGroupSpec=MoveGroupSpec(partitionName="replica_set")
        )
)

# After migration: spread replicas (different solver run)
solver.add_constraint(
        ConstraintSpec(
            groupCountSpec=GroupCountSpec(
        partitionName="replica_set",
        bound=GroupCountSpecBound.MAX,
        limit=Limit(globalLimit=1),
    )
        )
)
```

### 2. Partition Not Defined

**Problem**: Partition doesn't exist.

```python
# BAD: Forgot to define partition
solver.add_constraint(
        ConstraintSpec(
            moveGroupSpec=MoveGroupSpec(partitionName="service")
        )  # Error: partition doesn't exist!
)
```

**Solution**: Define partition first:

```python
# GOOD: Define partition
solver.add_partition("service", service_groups)

solver.add_constraint(
        ConstraintSpec(
            moveGroupSpec=MoveGroupSpec(partitionName="service")
        )
)
```

### 3. Groups Too Large

**Problem**: Groups are too large, making moves expensive.

```python
# BAD: Group has 100 objects, moving all together is expensive
large_group = {
    "mega_service": [f"instance_{i}" for i in range(100)]
}
solver.add_partition("service", large_group)

solver.add_constraint(
        ConstraintSpec(
            moveGroupSpec=MoveGroupSpec(partitionName="service")
        )
)
# Moving any instance forces moving all 100!
```

**Solution**: Create smaller, more granular groups:

```python
# GOOD: Smaller logical groups
groups = {
    f"service_shard_{i}": [f"instance_{i}_{j}" for j in range(5)]
    for i in range(20)
}
# Now each group has only 5 instances
```

### 4. Overconstrained Problem

**Problem**: Too many move group constraints make problem infeasible.

```python
# BAD: Everything must move together
solver.add_constraint(
        ConstraintSpec(
            moveGroupSpec=MoveGroupSpec(partitionName="partition1")
        ))
solver.add_constraint(
        ConstraintSpec(
            moveGroupSpec=MoveGroupSpec(partitionName="partition2")
        ))
solver.add_constraint(
        ConstraintSpec(
            moveGroupSpec=MoveGroupSpec(partitionName="partition3")
        ))
# May be impossible to satisfy all while respecting capacity
```

**Solution**: Use sparingly, only where truly necessary.

## Combining with Other Specs

### With ToFreeSpec

Atomic migration when draining hosts:

```python
# Drain hosts
solver.add_constraint(
        ConstraintSpec(
            toFreeSpec=ToFreeSpec(
        name="drain-old-hosts",
        containers=["old_host_1", "old_host_2"],
    )
        )
)

# Move services atomically (all instances together)
solver.add_constraint(
        ConstraintSpec(
            moveGroupSpec=MoveGroupSpec(
        name="atomic-service-moves",
        partitionName="service",
    )
        )
)
```

### With MinimizeMovementSpec

Prefer not moving, but when you do, move groups together:

```python
# Soft: Minimize movement
solver.add_goal(
        GoalSpec(
            minimizeMovementSpec=MinimizeMovementSpec(
        name="prefer-minimal-moves",
        dimension="data_size",
    )
        ),
    weight=2.0
)

# Hard: If moving, move groups together
solver.add_constraint(
        ConstraintSpec(
            moveGroupSpec=MoveGroupSpec(
        name="atomic-moves",
        partitionName="service",
    )
        )
)
```

## Verification Example

Verify groups moved together:

```python
def verify_move_group(initial, final, partition):
    """Verify groups moved atomically.

    Args:
        initial: Initial assignment
        final: Final assignment
        partition: Partition dict (group -> objects)

    Returns:
        True if all groups moved atomically
    """
    violations = []

    for group, objects in partition.items():
        # Find destination of each object
        destinations = set()
        for obj in objects:
            initial_host = find_host(obj, initial)
            final_host = find_host(obj, final)
            destinations.add(final_host)

        # All objects should be on same host
        if len(destinations) > 1:
            violations.append({
                'group': group,
                'destinations': destinations,
                'object_count': len(objects),
            })

    if not violations:
        print(f"✅ All groups moved atomically")
        return True
    else:
        print(f"❌ {len(violations)} groups split across containers:")
        for v in violations[:5]:
            print(f"   {v['group']}: {v['object_count']} objects on {len(v['destinations'])} containers")
        return False

def find_host(obj, assignment):
    """Find which host has object."""
    for host, objects in assignment.items():
        if obj in objects:
            return host
    return None
```

## Related Specs

- [GroupMoveLimitSpec](group-move-limit) - Limit moves per group
- [GroupCountSpec](group-count) - Spread groups across containers (opposite intent)
- [ColocateGroupsSpec](colocate-groups) - Prefer groups on same/different containers (soft)

## Source Code

- Thrift definition: `interface/thrift/ProblemSpecs.thrift:871` (MoveGroupSpec)
- Implementation: `solver/constraints/MoveGroup.cpp`
