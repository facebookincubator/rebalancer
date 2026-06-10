---
sidebar_position: 3
---

# SingleGreedy

**Move Type**: Basic
**Complexity**: O(1) to O(objects × containers) with early termination

Stop as soon as ANY improving move is found. Ultra-fast convergence, but may miss better moves. Use when speed matters more than solution quality.

## Overview

`SingleGreedy` evaluates single object moves but **stops immediately** when it finds the first move that improves the objective. Unlike `SingleFast` which explores at least one object fully, `SingleGreedy` can stop even earlier - mid-way through exploring destinations for a single object.

**Use when**:
- Speed is critical (interactive/real-time systems)
- Quick "good enough" solution needed
- Problem updates frequently (re-runs are cheap)
- Initial assignment is already decent

**Avoid when**:
- Solution quality is important
- Making many moves at once (may thrash)
- Problem is small (overhead dominates)
- Need deterministic/reproducible results

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_greedy_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_greedy_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| (none) | - | - | - | SingleGreedy has no configuration parameters |

## How It Works

Given a **hot container** (the container contributing most to the objective):

1. **Select object**: Pick object from hot container
2. **Try destination**: Pick a different container
3. **Evaluate move**: Test moving object to that container
4. **Early exit**: If move improves objective → **STOP and apply it**
5. **Continue**: Otherwise, try next destination or next object
6. **Worst case**: If no improving move found, try all combinations

### Visual Example

```
Hot Container (100 objects):
  obj1 → container_A: improvement found! ✓ STOP HERE

SingleGreedy returns after evaluating just 1 move!

Comparison with other move types:
- SingleGreedy:  Tries 1 move, finds improvement, stops
- SingleFast:    Tries 1000 moves (all destinations for obj1), picks best
- Single:        Tries 100,000 moves (all objects × all destinations), picks best
```

### Greedy vs Fast vs Full

| Move Type | Evaluation Strategy | Typical Moves Evaluated | Quality |
|-----------|---------------------|------------------------|---------|
| **SingleGreedy** | Stop at first improvement | 1-100 | Lowest |
| [SingleFast](single-fast) | Try all destinations for first improving object | 100-1K | Medium |
| [Single](single) | Try all objects and destinations | 10K-1M | Highest |

## Complexity

**Best case**: O(1) - First move tried improves objective

**Average case**: O(N × C / k) where k = improvement frequency

**Worst case**: O(N × C) - No improving moves found

Where:
- N = number of objects in hot container
- C = number of containers
- k = average number of moves tried before finding improvement

**Example - Interactive system**:
- Hot container: 1,000 objects
- System: 100 containers
- Typical run: Finds improvement in **10-100 moves** (vs 100K for Single)

## Usage Patterns

### Real-Time/Interactive Systems

Ultra-fast response for dashboards and UIs:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_greedy_examples.py start=interactive_start end=interactive_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_greedy_examples.cpp start=interactive_start end=interactive_end
```

</TabItem>
</Tabs>

### Continuous Rebalancing

Frequent small adjustments:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_greedy_examples.py start=continuous_start end=continuous_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_greedy_examples.cpp start=continuous_start end=continuous_end
```

</TabItem>
</Tabs>

### Combined with Slower Moves

Use greedy for quick wins, then thorough search:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_greedy_examples.py start=combined_start end=combined_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_greedy_examples.cpp start=combined_start end=combined_end
```

</TabItem>
</Tabs>

### Quick Warm-Start

Fast initial solution before refinement:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_greedy_examples.py start=warmstart_start end=warmstart_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_greedy_examples.cpp start=warmstart_start end=warmstart_end
```

</TabItem>
</Tabs>

## Performance Characteristics

### Speed vs Quality Trade-off

| Metric | SingleGreedy | SingleFast | Single |
|--------|--------------|------------|--------|
| **Speed** | Fastest | Fast | Slow |
| **Moves evaluated** | 1-100 | 100-1K | 10K-1M |
| **Solution quality** | 60-80% optimal | 80-95% optimal | 95-100% optimal |
| **Iteration time** | &lt;1ms | 1-10ms | 10-1000ms |
| **Convergence** | Fast (many iterations) | Medium | Slow (few iterations) |

### When Does It Help?

