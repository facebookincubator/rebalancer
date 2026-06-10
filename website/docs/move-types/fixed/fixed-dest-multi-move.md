---
sidebar_position: 3
---

# FixedDestMultiMove

**Move Type**: Fixed
**Complexity**: O(bundles × bundle_size) with parallel evaluation
**Primary Use**: RAS stackable solve

Move **bundles of related objects** from hot container to a **specific fixed destination**. The multi-object version of [FixedDest](fixed-dest), designed for RAS local search.

## Overview

`FixedDestMultiMove` (also known as `FIXED_DEST_MULTIPLE`) evaluates moving **bundles of related objects** from the hot container to a **single predetermined destination container**. Unlike [FixedDest](fixed-dest) which moves one object at a time, this move type moves entire object bundles together.

**Use when**:
- Using RAS stackable solve
- Moving bundles of related objects together
- Know exactly which destination to target
- Objects must move as coordinated groups
- Want parallel evaluation of move sets

**Avoid when**:
- Not using RAS local search
- Objects can move independently (use [FixedDest](fixed-dest))
- Don't know destination (use [Single](../basic/single))
- Need to explore multiple destinations
- Want solver to find best destination

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_multi_move_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_multi_move_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `specialContainer` | string | **Yes** | null | Fixed destination container name |
| `maxSamplesPerEquivSet` | int | No | 5 | Max move sets to consider per equivalent set |
| `rasLocalSearchMetadata` | RasLocalSearchMetadata | No | null | Metadata for RAS local search |

### Parameter Details

**specialContainer**:
- Name of the specific destination container
- All object bundles will target this container only
- Must be a valid container name

**maxSamplesPerEquivSet**:
- Limits move sets evaluated per equivalent set
- Equivalent sets are groups identical from local search perspective
- Higher values = better quality, more computation
- Default: 5 samples per equivalent set

**rasLocalSearchMetadata**:
- Metadata specific to RAS stackable solve
- Optional configuration for RAS local search behavior
- Only relevant for RAS use cases

## How It Works

Given a **hot container** (most broken):

1. **Select bundle**: Pick object bundle from hot container
2. **Evaluate move**: Test moving **all objects in bundle** to `specialContainer`
3. **Parallel evaluation**: All move sets evaluated in parallel (multi-threading)
4. **Sample**: Consider up to `maxSamplesPerEquivSet` per equivalent set
5. **Repeat**: Try all bundles in hot container
6. **Apply best**: Apply the bundle move that improves objective most

### Visual Example

```
Before move:                          After bundle move to specialContainer:
┌──────────────┐                     ┌──────────────┐
│ Hot          │                     │ Hot          │
│ Container    │  ───────────>       │ Container    │
│  Bundle1 ────┼──┐ obj1, obj2       │  Bundle2     │
│  • obj1      │  │                  │  • obj3      │
│  • obj2  ────┼──┘                  │  • obj4      │
│  Bundle2     │                     └──────────────┘
│  • obj3      │
│  • obj4      │                     ┌──────────────┐
└──────────────┘                     │ Special      │
                                     │ Container    │
┌──────────────┐                     │  • objA      │
│ Special      │                     │  • objB      │
│ Container    │                     │  Bundle1 ←───┼── Entire bundle moved!
│  • objA      │                     │  • obj1      │
│  • objB  <───┼──┐                  │  • obj2      │
└──────────────┘  └─ Bundle dest     └──────────────┘

Entire bundle moves together to specialContainer
```

### Comparison with FixedDest

| Aspect | FixedDest | FixedDestMultiMove |
|--------|-----------|---------------------|
| **Move unit** | Single object | Bundle of objects |
| **Use case** | General single moves | RAS stackable solve |
| **Evaluation** | Sequential | Parallel |
| **Complexity** | O(N) | O(B × S) where B=bundles, S=bundle_size |
| **Coordination** | Independent objects | Related object groups |

## Complexity

**Per iteration**: O(B × S)

Where:
- B = number of bundles in hot container
- S = average bundle size (objects per bundle)

**Sampling**: Limited by `maxSamplesPerEquivSet` per equivalent set

**Example - RAS stackable**:
- Hot container bundles: 20
- Average bundle size: 4 objects
- maxSamplesPerEquivSet: 5
- Evaluations: ~20 bundles (limited by sampling), 4 objects each
- Benefit: Parallel evaluation across bundles

## Usage Patterns

### RAS Migration

Move RAS bundles to new server:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_multi_move_examples.py start=ras_migration_start end=ras_migration_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_multi_move_examples.cpp start=ras_migration_start end=ras_migration_end
```

</TabItem>
</Tabs>

### Fill Specific Container

Target specific underutilized container with bundles:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_multi_move_examples.py start=fill_container_start end=fill_container_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_multi_move_examples.cpp start=fill_container_start end=fill_container_end
```

</TabItem>
</Tabs>

### Sampling Control

Control sampling of equivalent sets:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_multi_move_examples.py start=sampling_start end=sampling_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_multi_move_examples.cpp start=sampling_start end=sampling_end
```

</TabItem>
</Tabs>

### With RAS Metadata

Configure with RAS local search metadata:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_multi_move_examples.py start=ras_metadata_start end=ras_metadata_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_multi_move_examples.cpp start=ras_metadata_start end=ras_metadata_end
```

</TabItem>
</Tabs>

