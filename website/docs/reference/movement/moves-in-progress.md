---
sidebar_position: 13
---

# MovesInProgressSpec

**Type**: Constraint only

Track ongoing object moves and prevent rebalancing them until complete.

## Overview

`MovesInProgressSpec` marks objects as currently being moved. While a move is in progress, the solver treats the object as if it's already at its destination, preventing the object from being moved again until the current move completes.

**Use this when**: Running rebalancing in a production system with ongoing migrations. Prevents the solver from moving objects that are already in transit.

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=moves_in_progress_quick_example_start end=moves_in_progress_quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=moves_in_progress_quick_example_start end=moves_in_progress_quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Unique identifier for this constraint |
| `scope` | string | Yes | - | Scope level (e.g., `"host"`, `"rack"`) |
| `moves` | list&lt;MoveInProgress&gt; | Yes | - | List of ongoing moves |

### MoveInProgress Structure

| Field | Type | Description |
|-------|------|-------------|
| `object` | string | Object ID being moved |
| `destination` | string | Container ID where object is moving to |

## How It Works

For each move in progress:
1. Solver treats object as if it's already at `destination`
2. Object cannot be moved to a different destination
3. Current assignment still shows object at original location (until move completes)

**Example**:
```
Current assignment: shard_42 on host1
Move in progress: shard_42 → host5

Solver behavior:
- Treats shard_42 as already on host5 (for capacity, balance calculations)
- Won't move shard_42 to host2, host3, etc.
- Can leave shard_42 at host5 (the destination)
```

## Common Usage Patterns

### 1. Basic Production Rebalancing

Track ongoing moves between rebalancing rounds:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=basic_production_rebalancing_start end=basic_production_rebalancing_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=basic_production_rebalancing_start end=basic_production_rebalancing_end
```

</TabItem>
</Tabs>

### 2. Multi-Round Migration

Track moves across multiple rebalancing rounds:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=multi_round_migration_start end=multi_round_migration_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=multi_round_migration_start end=multi_round_migration_end
```

</TabItem>
</Tabs>

### 3. Separate Migration and Rebalancing

Use for systems with background migration separate from rebalancing:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=separate_migration_start end=separate_migration_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=separate_migration_start end=separate_migration_end
```

</TabItem>
</Tabs>

### 4. Resumable Rebalancing

Resume rebalancing after interruption:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=resumable_rebalancing_start end=resumable_rebalancing_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=resumable_rebalancing_start end=resumable_rebalancing_end
```

</TabItem>
</Tabs>

### 5. Prioritized Migration

Allow high-priority moves to complete before new rebalancing:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=prioritized_migration_start end=prioritized_migration_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=prioritized_migration_start end=prioritized_migration_end
```

</TabItem>
</Tabs>

### 6. Rack-Level Moves

Track moves at rack level:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=rack_level_moves_start end=rack_level_moves_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=rack_level_moves_start end=rack_level_moves_end
```

</TabItem>
</Tabs>

### 7. Integration with Move Tracking System

Integrate with production move tracking:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=move_tracker_class_start end=move_tracker_class_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=move_tracker_class_start end=move_tracker_class_end
```

</TabItem>
</Tabs>

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=move_tracker_usage_start end=move_tracker_usage_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=move_tracker_usage_start end=move_tracker_usage_end
```

</TabItem>
</Tabs>

## Interaction with Other Specs

### With AvoidMovingSpec

Combine to pin both in-progress and already-moved objects:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=moves_with_avoid_moving_start end=moves_with_avoid_moving_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=moves_with_avoid_moving_start end=moves_with_avoid_moving_end
```

</TabItem>
</Tabs>

### With MinimizeMovementSpec

Soft movement limit with in-progress tracking:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=moves_with_minimize_movement_start end=moves_with_minimize_movement_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=moves_with_minimize_movement_start end=moves_with_minimize_movement_end
```

</TabItem>
</Tabs>

## Performance Considerations

- **Impact**: Minimal - simple constraint check
- **Solver compatibility**: Works with both Local Search and Optimal solvers
- **Scaling**: O(number of ongoing moves)
- **Best practice**: Clean up completed moves from the list

