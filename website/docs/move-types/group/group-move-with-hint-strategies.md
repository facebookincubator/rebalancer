---
sidebar_position: 2
---

# GroupMoveWithHintStrategies

**Move Type**: Group
**Complexity**: O(T × N × S × K) - efficient for million-object problems
**Primary Use**: TorchRec shard placement, large-scale group-based placement

Move groups of objects using **strategy hints** per group. Designed for large-scale problems with multiple groups having different constraints.

## Overview

`GroupMoveWithHintStrategies` (also known as `GROUP_MOVE_WITH_HINT_STRATEGIES`) enables applying **different move strategies** for different groups based on their unique constraints. Instead of trying all possible combinations (which would be impractical for million-object problems), this move type uses hints to guide the solver toward feasible moves for each group.

**Use when**:
- Large-scale problems (millions of objects)
- Different groups have different constraints
- Know which strategies work well for each group
- Have nested partition structure (primary + secondary)
- TorchRec table sharding scenarios
- Need efficient moves for heterogeneous groups

**Avoid when**:
- Simple homogeneous problems
- No group-specific constraints
- Don't have strategy hints
- Single partition is sufficient

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/group_move_with_hint_strategies_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/group_move_with_hint_strategies_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `primaryPartition` | string | **Yes** | null | Outer partition (e.g., tables) |
| `secondaryPartition` | string | **Yes** | null | Inner partition (e.g., shard types) |
| `moveStrategies` | MoveStrategies | **Yes** | null | Map of group → strategy |
| `unassignedContainer` | string | No | null | Container for unassigned objects |
| `secondaryGroupReplacementConfig` | SecondaryGroupReplacementConfig | No | null | Allowed secondary group replacements |

### Parameter Details

**primaryPartition**:
- The outer partition defining primary groups
- Example: "table" partition in TorchRec
- Each primary group processed independently

**secondaryPartition**:
- The inner partition splitting the primary partition
- Example: "shard_type" partition in TorchRec
- Defines subgroups within each primary group

**moveStrategies**:
- Map: `group_name → MoveStrategy`
- Each `MoveStrategy` contains:
  - `type`: `RANDOM_SAMPLING_WITH_REPLACEMENT` or `RANDOM_SAMPLING_WITHOUT_REPLACEMENT`
  - `moveSetsGeneratedPerScopeItem`: Number of move sets to generate (default: 1)
  - `moveToScopeItems`: Destination scope items specification
  - `tertiaryPartition`: Optional third partition level
  - `numScopeItemsToExplorePerTertiaryGroup`: Optional sampling for tertiary groups

**unassignedContainer**:
- Optional container for initially unassigned objects
- Enables group replacement when specified

**secondaryGroupReplacementConfig**:
- Controls which secondary groups can replace each other
- Only used when `unassignedContainer` is set
- Map: `secondary_group → allowed_replacement_groups`

## How It Works

For each **primary group** (e.g., table):

1. **Identify secondary groups**: Find all secondary groups (e.g., shard types) within this primary group
2. **Apply strategy per group**: For each secondary group:
   - Look up the strategy hint for this group
   - Generate move sets according to the strategy
   - Sample with/without replacement as specified
3. **Evaluate move sets**: Test all generated move sets in parallel
4. **Select best**: Choose the move set that improves objective most
5. **Repeat**: Process all primary groups

### Visual Example

```
Primary Partition: Tables
Secondary Partition: Shard Types

Table1:
  ├─ row_wise shards    → Strategy: Random sampling with replacement
  ├─ column_wise shards → Strategy: Random sampling without replacement
  └─ data_parallel shard → Strategy: Random sampling with replacement

Table2:
  ├─ row_wise shards    → Strategy: Random sampling with replacement
  ├─ column_wise shards → Strategy: Random sampling without replacement
  └─ data_parallel shard → Strategy: Random sampling with replacement

Each table × shard type combination gets its own strategy
```

## Complexity

**Per primary group**: O(N × S × K)

Where:
- N = average objects per secondary group
- S = number of secondary groups
- K = move sets generated per secondary group

**Total**: O(T × N × S × K)

Where:
- T = number of primary groups (tables)

**Real-world TorchRec example**:
- Tables (T): 2,500
- Shards per group (N): 10
- Shard types (S): 10
- Move sets per group (K): 1
- **Total evaluations**: 2,500 × 10 × 10 × 1 = **250,000**
- **At 100K eval/sec**: ~2.5 seconds

**Worst case (10M shards, 5K tables)**:
- Tables (T): 5,000
- Shards per group (N): 200
- Shard types (S): 10
- Move sets per group (K): 1
- **Total evaluations**: **10,000,000**
- **At 100K eval/sec**: ~100 seconds

## Usage Patterns

### TorchRec Table Sharding

Different strategies for different shard types:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/group_move_with_hint_strategies_examples.py start=torchrec_start end=torchrec_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/group_move_with_hint_strategies_examples.cpp start=torchrec_start end=torchrec_end
```

</TabItem>
</Tabs>

### With Replacement vs Without

Different sampling strategies for different constraints:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/group_move_with_hint_strategies_examples.py start=sampling_types_start end=sampling_types_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/group_move_with_hint_strategies_examples.cpp start=sampling_types_start end=sampling_types_end
```

</TabItem>
</Tabs>