SingleGreedy helps when:
- **Frequent updates**: Problem changes often, re-runs are common
- **Good initial state**: Already near optimal, just need tweaks
- **Interactive use**: Human waiting for response
- **Many small improvements**: Easy to find some improvement quickly

SingleGreedy does NOT help when:
- **Poor initial state**: Need big improvements, not first improvement
- **Rare improvements**: Takes long to find ANY improvement
- **Quality critical**: Can't accept mediocre solutions
- **One-shot optimization**: Single run needs to be as good as possible

## Comparison with Variants

| Move Type | Early Exit Strategy | Exploration Level | Use Case |
|-----------|-------------------|-------------------|----------|
| [Single](single) | None | Full (all objects × all destinations) | Quality critical |
| [SingleFast](single-fast) | First improving object | One object fully | Balanced |
| **SingleGreedy** | First improving move | Minimal | Speed critical |
| [SingleRandomBatches](single-random-batches) | Batch-based | Random samples | Very large problems |

**Decision tree**:
1. Need best solution? → [Single](single)
2. Balanced quality/speed? → [SingleFast](single-fast)
3. Need fastest response? → **SingleGreedy**
4. Problem huge (100K+ objects)? → [SingleRandomBatches](single-random-batches)

## Troubleshooting

### Problem: Making many small moves, but objective not improving much

**Diagnosis**: Greedy approach finding local improvements but missing better global moves

**Solutions**:
- Switch to [SingleFast](single-fast) or [Single](single) for better moves
- Use SingleGreedy for quick wins, then switch to thorough search
- Combine with other move types (Swap, Chain)
- Increase iteration limit to let greedy make more sequential moves

### Problem: Not finding any improving moves

**Diagnosis**: Already at local optimum OR improvements are rare

**Solutions**:
- This is expected at local optimum - switch to more powerful move types
- Try [Swap](../swap/) or [SingleEndChain](../chain/single-end-chain)
- Check if constraints are too restrictive
- Verify objective function is correct

### Problem: Solution quality much worse than expected

**Diagnosis**: Greedy strategy too aggressive, missing much better moves

**Solutions**:
- Use [SingleFast](single-fast) for better quality (small speed cost)
- Use [Single](single) for best quality (larger speed cost)
- Run greedy multiple times with different random seeds
- Use greedy for warm-start, then refine with Single

### Problem: Still too slow for interactive use

**Diagnosis**: Even single move evaluation is expensive

**Solutions**:
- Check objective function efficiency (is evaluation O(1)?)
- Reduce problem size (fewer objects/containers)
- Set strict `solveTime` limit (e.g., 100ms)
- Pre-filter candidate destinations
- Consider if full rebalancing is needed for every change

## When to Use SingleGreedy

**DO use when**:
- Interactive systems (dashboards, UIs)
- Continuous rebalancing (frequent small changes)
- Speed critical (&lt;10ms response time needed)
- Initial solution is already decent
- Running frequently (can iterate to good solution over time)

**DO NOT use when**:
- One-shot optimization (single run needs to be optimal)
- Solution quality is critical
- Making infrequent large-scale changes
- Need deterministic/reproducible results
- Poor initial assignment (need big improvements)

## Related Move Types

**Similar speed optimization**:
- [SingleFast](single-fast) - Slightly slower, better quality
- [SingleRandomBatches](single-random-batches) - For very large problems

**Better quality alternatives**:
- [Single](single) - Full exploration, best single moves
- [Swap](../swap/) - Capacity-constrained problems

**Complementary**:
- Use SingleGreedy first for quick wins
- Follow with Single/Swap for refinement
- Combine in multi-stage solver

## Source Code

- Thrift definition: [`interface/thrift/SolverSpecs.thrift:518`](https://github.com/facebookincubator/rebalancer/blob/main/interface/thrift/SolverSpecs.thrift#L518)
- Implementation: [`solver/moves/SingleGreedyMoveType.h`](https://github.com/facebookincubator/rebalancer/blob/main/solver/moves/SingleGreedyMoveType.h)
- Tests: [`solver/moves/tests/`](https://github.com/facebookincubator/rebalancer/tree/main/solver/moves/tests/)

## Next Steps

- Try [SingleFast](single-fast) if SingleGreedy too aggressive
- Learn about [Single](single) for full exploration
- Review [Performance Guide](../../solvers/performance) for speed optimization
- See [Move Types Overview](../) for choosing move types
