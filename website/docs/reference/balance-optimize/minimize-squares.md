---
sidebar_position: 21
---

# MinimizeSquaresSpec

**Type**: Goal only

Minimize the sum of squares of container utilization for stronger balance enforcement.

## Overview

`MinimizeSquaresSpec` minimizes the sum of squared utilization values across containers. This creates a stronger penalty for imbalance than linear variance, pushing the solver toward more even distribution. It's an alternative to [BalanceSpec](balance) with quadratic instead of linear imbalance cost.

**Use this when**: You want very even distribution and are willing to accept more movement to achieve it. The squared penalty makes the solver work harder to eliminate imbalance.

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_squares_spec_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_squares_spec_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Unique identifier for this goal |
| `scope` | string | Yes | - | Scope level (e.g., `"host"`, `"rack"`) |
| `dimension` | string | Yes | - | Object dimension to balance |
| `filter` | Filter | No | all | Apply only to filtered containers |
| `lowerBound` | double | No | 0.0 | Minimum utilization for squaring |
| `upperBound` | double | No | none | Maximum utilization for squaring |
| `pieceCount` | int32 | No | 100 | Number of piecewise linear pieces for approximation |

## How It Works

For each container:
1. Calculate utilization: `util = sum(dimension) / capacity`
2. Clamp to bounds: `clamped_util = clamp(util, lowerBound, upperBound)`
3. Contribution: `clamped_util²`

**Objective**: Minimize `sum(util²)` across all containers

**Piecewise Approximation**: Since quadratic functions aren't natively supported in MIP solvers, the squared function is approximated using `pieceCount` linear pieces.

## Difference from BalanceSpec

| Aspect | MinimizeSquaresSpec | BalanceSpec |
|--------|---------------------|-------------|
| **Cost function** | Sum of squares (util²) | Variance or std dev |
| **Penalty** | Quadratic | Linear or quadratic (depending on formula) |
| **Imbalance penalty** | Stronger (accelerating) | Moderate |
| **Movement** | May cause more movement | Less aggressive |
| **Use case** | Very even distribution | General balancing |

**Example**:
- 4 hosts at [90%, 90%, 10%, 10%]
- **MinimizeSquaresSpec**: 0.9² + 0.9² + 0.1² + 0.1² = 1.64
- **After balancing** to [50%, 50%, 50%, 50%]: 4 × 0.5² = 1.0 (much better)

The quadratic penalty makes 90% utilization much more expensive than 50%, driving toward evenness.

## Common Usage Patterns

### 1. Strong Load Balancing

Enforce very even load distribution:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_squares_spec_examples.py start=strong_load_balancing_start end=strong_load_balancing_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_squares_spec_examples.cpp start=strong_load_balancing_start end=strong_load_balancing_end
```

</TabItem>
</Tabs>

### 2. Multi-Resource Strong Balance

Balance multiple resources with squared penalty:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_squares_spec_examples.py start=multi_resource_strong_balance_start end=multi_resource_strong_balance_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_squares_spec_examples.cpp start=multi_resource_strong_balance_start end=multi_resource_strong_balance_end
```

</TabItem>
</Tabs>

### 3. Filtered Strong Balance

Apply strong balancing only to specific hosts:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_squares_spec_examples.py start=filtered_strong_balance_start end=filtered_strong_balance_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_squares_spec_examples.cpp start=filtered_strong_balance_start end=filtered_strong_balance_end
```

</TabItem>
</Tabs>

### 4. Rack-Level Balance

Strong balance at rack level:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_squares_spec_examples.py start=rack_level_balance_start end=rack_level_balance_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_squares_spec_examples.cpp start=rack_level_balance_start end=rack_level_balance_end
```

</TabItem>
</Tabs>

### 5. Upper Bound for Headroom

Use upperBound to create headroom target:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_squares_spec_examples.py start=upper_bound_headroom_start end=upper_bound_headroom_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_squares_spec_examples.cpp start=upper_bound_headroom_start end=upper_bound_headroom_end
```

</TabItem>
</Tabs>

**Note**: upperBound doesn't enforce a hard limit - it just clips the squared penalty. Use [CapacitySpec](../capacity) or [UtilIncreaseCostSpec](../util-increase-cost) for actual limits.

### 6. Piecewise Approximation Tuning

Adjust pieceCount for accuracy vs performance:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_squares_spec_examples.py start=piecewise_tuning_high_accuracy_start end=piecewise_tuning_high_accuracy_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_squares_spec_examples.cpp start=piecewise_tuning_high_accuracy_start end=piecewise_tuning_high_accuracy_end
```

</TabItem>
</Tabs>

**Typical**: 100 pieces (default) works well for most cases.

## Performance Considerations

- **Impact**: Moderate - piecewise linear approximation adds variables
- **Solver compatibility**: Works with both Local Search and Optimal solvers
- **Scaling**: O(number of containers × pieceCount)
- **pieceCount trade-off**: More pieces = more accurate but slower

## Common Pitfalls

### 1. Bounds Misconfigured

