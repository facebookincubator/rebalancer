---
sidebar_position: 5
---

# FixedDestSwapMultiMove

**Move Type**: Fixed
**Complexity**: Traditional O(B_src × B_dst × sizes), Adaptive O(B_src × k)
**Primary Use**: RAS stackable solve with swap support

Swap **bundles of related objects** between hot container and **specific fixed destination**. Supports both traditional 1:1 swaps and adaptive 1:k uneven swaps based on dimension ratios.

## Overview

`FixedDestSwapMultiMove` (also known as `FIXED_DEST_SWAP_MULTIPLE`) evaluates swapping **bundles of related objects** between the hot container and a **single predetermined destination container**. This move type supports two distinct modes:

1. **Traditional Swap Mode**: 1:1 swaps between bundles
2. **Adaptive 1:k Swap Mode**: One object for k objects based on dimension ratios

**Use when**:
- Using RAS stackable solve with swaps
- Swapping bundles between containers
- Know exactly which destination to swap with
- Objects have different dimension values (e.g., capacity)
- Need 1:k uneven swaps for heterogeneous resources
- Want parallel evaluation of swap sets

**Avoid when**:
- Not using RAS local search
- Don't need swaps (use [FixedDestMultiMove](fixed-dest-multi-move))
- Objects all have same dimension values
- Don't know destination
- Need to explore multiple destinations

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_swap_multi_move_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_swap_multi_move_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `specialContainer` | string | **Yes** | null | Fixed destination container for swaps |
| `greedyOnSrc` | bool | No | false | Greedy source selection (required for 1:k) |
| `maxSamplesPerEquivSet` | int | No | 5 | Max move sets per equivalent set |
| `maxSampleSizeOnSrc` | int | No | null | Max source bundles to consider |
| `maxSampleSizeOnDst` | int | No | null | Max destination bundles to consider |
| `rasLocalSearchMetadata` | RasLocalSearchMetadata | No | null | RAS metadata with swap config |

### Parameter Details

**specialContainer**:
- Name of the specific destination container
- All swaps target this container only
- Must be a valid container name

**greedyOnSrc**:
- When `false`: Evaluate all source × destination combinations
- When `true`: Try each source object, exit early on success
- **Required** for adaptive 1:k swap mode
- Enables greedy termination for faster search

**maxSamplesPerEquivSet**:
- Limits move sets evaluated per equivalent set
- Equivalent sets are groups identical from local search perspective
- Default: 5 samples per equivalent set

**maxSampleSizeOnSrc**:
- Optional limit on source bundles to consider
- Reduces evaluations for large hot containers

**maxSampleSizeOnDst**:
- Optional limit on destination bundles to consider
- Reduces evaluations for large destination containers

**rasLocalSearchMetadata**:
- Metadata specific to RAS stackable solve
- **swapRatioDimension**: Dimension name for calculating swap ratios (e.g., "capacity")
- **useAdaptiveAllotments**: Enables adaptive 1:k swap mode
- Both must be set to activate adaptive swaps

## How It Works

### Traditional Swap Mode (Default)

When `swapRatioDimension` is not configured or `useAdaptiveAllotments` is disabled:

1. **Select source bundle**: Pick bundle from hot container
2. **Select dest bundle**: Pick bundle from special container
3. **Evaluate swap**: Test swapping all source with all dest objects
4. **Cartesian product**: All source × dest combinations evaluated
5. **Parallel evaluation**: All swaps evaluated concurrently
6. **Apply best**: Apply the swap that improves objective most

### Adaptive 1:k Swap Mode

When `swapRatioDimension` is configured and `useAdaptiveAllotments` is enabled:

1. **Select source object**: Pick single object from hot (greedy required)
2. **Calculate ratio**: For each dest equiv set: `k = ceil(hot_dim / cold_dim)`
   - If hot_dim ≤ cold_dim, k = 1
3. **Form dest bundle**: Create bundle with k objects from equiv set
4. **Evaluate swap**: Test swapping 1 hot for k cold objects
5. **Greedy exit**: Stop immediately when beneficial swap found
6. **Apply swap**: Apply the 1:k swap

### Visual Example - Traditional Mode

