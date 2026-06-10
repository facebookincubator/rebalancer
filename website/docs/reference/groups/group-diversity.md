---
sidebar_position: 21
---

# GroupDiversitySpec

**Type**: Constraint or Goal

Ensure each scope item receives objects from a minimum or maximum number of different groups, weighted by dimension values.

## Overview

`GroupDiversitySpec` controls how many distinct groups contribute to each scope item, weighted by a dimension. Unlike [GroupCountSpec](group-count) which counts groups, this spec uses dimension values to measure group presence.

**Use this when**: You need diversity requirements based on resource allocation (e.g., "each rack must have storage from at least 3 different databases") rather than simple counts.

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/reference/groups/group_diversity_spec_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/groups/group_diversity_spec_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Unique identifier for this spec |
| `scope` | string | Yes | - | Scope level (e.g., `"rack"`, `"host"`) |
| `partition` | string | Yes | - | Partition defining groups |
| `dimension` | string | Yes | - | Dimension for weighting group presence |
| `limit` | Limit | Yes | - | The diversity limit |
| `bound` | GroupDiversityBound | No | MIN | Minimum or maximum diversity |
| `filter` | Filter | No | null | Apply only to filtered objects |

## Bound Types

| Bound | Behavior | Example |
|-------|----------|---------|
| **MIN** | At least N groups must contribute dimension value | Each rack needs storage from ≥3 databases |
| **MAX** | At most N groups can contribute dimension value | Each host can have storage from ≤5 services |

## Common Usage Patterns

### 1. Minimum Storage Diversity

Ensure each rack has storage from multiple databases:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/groups/group_diversity_spec_examples.py start=min_diversity_start end=min_diversity_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/groups/group_diversity_spec_examples.cpp start=min_diversity_start end=min_diversity_end
```

</TabItem>
</Tabs>

### 2. Maximum Service Diversity

Limit how many different services can use CPU on each host:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/groups/group_diversity_spec_examples.py start=max_diversity_start end=max_diversity_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/groups/group_diversity_spec_examples.cpp start=max_diversity_start end=max_diversity_end
```

</TabItem>
</Tabs>

## How It Works

For each scope item, the spec counts how many groups have **non-zero** dimension value:
- A group "contributes" if any object from that group on the scope item has dimension > 0
- The count is compared against the limit
- MIN bound: scope item must have at least N contributing groups
- MAX bound: scope item can have at most N contributing groups

## Performance Considerations

- **Complexity**: O(groups × containers × objects)
- **Memory**: Tracks group presence per scope item
- **Optimization**: Use filter to reduce object set

## Comparison with Related Specs

| Spec | Measures | Use When |
|------|----------|----------|
| **GroupDiversitySpec** | Dimension-weighted group presence | Need resource-based diversity |
| [GroupCountSpec](group-count) | Count of groups (unweighted) | Simple count-based diversity |
| [GroupIsolationLimitSpec](group-isolation-limit) | Distinct groups per container | Isolation or mixing requirements |

## Troubleshooting

**Issue**: Diversity constraint not satisfied
- **Solution**: Check that enough groups exist and can be distributed

**Issue**: Too restrictive with MAX bound
- **Solution**: Increase the limit or review group distribution

## Related Specs

- [GroupCountSpec](group-count) - Count-based group limits
- [GroupIsolationLimitSpec](group-isolation-limit) - Distinct group limits
- [ColocateGroupsSpec](colocate-groups) - Group colocation

## Source Code

- **Thrift definition**: [`interface/thrift/ProblemSpecs.thrift` (line 1168)](https://github.com/facebookincubator/rebalancer/blob/main/interface/thrift/ProblemSpecs.thrift#L1168)
- **Implementation**: `solver/`
- **Tests**: `solver/tests/`

## Next Steps

- Review [Group Count Spec](group-count) for count-based diversity
- See [Group Isolation Limit](group-isolation-limit) for alternative approach
- Check [Cookbook: Disaster Recovery](../../cookbook/disaster-recovery) for real-world examples
