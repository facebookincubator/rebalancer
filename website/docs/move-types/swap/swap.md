---
sidebar_position: 1
---

# Swap

**Move Type**: Swap
**Complexity**: O(objects²)

Exchange two objects between containers. Essential for capacity-constrained problems where you can't simply add objects without removing others.

## Overview

`Swap` evaluates exchanging pairs of objects between the hot container and other containers. This is critical when containers are near capacity and single moves would violate constraints.

**Use when**:
- Capacity constraints are tight
- Moving an object requires making room first
- Single moves alone aren't finding improvements

**Avoid when**:
- Problem is unconstrained (Single is faster and sufficient)
- Problem is very large (>10K objects - use SwapSampled instead)

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/swap_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/swap_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `partitionNameToExploreSwapsWithinObjectGroup` | string | No | null | Only swap objects within same group |
| `greedyOnSrc` | bool | No | false | Exit early when swapping source objects |
| `greedyOnDst` | bool | No | false | Exit early when swapping destination objects |
| `destinationsToExplore` | DestinationsToExploreOptions | No | null | Restrict destination containers |
| `sampleSize` | SampleSize | No | null | Sample subset of objects on both sides |
| `swapRatioDimension` | StringKeyValueMap | No | null | Dimension for uneven k:1 swaps |
| `objectBundleFormationHints` | ObjectBundleFormationHints | No | null | Bundle objects for certain containers |

## How It Works

Given a **hot container** (the container contributing most to the objective):

1. **Select hot object**: Pick one object from the hot container
2. **Select destination container**: Pick a different container
3. **Select other object**: Pick one object from the destination container
4. **Evaluate**: Test swapping the two objects (hot object ↔ other object)
5. **Repeat**: Try all combinations of (hot object, destination container, other object)
6. **Apply best**: Apply the swap that improves the objective most

### Visual Example

```
Before:                          After (if swap applied):
┌─────────────┐                 ┌─────────────┐
│ Hot         │                 │ Hot         │
│ Container   │    ┌─swap─┐    │ Container   │
│  • obj1     │ ───┤      │──> │  • objX ←─┐ │
│  • obj2     │    │      │    │  • obj2   │ │
│  • obj3     │ ←──┤      │─── │  • obj3   │ │
└─────────────┘    └──────┘    └───────────┼─┘
┌─────────────┐                 ┌───────────┼─┐
│ Other       │                 │ Other     ↓ │
│ Container   │                 │ Container   │
│  • objX     │                 │  • obj1  ←  │
│  • objY     │                 │  • objY     │
└─────────────┘                 └─────────────┘
```

## Complexity

**Moves evaluated per iteration**: O(N × M × C)
- Simplifies to O(N²) when containers have similar sizes

Where:
- N = number of objects in hot container
- M = average number of objects in destination containers
- C = number of destination containers

**Example**:
- Hot container has 100 objects
- Each destination container has ~100 objects
- System has 50 containers
- Moves evaluated = 100 × 100 × 50 = 500,000 moves

**Warning**: Swap is **much more expensive** than Single. Use judiciously.

## Usage Patterns

### Basic Capacity-Constrained Problem

Most common usage - combine Single with Swap:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/swap_examples.py start=basic_capacity_constrained_start end=basic_capacity_constrained_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/swap_examples.cpp start=basic_capacity_constrained_start end=basic_capacity_constrained_end
```

</TabItem>
</Tabs>

### Greedy Swap (Faster)

Use greedy flags for faster convergence:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/swap_examples.py start=greedy_swap_start end=greedy_swap_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/swap_examples.cpp start=greedy_swap_start end=greedy_swap_end
```

</TabItem>
</Tabs>

### Sampled Swap (Large Problems)

For large problems, sample objects to reduce cost:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/swap_examples.py start=sampled_swap_start end=sampled_swap_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/swap_examples.cpp start=sampled_swap_start end=sampled_swap_end
```

</TabItem>
</Tabs>

### Swap Within Groups

Only swap objects within the same partition group:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/swap_examples.py start=swap_within_groups_start end=swap_within_groups_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/swap_examples.cpp start=swap_within_groups_start end=swap_within_groups_end
```

</TabItem>
</Tabs>

### Restricted Destinations