### Multiple Move Sets

Generate multiple move sets per group:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/group_move_with_hint_strategies_examples.py start=multiple_movesets_start end=multiple_movesets_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/group_move_with_hint_strategies_examples.cpp start=multiple_movesets_start end=multiple_movesets_end
```

</TabItem>
</Tabs>

### With Unassigned Container

Enable group replacement via unassigned container:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/group_move_with_hint_strategies_examples.py start=unassigned_start end=unassigned_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/group_move_with_hint_strategies_examples.cpp start=unassigned_start end=unassigned_end
```

</TabItem>
</Tabs>

## Performance Characteristics

### Strategy Comparison

| Strategy | Replacement | Use Case |
|----------|-------------|----------|
| `RANDOM_SAMPLING_WITH_REPLACEMENT` | Yes | Can place multiple objects on same container |
| `RANDOM_SAMPLING_WITHOUT_REPLACEMENT` | No | Each container used at most once |

### Scalability

| Objects | Tables | Shard Types | K | Evaluations | Time @100K/s |
|---------|--------|-------------|---|-------------|--------------|
| 250K | 2.5K | 10 | 1 | 250K | 2.5s |
| 250K | 2.5K | 10 | 5 | 1.25M | 12.5s |
| 10M | 5K | 10 | 1 | 10M | 100s |

**Key insight**: Even with 10 million objects, this approach is tractable due to strategy hints

### When Does It Help?

GroupMoveWithHintStrategies helps when:
- **Large scale**: Millions of objects to place
- **Heterogeneous groups**: Different groups have different constraints
- **Known strategies**: You know which approach works for each group
- **Nested structure**: Primary + secondary partition hierarchy
- **TorchRec workloads**: Table sharding scenarios

GroupMoveWithHintStrategies does NOT help when:
- **Small problems**: Overhead not worth it
- **Homogeneous groups**: All groups have same constraints
- **No hints available**: Don't know which strategies to use
- **Simple structure**: Single partition sufficient

## Comparison with Alternatives

| Move Type | Approach | Scale | Use Case |
|-----------|----------|-------|----------|
| [Single](../basic/single) | Explore all | Small | Independent objects |
| [ColocateGroups](colocate-groups) | Colocation | Medium | Related groups together |
| **GroupMoveWithHintStrategies** | Strategy hints | Large | Million+ objects with hints |

## Troubleshooting

### Problem: Too slow even with hints

**Diagnosis**: Too many move sets being generated

**Solutions**:
- Reduce `moveSetsGeneratedPerScopeItem` (keep at 1 initially)
- Ensure strategies are appropriate for each group
- Check if tertiary partition is needed
- Review number of secondary groups

### Problem: Poor solution quality

**Diagnosis**: Strategy hints not optimal for groups

**Solutions**:
- Review which strategy type works best for each group
- Try `RANDOM_SAMPLING_WITHOUT_REPLACEMENT` for exclusive placement
- Try `RANDOM_SAMPLING_WITH_REPLACEMENT` for flexible placement
- Increase `moveSetsGeneratedPerScopeItem` (e.g., 3-5)
- May need different `moveToScopeItems` per group

### Problem: Groups not moving as expected

**Diagnosis**: Partition or strategy configuration issue

**Solutions**:
- Verify primary and secondary partitions are correct
- Check that all secondary groups have strategies
- Review `moveToScopeItems` destinations
- Ensure unassignedContainer is set if using replacement

### Problem: Strategy not defined for group

**Diagnosis**: Missing strategy hint in `moveStrategies` map

**Solutions**:
- Add strategy for every secondary group
- Check group names match partition exactly
- Review partition definition

## When to Use GroupMoveWithHintStrategies

**DO use when**:
- Large-scale problems (100K+ objects)
- Have nested partition structure (primary + secondary)
- Know which strategies work for each group
- Different groups have different constraints
- TorchRec or similar workloads

**DO NOT use when**:
- Small problems (&lt;10K objects)
- Single partition sufficient
- All groups homogeneous
- Don't have strategy hints
- Need exploratory approach

## Related Move Types

**Group-based alternatives**:
- [ColocateGroups](colocate-groups) - Collocate related groups
- [GroupRouting](group-routing) - Group-aware routing
- [GreedyGroupToScopeItem](greedy-group-to-scope-item) - Greedy group placement

**Simpler alternatives**:
- [Single](../basic/single) - For independent objects
- [SingleGreedy](../basic/single-greedy) - Greedy single moves

## Source Code

- Thrift definition: [`interface/thrift/SolverSpecs.thrift:666`](https://github.com/facebookincubator/rebalancer/blob/main/interface/thrift/SolverSpecs.thrift#L666)
- Implementation: [`solver/moves/GroupMoveWithHintStrategiesMoveType.h`](https://github.com/facebookincubator/rebalancer/blob/main/solver/moves/GroupMoveWithHintStrategiesMoveType.h)
- Tests: [`solver/moves/tests/`](https://github.com/facebookincubator/rebalancer/tree/main/solver/moves/tests/)

## Next Steps

- Learn about [ColocateGroups](colocate-groups) for group colocation
- Try [GroupRouting](group-routing) for routing-based placement
- Review [Move Types Overview](../) for choosing move types
- See TorchRec documentation for real-world usage examples
