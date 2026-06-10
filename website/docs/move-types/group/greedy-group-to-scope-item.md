---
sidebar_position: 4
---

# GreedyGroupToScopeItem

**Move Type**: Group
**Complexity**: O(G × S × K) - greedy group placement
**Primary Use**: Place entire groups with unique container constraint

Move entire groups of objects to scope items where **each object gets a unique container**. Enforces anti-affinity within the group.

## Overview

`GreedyGroupToScopeItem` (also known as `GREEDY_GROUP_TO_SCOPE_ITEM`) is a specialized move type that places all objects in a group to containers within a single scope item, with the **critical constraint** that each object must go to a **different container**.

This enforces anti-affinity: if you have 5 objects in a group, they will be placed on 5 different containers within the chosen scope item.

**Use when**:
- Need anti-affinity within groups
- Each object in group requires unique container
- Placing replicas on different machines
- Task placement where tasks need different hosts
- Failure domain isolation within groups

**Avoid when**:
- Objects can share containers (use GroupMoveWithHintStrategies instead)
- Don't have enough containers per scope item
- No group structure
- Simple colocation without anti-affinity

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/greedy_group_to_scope_item_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/greedy_group_to_scope_item_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `groupMovesPartition` | string | **Yes** | null | Partition defining groups |
| `scopeItemMovesScope` | string | **Yes** | null | Scope to move groups into |
| `nSampleSetsToExplore` | int | No | 2 | Sample sets per scope item |

### Parameter Details

**groupMovesPartition**:
- The partition name defining the groups
- Each group will be moved as a unit
- All objects in a group move together to the same scope item
- But each object goes to a **different container** within that scope item

**scopeItemMovesScope**:
- The scope whose scope items will receive the groups
- Example: "rack", "datacenter", "availability_zone"
- Only scope items with enough containers are considered

**nSampleSetsToExplore**:
- Number of random container sets to try per scope item
- Higher values = better quality but slower
- For each scope item, generate k random sets of containers
- Each set has as many containers as objects in the group

## How It Works

For each **group** in the partition:

1. **Identify candidate scope items**: Find scope items in the target scope with at least N containers (where N = group size)
2. **For each scope item**: Generate `nSampleSetsToExplore` random sets of containers
   - Each set has exactly N unique containers
   - Random sampling for diversity
3. **Evaluate each set**: Test moving the group's objects to each container set
4. **Select best**: Choose the move that improves objective most
5. **Repeat**: Process all groups

### Visual Example

```
Group: job0 with 3 tasks [task0, task1, task2]

Scope: datacenter
ScopeItems:
  - dc1: 5 hosts [host1, host2, host3, host4, host5]
  - dc2: 3 hosts [host6, host7, host8]
  - dc3: 2 hosts [host9, host10]  ✗ Skip (not enough containers)

For dc1 (nSampleSetsToExplore=2):
  Sample Set 1: [host1, host3, host5]
    task0 → host1
    task1 → host3
    task2 → host5

  Sample Set 2: [host2, host4, host5]
    task0 → host2
    task1 → host4
    task2 → host5

For dc2 (nSampleSetsToExplore=2):
  Sample Set 1: [host6, host7, host8]
    task0 → host6
    task1 → host7
    task2 → host8

  Sample Set 2: [host6, host8, host7]
    task0 → host6
    task1 → host8
    task2 → host7

Evaluate all 4 sample sets, choose best
```

## Complexity

**Per group**: O(S × K)

Where:
- S = number of scope items in target scope
- K = `nSampleSetsToExplore`

**Total**: O(G × S × K)

Where:
- G = number of groups in partition

**Example calculation**:
- Groups (G): 100 jobs
- Scope items (S): 10 datacenters
- Sample sets (K): 2
- **Total evaluations**: 100 × 10 × 2 = **2,000**

## Usage Patterns

### Replica Anti-Affinity

Place replicas on different machines within same rack:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/greedy_group_to_scope_item_examples.py start=replica_antiaffinity_start end=replica_antiaffinity_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/greedy_group_to_scope_item_examples.cpp start=replica_antiaffinity_start end=replica_antiaffinity_end
```

</TabItem>
</Tabs>

### Job Task Placement

Place all tasks of a job in same datacenter on different hosts:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/greedy_group_to_scope_item_examples.py start=job_placement_start end=job_placement_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/greedy_group_to_scope_item_examples.cpp start=job_placement_start end=job_placement_end
```

</TabItem>
</Tabs>

### Higher Sampling