Limit which containers to explore:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/swap_examples.py start=restricted_destinations_start end=restricted_destinations_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/swap_examples.cpp start=restricted_destinations_start end=restricted_destinations_end
```

</TabItem>
</Tabs>

## Performance Optimization

### When Swap is Slow

**Greedy flags**: Exit early when improvement found
```python
SwapMoveTypeSpec(
    greedyOnSrc=True,  # Try src objects one by one, exit on improvement
    greedyOnDst=True,  # Try dst objects one by one, exit on improvement
)
```

**Sampling**: Evaluate subset of swaps
```python
SwapMoveTypeSpec(
    sampleSize=SampleSize(
        defaultSampleSize=100,  # Only sample 100 objects per side
    ),
)
```

**Destination filtering**: Limit containers to explore
```python
SwapMoveTypeSpec(
    destinationsToExplore=MoveToCurrentScopeItemSpec(
        scopeNameForExploringMovesToCurrentScopeItem="rack",
    ),
)
```

### Recommended Settings by Problem Size

| Objects | Containers | Configuration |
|---------|------------|---------------|
| &lt;1K | &lt;100 | Default Swap (no sampling) |
| 1K-10K | 100-1K | Greedy flags + sampling (100-500) |
| &gt;10K | &gt;1K | [SwapSampled](swap-sampled) with aggressive sampling |

## Comparison with Variants

| Move Type | Speed | Completeness | Use Case |
|-----------|-------|--------------|----------|
| **Swap** | Slow | Complete | Default swap, thorough |
| [SwapSampled](swap-sampled) | Medium | Sampled | Large problems |
| [SwapFullContainers](swap-full-containers) | Medium | Full containers only | Container-level swaps |
| [SwapFullWithEmpty](swap-full-with-empty) | Fast | Empty targets only | Consolidation |

## Troubleshooting

### Problem: Swap is too slow

**Diagnosis**: O(N²) complexity too expensive for problem size

**Solutions**:
- Enable `greedyOnSrc=True` and `greedyOnDst=True`
- Add sampling: `sampleSize=SampleSize(defaultSampleSize=100)`
- Switch to [SwapSampled](swap-sampled) for large problems
- Restrict destinations with `destinationsToExplore`
- Use [SwapFullWithEmpty](swap-full-with-empty) if applicable

### Problem: Swap not improving objective

**Diagnosis**: May need more complex moves

**Solutions**:
- Ensure Single moves tried first (Swap should be after Single)
- Add [SingleEndChain](../chain/single-end-chain) for 2-move sequences
- Check if swaps are actually beneficial for your problem
- Verify capacity constraints are preventing Single moves

### Problem: Swap causing constraint violations

**Diagnosis**: Swapped objects have different dimensions

**Solutions**:
- Check that swapped objects satisfy constraints
- Use `partitionNameToExploreSwapsWithinObjectGroup` to swap similar objects
- Review capacity and group count constraints
- May need different move type (e.g., SingleEndChain)

## Related Move Types

**Variants**:
- [SwapSampled](swap-sampled) - Sampled version for large problems
- [SwapFullContainers](swap-full-containers) - Swap all objects between containers
- [SwapFullWithEmpty](swap-full-with-empty) - Move all to empty container

**Alternatives**:
- [SingleEndChain](../chain/single-end-chain) - 2-move sequence (often better than Swap)
- [Single](../basic/single) - Simpler, use when capacity not tight

**Complementary**:
- [Single](../basic/single) - Always use Single before Swap

## Source Code

- Thrift definition: [`interface/thrift/SolverSpecs.thrift:537`](https://github.com/facebookincubator/rebalancer/blob/main/interface/thrift/SolverSpecs.thrift#L537)
- Implementation: [`solver/moves/SwapMoveType.h`](https://github.com/facebookincubator/rebalancer/blob/main/solver/moves/SwapMoveType.h)
- Tests: [`solver/moves/tests/SwapMoveTypeTest.cpp`](https://github.com/facebookincubator/rebalancer/tree/main/solver/moves/tests/SwapMoveTypeTest.cpp)

## Next Steps

- Try [SwapSampled](swap-sampled) for large-scale problems
- Learn about [SingleEndChain](../chain/single-end-chain) as alternative to Swap
- Review [Move Types Overview](../) for choosing move types
- See [Performance Guide](../../solvers/performance) for optimization strategies
