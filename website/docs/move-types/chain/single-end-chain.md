---
sidebar_position: 3
---

# SingleEndChain

**Move Type**: Chain
**Complexity**: O(objects² × containers)

Perform 2-move chains where an object leaves the hot container and another object moves to a different container (not back to hot container). Often **better than Swap** for capacity-constrained problems.

## Overview

`SingleEndChain` evaluates 2-move sequences where:
1. Object A moves from hot container → container X
2. Object B moves from container X → container Y (different from hot container)

The hot container is at the **end** of the chain (receives no object back), unlike [SingleChain](single-chain) where the hot container is in the **middle**.

**Use when**:
- Capacity constraints are tight (alternative to Swap)
- Need 2-move sequences to improve objective
- Swap moves aren't finding improvements

**Prefer over [SingleChain](single-chain)**:
- SingleEndChain is generally more effective
- Recommended default for chain moves

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_end_chain_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_end_chain_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| (none) | - | - | - | SingleEndChain has no configuration parameters |

## How It Works

Given a **hot container** (the container contributing most to the objective):

1. **Select hot object**: Pick object from hot container
2. **Select intermediate container**: Pick container X (receives hot object)
3. **Select other object**: Pick object from container X
4. **Select final container**: Pick container Y ≠ hot container
5. **Evaluate chain**: Test moving hot object → X, other object → Y (simultaneously)
6. **Repeat**: Try all combinations
7. **Apply best**: Apply the 2-move chain improving objective most

### Visual Example

```
Before:                          After (if chain applied):
┌─────────────┐                 ┌─────────────┐
│ Hot         │                 │ Hot         │
│ Container   │  ─────(1)────>  │ Container   │
│  • obj1     │                 │  • obj2     │
│  • obj2     │                 │  • obj3     │
│  • obj3     │                 └─────────────┘
└─────────────┘
┌─────────────┐                 ┌─────────────┐
│ Container X │                 │ Container X │
│  • objA     │  ─────(2)────>  │  • obj1  ←─┐│
│  • objB     │                 │  • objB   ││
└─────────────┘                 └───────────┼┘
┌─────────────┐                 ┌───────────┼┐
│ Container Y │                 │ Container Y││
│  • objX     │                 │  • objX   ││
│  • objY     │                 │  • objY   ││
└─────────────┘                 │  • objA ←─┘│
                                └─────────────┘

Chain: obj1 (Hot→X), objA (X→Y)
```

### Comparison with SingleChain

**SingleEndChain** (Recommended):
```
Hot Container → Container X → Container Y
  gives obj1      gives objA    receives objA
  (net: -1)       (net: 0)      (net: +1)
```

**SingleChain** (Less recommended):
```
Hot Container ← Container X → Container Y
  receives objB   gives objA    receives objA
  (net: 0)       gives objB     (net: +1)
                 (net: -1)
```

**Why SingleEndChain is better**: Hot container is "broken" (highest cost), so we want to **reduce** its load, not keep it the same.

## Complexity

**Moves evaluated per iteration**: O(N × M × C²)
- Simplifies to O(N² × C) for uniform container sizes

Where:
- N = number of objects in hot container
- M = average number of objects per container
- C = number of containers

**Example**:
- Hot container has 100 objects
- Each container has ~100 objects
- System has 50 containers
- Moves evaluated = 100 × 100 × 50² ≈ 25M moves

**Warning**: Very expensive! Use after simpler move types.

## Usage Patterns

### Basic Configuration

Use after Single and Swap for better quality:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_end_chain_examples.py start=basic_configuration_start end=basic_configuration_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_end_chain_examples.cpp start=basic_configuration_start end=basic_configuration_end
```

</TabItem>
</Tabs>

### Capacity-Constrained with High Quality Needs

When you need best possible solution for tight capacity:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_end_chain_examples.py start=high_quality_start end=high_quality_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_end_chain_examples.cpp start=high_quality_start end=high_quality_end
```

</TabItem>
</Tabs>

### Production Rebalancing

