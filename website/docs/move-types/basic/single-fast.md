---
sidebar_position: 2
---

# SingleFast

**Move Type**: Basic
**Complexity**: O(objects × containers) with early termination

Move one object at a time but stop early after finding an improvement. Faster than Single but may miss better moves.

## Overview

`SingleFast` evaluates moving each object from the hot container to every possible destination container, but uses **early termination** to improve performance. After fully exploring `minHotObjects` (default: 1), it returns the best improving move found so far.

**Use when**:
- Speed is more important than finding the absolute best move
- Want faster convergence than Single
- Willing to accept local optima for performance

**Avoid when**:
- Need thorough exploration of all moves
- Problem is small enough for complete Single search
- Quality is more important than speed

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_fast_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_fast_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `destinationsToExplore` | DestinationsToExploreOptions | No | null | Restrict which containers to explore |
| `minHotObjects` | int32 | No | 1 | Minimum objects to fully explore before returning |

## How It Works

Given a **hot container** (the container contributing most to the objective):

1. **Select object**: Pick first object from the hot container
2. **Evaluate all destinations**: Test moving this object to every possible destination container (in parallel)
3. **Check improvement**: If any move improves the objective, remember the best one
4. **Early exit check**: If we've explored `minHotObjects` and found an improving move, **stop and return it**
5. **Continue if needed**: If no improvement found, move to next object
6. **Repeat**: Until improvement found or all objects exhausted

### Visual Example

```
Iteration 1 - Explore obj1:
┌─────────────┐        Test all destinations (parallel)
│ Hot         │  ──> dest1: +5 improvement ✓
│ Container   │  ──> dest2: -2 worse
│  • obj1  ←  │  ──> dest3: +3 improvement
│  • obj2     │
│  • obj3     │  Best: obj1→dest1 (+5)
└─────────────┘
                 minHotObjects=1 reached → STOP, return obj1→dest1

Without early exit (Single), would also explore:
  obj2 → all dests
  obj3 → all dests
```

### Difference from Single

| Aspect | Single | SingleFast |
|--------|--------|------------|
| **Exploration** | All objects, all destinations | Stops after minHotObjects if improvement found |
| **Parallelization** | All moves in parallel | Per-object destinations in parallel |
| **Guarantee** | Finds best single move | Finds early improving move |
| **Speed** | Slower | Faster |

## Complexity

**Minimum moves evaluated**: O(minHotObjects × C)
**Maximum moves evaluated**: O(N × C) (same as Single if no improvements found)

Where:
- N = number of objects in hot container
- C = number of destination containers
- minHotObjects = parameter (default: 1)

**Example - Best case** (improvement found early):
- Hot container has 1000 objects
- System has 100 containers
- minHotObjects = 1 (default)
- Moves evaluated = 1 × 100 = **100 moves** (vs 100,000 for Single)

**Example - Worst case** (no improvements):
- Moves evaluated = 1000 × 100 = 100,000 moves (same as Single)

## Usage Patterns

### Basic Fast Configuration

Default usage for faster convergence:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_fast_examples.py start=basic_configuration_start end=basic_configuration_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_fast_examples.cpp start=basic_configuration_start end=basic_configuration_end
```

</TabItem>
</Tabs>

### Combined with Single

Try SingleFast first, fall back to Single:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_fast_examples.py start=combined_with_single_start end=combined_with_single_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_fast_examples.cpp start=combined_with_single_start end=combined_with_single_end
```

</TabItem>
</Tabs>

### Explore More Objects Before Stopping

Increase minHotObjects for better quality:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_fast_examples.py start=more_objects_start end=more_objects_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_fast_examples.cpp start=more_objects_start end=more_objects_end
```

</TabItem>
</Tabs>

### Restrict Destinations

Limit search space for even faster convergence:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_fast_examples.py start=restrict_destinations_start end=restrict_destinations_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_fast_examples.cpp start=restrict_destinations_start end=restrict_destinations_end
```

</TabItem>
</Tabs>

### Interactive Applications

