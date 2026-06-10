---
sidebar_position: 4
---

# UtilIncreaseCostSpec

**Type**: Goal only

Penalize high utilization with exponentially increasing costs to maintain headroom.

## Overview

`UtilIncreaseCostSpec` creates a non-linear cost function that penalizes high utilization more heavily than low utilization. Unlike `BalanceSpec` which treats all imbalance equally, this spec exponentially increases the cost as containers approach capacity.

**Use this when**: You want to maintain headroom on containers, avoid hotspots, or create a "soft capacity limit" that strongly discourages high utilization without making it a hard constraint.

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Unique identifier for this goal |
| `scope` | string | Yes | - | Scope level (e.g., `"host"`, `"rack"`) |
| `dimension` | string | Yes | - | Object dimension to sum (e.g., `"cpu_usage"`) |
| `exponent` | double | No | 2.0 | Exponent for cost function (higher = stronger penalty) |
| `maxUtilization` | double | No | 1.0 | Max utilization threshold (as fraction of capacity) |

## How It Works

The cost function is:

```
cost = sum over containers: (utilization ^ exponent)
```

Where:
- `utilization = sum(dimension) / capacity`
- Higher `exponent` creates stronger penalty for high utilization
- `maxUtilization` clips utilization before applying exponent

### Cost Comparison by Exponent

For a container at 80% utilization:

| Exponent | Cost | Behavior |
|----------|------|----------|
| 1.0 | 0.80 | Linear (same as balance) |
| 2.0 | 0.64 | Quadratic (default) |
| 3.0 | 0.51 | Cubic (strong penalty) |
| 4.0 | 0.41 | Very strong penalty |

**Key insight**: With exponent=2.0, a container at 90% costs (0.9)² = 0.81, while two containers at 45% each cost 2×(0.45)² = 0.405 (half the cost). This naturally spreads load.

## Common Usage Patterns

### 1. Maintain Headroom

Reserve capacity for traffic spikes:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.py start=maintain_headroom_start end=maintain_headroom_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.cpp start=maintain_headroom_start end=maintain_headroom_end
```

</TabItem>
</Tabs>

### 2. Avoid Hotspots

Prevent overloaded containers:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.py start=avoid_hotspots_start end=avoid_hotspots_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.cpp start=avoid_hotspots_start end=avoid_hotspots_end
```

</TabItem>
</Tabs>

### 3. Soft Capacity Limit

Create a "soft" capacity that's lower than hard capacity:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.py start=soft_capacity_limit_start end=soft_capacity_limit_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.cpp start=soft_capacity_limit_start end=soft_capacity_limit_end
```

</TabItem>
</Tabs>

### 4. Multi-Resource Headroom

Maintain headroom on multiple resources:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.py start=multi_resource_headroom_start end=multi_resource_headroom_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.cpp start=multi_resource_headroom_start end=multi_resource_headroom_end
```

</TabItem>
</Tabs>

### 5. Rack-Level Headroom

Maintain headroom at rack level for network capacity:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.py start=rack_level_headroom_start end=rack_level_headroom_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.cpp start=rack_level_headroom_start end=rack_level_headroom_end
```

</TabItem>
</Tabs>

### 6. Progressive Penalty

Different penalties for different utilization ranges:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.py start=progressive_penalty_start end=progressive_penalty_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.cpp start=progressive_penalty_start end=progressive_penalty_end
```

</TabItem>
</Tabs>

### 7. Dynamic Exponent Based on Time

Adjust penalty based on time of day:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.py start=dynamic_exponent_start end=dynamic_exponent_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.cpp start=dynamic_exponent_start end=dynamic_exponent_end
```

</TabItem>
</Tabs>

## Exponent Selection Guide

| Exponent | Use Case | Behavior |
|----------|----------|----------|
| 1.0 | No headroom needed | Linear cost (equivalent to balance) |
| 1.5 | Slight preference for spread | Gentle non-linearity |
| 2.0 | **Default** | Quadratic penalty, good general purpose |
| 2.5-3.0 | Moderate headroom | Strong preference for spreading |
| 4.0+ | Critical headroom | Very aggressive spreading |

## Difference from BalanceSpec

| Aspect | UtilIncreaseCostSpec | BalanceSpec |
|--------|---------------------|-------------|
| **Cost function** | Exponential (utilization^exponent) | Variance or std dev |
| **Penalty shape** | Non-linear, accelerating | Linear or quadratic |
| **Best for** | Maintaining headroom | Even distribution |
| **Empty containers** | Low cost | May create imbalance |
| **High utilization** | Exponentially expensive | Linearly expensive |

Example: 4 containers at [90%, 90%, 10%, 10%]
- **UtilIncreaseCostSpec** (exp=2): (0.9)² + (0.9)² + (0.1)² + (0.1)² = 1.62 + 0.02 = 1.64
- **After rebalance** to [50%, 50%, 50%, 50%]: 4 × (0.5)² = 1.0 (much better!)

## Performance Considerations

- **Impact**: Low - simple arithmetic operation
- **Solver compatibility**: Works with both Local Search and Optimal solvers
- **Scaling**: O(number of containers in scope)
- **Convergence**: Higher exponents may slow convergence (more non-linear)

## Common Pitfalls

### 1. Exponent Too High

**Problem**: Exponent too high causes numerical issues or poor convergence.

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.py start=pitfall_exponent_bad_start end=pitfall_exponent_bad_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.cpp start=pitfall_exponent_bad_start end=pitfall_exponent_bad_end
```

