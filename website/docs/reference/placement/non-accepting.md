---
sidebar_position: 16
---

# NonAcceptingSpec

**Type**: Constraint only

Prevent all new object placements on specified containers.

## Overview

`NonAcceptingSpec` blocks containers from accepting any new objects (objects not currently assigned to them). Existing objects can stay, but no new objects can be placed. This is useful for draining hosts gradually or preventing new load on overloaded containers.

**Use this when**: You want to prevent new placements on containers while keeping existing assignments intact, typically during graceful shutdown or capacity management.

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/placement/non_accepting_spec_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/placement/non_accepting_spec_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Unique identifier for this constraint |
| `scope` | string | Yes | - | Scope level (e.g., `"host"`, `"rack"`) |
| `items` | list&lt;string&gt; | Yes | - | Container IDs that shouldn't accept new objects |

## Common Usage Patterns

### 1. Gradual Host Draining

Prevent new placements as first step in draining:

```python
# Step 1: Block new placements
solver.add_constraint(
        ConstraintSpec(
            nonAcceptingSpec=NonAcceptingSpec(
        name="block-new-placements",
        scope="host",
        items=hosts_to_drain,
    )
        )
)

# Step 2 (later): Actively drain existing objects
# solver.add_constraint(
        ConstraintSpec(
            toFreeSpec=ToFreeSpec(containers=hosts_to_drain)
        ))
```

### 2. Capacity Management

Prevent overloaded hosts from getting more objects:

```python
# Identify overloaded hosts (>90% utilization)
overloaded_hosts = [
    host for host, util in host_utilization.items()
    if util > 0.9
]

solver.add_constraint(
        ConstraintSpec(
            nonAcceptingSpec=NonAcceptingSpec(
        name="block-overloaded-hosts",
        scope="host",
        items=overloaded_hosts,
    )
        )
)
```

### 3. Maintenance Preparation

Freeze assignments before maintenance window:

```python
# Hosts entering maintenance in 24 hours
upcoming_maintenance = ["host3", "host7"]

# Don't add new load before maintenance
solver.add_constraint(
        ConstraintSpec(
            nonAcceptingSpec=NonAcceptingSpec(
        name="freeze-pre-maintenance",
        scope="host",
        items=upcoming_maintenance,
    )
        )
)
```

### 4. Failed or Degraded Hosts

Keep existing workloads but don't add new ones:

```python
# Hosts with degraded performance
degraded_hosts = get_hosts_with_failures()

solver.add_constraint(
        ConstraintSpec(
            nonAcceptingSpec=NonAcceptingSpec(
        name="no-new-on-degraded",
        scope="host",
        items=degraded_hosts,
    )
        )
)
```

### 5. Rack-Level Blocking

Block entire racks from new placements:

```python
# Rack scheduled for network maintenance
solver.add_constraint(
        ConstraintSpec(
            nonAcceptingSpec=NonAcceptingSpec(
        name="block-rack-maintenance",
        scope="rack",
        items=["rack2", "rack5"],
    )
        )
)
```

### 6. Decommissioning Pipeline

Gradually decommission hosts over time:

```python
def decommission_pipeline(hosts, stages=3):
    """
    Stage 1: NonAccepting (keep existing, block new)
    Stage 2: ToFree (drain existing)
    Stage 3: Remove from cluster
    """
    hosts_per_stage = len(hosts) // stages

    # Stage 1 hosts: Block new placements
    stage1_hosts = hosts[0:hosts_per_stage]
    solver.add_constraint(
        ConstraintSpec(
            nonAcceptingSpec=NonAcceptingSpec(
            name="stage1-block-new",
            scope="host",
            items=stage1_hosts,
        )
        )
    )

    # Stage 2 hosts: Actively drain
    stage2_hosts = hosts[hosts_per_stage:2*hosts_per_stage]
    solver.add_constraint(
        ConstraintSpec(
            toFreeSpec=ToFreeSpec(
            name="stage2-drain",
            containers=stage2_hosts,
        )
        )
    )

    # Stage 3 hosts: Already drained, ready to remove
    # (not included in solver)
```