```
Before swap:                          After 1:1 bundle swap:
┌──────────────┐                     ┌──────────────┐
│ Hot          │                     │ Hot          │
│ Container    │  ↔──────────>       │ Container    │
│  Bundle1 ────┼──┐                  │  Bundle2 ←───┼── From dest!
│  • obj1      │  │                  │  • obj3      │
│  • obj2  ────┼──┘ Swap             │  • obj4      │
└──────────────┘                     └──────────────┘

┌──────────────┐                     ┌──────────────┐
│ Special      │                     │ Special      │
│ Container    │  <──────────↔       │ Container    │
│  Bundle2 <───┼──┐                  │  Bundle1 ←───┼── From hot!
│  • obj3      │  │                  │  • obj1      │
│  • obj4  ────┼──┘ Swap             │  • obj2      │
└──────────────┘                     └──────────────┘

1:1 bundle swap between hot and specialContainer
```

### Visual Example - Adaptive 1:k Mode

```
Before adaptive swap (k=2):           After 1:2 adaptive swap:
┌──────────────┐                     ┌──────────────┐
│ Hot          │                     │ Hot          │
│ Container    │  ───────────>       │ Container    │
│  • obj1  ────┼──┐ dim=2.0          │  • obj2      │
│  • obj2      │  │                  │  (obj3, obj4)┼← Received 2 objects!
└──────────────┘  │                  └──────────────┘
                  │
┌──────────────┐  │                  ┌──────────────┐
│ Special      │  │                  │ Special      │
│ Container    │  │                  │ Container    │
│  • obj3  <───┼──┤ dim=1.0          │  • obj1  ←───┼── Received 1 object!
│  • obj4  <───┼──┘ dim=1.0          └──────────────┘
└──────────────┘

Swap 1 obj (dim=2.0) for 2 objs (dim=1.0 each)
k = ceil(2.0/1.0) = 2
```

## Complexity

### Traditional Swap Mode

**Per iteration**: O(S × D × |S_bundle| × |D_bundle|)

Where:
- S = source bundles (limited by `maxSampleSizeOnSrc`)
- D = destination bundles (limited by `maxSampleSizeOnDst`)
- |S_bundle| = objects per source bundle
- |D_bundle| = objects per destination bundle

### Adaptive 1:k Swap Mode

**Per iteration**: O(S × k̄)

Where:
- S = source objects (limited by `maxSampleSizeOnSrc`)
- k̄ = average swap ratio across equiv sets
- Significantly fewer due to greedy termination and no cartesian product

**Example - Heterogeneous servers**:
- Hot container: 10 high-capacity objects (dim=2.0)
- Special container: 50 low-capacity objects (dim=1.0)
- Traditional: 10 × 50 = 500 swap evaluations
- Adaptive: ~10 evaluations (k=2 each, greedy exit)

## Usage Patterns

### Traditional 1:1 Swaps

Standard bundle swaps between containers:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_swap_multi_move_examples.py start=traditional_swap_start end=traditional_swap_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_swap_multi_move_examples.cpp start=traditional_swap_start end=traditional_swap_end
```

</TabItem>
</Tabs>

### Adaptive 1:k Swaps

Uneven swaps based on dimension ratios:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_swap_multi_move_examples.py start=adaptive_swap_start end=adaptive_swap_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_swap_multi_move_examples.cpp start=adaptive_swap_start end=adaptive_swap_end
```

</TabItem>
</Tabs>

### Greedy Source Selection

Fast greedy search with early exit:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_swap_multi_move_examples.py start=greedy_start end=greedy_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_swap_multi_move_examples.cpp start=greedy_start end=greedy_end
```

</TabItem>
</Tabs>

### Sampling Control

Control sampling on both source and destination:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_swap_multi_move_examples.py start=sampling_start end=sampling_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/fixed_dest_swap_multi_move_examples.cpp start=sampling_start end=sampling_end
```

</TabItem>
</Tabs>

## Performance Characteristics

### Mode Comparison

| Mode | Evaluations | Early Exit | Use Case |
|------|-------------|------------|----------|
| Traditional | O(S × D × sizes) | No | Uniform objects |
| Adaptive 1:k | O(S × k̄) | Yes (greedy) | Heterogeneous objects |

### When Does It Help?

FixedDestSwapMultiMove helps when:
- **RAS stackable with swaps**: Designed for RAS swap scenarios
- **Bundle swaps**: Need to swap groups of objects
- **Known destination**: Exactly which container to swap with
- **Heterogeneous resources**: Objects have different capacities/values
- **Capacity matching**: 1:k swaps balance different resource types

