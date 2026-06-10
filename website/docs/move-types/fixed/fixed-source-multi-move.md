---
sidebar_position: 4
---

# FixedSourceMultiMove

**Move Type**: Fixed
**Complexity**: O(bundles × bundle_size) with parallel evaluation
**Primary Use**: RAS stackable solve

Move **bundles of related objects** from a **specific fixed source** to hot container. The inverse of [FixedDestMultiMove](fixed-dest-multi-move), designed for RAS local search.

## Overview

`FixedSourceMultiMove` (also known as `FIXED_SOURCE_MULTIPLE`) evaluates moving **bundles of related objects** from a **single predetermined source container** to the hot (underutilized) container. Unlike [FixedSource](fixed-source) which moves one object at a time, this move type moves entire object bundles together.

**Use when**:
- Using RAS stackable solve
- Draining bundles from specific source
- Know exactly which source to pull from
- Objects must move as coordinated groups
- Hot container needs filling with bundles
- Want parallel evaluation of move sets

**Avoid when**:
- Not using RAS local search
- Objects can move independently (use [FixedSource](fixed-source))
- Don't know source (use [Single](../basic/single))
- Need to explore multiple sources
- Want solver to find best sources

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/fixed_source_multi_move_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/fixed_source_multi_move_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `specialContainer` | string | **Yes** | null | Fixed source container name |
| `maxSamplesPerEquivSet` | int | No | 5 | Max move sets to consider per equivalent set |
| `rasLocalSearchMetadata` | RasLocalSearchMetadata | No | null | Metadata for RAS local search |

### Parameter Details

**specialContainer**:
- Name of the specific source container
- All object bundles will be pulled from this container only
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

Given a **hot container** (underutilized destination):

1. **Select bundle**: Pick object bundle from `specialContainer` (source)
2. **Evaluate move**: Test moving **all objects in bundle** to hot container
3. **Parallel evaluation**: All move sets evaluated in parallel (multi-threading)
4. **Sample**: Consider up to `maxSamplesPerEquivSet` per equivalent set
5. **Repeat**: Try all bundles in special source container
6. **Apply best**: Apply the bundle move that improves objective most

### Visual Example

```
Before move:                          After bundle move from specialContainer:
┌──────────────┐                     ┌──────────────┐
│ Hot          │                     │ Hot          │
│ Container    │  <──────────        │ Container    │
│  (empty)     │                  ┌──┤  Bundle1 ←───┼── Pulled from source!
└──────────────┘                  │  │  • obj1      │
                                  │  │  • obj2      │
┌──────────────┐                  │  └──────────────┘
│ Special      │                  │
│ Container    │  ────────────────┘  ┌──────────────┐
│  Bundle1 ────┼──┐                  │ Special      │
│  • obj1      │  │                  │ Container    │
│  • obj2  ────┼──┘ Source           │  Bundle2     │
│  Bundle2     │                     │  • obj3      │
│  • obj3      │                     │  • obj4      │
│  • obj4      │                     └──────────────┘
└──────────────┘

Entire bundle moves from specialContainer to hot container
```

### Comparison with FixedDestMultiMove

| Aspect | FixedDestMultiMove | FixedSourceMultiMove |
|--------|--------------------|----------------------|
| **Fixed** | Destination | Source |
| **Hot container** | Source (gives) | Destination (receives) |
| **Use case** | Push bundles to dest | Pull bundles from source |
| **Move direction** | Hot → Fixed | Fixed → Hot |

## Complexity

**Per iteration**: O(B × S)

Where:
- B = number of bundles in special source container
- S = average bundle size (objects per bundle)

**Sampling**: Limited by `maxSamplesPerEquivSet` per equivalent set

**Example - RAS draining**:
- Source container bundles: 20
- Average bundle size: 4 objects
- maxSamplesPerEquivSet: 5
- Evaluations: ~20 bundles (limited by sampling), 4 objects each
- Benefit: Parallel evaluation across bundles

## Usage Patterns

### Drain RAS Server

Pull RAS bundles from server being decommissioned:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/fixed_source_multi_move_examples.py start=drain_server_start end=drain_server_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/fixed_source_multi_move_examples.cpp start=drain_server_start end=drain_server_end
```

</TabItem>
</Tabs>

### Fill Underutilized Container

Pull bundles from specific source to fill hot container:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/fixed_source_multi_move_examples.py start=fill_container_start end=fill_container_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/fixed_source_multi_move_examples.cpp start=fill_container_start end=fill_container_end
```

</TabItem>
</Tabs>

### Sampling Control

Control sampling of equivalent sets:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/fixed_source_multi_move_examples.py start=sampling_start end=sampling_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/fixed_source_multi_move_examples.cpp start=sampling_start end=sampling_end
```

</TabItem>
</Tabs>

### With RAS Metadata

Configure with RAS local search metadata:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/fixed_source_multi_move_examples.py start=ras_metadata_start end=ras_metadata_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/fixed_source_multi_move_examples.cpp start=ras_metadata_start end=ras_metadata_end
```

