---
sidebar_position: 4
---

# SwapSampled

**Move Type**: Swap
**Complexity**: O(sample²) instead of O(objects²)

Sample a subset of objects for swapping instead of trying all combinations. Essential for large-scale capacity-constrained problems.

## Overview

`SwapSampled` uses `SwapMoveTypeSpec` with the `sampleSize` parameter to limit the number of swap combinations evaluated. Instead of trying all possible object pairs, it samples objects from both source and destination containers.

**Use when**:
- Problem is large (&gt;10K objects)
- Full Swap is too slow
- Capacity constraints require swaps but exhaustive search is impractical

**Avoid when**:
- Problem is small enough for full Swap
- Need to explore all possible swaps
- Sample size might miss important swaps

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/swap_sampled_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/swap_sampled_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

SwapSampled uses `SwapMoveTypeSpec` with these key parameters:

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `sampleSize` | SampleSize | **Yes** | null | Controls sampling on both src and dst |
| `partitionNameToExploreSwapsWithinObjectGroup` | string | No | null | Only swap within same group |
| `greedyOnSrc` | bool | No | false | Exit early when trying src objects |
| `greedyOnDst` | bool | No | false | Exit early when trying dst objects |
| `destinationsToExplore` | DestinationsToExploreOptions | No | null | Restrict destination containers |

### SampleSize Structure

| Field | Type | Description |
|-------|------|-------------|
| `defaultSampleSize` | int32 | Default sample size for all objects |
| `objectToSampleSize` | map&lt;string, int32&gt; | Per-object sample size override |

## How It Works

Given a **hot container** and **cold container**:

1. **Sample source objects**: Select up to `sampleSize` objects from hot container
2. **Sample destination objects**: Select up to `sampleSize` objects from cold container
3. **Evaluate swaps**: Test swapping each sampled source object with each sampled destination object
4. **Apply best**: Apply the best swap that improves the objective

### Visual Example

```
Full Swap (100 × 100 = 10,000 swaps):
Hot Container (100 objects) × Cold Container (100 objects)
  = 10,000 swap combinations to evaluate

SwapSampled with sampleSize=10 (10 × 10 = 100 swaps):
Hot Container (sample 10) × Cold Container (sample 10)
  = 100 swap combinations to evaluate (100x speedup!)
```

### Comparison with Full Swap

| Aspect | Swap | SwapSampled |
|--------|------|-------------|
| **Combinations** | N × M | sample × sample |
| **Speed** | Slow for large N,M | Much faster |
| **Quality** | Best swap guaranteed | Good swap likely |
| **Use case** | Small/medium problems | Large problems |

## Complexity

**Moves evaluated per iteration**: O(S² × C)

Where:
- S = sample size (from `sampleSize` parameter)
- C = number of cold containers
- Compare to full Swap: O(N × M × C) where N,M = object counts

**Example - Large problem**:
- Hot container: 10,000 objects
- Cold containers: 1,000 containers with ~10,000 objects each
- Full Swap: 10,000 × 10,000 × 1,000 = **100 billion** evaluations
- SwapSampled (sample=100): 100 × 100 × 1,000 = **10 million** evaluations (10,000x speedup!)

## Usage Patterns

### Basic Sampling

Default usage with reasonable sample size:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/swap_sampled_examples.py start=basic_sampling_start end=basic_sampling_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/swap_sampled_examples.cpp start=basic_sampling_start end=basic_sampling_end
```

</TabItem>
</Tabs>

### Per-Object Sample Size

Different sample sizes for different objects:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/swap_sampled_examples.py start=per_object_sampling_start end=per_object_sampling_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/swap_sampled_examples.cpp start=per_object_sampling_start end=per_object_sampling_end
```

</TabItem>
</Tabs>

### Large Problem Configuration

Aggressive sampling for very large problems:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/swap_sampled_examples.py start=large_problem_start end=large_problem_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/swap_sampled_examples.cpp start=large_problem_start end=large_problem_end
```

</TabItem>
</Tabs>

### Combined with Greedy Flags

Sample + early exit for maximum speed:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/swap_sampled_examples.py start=greedy_sampling_start end=greedy_sampling_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/swap_sampled_examples.cpp start=greedy_sampling_start end=greedy_sampling_end
```

</TabItem>
</Tabs>

### Adaptive Sampling Strategy