## Common Pitfalls

### 1. Stale Move Tracking

**Problem**: Moves completed but still marked as in-progress.

```python
# BAD: Never remove completed moves
ongoing_moves = [
    MoveInProgress(object="shard_1", destination="host2"),
    MoveInProgress(object="shard_2", destination="host3"),
    # shard_1 actually finished 10 minutes ago!
]

solver.add_constraint(
        ConstraintSpec(
            movesInProgressSpec=MovesInProgressSpec(moves=ongoing_moves)
        )
)
# Solver thinks shard_1 is still moving, won't plan correctly
```

**Solution**: Actively track and remove completed moves:

```python
# GOOD: Only include actually in-progress moves
ongoing_moves = [
    move for move in all_moves
    if move_tracker.is_still_in_progress(move.object)
]

solver.add_constraint(
        ConstraintSpec(
            movesInProgressSpec=MovesInProgressSpec(moves=ongoing_moves)
        )
)
```

### 2. Conflicting Destination

**Problem**: Move in progress to destination A, but current assignment shows object still at source.

```python
# Current assignment (what's actually running)
assignment = {
    "host1": ["shard_42"],  # Still here
    "host5": [],
}

# Move in progress
moves_in_progress = [
    MoveInProgress(object="shard_42", destination="host5")
]

# This is correct! Object physically at host1, moving to host5
# Solver treats it as if already at host5 for planning purposes
```

**Note**: This is not an error - it's the expected behavior. The spec is about where the object *will be*, not where it currently is.

### 3. Forgetting Scope Match

**Problem**: Destination doesn't match spec scope.

```python
# BAD: Scope is "rack" but destination is a host
solver.add_constraint(
        ConstraintSpec(
            movesInProgressSpec=MovesInProgressSpec(
        scope="rack",
        moves=[
            MoveInProgress(object="obj", destination="host3")  # host, not rack!
        ],
    )
        )
)
```

**Solution**: Match destination to scope:

```python
# GOOD: Scope and destination match
solver.add_constraint(
        ConstraintSpec(
            movesInProgressSpec=MovesInProgressSpec(
        scope="host",
        moves=[
            MoveInProgress(object="obj", destination="host3")
        ],
    )
        )
)
```

### 4. Not Updating Between Rounds

**Problem**: Using same MovesInProgressSpec across rounds without updating.

```python
# BAD: Define once, use forever
moves_spec = MovesInProgressSpec(moves=initial_moves)

for round in range(10):
    solver.add_constraint(moves_spec)  # Same moves every round!
    # Some moves finished, new ones started - spec is stale
```

**Solution**: Update moves each round:

```python
# GOOD: Update each round
for round in range(10):
    current_moves = get_current_in_progress_moves()
    moves_spec = MovesInProgressSpec(
        name="ongoing",
        scope="host",
        moves=current_moves,  # Fresh list each time
    )
    solver.add_constraint(moves_spec)
```

## Verification Example

Verify move tracking is accurate:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=verify_move_tracking_start end=verify_move_tracking_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=verify_move_tracking_start end=verify_move_tracking_end
```

</TabItem>
</Tabs>

## Move Lifecycle Tracking

Complete example of tracking move lifecycle:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=move_lifecycle_manager_start end=move_lifecycle_manager_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=move_lifecycle_manager_start end=move_lifecycle_manager_end
```

</TabItem>
</Tabs>

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=move_lifecycle_usage_start end=move_lifecycle_usage_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=move_lifecycle_usage_start end=move_lifecycle_usage_end
```

</TabItem>
</Tabs>

## Related Specs

- [AvoidMovingSpec](../movement/avoid-moving) - Pin objects (don't move at all)
- [MinimizeMovementSpec](../movement/minimize-movement) - Soft movement minimization
- [ToFreeSpec](../to-free) - Plan moves to drain containers

## Source Code

- Thrift definition: `interface/thrift/ProblemSpecs.thrift` (MovesInProgressSpec, MoveInProgress)
- Implementation: `solver/constraints/MovesInProgress.cpp`
