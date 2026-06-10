---
sidebar_position: 16
---

# ExclusiveScopeItemsSpec

**Type**: Constraint or Goal

Ensure certain scope items are mutually exclusive - they cannot both be utilized concurrently.

## Overview

`ExclusiveScopeItemsSpec` defines pairs or groups of scope items that conflict with each other. When scope items are exclusive, objects (or objects from the same group) cannot utilize both conflicting scope items simultaneously.

**Use this when**: You need anti-affinity constraints like "replicas cannot be on racks 1 and 2 simultaneously" or "these two datacenters are mutually exclusive for this service."

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/reference/placement/exclusive_scope_items_spec_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/placement/exclusive_scope_items_spec_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Unique identifier for this spec |
| `scope` | string | Yes | - | Scope level (e.g., `"rack"`, `"datacenter"`) |
| `dimension` | string | Yes | - | Dimension to check for utilization |
| `partitionName` | string | No | null | If set, exclusivity applies per-group |
| `conflictInfoList` | list&lt;ScopeItemConflictInfo&gt; | Yes | - | Modern conflict definitions |
| `scopeItemWeights` | map&lt;string, double&gt; | No | {} | Weights for AGGRESSIVE_PACKING formula |
| `formula` | ExclusiveScopeItemsFormula | No | MINIMIZE_INVALIDATED_SCOPE_ITEMS_COUNT | Packing formula |

## Conflict Info Structure

**ScopeItemConflictInfo**:
- `scopeItem` (string): The scope item
- `conflictingScopeItemsWithOverlap` (list&lt;ConflictingScopeItemInfo&gt;): Items it conflicts with

**ConflictingScopeItemInfo**:
- `conflictingScopeItem` (string): The conflicting scope item name
- `overlap` (double): Overlap weight (default: 1.0)

## Formulas

| Formula | Behavior |
|---------|----------|
| **MINIMIZE_INVALIDATED_SCOPE_ITEMS_COUNT** | Default, simple counting of invalidated items |
| **AGGRESSIVE_PACKING** | Advanced packing using weights and overlaps |

## Common Usage Patterns

### 1. Rack Anti-Affinity

Ensure replicas of the same database don't use conflicting racks:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/placement/exclusive_scope_items_spec_examples.py start=rack_anti_affinity_start end=rack_anti_affinity_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/placement/exclusive_scope_items_spec_examples.cpp start=rack_anti_affinity_start end=rack_anti_affinity_end
```

</TabItem>
</Tabs>

### 2. Datacenter Exclusivity

Certain datacenters cannot both be used by the same service:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/placement/exclusive_scope_items_spec_examples.py start=datacenter_exclusivity_start end=datacenter_exclusivity_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/placement/exclusive_scope_items_spec_examples.cpp start=datacenter_exclusivity_start end=datacenter_exclusivity_end
```

</TabItem>
</Tabs>

## Performance Considerations

- **Complexity**: O(scope items × conflicts)
- **Memory**: Stores conflict graph
- **Optimization**: Minimize number of conflict relationships

## Comparison with Related Specs

| Spec | Use When |
|------|----------|
| **ExclusiveScopeItemsSpec** | Scope items are mutually exclusive |
| [GroupCountSpec](../groups/group-count) | Limit groups per scope item (not mutual exclusion) |
| [AvoidAssignmentsSpec](avoid-assignments) | Prevent specific object-container assignments |

## Troubleshooting

**Issue**: Conflicts not being enforced
- **Solution**: Verify conflictInfoList is properly structured with correct scope item names

**Issue**: Over-constrained problem
- **Solution**: Review conflict graph - may have too many exclusions

## Related Specs

- [AvoidAssignmentsSpec](avoid-assignments) - Forbidden assignments
- [GroupCountSpec](../groups/group-count) - Group count limits
- [NonAcceptingSpec](../placement/non-accepting) - Container exclusions

## Source Code

- **Thrift definition**: [`interface/thrift/ProblemSpecs.thrift` (line 537)](https://github.com/facebookincubator/rebalancer/blob/main/interface/thrift/ProblemSpecs.thrift#L537)
- **Implementation**: `solver/`
- **Tests**: `solver/tests/`

## Next Steps

- Review [Avoid Assignments](avoid-assignments) for object-level exclusions
- See [Group Count](../groups/group-count) for diversity constraints
- Check [Cookbook: Disaster Recovery](../../cookbook/disaster-recovery) for real-world examples