Start with small sample, increase if needed:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/swap_sampled_examples.py start=adaptive_sampling_start end=adaptive_sampling_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/swap_sampled_examples.cpp start=adaptive_sampling_start end=adaptive_sampling_end
```

</TabItem>
</Tabs>

## Performance Characteristics

### Recommended Sample Sizes

| Problem Size | Objects per Container | Recommended Sample Size | Speedup vs Full |
|--------------|----------------------|------------------------|-----------------|
| Medium | 100-1K | 50-100 | 10-100x |
| Large | 1K-10K | 100-500 | 100-1000x |
| Very Large | &gt;10K | 500-1000 | 1000-10000x |

### Sampling Strategy

**Conservative** (higher quality, slower):
- Sample size = sqrt(object count)
- Example: 10,000 objects → sample 100

**Balanced** (good trade-off):
- Sample size = 100-200 (fixed)
- Works well for most problems

**Aggressive** (maximum speed):
- Sample size = 50 or less
- Accept lower quality for speed

## Comparison with Variants

| Move Type | Speed | Completeness | Use Case |
|-----------|-------|--------------|----------|
| [Swap](swap.md) | Slow | Complete | Small/medium (&lt;10K objects) |
| **SwapSampled** | Fast | Sampled | Large (&gt;10K objects) |
| [SwapFullContainers](swap-full-containers) | Medium | Container-level | Full container swaps |
| [SwapFullWithEmpty](swap-full-with-empty) | Fast | Empty targets only | Consolidation |

## Troubleshooting

### Problem: SwapSampled not finding good moves

**Diagnosis**: Sample size too small, missing good swaps

**Solutions**:
- Increase `defaultSampleSize` (try doubling it)
- Use per-object sampling for important objects
- Try multiple runs with different random seeds
- Check if full Swap finds better moves (on smaller subset)

### Problem: Still too slow

**Diagnosis**: Sample size too large or problem extremely large

**Solutions**:
- Reduce sample size (try 50 or less)
- Enable `greedyOnSrc` and `greedyOnDst` for early exit
- Restrict destinations with `destinationsToExplore`
- Consider if swaps are necessary (try other move types)

### Problem: Solution quality much worse than expected

**Diagnosis**: Sampling missing critical swaps

**Solutions**:
- Increase sample size for better coverage
- Use stratified sampling (sample from different object groups)
- Combine with full Swap in multi-stage approach
- Try [SingleEndChain](../chain/single-end-chain) as alternative

### Problem: Non-deterministic results

**Diagnosis**: Random sampling produces different results each run

**Solutions**:
- Set `randomSeed` in LocalSearchSolverSpec for reproducibility
- Use larger sample size for more stable results
- Run multiple times and pick best result
- Accept variance as trade-off for speed

## Choosing Sample Size

**Rule of thumb**: Sample size² should be &gt; number of containers

**Example calculation**:
- 1000 containers
- sqrt(1000) ≈ 32
- Recommended sample size: 50-100

**Validation**:
- Run with sample size S
- Run with sample size 2×S
- If results similar → S is sufficient
- If results much better with 2×S → increase S

## Related Move Types

**Variants**:
- [Swap](swap.md) - Full exhaustive swap (no sampling)
- [SwapFullContainers](swap-full-containers) - Swap entire containers
- [SwapFullWithEmpty](swap-full-with-empty) - Move to empty containers

**Alternatives**:
- [SingleEndChain](../chain/single-end-chain) - 2-move sequences (alternative to swaps)
- [SingleFast](../basic/single-fast) - Faster single moves with early exit

**Complementary**:
- [Single](../basic/single) - Try before SwapSampled (simpler)
- Greedy flags - Combine for even faster convergence

## Source Code

- Thrift definition: [`interface/thrift/SolverSpecs.thrift:537`](https://github.com/facebookincubator/rebalancer/blob/main/interface/thrift/SolverSpecs.thrift#L537) (SwapMoveTypeSpec)
- Sample size struct: [`interface/thrift/SolverSpecs.thrift:458`](https://github.com/facebookincubator/rebalancer/blob/main/interface/thrift/SolverSpecs.thrift#L458)
- Implementation: [`solver/moves/SwapMoveType.h`](https://github.com/facebookincubator/rebalancer/blob/main/solver/moves/SwapMoveType.h)
- Tests: [`solver/moves/tests/SwapMoveTypeTest.cpp`](https://github.com/facebookincubator/rebalancer/tree/main/solver/moves/tests/SwapMoveTypeTest.cpp)

## Next Steps

- Learn about [Swap](swap.md) for full exhaustive search
- Try [SwapFullContainers](swap-full-containers) for container-level swaps
- Review [Performance Guide](../../solvers/performance) for tuning sample sizes
- See [Move Types Overview](../) for choosing move types
