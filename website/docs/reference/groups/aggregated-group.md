---
sidebar_position: 19
---

# AggregatedGroupSpec

**Type**: Constraint or Goal

Aggregate and limit group utilization across containers using flexible aggregation functions.

## Overview

`AggregatedGroupSpec` provides powerful control over how group utilization is aggregated and limited. Unlike basic capacity limits, it supports multi-level aggregation: within each group, across groups, and across containers. This enables complex constraints like "the max total of the top 3 groups per container must not exceed X."

**Key features**:
- **Three aggregation levels**: within-group, across-groups, across-containers
- **Multiple aggregation types**: SUM, MAX, MIN, LINEAR_SUM
- **Flexible limits**: Global or per-item limits with optional contributions
- **Filter support**: Apply only to subset of objects

**Use this when**: You need advanced aggregation patterns beyond simple capacity limits, such as limiting peak group totals or complex hierarchical aggregations.

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/reference/groups/aggregated_group_spec_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/groups/aggregated_group_spec_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Unique identifier for this spec |
| `scope` | string | Yes | - | Scope to apply aggregation (e.g., `"host"`, `"rack"`) |
| `dimension` | string | Yes | - | Dimension to aggregate |
| `partitionName` | string | Yes | - | Partition defining groups |
| `limit` | Limit | No | ABSOLUTE | The limit value |
| `withinGroupAggregationType` | AggregatedGroupSpecAggType | No | SUM | How to aggregate within each group |
| `groupAggregationType` | AggregatedGroupSpecAggType | No | MAX | How to aggregate across groups |
| `containerAggregationType` | AggregatedGroupSpecAggType | No | SUM | How to aggregate across containers |
| `filter` | Filter | No | null | Apply only to filtered objects |
| `contributions` | map&lt;string, Limit&gt; | No | null | Per-item contributions (for LINEAR_SUM) |

## Aggregation Types

| Type | Description | Use Case |
|------|-------------|----------|
| **SUM** | Sum all values | Total utilization |
| **MAX** | Take maximum value | Peak utilization |
| **MIN** | Take minimum value | Minimum presence |
| **LINEAR_SUM** | Weighted sum using contributions | Custom weighting |

## Aggregation Flow

The three-level aggregation works as follows:

1. **Within-group**: Aggregate objects within each group on each container
2. **Across-groups**: Aggregate the results across different groups on each container
3. **Across-containers**: Aggregate the container results across all containers in scope

Example: `withinGroup=SUM`, `group=MAX`, `container=SUM`
- Within each group: SUM object utilizations
- Across groups: take MAX group total per container
- Across containers: SUM the max values

## Common Usage Patterns

### 1. Limit Peak Group Total

Ensure the largest group on each host doesn't exceed a limit:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/groups/aggregated_group_spec_examples.py start=peak_group_total_start end=peak_group_total_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/groups/aggregated_group_spec_examples.cpp start=peak_group_total_start end=peak_group_total_end
```

</TabItem>
</Tabs>

### 2. Total Across Top N Groups

Limit the sum of the top N groups across all containers:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/groups/aggregated_group_spec_examples.py start=top_n_groups_start end=top_n_groups_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/groups/aggregated_group_spec_examples.cpp start=top_n_groups_start end=top_n_groups_end
```

</TabItem>
</Tabs>

### 3. Weighted Aggregation

Use LINEAR_SUM with custom contributions for weighted aggregation:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/groups/aggregated_group_spec_examples.py start=weighted_aggregation_start end=weighted_aggregation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/groups/aggregated_group_spec_examples.cpp start=weighted_aggregation_start end=weighted_aggregation_end
```

</TabItem>
</Tabs>

## Performance Considerations

- **Complexity**: O(groups × containers × objects) for full aggregation
- **Memory**: Stores intermediate aggregation results
- **Optimization**: Filter to reduce objects being aggregated

## Comparison with Related Specs

| Spec | Use When |
|------|----------|
| **AggregatedGroupSpec** | Need multi-level aggregation (MAX, MIN, custom weights) |
| [GroupCapacitySpec](group-capacity) | Simple per-group capacity limits |
| [CapacitySpec](../capacity) | Simple per-container capacity limits |
| [GroupCountSpec](group-count) | Counting groups, not aggregating utilization |

## Troubleshooting

**Issue**: Aggregation result unexpected
- **Solution**: Verify each aggregation level separately - check within-group, across-groups, and across-containers logic

**Issue**: Performance slow with many groups
- **Solution**: Use filter to reduce object set, or simplify aggregation types

**Issue**: LINEAR_SUM not working
- **Solution**: Ensure `contributions` map is properly populated for all relevant items

## Related Specs

- [GroupCapacitySpec](group-capacity) - Simpler per-group capacity
- [CapacitySpec](../capacity) - Basic capacity limits
- [GroupCountSpec](group-count) - Count-based group limits

## Source Code

- **Thrift definition**: [`interface/thrift/ProblemSpecs.thrift` (line 91)](https://github.com/facebookincubator/rebalancer/blob/main/interface/thrift/ProblemSpecs.thrift#L91)
- **Implementation**: `solver/`
- **Tests**: `solver/tests/`

## Next Steps

- Review [Group Capacity](group-capacity) for simpler group limits
- See [Capacity Spec](../capacity) for basic capacity constraints
- Check [Cookbook: Multi-tenant](../../cookbook/multi-tenant) for real-world examples
