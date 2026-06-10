---
sidebar_position: 1
---

# FixedDest

**Move Type**: Fixed
**Complexity**: O(sample_size) instead of O(objects × containers)

Move objects from hot container to a **specific fixed destination**. Useful when you know exactly where objects should go.

## Overview

`FixedDest` (also known as `SINGLE_FIXED_DEST`) evaluates moving objects from the hot container to a **single predetermined destination container**. Instead of exploring all possible destinations, it only considers moves to one specific container.

**Use when**:
- Know exactly where objects should move (e.g., new server, specific region)
- Migrating workload to specific destination
- Filling a specific underutilized container
- Testing "what if we moved to container X"

**Avoid when**:
- Don't know destination (use [Single](../basic/single))
- Need to explore multiple destinations
- Destination keeps changing
- Want solver to find best destination

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `specialContainer` | string | **Yes** | null | Fixed destination container name |
| `sampleSize` | SampleSize | No | null | Sample subset of objects (probabilistic) |

### Parameter Details

**specialContainer**:
- Name of the specific destination container
- All object moves will target this container only
- Must be a valid container name

**sampleSize**:
- Optional sampling to reduce evaluations
- Each object sampled with probability = `sampleSize / (objects in hot container)`
- Useful for very large hot containers

## How It Works

Given a **hot container** (most broken):

1. **Select object**: Pick object from hot container
2. **Sample**: With probability `sampleSize / N`, evaluate this object (if sampling enabled)
3. **Evaluate move**: Test moving object to `specialContainer`
4. **Repeat**: Try all objects in hot container
5. **Apply best**: Apply the move to `specialContainer` that improves objective most

### Visual Example

```
Before move:                          After move to specialContainer:
┌──────────────┐                     ┌──────────────┐
│ Hot          │                     │ Hot          │
│ Container    │  ───────────>       │ Container    │
│  • obj1  ────┼──┐                  │  • obj2      │
│  • obj2      │  │                  │  • obj3      │
│  • obj3      │  │                  └──────────────┘
└──────────────┘  │
                  │                  ┌──────────────┐
┌──────────────┐  │                  │ Special      │
│ Special      │  │                  │ Container    │
│ Container    │  │                  │  • objA      │
│  • objA      │  │                  │  • objB      │
│  • objB  <───┼──┘                  │  • obj1 ←────┼── Moved here!
└──────────────┘                     └──────────────┘

Only one destination considered: specialContainer
```

### Comparison with Regular Single

| Aspect | Single | FixedDest |
|--------|--------|-----------|
| **Destinations explored** | All containers | One specific container |
| **Complexity** | O(N × C) | O(N) or O(S) if sampled |
| **Flexibility** | Finds best destination | Tests specific destination |
| **Use case** | Explore all options | Know where to move |

## Complexity

**Without sampling**: O(N)
**With sampling**: O(S)

Where:
- N = number of objects in hot container
- S = sample size

**Example - Directed migration**:
- Hot container: 10,000 objects
- Without sampling: Evaluate **10,000** moves to special container
- With sampleSize=100: Evaluate ~**100** moves to special container

**Speedup vs Single**: C× (where C = number of containers in system)

## Usage Patterns

### Server Migration

Move objects to new server:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_examples.py start=server_migration_start end=server_migration_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_examples.cpp start=server_migration_start end=server_migration_end
```

</TabItem>
</Tabs>

### Fill Specific Container

Target specific underutilized container:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_examples.py start=fill_container_start end=fill_container_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_examples.cpp start=fill_container_start end=fill_container_end
```

</TabItem>
</Tabs>

### With Sampling for Large Containers

Sample subset when hot container is very large:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_examples.py start=sampling_start end=sampling_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_examples.cpp start=sampling_start end=sampling_end
```

</TabItem>
</Tabs>

### Multi-Destination Strategy

Use multiple FixedDest in sequence for different destinations:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_examples.py start=multi_dest_start end=multi_dest_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_examples.cpp start=multi_dest_start end=multi_dest_end
```

</TabItem>
</Tabs>

## Performance Characteristics

### Speedup Analysis

