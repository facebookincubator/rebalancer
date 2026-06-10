---
sidebar_position: 11
---

# AvoidMovingSpec

**Type**: Constraint only

Prevent specific objects from being moved from their current containers.

## Overview

`AvoidMovingSpec` explicitly pins objects to their current assignments, ensuring they won't move during rebalancing. This is a hard constraint - the solver will fail if it can't satisfy capacity/balance goals without moving these objects.

**Use this when**: You have specific objects that must not move due to operational constraints, ongoing operations, or SLAs.

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=avoid_moving_quick_example_start end=avoid_moving_quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=avoid_moving_quick_example_start end=avoid_moving_quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Unique identifier for this constraint |
| `objects` | list&lt;string&gt; | Yes | - | List of object IDs that must not move |

## Common Usage Patterns

### 1. Pin Critical Objects

Prevent moving objects with strict SLAs:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=pin_critical_objects_start end=pin_critical_objects_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=pin_critical_objects_start end=pin_critical_objects_end
```

</TabItem>
</Tabs>

### 2. Pin Objects with Ongoing Operations

Prevent moving objects currently being migrated or under maintenance:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=pin_ongoing_operations_start end=pin_ongoing_operations_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=pin_ongoing_operations_start end=pin_ongoing_operations_end
```

</TabItem>
</Tabs>

### 3. Pin Recently Moved Objects

Avoid moving objects that were recently relocated:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=pin_recently_moved_start end=pin_recently_moved_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=pin_recently_moved_start end=pin_recently_moved_end
```

</TabItem>
</Tabs>

### 4. Gradual Rebalancing

Incrementally rebalance by pinning most objects:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=gradual_rebalancing_start end=gradual_rebalancing_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=gradual_rebalancing_start end=gradual_rebalancing_end
```

</TabItem>
</Tabs>

### 5. Pin by Tag/Attribute

Pin objects matching certain criteria:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=pin_by_tag_start end=pin_by_tag_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=pin_by_tag_start end=pin_by_tag_end
```

</TabItem>
</Tabs>

## How It Works

For each object in the `objects` list:
- Solver ensures the object stays on its current container
- If solution requires moving these objects, solver fails with infeasibility error

This is a **hard constraint** - there's no partial satisfaction.

## Difference from MinimizeMovementSpec

| Aspect | AvoidMovingSpec | MinimizeMovementSpec |
|--------|----------------|---------------------|
| **Type** | Constraint (hard) | Goal (soft) |
| **Behavior** | Objects **cannot** move | Objects **prefer** not to move |
| **Failure** | Fails if must move | Always succeeds (with penalty) |
| **Specificity** | Exact objects | All objects (weighted) |
| **Use case** | Operational pins | General stability |

**When to use which**:
- Use **AvoidMovingSpec** when objects **must not** move
- Use **MinimizeMovementSpec** when you **prefer** objects don't move

## Performance Considerations

- **Impact**: Minimal - simple constraint check
- **Solver compatibility**: Works with both Local Search and Optimal solvers
- **Scaling**: O(number of pinned objects)
- **Search space**: Reduces solution space (can help or hurt)

## Common Pitfalls

### 1. Over-Constraining

**Problem**: Pinning too many objects makes rebalancing impossible.

```python
# BAD: Pin 95% of objects
solver.add_constraint(
        ConstraintSpec(
            avoidMovingSpec=AvoidMovingSpec(
        objects=most_objects,  # Too many!
    )
        )
)
# Solver may fail to find solution
```

**Solution**: Only pin truly critical objects, or use MinimizeMovementSpec instead.

### 2. Conflicting with Capacity

**Problem**: Pinned objects cause capacity violations.

```python
# Pin objects on overloaded host
solver.add_constraint(
        ConstraintSpec(
            avoidMovingSpec=AvoidMovingSpec(objects=["obj_on_overloaded_host"])
        )
)

# Capacity constraint requires moving them!
solver.add_constraint(
        ConstraintSpec(
            capacitySpec=CapacitySpec(scope="host", dimension="cpu")
        )
)
# Result: Infeasible
```

**Solution**: Review capacity before pinning, or use MinimizeMovementSpec with high weight.

### 3. Stale Object IDs

**Problem**: Object IDs changed or objects no longer exist.

```python
# BAD: Pin objects that don't exist
solver.add_constraint(
        ConstraintSpec(
            avoidMovingSpec=AvoidMovingSpec(objects=["deleted_object_123"])
        )
)
# Solver ignores non-existent objects
```

**Solution**: Validate object IDs exist in current assignment before pinning.

### 4. Using When You Want MinimizeMovement

**Problem**: Want to minimize moves but used hard constraint.

```python
# BAD: Using wrong spec for preference
solver.add_constraint(
        ConstraintSpec(
            avoidMovingSpec=AvoidMovingSpec(objects=all_objects)
        )
)
# Can't balance at all!
```

**Solution**: Use MinimizeMovementSpec for preferences:

```python
# GOOD: Soft preference to avoid moves
solver.add_goal(
        GoalSpec(
            minimizeMovementSpec=MinimizeMovementSpec(scope="host", dimension="count")
        ),
    weight=0.5
)
```

## Combining with Other Constraints

### With CapacitySpec

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=avoid_moving_with_capacity_start end=avoid_moving_with_capacity_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=avoid_moving_with_capacity_start end=avoid_moving_with_capacity_end
```

</TabItem>
</Tabs>

### With AvoidAssignmentsSpec

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=avoid_moving_with_avoid_assignments_start end=avoid_moving_with_avoid_assignments_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=avoid_moving_with_avoid_assignments_start end=avoid_moving_with_avoid_assignments_end
```

</TabItem>
</Tabs>

## Migration Strategy Example

Gradually migrate objects using rounds:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=gradual_migration_start end=gradual_migration_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=gradual_migration_start end=gradual_migration_end
```

</TabItem>
</Tabs>

## Related Specs

- [MinimizeMovementSpec](../movement/minimize-movement) - Soft preference to minimize movement (goal)
- [AvoidAssignmentsSpec](../placement/avoid-assignments) - Block specific object→container assignments
- [MovesInProgressSpec](moves-in-progress) - Account for ongoing moves in solution

## Source Code

- Thrift definition: `interface/thrift/ProblemSpecs.thrift:156`
- Implementation: `solver/constraints/AvoidMoving.cpp`
