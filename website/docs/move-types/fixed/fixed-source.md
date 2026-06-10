---
sidebar_position: 2
---

# FixedSource

**Move Type**: Fixed
**Complexity**: O(sources × sample_size) instead of O(objects × containers)

Move objects from **specific fixed source containers** to hot container. The inverse of [FixedDest](fixed-dest) - you know where to pull from, not where to send.

## Overview

`FixedSource` (also known as `SINGLE_FIXED_SOURCE`) evaluates moving objects from **predetermined source containers** to the hot (cold/underutilized) container. Instead of exploring all possible sources, it only considers objects from specific containers.

**Use when**:
- Know exactly which containers to pull objects from
- Draining specific servers/containers
- Hot container is underutilized and needs objects
- Testing "what if we pulled from container X"

**Avoid when**:
- Don't know sources (use [Single](../basic/single))
- Need to explore all possible sources
- Sources keep changing
- Want solver to find best sources

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/fixed_source_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/fixed_source_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `scopeItemList` | ScopeItemList | No* | null | List of scope items containing source containers |
| `specialContainer` | string | No* | null | Single source container name |
| `sampleSize` | SampleSize | No | null | Sample subset of objects (probabilistic) |
| `stopEarlyAtScopeItemThatImprovesObjective` | bool | No | false | Stop after first scope item with improvement |

*Either `scopeItemList` or `specialContainer` must be specified. If both provided, `scopeItemList` is used.

### Parameter Details

**scopeItemList**:
- List of scope items containing source containers
- All containers in these scope items become sources
- Evaluated in order specified

**specialContainer**:
- Single specific source container
- Simpler alternative to `scopeItemList`
- Used only if `scopeItemList` not provided

**sampleSize**:
- Optional sampling to reduce evaluations
- Each object sampled with probability = `sampleSize / (objects in source)`
- Applied per source container

**stopEarlyAtScopeItemThatImprovesObjective**:
- When `true`: Stop after first scope item with improving move
- When `false`: Evaluate all scope items, pick best move
- Only relevant when using `scopeItemList`

## How It Works

Given a **hot container** (underutilized destination):

1. **Select source**: Pick one of the specified source containers
2. **Select object**: Pick object from source container
3. **Sample**: With probability `sampleSize / N`, evaluate this object (if sampling enabled)
4. **Evaluate move**: Test moving object from source to hot container
5. **Repeat**: Try all objects in all specified sources
6. **Apply best**: Apply the move from source that improves objective most

### Visual Example

```
Before move:                          After move from specialContainer:
┌──────────────┐                     ┌──────────────┐
│ Hot          │                     │ Hot          │
│ Container    │  <──────────        │ Container    │
│  (empty)     │                  ┌──┤  • obj1 ←────┼── Pulled from source!
└──────────────┘                  │  │  • obj2      │
                                  │  └──────────────┘
┌──────────────┐                  │
│ Special      │                  │  ┌──────────────┐
│ Container    │  ────────────────┘  │ Special      │
│  • obj1  ────┼──┐                  │ Container    │
│  • obj2      │  │                  │  • obj2      │
│  • obj3      │  │                  │  • obj3      │
└──────────────┘  └─ Source          └──────────────┘

Only one source considered: specialContainer
```

### Comparison with FixedDest

| Aspect | FixedDest | FixedSource |
|--------|-----------|-------------|
| **Fixed** | Destination | Source |
| **Hot container** | Source (gives) | Destination (receives) |
| **Use case** | Know where to send | Know where to pull from |

## Complexity

**Without sampling**: O(N × S)
**With sampling**: O(Sample × S)

Where:
- N = average objects per source container
- S = number of source containers
- Sample = sample size

**Example - Draining servers**:
- Source containers: 5 servers to drain
- Objects per server: 1,000
- Without sampling: Evaluate **5,000** moves
- With sampleSize=100: Evaluate ~**500** moves (100 per server)

## Usage Patterns

### Drain Specific Servers

Pull objects from servers being decommissioned:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/fixed_source_examples.py start=drain_servers_start end=drain_servers_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/fixed_source_examples.cpp start=drain_servers_start end=drain_servers_end
```

</TabItem>
</Tabs>

### Fill Underutilized Container

Pull from specific sources to fill hot container:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/fixed_source_examples.py start=fill_container_start end=fill_container_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/fixed_source_examples.cpp start=fill_container_start end=fill_container_end
```

</TabItem>
</Tabs>

### With Sampling

Sample subset when sources are very large:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/fixed_source_examples.py start=sampling_start end=sampling_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/fixed_source_examples.cpp start=sampling_start end=sampling_end
```

</TabItem>
</Tabs>

### Early Exit Strategy

Stop after first good source:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/fixed_source_examples.py start=early_exit_start end=early_exit_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/fixed_source_examples.cpp start=early_exit_start end=early_exit_end
```