| Containers | Objects | FixedDest | Single | Speedup |
|------------|---------|-----------|--------|---------|
| 100 | 1K | 1K | 100K | 100× |
| 1K | 10K | 10K | 10M | 1000× |
| 10K | 10K | 10K | 100M | 10000× |

### When Does It Help?

FixedDest helps when:
- **Known destination**: Exactly where objects should go
- **Directed migration**: Moving to specific new server/region
- **Testing**: "What if we moved to container X?"
- **Filling specific container**: Target underutilized container
- **Avoiding exploration overhead**: Don't need to search all destinations

FixedDest does NOT help when:
- **Unknown destination**: Need solver to find best destination
- **Exploring options**: Want to try multiple destinations
- **General optimization**: Use [Single](../basic/single) instead
- **Destination changes**: Different destination each iteration

## Comparison with Variants

| Move Type | Destination | Source | Use Case |
|-----------|-------------|--------|----------|
| [Single](../basic/single) | Any container | Hot container | General moves |
| **FixedDest** | Fixed specific | Hot container | Directed migration |
| [FixedSource](fixed-source) | Hot container | Fixed specific | Pull from specific source |
| [FixedDestMultiMove](fixed-dest-multi-move) | Fixed specific | Multiple sources | Multi-source to one dest |

**Decision tree**:
1. **Know destination?** → **FixedDest**
2. **Know source?** → [FixedSource](fixed-source)
3. **Neither fixed?** → [Single](../basic/single)

## Troubleshooting

### Problem: No improving moves found

**Diagnosis**: Objects can't beneficially move to special container

**Solutions**:
- Verify `specialContainer` is correct destination
- Check capacity constraints on special container
- May already be optimal for this destination
- Try different destination or use [Single](../basic/single)

### Problem: Wrong objects moving

**Diagnosis**: Objective function or constraints issue

**Solutions**:
- Verify objective function rewards correct moves
- Check constraints (capacity, affinity, etc.)
- May need different objective or constraints
- Review which objects the solver is selecting

### Problem: Too slow even with fixed destination

**Diagnosis**: Hot container too large

**Solutions**:
- Enable sampling with `sampleSize` parameter
- Start with small sample (e.g., 100)
- Verify special container can accept objects
- Check objective function efficiency

### Problem: Sampling missing good moves

**Diagnosis**: Sample size too small, random sampling misses best objects

**Solutions**:
- Increase `sampleSize`
- Remove sampling (evaluate all objects)
- Run multiple times with different random seeds
- May need deterministic selection (use [Single](../basic/single))

## When to Use FixedDest

**DO use when**:
- Know exactly where objects should move
- Migrating to new server/container
- Filling specific underutilized container
- Testing specific destination scenario
- Want to avoid destination exploration overhead

**DO NOT use when**:
- Need solver to find best destination
- Want to explore multiple destinations
- Destination is not predetermined
- General optimization (use [Single](../basic/single))

## Related Move Types

**Fixed variants**:
- **FixedDest** - Fixed destination, hot source (this)
- [FixedSource](fixed-source) - Hot destination, fixed source
- [FixedDestMultiMove](fixed-dest-multi-move) - Multiple sources to fixed dest
- [FixedSourceMultiMove](fixed-source-multi-move) - Fixed source to multiple dests

**General alternatives**:
- [Single](../basic/single) - Explore all destinations
- [SingleFast](../basic/single-fast) - Fast exploration with early exit

**Use together**:
- Try FixedDest for known destinations
- Fall back to [Single](../basic/single) for exploration

## Source Code

- Thrift definition: [`interface/thrift/SolverSpecs.thrift:578`](https://github.com/facebookincubator/rebalancer/blob/main/interface/thrift/SolverSpecs.thrift#L578)
- Implementation: [`solver/moves/FixedDestMoveType.h`](https://github.com/facebookincubator/rebalancer/blob/main/solver/moves/FixedDestMoveType.h)
- Tests: [`solver/moves/tests/`](https://github.com/facebookincubator/rebalancer/tree/main/solver/moves/tests/)

## Next Steps

- Learn about [FixedSource](fixed-source) for pulling from specific source
- Try [FixedDestMultiMove](fixed-dest-multi-move) for multi-source scenarios
- Review [Move Types Overview](../) for choosing move types
- See [Single](../basic/single) for general single moves