</TabItem>
</Tabs>

## Performance Characteristics

### Speedup Analysis

| Bundles | Bundle Size | FixedSourceMultiMove | Single | Speedup |
|---------|-------------|----------------------|--------| ---------|
| 20 | 4 | 80 | 100K | ~1250× |
| 50 | 8 | 400 | 1M | ~2500× |
| 100 | 10 | 1K | 10M | ~10000× |

**Benefits**:
- **Parallel evaluation**: All move sets evaluated concurrently
- **Bundle efficiency**: Move related objects together
- **Sampling control**: `maxSamplesPerEquivSet` limits computation
- **Fixed source**: No source exploration overhead

### When Does It Help?

FixedSourceMultiMove helps when:
- **RAS stackable solve**: Designed for RAS local search
- **Bundle coordination**: Objects must move together
- **Known source**: Exactly which source to drain
- **Filling hot container**: Underutilized container needs bundles
- **Parallel benefits**: Can leverage multi-threading

FixedSourceMultiMove does NOT help when:
- **Not RAS**: Use [FixedSource](fixed-source) for single objects
- **Independent objects**: Objects can move separately
- **Unknown source**: Need solver to find best source
- **Exploring options**: Want to try multiple sources

## Comparison with Variants

| Move Type | Destination | Source | Move Unit | Use Case |
|-----------|-------------|--------|-----------|----------|
| [FixedSource](fixed-source) | Hot container | Fixed specific | Single object | General pull from source |
| **FixedSourceMultiMove** | Hot container | Fixed specific | Object bundle | RAS pull from source |
| [FixedDestMultiMove](fixed-dest-multi-move) | Fixed specific | Hot container | Object bundle | RAS push to dest |
| [FixedDestSwapMultiMove](fixed-dest-swap-multi-move) | Fixed specific | Hot container | Bundle swap | RAS swaps |

**Decision tree**:
1. **RAS stackable + know source?** → **FixedSourceMultiMove**
2. **RAS stackable + know dest?** → [FixedDestMultiMove](fixed-dest-multi-move)
3. **Single objects + know source?** → [FixedSource](fixed-source)
4. **Neither?** → [Single](../basic/single)

## Troubleshooting

### Problem: No improving moves found

**Diagnosis**: Bundles can't beneficially move from source to hot container

**Solutions**:
- Verify `specialContainer` is correct source
- Check capacity constraints on hot container
- Bundle sizes may be too large for destination
- May already be optimal
- Try different source or [Single](../basic/single)

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

### Problem: Hot container not filling

**Diagnosis**: Capacity constraints or bundles don't fit

**Solutions**:
- Verify hot container has capacity for bundles
- Check objective function rewards filling hot container
- Bundle sizes may exceed hot container capacity
- May need different source
- Review capacity constraints

## When to Use FixedSourceMultiMove

**DO use when**:
- Using RAS stackable solve
- Draining bundles from specific source
- Know exactly which source to pull from
- Objects must move as coordinated groups
- Hot container needs filling with bundles

**DO NOT use when**:
- Not using RAS local search
- Objects can move independently (use [FixedSource](fixed-source))
- Don't know source
- Need to explore multiple sources
- General optimization (use [Single](../basic/single))

## Related Move Types

**Fixed bundle variants**:
- [FixedSource](fixed-source) - Single object from fixed source
- **FixedSourceMultiMove** - Bundle from fixed source (this)
- [FixedDestMultiMove](fixed-dest-multi-move) - Bundle to fixed dest
- [FixedDestSwapMultiMove](fixed-dest-swap-multi-move) - Bundle swaps to fixed dest

**RAS move types**:
- All three above are used exclusively for RAS stackable solve
- Work together in RAS local search strategies

**General alternatives**:
- [FixedSource](fixed-source) - Single objects from fixed source
- [Single](../basic/single) - Explore all sources

## Source Code

- Thrift definition: [`interface/thrift/SolverSpecs.thrift:588`](https://github.com/facebookincubator/rebalancer/blob/main/interface/thrift/SolverSpecs.thrift#L588)
- Implementation: [`solver/moves/FixedSrcMultiMoveType.h`](https://github.com/facebookincubator/rebalancer/blob/main/solver/moves/FixedSrcMultiMoveType.h)
- Tests: [`solver/moves/tests/`](https://github.com/facebookincubator/rebalancer/tree/main/solver/moves/tests/)

## Next Steps

- Learn about [FixedDestMultiMove](fixed-dest-multi-move) for pushing bundles to specific destination
- Try [FixedDestSwapMultiMove](fixed-dest-swap-multi-move) for bundle swaps
- Review [FixedSource](fixed-source) for single-object moves
- See [Move Types Overview](../) for choosing move types