Fast responses for UI/dashboards:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_fast_examples.py start=interactive_start end=interactive_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_fast_examples.cpp start=interactive_start end=interactive_end
```

</TabItem>
</Tabs>

## Performance Characteristics

### Scalability

| Problem Size | Objects/Container | Containers | SingleFast (typical) | Single (always) |
|--------------|-------------------|------------|---------------------|-----------------|
| Small | 10 | 10 | &lt;1ms | &lt;1ms |
| Medium | 100 | 100 | 1-5ms | 10-50ms |
| Large | 1,000 | 1,000 | 10-100ms | 0.5-2s |
| Very Large | 10,000 | 10,000 | 100ms-2s | 10-60s |

**Note**: Times vary based on when improvements are found. Best case is 10-100x faster than Single.

### Speedup Factor

Actual speedup depends on problem characteristics:

- **Best case** (improvements found early): 10-100x faster
- **Average case** (improvements found midway): 2-10x faster
- **Worst case** (no improvements): Same as Single

### Multi-threading

- Per-object destinations evaluated in parallel
- Scales well up to 8-16 cores
- Less parallel work than Single (explores fewer objects)

## Comparison with Variants

| Move Type | Speed | Thoroughness | Quality | Use Case |
|-----------|-------|--------------|---------|----------|
| **SingleFast** | Fast | Early exit | Good | Default fast choice |
| [Single](single) | Medium | Complete | Best | Need best single move |
| [SingleGreedy](single-greedy) | Fastest | Greedy | Fair | Speed critical, single-threaded OK |
| [SingleRandomBatches](single-random-batches) | Fast | Batched | Good | Parallel batching preferred |

**Recommendation**:
- Use **SingleFast** as default for faster convergence
- Use **Single** when quality is critical and time is available
- Use **SingleGreedy** for single-threaded or extremely time-sensitive cases

## Troubleshooting

### Problem: SingleFast not much faster than Single

**Diagnosis**: Few or no improvements being found early

**Solutions**:
- This is expected for highly constrained or broken initial assignments
- Try improving initial assignment quality
- Consider using SingleGreedy if speed is critical
- Check if problem has many feasible moves

### Problem: Solution quality worse than Single

**Diagnosis**: Early termination missing better moves

**Solutions**:
- Increase `minHotObjects` to explore more before stopping
- Use SingleFast for initial passes, switch to Single for final refinement
- Combine both: `[SingleFastMoveTypeSpec(), SingleMoveTypeSpec()]`
- Accept the trade-off (speed vs quality)

### Problem: Still too slow

**Diagnosis**: Worst-case behavior (no early improvements)

**Solutions**:
- Reduce destinations with `destinationsToExplore`
- Use [SingleGreedy](single-greedy) for single-threaded speed
- Use [SingleRandomBatches](single-random-batches) for parallel batching
- Check if initial assignment allows any improvements

### Problem: Getting stuck in local optima

**Diagnosis**: Early termination prevents finding escape moves

**Solutions**:
- Add more powerful move types after SingleFast (Swap, Chain)
- Increase `minHotObjects` to be less greedy
- Use Single periodically for thorough search
- Try multiple solver runs with different seeds

## Related Move Types

**Variants**:
- [Single](single) - Thorough version without early termination
- [SingleGreedy](single-greedy) - Even faster with greedy destination selection
- [SingleRandomBatches](single-random-batches) - Parallel batched approach

**Complementary**:
- [Swap](../swap/) - For capacity-constrained problems
- [SingleEndChain](../chain/single-end-chain) - For 2-move sequences

**When to use which**:
- Small problem (&lt;1K objects): Use **Single** (thoroughness worth the time)
- Medium problem (1-10K objects): Use **SingleFast** (good speed/quality balance)
- Large problem (&gt;10K objects): Use **SingleFast** or **SingleGreedy**
- Interactive/UI: Use **SingleFast** with low time limits

## Source Code

- Thrift definition: [`interface/thrift/SolverSpecs.thrift:513`](https://github.com/facebookincubator/rebalancer/blob/main/interface/thrift/SolverSpecs.thrift#L513)
- Implementation: [`solver/moves/SingleFastMoveType.h`](https://github.com/facebookincubator/rebalancer/blob/main/solver/moves/SingleFastMoveType.h)
- Tests: [`solver/moves/tests/`](https://github.com/facebookincubator/rebalancer/tree/main/solver/moves/tests/)

## Next Steps

- Learn about [Single](single) for thorough exploration
- Try [SingleGreedy](single-greedy) for even faster single-threaded speed
- Review [Move Types Overview](../) for choosing move types
- Check [Performance Guide](../../solvers/performance) for tuning
