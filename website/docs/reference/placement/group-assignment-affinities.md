---
sidebar_position: 20
---

# GroupAssignmentAffinitiesSpec

**Type**: Goal only

Create affinity between groups and specific scope items based on target dimension values.

## Overview

`GroupAssignmentAffinitiesSpec` assigns preference scores for placing groups on specific containers. Each affinity specifies a target dimension value (what fraction of the group should go to a container) and an affinity score (how strongly to prefer that placement).

**Use this when**: You want specific groups to prefer specific containers with configurable strength, such as routing certain services to designated hosts or keeping high-priority workloads on premium infrastructure.

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/reference/placement/group_assignment_affinities_spec_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/placement/group_assignment_affinities_spec_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Unique identifier for this spec |
| `scope` | string | Yes | - | Scope level (e.g., `"host"`, `"rack"`) |
| `partition` | string | Yes | - | Partition defining groups |
| `dimension` | string | Yes | - | Dimension for target values |
| `affinities` | list&lt;GroupScopeItemAffinity&gt; | Yes | - | List of group-container affinities |

## GroupScopeItemAffinity Structure

Each affinity entry specifies:

| Field | Type | Description |
|-------|------|-------------|
| `group` | string | Group name from partition |
| `scopeItem` | string | Target scope item (container) |
| `targetDimensionValue` | double | Target fraction of group to place here |
| `affinity` | double | Preference strength (higher = stronger) |

## Common Usage Patterns

### 1. Route Services to Designated Hosts

Prefer specific services on specific hosts:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/placement/group_assignment_affinities_spec_examples.py start=designated_hosts_start end=designated_hosts_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/placement/group_assignment_affinities_spec_examples.cpp start=designated_hosts_start end=designated_hosts_end
```

</TabItem>
</Tabs>

### 2. Distribute Group Across Multiple Containers

Split a group across several containers with target fractions:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/placement/group_assignment_affinities_spec_examples.py start=split_group_start end=split_group_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/placement/group_assignment_affinities_spec_examples.cpp start=split_group_start end=split_group_end
```

</TabItem>
</Tabs>

### 3. Priority-Based Placement

Use affinity scores to express priority tiers:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/placement/group_assignment_affinities_spec_examples.py start=priority_placement_start end=priority_placement_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/placement/group_assignment_affinities_spec_examples.cpp start=priority_placement_start end=priority_placement_end
```

</TabItem>
</Tabs>

## How Affinity Scoring Works

The contribution to the objective function is:
```
score = affinity × (actual_dimension_value - target_dimension_value)²
```

- **Higher affinity** = stronger preference
- **Closer to target** = better score
- **Squared difference** = quadratic penalty for deviation

## Performance Considerations

- **Complexity**: O(affinities × objects)
- **Memory**: Stores affinity list
- **Optimization**: Use only for groups that need specific placement

## Comparison with Related Specs

| Spec | Use When |
|------|----------|
| **GroupAssignmentAffinitiesSpec** | Group-level affinities with target fractions |
| [AssignmentAffinitiesSpec](assignment-affinities) | Object-level affinities (individual objects) |
| [ScopeAffinitiesSpec](scope-affinities) | Container-level affinities based on object dimension |
| [PairAffinitiesSpec](pair-affinities) | Pair colocation preferences |

## Troubleshooting

**Issue**: Groups not reaching target fractions
- **Solution**: Increase affinity score or check for conflicting constraints

**Issue**: Unbalanced placement despite affinities
- **Solution**: Balance affinity weights against other goals (e.g., BalanceSpec)

**Issue**: Performance degradation with many affinities
- **Solution**: Limit affinities to critical groups only

## Related Specs

- [AssignmentAffinitiesSpec](assignment-affinities) - Object-level affinities
- [ScopeAffinitiesSpec](scope-affinities) - Container affinity scores
- [PairAffinitiesSpec](pair-affinities) - Pair colocation

## Source Code

- **Thrift definition**: [`interface/thrift/ProblemSpecs.thrift` (line 134)](https://github.com/facebookincubator/rebalancer/blob/main/interface/thrift/ProblemSpecs.thrift#L134)
- **Implementation**: `solver/`
- **Tests**: `solver/tests/`

## Next Steps

- Review [Assignment Affinities](assignment-affinities) for object-level preferences
- See [Scope Affinities](scope-affinities) for container-based affinities
- Check [Cookbook: Affinity Placement](../../cookbook/affinity-placement) for real-world examples