## Performance Characteristics

### Speedup Analysis

| Bundles | Bundle Size | FixedDestMultiMove | Single | Speedup |
|---------|-------------|---------------------|--------| ---------|
| 20 | 4 | 80 | 100K | ~1250× |
| 50 | 8 | 400 | 1M | ~2500× |
| 100 | 10 | 1K | 10M | ~10000× |

**Benefits**:
- **Parallel evaluation**: All move sets evaluated concurrently
- **Bundle efficiency**: Move related objects together
- **Sampling control**: `maxSamplesPerEquivSet` limits computation
- **Fixed destination**: No destination exploration overhead

### When Does It Help?

FixedDestMultiMove helps when:
- **RAS stackable solve**: Designed for RAS local search
- **Bundle coordination**: Objects must move together
- **Known destination**: Exactly where bundles should go
- **Parallel benefits**: Can leverage multi-threading
- **Large bundles**: Moving groups is more efficient than individuals

FixedDestMultiMove does NOT help when:
- **Not RAS**: Use [FixedDest](fixed-dest) for single objects
- **Independent objects**: Objects can move separately
- **Unknown destination**: Need solver to find best destination
- **Exploring options**: Want to try multiple destinations

## Comparison with Variants

| Move Type | Destination | Source | Move Unit | Use Case |
|-----------|-------------|--------|-----------|----------|
| [FixedDest](fixed-dest) | Fixed specific | Hot container | Single object | General moves |
| **FixedDestMultiMove** | Fixed specific | Hot container | Object bundle | RAS stackable |
| [FixedSourceMultiMove](fixed-source-multi-move) | Multiple dests | Fixed specific | Object bundle | RAS pull from source |
| [FixedDestSwapMultiMove](fixed-dest-swap-multi-move) | Fixed specific | Hot container | Bundle swap | RAS swaps |

**Decision tree**:
1. **RAS stackable + know dest?** → **FixedDestMultiMove**
2. **RAS stackable + know source?** → [FixedSourceMultiMove](fixed-source-multi-move)
3. **Single objects + know dest?** → [FixedDest](fixed-dest)
4. **Neither?** → [Single](../basic/single)

## Troubleshooting

### Problem: No improving moves found

**Diagnosis**: Bundles can't beneficially move to special container

**Solutions**:
- Verify `specialContainer` is correct destination
- Check capacity constraints on special container
- Bundle sizes may be too large for destination
- May already be optimal for this destination
- Try different destination or [Single](../basic/single)

### Problem: Wrong bundles moving

**Diagnosis**: Bundle formation or objective function issue

**Solutions**:
- Verify bundle formation logic is correct
- Check objective function rewards correct bundle moves
- Review equivalent set definitions
- May need different objective or constraints
- Examine which bundles the solver is selecting

### Problem: Too many evaluations

**Diagnosis**: Too many bundles or large equivalent sets

**Solutions**:
- Reduce `maxSamplesPerEquivSet` (default: 5)
- Start with smaller sample (e.g., 2-3 per set)
- Review bundle formation to reduce bundle count
- Check if bundles can be simplified

### Problem: Missing good bundle moves

**Diagnosis**: `maxSamplesPerEquivSet` too small, sampling misses best bundles

**Solutions**:
- Increase `maxSamplesPerEquivSet` (e.g., 10, 20)
- Review equivalent set definitions
- May need to evaluate all bundles (remove sampling)
- Run multiple times with different random seeds

## When to Use FixedDestMultiMove

**DO use when**:
- Using RAS stackable solve
- Moving bundles of related objects
- Know exactly which destination to target
- Objects must move as coordinated groups
- Want to benefit from parallel evaluation

**DO NOT use when**:
- Not using RAS local search
- Objects can move independently (use [FixedDest](fixed-dest))
- Don't know destination
- Need to explore multiple destinations
- General optimization (use [Single](../basic/single))

## Related Move Types

**Fixed bundle variants**:
- [FixedDest](fixed-dest) - Single object to fixed dest
- **FixedDestMultiMove** - Bundle to fixed dest (this)
- [FixedSourceMultiMove](fixed-source-multi-move) - Bundle from fixed source
- [FixedDestSwapMultiMove](fixed-dest-swap-multi-move) - Bundle swaps to fixed dest

**RAS move types**:
- All three above are used exclusively for RAS stackable solve
- Work together in RAS local search strategies

**General alternatives**:
- [FixedDest](fixed-dest) - Single objects to fixed destination
- [Single](../basic/single) - Explore all destinations

## Source Code

- Thrift definition: [`interface/thrift/SolverSpecs.thrift:596`](https://github.com/facebookincubator/rebalancer/blob/main/interface/thrift/SolverSpecs.thrift#L596)
- Implementation: [`solver/moves/FixedDestMultiMoveType.h`](https://github.com/facebookincubator/rebalancer/blob/main/solver/moves/FixedDestMultiMoveType.h)
- Tests: [`solver/moves/tests/`](https://github.com/facebookincubator/rebalancer/tree/main/solver/moves/tests/)

## Next Steps

- Learn about [FixedSourceMultiMove](fixed-source-multi-move) for pulling bundles from specific sources
- Try [FixedDestSwapMultiMove](fixed-dest-swap-multi-move) for bundle swaps
- Review [FixedDest](fixed-dest) for single-object moves
- See [Move Types Overview](../) for choosing move types