## How It Works

For each container in `items`:
- Objects currently on the container can stay
- No new objects (not currently assigned) can be placed
- Moving objects away is allowed

```python
# Example behavior
# Initial: host1 has [obj1, obj2], host2 has [obj3]
# NonAccepting: host1

# Allowed:
# - obj1 stays on host1 ✓
# - obj2 stays on host1 ✓
# - obj1 moves away from host1 ✓
# - obj2 moves away from host1 ✓

# Blocked:
# - obj3 moves to host1 ✗ (new placement)
```

## Difference from Related Specs

| Aspect | NonAcceptingSpec | ToFreeSpec | AvoidAssignmentsSpec |
|--------|-----------------|-----------|---------------------|
| **Blocks** | New objects only | All objects (drains) | Specific object→container pairs |
| **Existing objects** | Can stay | Must leave | Depends on spec |
| **Granularity** | Per-container | Per-container | Per-object, per-container |
| **Use case** | Gradual draining | Active draining | Incompatibility |

## Performance Considerations

- **Impact**: Minimal - simple constraint check
- **Solver compatibility**: Works with both Local Search and Optimal solvers
- **Scaling**: O(number of non-accepting containers)
- **Search space**: Reduces solution space

## Common Pitfalls

### 1. Expecting Active Draining

**Problem**: NonAccepting doesn't move existing objects.

```python
# BAD: Expecting host to be empty
solver.add_constraint(
        ConstraintSpec(
            nonAcceptingSpec=NonAcceptingSpec(items=["host1"])
        )
)
# Host1 still has existing objects!
```

**Solution**: Use ToFreeSpec for active draining:

```python
# GOOD: Actually drain the host
solver.add_constraint(
        ConstraintSpec(
            toFreeSpec=ToFreeSpec(containers=["host1"])
        )
)
```

### 2. Insufficient Capacity Elsewhere

**Problem**: Blocking too many hosts reduces capacity.

```python
# BAD: Block 80% of hosts
solver.add_constraint(
        ConstraintSpec(
            nonAcceptingSpec=NonAcceptingSpec(items=most_hosts)
        )
)
# Remaining 20% can't handle load!
```

**Solution**: Only block hosts with available capacity elsewhere.

### 3. Conflicting with Balance Goals

**Problem**: NonAccepting prevents rebalancing.

```python
# Can't achieve balance if can't move to certain hosts
solver.add_constraint(
        ConstraintSpec(
            nonAcceptingSpec=NonAcceptingSpec(items=half_of_hosts)
        )
)
solver.add_goal(
        GoalSpec(
            balanceSpec=BalanceSpec(scope="host", dimension="cpu")
        ),
    weight=1.0
)
# May achieve poor balance
```

**Solution**: Understand that NonAccepting constrains balance quality.

### 4. Wrong Scope Level

**Problem**: Specifying items at wrong scope level.

```python
# BAD: Items are hosts but scope is rack
solver.add_constraint(
        ConstraintSpec(
            nonAcceptingSpec=NonAcceptingSpec(
        scope="rack",
        items=["host1", "host2"],  # These are hosts!
    )
        )
)
```

**Solution**: Match scope and items:

```python
# GOOD: Scope and items match
solver.add_constraint(
        ConstraintSpec(
            nonAcceptingSpec=NonAcceptingSpec(
        scope="host",
        items=["host1", "host2"],
    )
        )
)
```

## Combining with Other Specs

### With ToFreeSpec (Two-Stage Draining)

```python
# Stage 1: Block new (NonAccepting)
solver.add_constraint(
        ConstraintSpec(
            nonAcceptingSpec=NonAcceptingSpec(
        name="stage1-freeze",
        scope="host",
        items=stage1_hosts,
    )
        )
)

# Stage 2: Active drain (ToFree)
solver.add_constraint(
        ConstraintSpec(
            toFreeSpec=ToFreeSpec(
        name="stage2-drain",
        containers=stage2_hosts,
    )
        )
)
```

