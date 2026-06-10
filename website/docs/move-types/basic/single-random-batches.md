---
sidebar_position: 4
---

# SingleRandomBatches

**Move Type**: Basic
**Complexity**: O(objects × containers) with batched evaluation and early termination

Evaluate destination containers in random batches with parallel processing. Combines speed (early exit) with multi-threading. Use for very large problems where full exploration is too slow.

## Overview

`SingleRandomBatches` evaluates single object moves but processes destination containers in **random batches**, stopping as soon as a batch contains an improving move. This move type is designed for **parallelization** - each batch of destinations can be evaluated concurrently.

**Use when**:
- Problem is very large (100K+ objects, 1K+ containers)
- Have multiple CPU cores available
- Need faster convergence than Single
- Can benefit from randomization and parallelism

**Avoid when**:
- Problem is small/medium (use Single or SingleFast)
- Single-threaded execution
- Need deterministic results
- Already using sampling approaches

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_random_batches_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_random_batches_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `randomContainerBatchSize` | int32 | No | 10 | Number of destination containers to process in each batch |

### Parameter Details

**randomContainerBatchSize**:
- **Purpose**: Controls parallelization granularity and early exit frequency
- **Small values** (5-10): Exit earlier, less parallelism, faster response
- **Large values** (50-100): More parallelism, better move quality, more work
- **Recommendation**: Start with default (10), increase for better multi-core utilization

## How It Works

Given a **hot container** (the container contributing most to the objective):

1. **Select object**: Pick object from hot container
2. **Shuffle containers**: Randomly order all destination containers
3. **Create batch**: Take next `randomContainerBatchSize` containers
4. **Evaluate in parallel**: Test moving object to each container in batch (multi-threaded)
5. **Check for improvement**: If any move in batch improves objective → apply best from batch and STOP
6. **Next batch**: If no improvement, take next batch and repeat
7. **Next object**: If all batches exhausted, try next object

### Visual Example

```
Problem: 1,000 containers, batch size = 10

Object 1:
  Batch 1 (containers 1-10):    [evaluate in parallel] → No improvement
  Batch 2 (containers 11-20):   [evaluate in parallel] → Improvement found! ✓
  → Apply best move from Batch 2, STOP

Evaluated only 20 destinations instead of 1,000 (50x speedup!)
```

### Batching Strategy

```
Traditional Single:
  obj1 → [c1, c2, c3, ..., c1000] (sequential, 1000 evaluations)

SingleRandomBatches (batch=10):
  obj1 → Batch 1: [c847, c23, c901, c456, c12, c789, c234, c567, c890, c345]
         ↓ Evaluate in parallel (multi-threaded)
         ↓ Found improvement → STOP (10 evaluations only!)
```

## Complexity

**Best case**: O(batch_size) - First batch has improvement

**Average case**: O(objects × containers / k) where k = improvement frequency

**Worst case**: O(objects × containers) - No improvements found

Where:
- batch_size = `randomContainerBatchSize`
- k = number of batches before finding improvement

**Example - Very large problem**:
- Hot container: 100,000 objects
- System: 10,000 containers
- Batch size: 10
- Typical run: **1-10 batches per object** = 10-100 destinations evaluated
- Full Single would evaluate: 100,000 × 10,000 = **1 billion** moves

## Usage Patterns

### Very Large Scale

For problems too large for regular Single:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_random_batches_examples.py start=large_scale_start end=large_scale_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_random_batches_examples.cpp start=large_scale_start end=large_scale_end
```

</TabItem>
</Tabs>

### Tuning Batch Size

Adjust batch size for your workload:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_random_batches_examples.py start=batch_tuning_start end=batch_tuning_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_random_batches_examples.cpp start=batch_tuning_start end=batch_tuning_end
```

</TabItem>
</Tabs>

### Multi-Core Optimization

Maximize parallelism on multi-core systems:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_random_batches_examples.py start=multicore_start end=multicore_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_random_batches_examples.cpp start=multicore_start end=multicore_end
```

</TabItem>
</Tabs>

### Combined Strategy

Use with other move types for balance:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_random_batches_examples.py start=combined_start end=combined_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_random_batches_examples.cpp start=combined_start end=combined_end
```

</TabItem>
</Tabs>

## Performance Characteristics

### Speedup Analysis

| Batch Size | Parallelism | Early Exit Frequency | Quality | Use Case |
|------------|-------------|---------------------|---------|----------|
| 5 | Low | High (every 5) | Lower | Fast response, few cores |
| 10 (default) | Medium | Medium (every 10) | Medium | Balanced |
| 50 | High | Low (every 50) | Higher | Many cores, quality focus |
| 100 | Very High | Very Low (every 100) | Highest | Maximum parallelism |

### Multi-Threading Benefit