Increase sampling for better quality:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/greedy_group_to_scope_item_examples.py start=higher_sampling_start end=higher_sampling_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/greedy_group_to_scope_item_examples.cpp start=higher_sampling_start end=higher_sampling_end
```

</TabItem>
</Tabs>

## Performance Characteristics

### When Does It Help?

GreedyGroupToScopeItem helps when:
- **Anti-affinity required**: Objects in group must be on different containers
- **Failure domain isolation**: Spread replicas/tasks across machines
- **Rack/datacenter placement**: Group together in scope but spread across containers
- **Job scheduling**: Tasks on different hosts within cluster
- **High availability**: Ensure group members isolated

GreedyGroupToScopeItem does NOT help when:
- **Objects can share containers**: Use other group move types
- **Not enough containers**: Scope items have fewer containers than group size
- **No group structure**: Objects not organized in groups
- **Simple placement**: Don't need anti-affinity enforcement

### Sampling Trade-off

| nSampleSetsToExplore | Quality | Speed | Use Case |
|----------------------|---------|-------|----------|
| 1 | Lower | Fast | Quick placement |
| 2 | Good | Moderate | Default balanced |
| 5 | Better | Slower | High quality needed |
| 10 | Best | Slow | Critical placement |

## Comparison with Alternatives

| Move Type | Anti-Affinity | Constraint | Use Case |
|-----------|---------------|------------|----------|
| [ColocateGroups](colocate-groups) | No | Groups in same scope item | Colocation |
| **GreedyGroupToScopeItem** | Yes | Unique containers per object | Anti-affinity |
| [GroupMoveWithHintStrategies](group-move-with-hint-strategies) | Optional | Strategy-based | Large-scale with hints |

## Troubleshooting

### Problem: No moves found

**Diagnosis**: Scope items don't have enough containers

**Solutions**:
- Check group sizes vs containers per scope item
- Ensure scope items have ≥ N containers (N = max group size)
- Review partition definition
- May need to use different scope with more containers

### Problem: Groups not moving together

**Diagnosis**: Partition or scope configuration issue

**Solutions**:
- Verify `groupMovesPartition` is correct
- Check `scopeItemMovesScope` is defined
- Ensure objects are in the partition groups
- Review scope definition and membership

### Problem: Poor solution quality

**Diagnosis**: Not enough sampling diversity

**Solutions**:
- Increase `nSampleSetsToExplore` (try 5 or 10)
- Check if scope items are balanced
- May need additional move types
- Review objective function

### Problem: Too slow

**Diagnosis**: Too many groups or sample sets

**Solutions**:
- Reduce `nSampleSetsToExplore` to 1
- Check number of groups (may be very large)
- Consider batching groups
- May need different move type for this scale

## When to Use GreedyGroupToScopeItem

**DO use when**:
- Need anti-affinity within groups
- Each object requires unique container
- Placing replicas on different machines
- Task scheduling with host diversity
- Failure domain isolation

**DO NOT use when**:
- Objects can share containers
- Insufficient containers per scope item
- No group structure
- Don't need anti-affinity
- Looking for colocation without spreading

## Related Move Types

**Group-based alternatives**:
- [ColocateGroups](colocate-groups) - Collocate without anti-affinity
- [GroupMoveWithHintStrategies](group-move-with-hint-strategies) - Strategy-based group placement
- [GroupRouting](group-routing) - Routing-aware group placement

**General alternatives**:
- [Single](../basic/single) - Individual object moves
- [SingleGreedy](../basic/single-greedy) - Greedy single moves

## Source Code

- Thrift definition: [`interface/thrift/SolverSpecs.thrift:683`](https://github.com/facebookincubator/rebalancer/blob/main/interface/thrift/SolverSpecs.thrift#L683)
- Implementation: [`solver/moves/GreedyGroupToScopeItemMoveType.h`](https://github.com/facebookincubator/rebalancer/blob/main/solver/moves/GreedyGroupToScopeItemMoveType.h)
- Tests: [`interface/tests/GreedyGroupToScopeItemMoveTypeTest.cpp`](https://github.com/facebookincubator/rebalancer/blob/main/interface/tests/GreedyGroupToScopeItemMoveTypeTest.cpp)

## Next Steps

- Learn about [ColocateGroups](colocate-groups) for group colocation
- Try [GroupMoveWithHintStrategies](group-move-with-hint-strategies) for large-scale problems
- Review [Move Types Overview](../) for choosing move types
- See anti-affinity constraint documentation

## Notes

⚠️ **Unique Container Constraint**: This move type enforces that each object in a group goes to a **different container** within the chosen scope item. This is a hard constraint - scope items without enough containers will be skipped.