</TabItem>
</Tabs>

**Solution**: Use exponents in range 1.5-4.0:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.py start=pitfall_exponent_good_start end=pitfall_exponent_good_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.cpp start=pitfall_exponent_good_start end=pitfall_exponent_good_end
```

</TabItem>
</Tabs>

### 2. Conflicting with Capacity

**Problem**: UtilIncreaseCost with maxUtilization conflicts with actual capacity.

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.py start=pitfall_capacity_bad_start end=pitfall_capacity_bad_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.cpp start=pitfall_capacity_bad_start end=pitfall_capacity_bad_end
```

</TabItem>
</Tabs>

**Solution**: Set maxUtilization to match desired operating point:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.py start=pitfall_capacity_good_start end=pitfall_capacity_good_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.cpp start=pitfall_capacity_good_start end=pitfall_capacity_good_end
```

</TabItem>
</Tabs>

### 3. Weight Too Low

**Problem**: Weight too low, headroom goal ignored.

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.py start=pitfall_weight_bad_start end=pitfall_weight_bad_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.cpp start=pitfall_weight_bad_start end=pitfall_weight_bad_end
```

</TabItem>
</Tabs>

**Solution**: Balance weights appropriately:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.py start=pitfall_weight_good_start end=pitfall_weight_good_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.cpp start=pitfall_weight_good_start end=pitfall_weight_good_end
```

</TabItem>
</Tabs>

### 4. Missing Capacity Dimension

**Problem**: No capacity dimension defined, utilization can't be calculated.

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.py start=pitfall_missing_capacity_bad_start end=pitfall_missing_capacity_bad_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.cpp start=pitfall_missing_capacity_bad_start end=pitfall_missing_capacity_bad_end
```

</TabItem>
</Tabs>

**Solution**: Always define capacity dimension:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.py start=pitfall_missing_capacity_good_start end=pitfall_missing_capacity_good_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.cpp start=pitfall_missing_capacity_good_start end=pitfall_missing_capacity_good_end
```

</TabItem>
</Tabs>

## Combining with Other Specs

### With BalanceSpec

Use both for balanced distribution with headroom:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.py start=combining_balance_start end=combining_balance_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.cpp start=combining_balance_start end=combining_balance_end
```

</TabItem>
</Tabs>

### With CapacitySpec

Soft limit below hard limit:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.py start=combining_capacity_start end=combining_capacity_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.cpp start=combining_capacity_start end=combining_capacity_end
```

</TabItem>
</Tabs>

### With MinimizeMovementSpec

Balance headroom with minimal disruption:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.py start=combining_minimize_movement_start end=combining_minimize_movement_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.cpp start=combining_minimize_movement_start end=combining_minimize_movement_end
```

</TabItem>
</Tabs>

## Verification Example

Verify headroom is maintained:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.py start=verify_headroom_start end=verify_headroom_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.cpp start=verify_headroom_start end=verify_headroom_end
```

</TabItem>
</Tabs>

## Headroom Analysis

Analyze utilization distribution:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.py start=analyze_utilization_start end=analyze_utilization_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/util_increase_cost/util_increase_cost_spec_examples.cpp start=analyze_utilization_start end=analyze_utilization_end
```

</TabItem>
</Tabs>

## Related Specs

- [BalanceSpec](balance-optimize/balance) - Even distribution (linear cost)
- [CapacitySpec](capacity) - Hard capacity limits
- [MinimizeMovementSpec](movement/minimize-movement) - Reduce churn while maintaining headroom

## Source Code

- Thrift definition: `interface/thrift/ProblemSpecs.thrift` (UtilIncreaseCostSpec)
- Implementation: `solver/goals/UtilIncreaseCost.cpp`
