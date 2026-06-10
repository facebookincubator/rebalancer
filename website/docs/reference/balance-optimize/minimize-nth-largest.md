---
sidebar_position: 22
---

# MinimizeNthLargestSpec

**Type**: Goal only

Minimize the Nth largest utilization value across containers (peak load minimization).

## Overview

`MinimizeNthLargestSpec` minimizes a specific order statistic of container utilization. Instead of minimizing the average (like Balance) or the maximum (like MinMax), it targets the Nth highest value. This is useful for capping peak load while allowing some flexibility in the tail.

**Use this when**: You want to minimize peak load but are willing to tolerate a few outliers, such as "keep the 95th percentile under X" or "minimize the 3rd highest load."

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_nth_largest_spec_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_nth_largest_spec_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Unique identifier for this spec |
| `scope` | string | Yes | - | Scope level (e.g., `"host"`, `"rack"`) |
| `dimension` | string | Yes | - | Dimension to minimize |
| `n` | int32 | Yes | - | 0-based index (0 = largest, 1 = 2nd largest, etc.) |
| `filter` | Filter | No | null | Apply only to filtered objects |
| `targetUtilization` | double | No | 0.0 | Don't minimize below this target |

## Understanding the N Parameter

The `n` parameter is **0-based**:
- `n=0`: Minimize the **largest** value (same as minimize max)
- `n=1`: Minimize the **2nd largest** value
- `n=2`: Minimize the **3rd largest** value
- `n=k`: Minimize the **(k+1)th largest** value

## Common Usage Patterns

### 1. Minimize 95th Percentile

Target the 95th percentile instead of absolute max:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_nth_largest_spec_examples.py start=percentile_95_start end=percentile_95_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_nth_largest_spec_examples.cpp start=percentile_95_start end=percentile_95_end
```

</TabItem>
</Tabs>

### 2. Cap Top 3 Hosts

Minimize the 3rd highest host load:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_nth_largest_spec_examples.py start=cap_top_3_start end=cap_top_3_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_nth_largest_spec_examples.cpp start=cap_top_3_start end=cap_top_3_end
```

</TabItem>
</Tabs>

### 3. With Target Utilization

Don't minimize below a target (useful for avoiding under-utilization):

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_nth_largest_spec_examples.py start=with_target_start end=with_target_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_nth_largest_spec_examples.cpp start=with_target_start end=with_target_end
```

</TabItem>
</Tabs>

## Performance Considerations

- **Complexity**: O(containers × log(containers)) for sorting
- **Memory**: Stores utilization values
- **Optimization**: Use filter to reduce object set

## Comparison with Related Specs

| Spec | Minimizes | Use When |
|------|-----------|----------|
| **MinimizeNthLargestSpec** | Nth highest value | Want percentile-based optimization |
| [BalanceSpec](balance) | Variance/imbalance | Want even distribution |
| [MinimizeSquaresSpec](minimize-squares) | Sum of squares | Want strong balance with quadratic penalty |
| [CapacitySpec](../capacity) | Violations | Hard capacity limits |

## Troubleshooting

**Issue**: Not achieving target percentile
- **Solution**: Increase goal weight or check for conflicting constraints

**Issue**: Unclear which N to use
- **Solution**: n = ceil(containers × (1 - percentile)) for percentile-based targets

**Issue**: Too aggressive optimization
- **Solution**: Set targetUtilization to prevent under-utilization

## Related Specs

- [BalanceSpec](balance) - Even distribution
- [MinimizeSquaresSpec](minimize-squares) - Quadratic balance penalty
- [CapacitySpec](../capacity) - Hard capacity limits

## Source Code

- **Thrift definition**: [`interface/thrift/ProblemSpecs.thrift` (line 846)](https://github.com/facebookincubator/rebalancer/blob/main/interface/thrift/ProblemSpecs.thrift#L846)
- **Implementation**: `solver/`
- **Tests**: `solver/tests/`

## Next Steps

- Review [Balance Spec](balance) for standard balancing
- See [Minimize Squares](minimize-squares) for quadratic penalties
- Check [Cookbook: Load Balancing](../../cookbook/load-balancing) for real-world examples