Assumes `N` CPU cores available:

| Cores | Batch Size | Parallel Speedup | Total Speedup vs Single |
|-------|------------|------------------|------------------------|
| 1 | 10 | 1x | ~5-10x (early exit only) |
| 4 | 10 | ~4x | ~20-40x |
| 8 | 10 | ~8x | ~40-80x |
| 16 | 50 | ~16x | ~100-200x |

**Note**: Speedup depends on objective function evaluation cost and improvement frequency.

## Comparison with Variants

| Move Type | Parallelism | Early Exit | Randomization | Use Case |
|-----------|-------------|------------|---------------|----------|
| [Single](single) | No | No | No | Best quality |
| [SingleFast](single-fast) | No | Yes (per object) | No | Balanced |
| [SingleGreedy](single-greedy) | No | Yes (per move) | No | Fastest single-thread |
| **SingleRandomBatches** | Yes | Yes (per batch) | Yes | Very large + multi-core |
| [SingleRandomStratified](single-random-stratified) | No | No | Yes (stratified) | Large with strata |

**Decision tree**:
1. **Small problem** (&lt;10K objects) → [Single](single) or [SingleFast](single-fast)
2. **Medium problem** (10K-100K objects) → [SingleFast](single-fast)
3. **Large + single-threaded** → [SingleGreedy](single-greedy)
4. **Large + multi-core** → **SingleRandomBatches**
5. **Stratified structure** → [SingleRandomStratified](single-random-stratified)

## Troubleshooting

### Problem: Not seeing parallelism speedup

**Diagnosis**: Objective evaluation too fast or batch size too small

**Solutions**:
- Increase `randomContainerBatchSize` (try 50-100)
- Check if objective evaluation is CPU-bound
- Verify multi-threading is enabled
- Profile to find bottlenecks

### Problem: Poor solution quality

**Diagnosis**: Exiting too early, missing better moves due to randomization

**Solutions**:
- Increase batch size for more exploration per batch
- Run multiple times with different random seeds
- Combine with [Single](single) for final refinement
- Consider [SingleRandomStratified](single-random-stratified) for structured sampling

### Problem: Non-deterministic results

**Diagnosis**: Random shuffling produces different results each run

**Solutions**:
- Set `randomSeed` in LocalSearchSolverSpec
- Accept variance as trade-off for speed
- Run multiple times and pick best result
- Use deterministic move types if reproducibility critical

### Problem: Still too slow

**Diagnosis**: Problem too large even with batching

**Solutions**:
- Reduce batch size for faster early exit
- Enable [SingleGreedy](single-greedy) for even faster convergence
- Use destination filtering (`destinationsToExplore`)
- Consider sampling approaches (SwapSampled for swaps)
- Pre-process to reduce problem size

## Choosing Batch Size

**General guidelines**:

```
Batch size = min(
  number of CPU cores × 2,
  containers / 100
)
```

**Examples**:
- 8 cores, 1000 containers → batch = min(16, 10) = **10** ✓
- 16 cores, 10000 containers → batch = min(32, 100) = **32**
- 4 cores, 100 containers → batch = min(8, 1) = **1** (use SingleFast instead)

**Tuning approach**:
1. Start with default (10)
2. Profile CPU utilization
3. If CPUs underutilized → increase batch size
4. If quality poor → increase batch size
5. If too slow to exit → decrease batch size

## Related Move Types

**Single move variants**:
- [Single](single) - Full exploration (no batching)
- [SingleFast](single-fast) - Early exit per object
- [SingleGreedy](single-greedy) - Early exit per move

**Randomized alternatives**:
- [SingleRandomStratified](single-random-stratified) - Stratified sampling
- [SingleColdestStratified](single-coldest-stratified) - Coldest-first sampling
- [SingleRandomObjectStratified](single-random-object-stratified) - Object-level stratification

**Complementary**:
- Use SingleRandomBatches for initial improvement
- Follow with Single for final refinement
- Combine with Swap for capacity constraints

## Source Code

- Thrift definition: [`interface/thrift/SolverSpecs.thrift:529`](https://github.com/facebookincubator/rebalancer/blob/main/interface/thrift/SolverSpecs.thrift#L529)
- Implementation: [`solver/moves/SingleRandomBatchesMoveType.h`](https://github.com/facebookincubator/rebalancer/blob/main/solver/moves/SingleRandomBatchesMoveType.h)
- Tests: [`solver/moves/tests/`](https://github.com/facebookincubator/rebalancer/tree/main/solver/moves/tests/)

## Next Steps

- Try [SingleRandomStratified](single-random-stratified) for stratified sampling
- Learn about [Single](single) for full exploration
- Review [Performance Guide](../../solvers/performance) for parallelization
- See [Move Types Overview](../) for choosing move types