Balance quality and time:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_end_chain_examples.py start=production_rebalancing_start end=production_rebalancing_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_end_chain_examples.cpp start=production_rebalancing_start end=production_rebalancing_end
```

</TabItem>
</Tabs>

## Performance Characteristics

### Scalability

| Objects | Containers | Typical Time per Iteration | Recommendation |
|---------|------------|---------------------------|----------------|
| &lt;100 | &lt;10 | &lt;1s | Safe to use |
| 100-1K | 10-100 | 1-30s | Use with time limits |
| &gt;1K | &gt;100 | &gt;30s | Consider avoiding |

### Time Limits

Since SingleEndChain is expensive, often used with time limits:

```python
LocalSearchSolverSpec(
    solveTime=300,  # 5 minutes total
    moveTypeList=[
        SingleMoveTypeSpec(),      # Fast
        SwapMoveTypeSpec(),        # Medium
        SingleEndChainMoveTypeSpec(),  # Expensive - gets remaining time
    ]
)
```

### Recommended Problem Sizes

- **Use freely**: &lt;1K objects, &lt;100 containers
- **Use cautiously**: 1K-10K objects, 100-1K containers (with time limits)
- **Avoid**: &gt;10K objects or &gt;1K containers

## Comparison with Alternatives

| Move Type | Speed | Quality | Capacity-Aware | Use Case |
|-----------|-------|---------|----------------|----------|
| [Single](../basic/single) | Fast | Good | No | Unconstrained problems |
| [Swap](../swap/) | Medium | Better | Yes | Capacity-constrained |
| **SingleEndChain** | Slow | Best | Yes | High-quality capacity solutions |
| [SingleChain](single-chain) | Slow | Good | Yes | Less effective than SingleEndChain |
| [SingleChainFast](single-chain-fast) | Medium | Good | Yes | Faster chain variant |

**Recommendation**: For capacity-constrained problems:
1. Start with Single + Swap
2. Add SingleEndChain if you need better quality and can afford the time
3. Consider [SingleChainFast](single-chain-fast) as middle ground

## Troubleshooting

### Problem: SingleEndChain too slow

**Diagnosis**: O(N² × C) too expensive for problem size

**Solutions**:
- Remove SingleEndChain for large problems (>10K objects)
- Use [SingleChainFast](single-chain-fast) instead (parallelized with early exit)
- Add `solveTime` limit to prevent excessive time
- Try Swap instead (simpler, often sufficient)

### Problem: Not finding improvements

**Diagnosis**: May need even more complex moves or better initial state

**Solutions**:
- Ensure Single and Swap tried first (SingleEndChain should be last)
- Add [TripleLoop](../advanced/triple-loop) for 3-object moves (very expensive)
- Check if initial assignment is feasible
- Verify constraints allow chain moves

### Problem: Taking too long per iteration

**Diagnosis**: Many objects/containers = expensive evaluation

**Solutions**:
- Set `solveTime` to limit total time
- Use SingleEndChain only in final optimization stage
- Consider whether chain moves are necessary for your problem
- Profile to check if other issues (slow constraints/objectives)

## When to Use vs Swap

**Use Swap when**:
- Problem size is large (&gt;10K objects)
- Need capacity-aware moves quickly
- Simple swaps are sufficient

**Use SingleEndChain when**:
- Need highest quality solution
- Have time budget for expensive search
- Swap alone isn't finding good solutions
- Problem size is small-medium (&lt;10K objects)

**Use both when**:
```python
moveTypeList=[
    SingleMoveTypeSpec(),
    SwapMoveTypeSpec(),          # Try swaps first (faster)
    SingleEndChainMoveTypeSpec(),  # Then chains (slower, higher quality)
]
```

## Related Move Types

**Variants**:
- [SingleChain](single-chain) - Chain with hot container in middle (less effective)
- [SingleChainFast](single-chain-fast) - Parallelized chain with early exit

**Alternatives**:
- [Swap](../swap/) - Simpler 2-object exchange (often sufficient)
- [Single](../basic/single) - Simple 1-object moves

**More Complex**:
- [TripleLoop](../advanced/triple-loop) - 3-object cyclic moves (even more expensive)

## Source Code

- Thrift definition: [`interface/thrift/SolverSpecs.thrift:535`](https://github.com/facebookincubator/rebalancer/blob/main/interface/thrift/SolverSpecs.thrift#L535)
- Implementation: [`solver/moves/SingleEndChainMoveType.h`](https://github.com/facebookincubator/rebalancer/blob/main/solver/moves/SingleEndChainMoveType.h)
- Tests: [`solver/moves/tests/`](https://github.com/facebookincubator/rebalancer/tree/main/solver/moves/tests/)

## Next Steps

- Compare with [SingleChain](single-chain) to understand the difference
- Try [SingleChainFast](single-chain-fast) for faster chain moves
- Learn about [Swap](../swap/) as simpler alternative
- Review [Performance Guide](../../solvers/performance) for optimization