</TabItem>
</Tabs>

## Performance Characteristics

### Speedup Analysis

| Source Containers | Objects/Source | FixedSource | Single | Speedup |
|-------------------|----------------|-------------|--------|---------|
| 5 | 1K | 5K | ~100K | 20× |
| 10 | 10K | 100K | ~10M | 100× |
| 100 | 10K | 1M | ~100M | 100× |

### When Does It Help?

FixedSource helps when:
- **Known sources**: Exactly which containers to drain
- **Server decommissioning**: Pulling from specific servers
- **Filling underutilized**: Hot container needs objects from specific sources
- **Testing**: "What if we pulled from these containers?"
- **Avoiding exploration overhead**: Don't need to search all sources

FixedSource does NOT help when:
- **Unknown sources**: Need solver to find best sources
- **Exploring options**: Want to try many sources
- **General optimization**: Use [Single](../basic/single) instead
- **Sources change**: Different sources each iteration

## Comparison with Variants

| Move Type | Destination | Source | Use Case |
|-----------|-------------|--------|----------|
| [Single](../basic/single) | Any container | Hot container | General moves |
| [FixedDest](fixed-dest) | Fixed specific | Hot container | Push to specific dest |
| **FixedSource** | Hot container | Fixed specific | Pull from specific sources |
| [FixedSourceMultiMove](fixed-source-multi-move) | Multiple dests | Fixed specific | One source to many dests |

**Decision tree**:
1. **Know sources?** → **FixedSource**
2. **Know destination?** → [FixedDest](fixed-dest)
3. **Neither fixed?** → [Single](../basic/single)

## Troubleshooting

### Problem: No improving moves found

**Diagnosis**: Objects can't beneficially move from sources to hot container

**Solutions**:
- Verify source containers are correct
- Check capacity constraints on hot container
- May already be optimal
- Try different sources or use [Single](../basic/single)

### Problem: Wrong objects moving

**Diagnosis**: Objective function or constraints issue

**Solutions**:
- Verify objective function rewards correct moves
- Check constraints (capacity, affinity, etc.)
- May need different objective or constraints
- Review which objects the solver is selecting

### Problem: Too slow even with fixed sources

**Diagnosis**: Source containers too large

**Solutions**:
- Enable sampling with `sampleSize` parameter
- Start with small sample (e.g., 100 per source)
- Use `stopEarlyAtScopeItemThatImprovesObjective=true`
- Check objective function efficiency

### Problem: Hot container not filling as expected

**Diagnosis**: Capacity constraints or objective not favoring moves

**Solutions**:
- Verify hot container has capacity
- Check objective function rewards filling hot container
- May need different sources
- Review capacity constraints

## When to Use FixedSource

**DO use when**:
- Know exactly which containers to pull from
- Draining specific servers/containers
- Hot container is underutilized and needs filling
- Testing specific source scenario
- Want to avoid source exploration overhead

**DO NOT use when**:
- Need solver to find best sources
- Want to explore multiple source options
- Sources are not predetermined
- General optimization (use [Single](../basic/single))

## Related Move Types

**Fixed variants**:
- [FixedDest](fixed-dest) - Fixed destination, hot source
- **FixedSource** - Hot destination, fixed sources (this)
- [FixedSourceMultiMove](fixed-source-multi-move) - Fixed source to multiple dests
- [FixedDestMultiMove](fixed-dest-multi-move) - Multiple sources to fixed dest

**General alternatives**:
- [Single](../basic/single) - Explore all sources
- [SingleFast](../basic/single-fast) - Fast exploration with early exit

**Use together**:
- Try FixedSource for known sources
- Fall back to [Single](../basic/single) for exploration

## Source Code

- Thrift definition: [`interface/thrift/SolverSpecs.thrift:640`](https://github.com/facebookincubator/rebalancer/blob/main/interface/thrift/SolverSpecs.thrift#L640)
- Implementation: [`solver/moves/SingleFixedSourceMoveType.h`](https://github.com/facebookincubator/rebalancer/blob/main/solver/moves/SingleFixedSourceMoveType.h)
- Tests: [`solver/moves/tests/`](https://github.com/facebookincubator/rebalancer/tree/main/solver/moves/tests/)

## Next Steps

- Learn about [FixedDest](fixed-dest) for pushing to specific destination
- Try [FixedSourceMultiMove](fixed-source-multi-move) for multi-destination scenarios
- Review [Move Types Overview](../) for choosing move types
- See [Single](../basic/single) for general single moves