**Problem**: Bounds don't match actual utilization range.

```python
# BAD: Utilization is in GB (0-1000), but bounds assume [0, 1]
solver.add_goal(
        GoalSpec(
            minimizeSquaresSpec=MinimizeSquaresSpec(
        dimension="memory_usage",  # In GB, not fraction!
        lowerBound=0.0,
        upperBound=1.0,  # Should be ~1000!
    )
        ),
    weight=1.0
)
```

**Solution**: Set bounds to match dimension scale:

```python
# GOOD: Bounds match actual range
solver.add_goal(
        GoalSpec(
            minimizeSquaresSpec=MinimizeSquaresSpec(
        dimension="memory_usage",  # GB
        lowerBound=0.0,
        upperBound=1000.0,  # Match capacity
    )
        ),
    weight=1.0
)
```

**Or normalize** utilization to [0, 1] by using utilization dimension:

```python
# Normalize to [0, 1]
for host in hosts:
    util_fraction = memory_usage[host] / memory_capacity[host]
    # Use this normalized value
```

### 2. Too Many Pieces

**Problem**: pieceCount too high causes slow solving.

```python
# BAD: Excessive pieces
solver.add_goal(
        GoalSpec(
            minimizeSquaresSpec=MinimizeSquaresSpec(
        pieceCount=1000,  # Way too many!
    )
        ),
    weight=1.0
)
# Solve time very slow
```

**Solution**: Use reasonable pieceCount (50-200):

```python
# GOOD: Reasonable piece count
solver.add_goal(
        GoalSpec(
            minimizeSquaresSpec=MinimizeSquaresSpec(
        pieceCount=100,  # Default is fine
    )
        ),
    weight=1.0
)
```

### 3. Expecting Hard Upper Bound

**Problem**: Thinking upperBound enforces capacity limit.

```python
# BAD: upperBound doesn't prevent exceeding capacity
solver.add_goal(
        GoalSpec(
            minimizeSquaresSpec=MinimizeSquaresSpec(
        upperBound=0.8,  # NOT a hard limit!
    )
        ),
    weight=1.0
)
# Hosts can still go above 80% if needed
```

**Solution**: Use CapacitySpec for hard limits:

```python
# GOOD: Hard capacity limit + soft balance
solver.add_constraint(
        ConstraintSpec(
            capacitySpec=CapacitySpec(
        name="capacity-limit",
        dimension="cpu_usage",
    )
        )
)

solver.add_goal(
        GoalSpec(
            minimizeSquaresSpec=MinimizeSquaresSpec(
        name="balance",
        dimension="cpu_usage",
        upperBound=0.8,  # Just for penalty calculation
    )
        ),
    weight=1.0
)
```

### 4. Dimension Not Defined

**Problem**: Dimension doesn't exist.

```python
# BAD: Dimension "load" not defined
solver.add_goal(
    MinimizeSquaresSpec(
        dimension="load",  # Doesn't exist!
    ),
    weight=1.0
)
```

**Solution**: Define dimension first:

```python
# GOOD: Define dimension
solver.add_object_dimension("cpu_usage", cpu_values)

solver.add_goal(
        GoalSpec(
            minimizeSquaresSpec=MinimizeSquaresSpec(
        dimension="cpu_usage",
    )
        ),
    weight=1.0
)
```

## Combining with Other Specs

### With CapacitySpec

Balance within capacity limits:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_squares_spec_examples.py start=combining_capacity_start end=combining_capacity_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_squares_spec_examples.cpp start=combining_capacity_start end=combining_capacity_end
```

</TabItem>
</Tabs>

### With MinimizeMovementSpec

Balance but limit churn:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_squares_spec_examples.py start=combining_minimize_movement_start end=combining_minimize_movement_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_squares_spec_examples.cpp start=combining_minimize_movement_start end=combining_minimize_movement_end
```

</TabItem>
</Tabs>

## Verification Example

Verify balance quality:

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_squares_spec_examples.py start=verification_example_start end=verification_example_end
```

## When to Use MinimizeSquaresSpec vs BalanceSpec

| Situation | Recommendation |
|-----------|---------------|
| General balancing | Use **BalanceSpec** (simpler) |
| Very even distribution needed | Use **MinimizeSquaresSpec** |
| Minimal movement preferred | Use **BalanceSpec** (less aggressive) |
| Hotspots must be eliminated | Use **MinimizeSquaresSpec** |
| Large-scale problems | Use **BalanceSpec** (faster) |
| Small problems with OptimalSolver | Use **MinimizeSquaresSpec** (accurate) |

## Related Specs

- [BalanceSpec](balance) - General balance (recommended for most use cases)
- [UtilIncreaseCostSpec](../util-increase-cost) - Exponential penalty (maintains headroom)
- [CapacitySpec](../capacity) - Hard capacity limits

## Source Code

- Thrift definition: `interface/thrift/ProblemSpecs.thrift:860` (MinimizeSquaresSpec)
- Implementation: `solver/goals/MinimizeSquares.cpp`