FixedDestSwapMultiMove does NOT help when:
- **Not RAS**: Use simpler move types
- **Don't need swaps**: Use [FixedDestMultiMove](fixed-dest-multi-move)
- **Uniform objects**: All objects have same dimension values
- **Unknown destination**: Need solver to find swap partner

## Comparison with Variants

| Move Type | Operation | Mode | Use Case |
|-----------|-----------|------|----------|
| [FixedDestMultiMove](fixed-dest-multi-move) | Move bundles | Push | RAS push to dest |
| [FixedSourceMultiMove](fixed-source-multi-move) | Move bundles | Pull | RAS pull from source |
| **FixedDestSwapMultiMove** | Swap bundles | 1:1 or 1:k | RAS swaps (traditional or adaptive) |

**Decision tree**:
1. **Need swaps between hot and fixed dest?** → **FixedDestSwapMultiMove**
2. **Just push to dest?** → [FixedDestMultiMove](fixed-dest-multi-move)
3. **Just pull from source?** → [FixedSourceMultiMove](fixed-source-multi-move)
4. **Neither?** → [Single](../basic/single)

## Troubleshooting

### Problem: No improving swaps found

**Diagnosis**: Swaps can't beneficially exchange objects

**Solutions**:
- Verify `specialContainer` is correct swap partner
- Check capacity constraints on both containers
- Bundle sizes may not fit after swap
- May already be optimal
- Try different destination

### Problem: Adaptive mode not working

**Diagnosis**: Configuration issue with adaptive settings

**Solutions**:
- Verify `greedyOnSrc=true` (required for adaptive)
- Check `rasLocalSearchMetadata.swapRatioDimension` is set
- Ensure `rasLocalSearchMetadata.useAdaptiveAllotments=true`
- All three must be configured for adaptive mode

### Problem: Wrong swap ratios

**Diagnosis**: Dimension values or calculation issue

**Solutions**:
- Verify dimension values are correct
- Check `swapRatioDimension` matches actual dimension name
- Ratio k = ceil(hot_dim / cold_dim)
- If hot_dim ≤ cold_dim, ratio is 1
- Review dimension setup

### Problem: Too many evaluations

**Diagnosis**: Large source or destination bundles

**Solutions**:
- Enable `greedyOnSrc=true` for early exit
- Reduce `maxSampleSizeOnSrc` and `maxSampleSizeOnDst`
- Reduce `maxSamplesPerEquivSet`
- Consider adaptive mode (fewer evaluations)

## When to Use FixedDestSwapMultiMove

**DO use when**:
- Using RAS stackable solve with swaps
- Swapping bundles between containers
- Know exactly which destination to swap with
- Objects have heterogeneous dimensions
- Need 1:k uneven swaps

**DO NOT use when**:
- Not using RAS local search
- Don't need swaps (use [FixedDestMultiMove](fixed-dest-multi-move))
- All objects uniform
- Don't know destination
- General optimization

## Related Move Types

**RAS Fixed bundle variants**:
- [FixedDestMultiMove](fixed-dest-multi-move) - Bundle to fixed dest
- [FixedSourceMultiMove](fixed-source-multi-move) - Bundle from fixed source
- **FixedDestSwapMultiMove** - Bundle swaps (this)

**All three used exclusively for RAS stackable solve**

**General swap alternatives**:
- [Swap](../swap/) - General 2-object swaps
- [SwapSampled](../swap/swap-sampled) - Sampled swaps

## Source Code

- Thrift definition: [`interface/thrift/SolverSpecs.thrift:602`](https://github.com/facebookincubator/rebalancer/blob/main/interface/thrift/SolverSpecs.thrift#L602)
- Implementation: [`solver/moves/FixedDestSwapMultiMoveType.h`](https://github.com/facebookincubator/rebalancer/blob/main/solver/moves/FixedDestSwapMultiMoveType.h)
- Tests: [`solver/moves/tests/`](https://github.com/facebookincubator/rebalancer/tree/main/solver/moves/tests/)

## Next Steps

- Review [FixedDestMultiMove](fixed-dest-multi-move) for moves without swaps
- Try [FixedSourceMultiMove](fixed-source-multi-move) for pulling bundles
- See [Move Types Overview](../) for choosing move types
- Learn about RAS local search in the solver strategies guide