### With CapacitySpec

```python
# Ensure non-accepting doesn't cause capacity issues

solver.add_constraint(
        ConstraintSpec(
            nonAcceptingSpec=NonAcceptingSpec(
        name="block-degraded",
        scope="host",
        items=degraded_hosts,
    )
        )
)

solver.add_constraint(
        ConstraintSpec(
            capacitySpec=CapacitySpec(
        name="capacity",
        scope="host",
        dimension="memory",
    )
        )
)
```

### With BalanceSpec

```python
# Balance across remaining accepting hosts

solver.add_constraint(
        ConstraintSpec(
            nonAcceptingSpec=NonAcceptingSpec(
        name="block-maintenance",
        scope="host",
        items=maintenance_hosts,
    )
        )
)

# Balance will work around non-accepting hosts
solver.add_goal(
        GoalSpec(
            balanceSpec=BalanceSpec(
        name="balance-accepting-hosts",
        scope="host",
        dimension="cpu",
    )
        ),
    weight=1.0
)
```

## Multi-Round Draining Example

Gradually drain hosts over multiple solver runs:

```python
def gradual_drain_over_rounds(hosts_to_drain, rounds=5):
    """Drain hosts gradually over multiple rounds."""
    hosts_per_round = len(hosts_to_drain) // rounds

    for round_num in range(rounds):
        solver = ProblemSolver(...)

        # Already drained (ToFree in previous rounds)
        already_drained = hosts_to_drain[0:round_num * hosts_per_round]

        # Currently draining (ToFree this round)
        drain_now = hosts_to_drain[
            round_num * hosts_per_round:
            (round_num + 1) * hosts_per_round
        ]

        # Will drain later (NonAccepting now)
        drain_later = hosts_to_drain[(round_num + 1) * hosts_per_round:]

        # Block new placements on future drain candidates
        if drain_later:
            solver.add_constraint(
        ConstraintSpec(
            nonAcceptingSpec=NonAcceptingSpec(
                    name="freeze-future-drains",
                    scope="host",
                    items=drain_later,
                )
        )
            )

        # Actively drain current round
        solver.add_constraint(
        ConstraintSpec(
            toFreeSpec=ToFreeSpec(
                name="drain-current-round",
                containers=drain_now,
            )
        )
        )

        solution = solver.solve()
        apply_moves(solution)

        print(f"Round {round_num + 1}:")
        print(f"  Drained: {len(drain_now)} hosts")
        print(f"  Frozen: {len(drain_later)} hosts")
        print(f"  Completed: {len(already_drained) + len(drain_now)} hosts")
```

## Verification Example

Check that non-accepting constraint is respected:

```python
def verify_non_accepting(initial_assignment, solution, non_accepting_hosts):
    """Verify non-accepting hosts didn't receive new objects."""
    violations = []

    for host in non_accepting_hosts:
        initial_objects = set(initial_assignment.get(host, []))
        final_objects = set(solution["assignment"].get(host, []))

        # Find new objects (in final but not in initial)
        new_objects = final_objects - initial_objects

        if new_objects:
            violations.append({
                "host": host,
                "new_objects": list(new_objects),
                "count": len(new_objects)
            })
            print(f"❌ {host} received {len(new_objects)} new objects: {new_objects}")
        else:
            removed = len(initial_objects - final_objects)
            print(f"✅ {host} accepted no new objects ({removed} removed)")

    return len(violations) == 0
```

## Related Specs

- [ToFreeSpec](../to-free) - Actively drain containers (next step after NonAccepting)
- [AvoidAssignmentsSpec](avoid-assignments) - Block specific object→container pairs
- [CapacitySpec](../capacity) - Enforce capacity limits

## Source Code

- Thrift definition: `interface/thrift/ProblemSpecs.thrift:918`
- Implementation: `solver/constraints/NonAccepting.cpp`
