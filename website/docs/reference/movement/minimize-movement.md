---
sidebar_position: 10
---

# MinimizeMovementSpec

**Type**: Goal only

Minimize the total amount of movement (weighted by dimension) when rebalancing.

## Overview

`MinimizeMovementSpec` penalizes moving objects, helping to maintain stability during rebalancing. This is typically used as a secondary goal with lower weight than primary balance/capacity goals.

**Use this when**: You want to minimize disruption during rebalancing, keeping as many objects in place as possible while still achieving balance.

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=minimize_movement_quick_example_start end=minimize_movement_quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=minimize_movement_quick_example_start end=minimize_movement_quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Unique identifier for this goal |
| `scope` | string | Yes | - | Scope over which to minimize movement (usually container scope) |
| `dimension` | string | Yes | - | Dimension to weight movements (e.g., `"data_size"`, `"memory"`) |
| `magicScaling` | bool | No | true | Apply internal heuristic weights to balance goal importance |
| `doNotNormalize` | bool | No | false | Skip normalization by item count and capacity |
| `allowance` | double | No | 0.0 | Allow this much movement without penalty |

## Common Usage Patterns

### 1. Minimize Data Movement

Minimize moving large objects while rebalancing:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=minimize_data_movement_start end=minimize_data_movement_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=minimize_data_movement_start end=minimize_data_movement_end
```

</TabItem>
</Tabs>

### 2. Stability with Allowance

Allow some movement, but penalize beyond a threshold:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=stability_with_allowance_start end=stability_with_allowance_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=stability_with_allowance_start end=stability_with_allowance_end
```

</TabItem>
</Tabs>

### 3. Minimize Move Count

Minimize number of objects moved (use count dimension):

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=minimize_move_count_start end=minimize_move_count_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=minimize_move_count_start end=minimize_move_count_end
```

</TabItem>
</Tabs>

### 4. Hierarchical Movement Minimization

Prefer moving within racks over across racks:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=hierarchical_movement_start end=hierarchical_movement_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=hierarchical_movement_start end=hierarchical_movement_end
```

</TabItem>
</Tabs>

### 5. Disable Magic Scaling

Use raw movement values without heuristic scaling:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=disable_magic_scaling_start end=disable_magic_scaling_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=disable_magic_scaling_start end=disable_magic_scaling_end
```

</TabItem>
</Tabs>

## How It Works

The objective function adds a penalty for each moved object:

```
penalty = sum(dimension_value_of_moved_object for each moved object)
```

With `allowance`:
```
penalty = max(0, total_movement - allowance)
```

With `magicScaling=True` (default), internal heuristics adjust the penalty to balance importance with other goals.

## Weight Guidelines

Relative to primary goals (balance, capacity):

| Use Case | Recommended Weight | Reasoning |
|----------|-------------------|-----------|
| **Gentle nudge** | 0.01 - 0.05 | Break ties, minimal impact |
| **Moderate preference** | 0.1 - 0.3 | Noticeable stability |
| **Strong preference** | 0.5 - 1.0 | Significant disruption penalty |
| **Strict stability** | 2.0+ | Extreme reluctance to move |

**Note**: Weights too high may prevent achieving balance entirely.

## Performance Considerations

- **Impact**: Minimal - simple linear term
- **Solver compatibility**: Works with both Local Search and Optimal solvers
- **Scaling**: O(number of moved objects)

## Common Pitfalls

### 1. Weight Too High

**Problem**: Can't achieve balance because movement is too expensive.

```python
# TOO HIGH - may prevent balancing
solver.add_goal(
        GoalSpec(
            minimizeMovementSpec=MinimizeMovementSpec(...)
        ), weight=10.0)
```

**Solution**: Use weights 0.1-1.0 relative to primary balance goals.

### 2. Wrong Dimension

**Problem**: Minimizing count when you care about data size.

```python
# BAD: Moves 1 large object + keeps 10 small unmoved
solver.add_goal(
        GoalSpec(
            minimizeMovementSpec=MinimizeMovementSpec(
        scope="host",
        dimension="count",  # Wrong!
    )
        ),
    weight=0.5
)
```

**Solution**: Use the dimension you actually care about (data_size, memory, etc.).

### 3. Missing Allowance

**Problem**: Penalizing all movement when some is acceptable.

```python
# SUBOPTIMAL: Even 1 byte penalized
solver.add_goal(
        GoalSpec(
            minimizeMovementSpec=MinimizeMovementSpec(scope="host", dimension="data_size")
        ),
    weight=0.5
)
```

**Solution**: Set `allowance` to acceptable movement threshold:

```python
solver.add_goal(
        GoalSpec(
            minimizeMovementSpec=MinimizeMovementSpec(
        scope="host",
        dimension="data_size",
        allowance=50.0,  # 50GB free movement
    )
        ),
    weight=0.5
)
```

## Combining with Other Goals

### With BalanceSpec

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=minimize_with_balance_start end=minimize_with_balance_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=minimize_with_balance_start end=minimize_with_balance_end
```

</TabItem>
</Tabs>

### With CapacitySpec

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.py start=minimize_with_capacity_start end=minimize_with_capacity_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/movement/movement_spec_examples.cpp start=minimize_with_capacity_start end=minimize_with_capacity_end
```

</TabItem>
</Tabs>

## Related Specs

- [AvoidMovingSpec](../movement/avoid-moving) - Explicitly prevent specific objects from moving (constraint)
- [BalanceSpec](../balance) - Primary balancing goal (often paired with this)
- [CapacitySpec](../capacity) - Capacity constraints (often paired with this)

## Source Code

- Thrift definition: `interface/thrift/ProblemSpecs.thrift:827`
- Implementation: `solver/goals/MinimizeMovement.cpp`
