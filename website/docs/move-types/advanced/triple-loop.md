---
sidebar_position: 1
---

# TripleLoop

**Move Type**: Advanced
**Complexity**: O(objects³)

Evaluate 3-object cyclic moves to escape deep local optima. **Very expensive** - use only when simpler move types fail.

## Overview

`TripleLoop` evaluates moving three objects in a cycle: object A from hot container to container X, object B from container X to container Y, and object C from container Y back to hot container. This powerful but expensive move type can escape local optima that simpler moves cannot.

**Use when**:
- Stuck in deep local optimum with simpler move types
- Problem requires complex multi-object rearrangements
- Solution quality is critical and time is available
- Problem size is small (&lt;1K objects)

**Avoid when**:
- Problem is large (&gt;1K objects) - will be extremely slow
- Simpler move types (Single, Swap, Chain) work
- Time constraints are strict
- Running in production with strict SLAs

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/triple_loop_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/triple_loop_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| (none) | - | - | - | TripleLoop has no configuration parameters |

## How It Works

Given a **hot container** (the container contributing most to the objective):

1. **Select hot object**: Pick object A from hot container
2. **Select container X**: Pick different container X
3. **Select object B**: Pick object B from container X
4. **Select container Y**: Pick different container Y (≠ hot, ≠ X)
5. **Select object C**: Pick object C from container Y
6. **Evaluate cycle**: Test the 3-move cycle:
   - Move A: hot → X
   - Move B: X → Y
   - Move C: Y → hot
7. **Repeat**: Try all combinations
8. **Apply best**: Apply the 3-move cycle improving objective most

### Visual Example

```
Before:                          After (if triple loop applied):
┌─────────────┐                 ┌─────────────┐
│ Hot         │  ─────(1)────>  │ Hot         │
│ Container   │                 │ Container   │
│  • objA ──┐ │  <────(3)─────  │  • objC  ←─┐│
│  • obj2   │ │                 │  • obj2    ││
└───────────┼─┘                 └────────────┼┘
            │                                │
┌───────────┼─┐                 ┌────────────┼┐
│ Container X│ │                 │ Container X││
│  • objB ──┼─┘  ─────(2)────>  │  • objA ←──┘│
│  • objY   │                   │  • objY     │
└───────────┘                   └─────────────┘
┌─────────────┐                 ┌─────────────┐
│ Container Y │                 │ Container Y │
│  • objC  ───┘                 │  • objB  ←──┘
│  • objZ     │                 │  • objZ     │
└─────────────┘                 └─────────────┘

Cycle: A (Hot→X), B (X→Y), C (Y→Hot)
```

### Why TripleLoop Helps

**Problem**: Swap and Chain moves can't fix this:
- Swapping any two objects makes things worse
- 2-move chains don't create right configuration

**Solution**: 3-object cycle can rearrange in ways that 2-object moves cannot
- Explores neighborhood unreachable by simpler moves
- Can escape "deep" local optima

## Complexity

**Moves evaluated per iteration**: O(N³ × C)
- Simplified to O(N³) when containers have similar sizes

Where:
- N = number of objects in hot container
- C = number of containers

**Example - Small problem**:
- Hot container: 100 objects
- System: 50 containers
- Moves evaluated ≈ 100³ × 50 = **50 million moves** per iteration

**Example - Medium problem** (impractical):
- Hot container: 1,000 objects
- System: 100 containers
- Moves evaluated ≈ 1,000³ × 100 = **100 billion moves** per iteration

**Warning**: This is **extremely expensive**. Only use for small problems.

## Usage Patterns

### Last Resort for Stuck Solver

Use after simpler moves fail:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/triple_loop_examples.py start=last_resort_start end=last_resort_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/triple_loop_examples.cpp start=last_resort_start end=last_resort_end
```

</TabItem>
</Tabs>

### Small Problem Only

Only for problems known to be small:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/triple_loop_examples.py start=small_problem_start end=small_problem_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/triple_loop_examples.cpp start=small_problem_start end=small_problem_end
```

</TabItem>
</Tabs>

### With Strict Time Limits

Prevent TripleLoop from running too long:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/triple_loop_examples.py start=time_limits_start end=time_limits_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/triple_loop_examples.cpp start=time_limits_start end=time_limits_end
```

</TabItem>
</Tabs>

### Offline Optimization

Taking time for best possible solution:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/triple_loop_examples.py start=offline_start end=offline_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/triple_loop_examples.cpp start=offline_start end=offline_end
```

</TabItem>
</Tabs>

## Performance Characteristics

### Scalability

| Objects | Containers | Time per Iteration | Practical? |
|---------|------------|-------------------|------------|
| 10 | 10 | &lt;1s | ✓ Yes |
| 50 | 50 | 10-60s | △ Marginal |
| 100 | 100 | 5-30 minutes | ✗ No |
| &gt;100 | &gt;100 | Hours+ | ✗ Absolutely not |

**Hard limit**: Do not use TripleLoop with &gt;100 objects per container.

### When Does It Help?

TripleLoop helps when:
- **Graph partitioning** problems (need 3-way swaps)
- **Highly constrained** problems (few feasible moves)
- **Complex dependencies** between objects
- Stuck at local optimum, no simpler moves improve

TripleLoop does NOT help when:
- Problem is just large (won't finish)
- Initial assignment is terrible (too many broken things)
- Constraints are impossible (no amount of searching helps)

## Comparison with Alternatives

| Move Type | Complexity | Max Problem Size | Escape Power | Use Case |
|-----------|------------|------------------|--------------|----------|
| [Single](../basic/single) | O(N × C) | 100K+ objects | Low | Default choice |
| [Swap](../swap/) | O(N²) | 10K objects | Medium | Capacity-constrained |
| [SingleEndChain](../chain/single-end-chain) | O(N² × C) | 1K objects | Medium-High | 2-move sequences |
| **TripleLoop** | O(N³) | 100 objects | Highest | Deep local optima |
| [KLSearch](kl-search) | Expensive | 1K objects | Very High | Graph partitioning |

**Recommendation**: Try in order: Single → Swap → Chain → TripleLoop (last resort)

## Troubleshooting

### Problem: TripleLoop taking forever

**Diagnosis**: O(N³) too expensive for problem size

**Solutions**:
- **Remove TripleLoop** if &gt;100 objects per container
- Set strict `solveTime` limit
- Only use for final refinement stage with small time budget
- Check if problem is actually small enough

### Problem: TripleLoop not helping

**Diagnosis**: Not the right move type for this problem

**Solutions**:
- Problem may not need 3-object cycles
- Try [KLSearch](kl-search) instead (graph partitioning)
- Check if constraints allow ANY improvement
- Improve initial assignment instead
- Accept current local optimum

### Problem: Running out of memory

**Diagnosis**: Too many move evaluations

**Solutions**:
- Problem is too large for TripleLoop
- Use simpler move types
- Reduce problem size (fewer objects/containers)
- This is a sign TripleLoop is wrong choice

### Problem: Solution not improving despite time

**Diagnosis**: Exploring many moves but none improve

**Solutions**:
- May already be at good local optimum
- Check if constraints are blocking all moves
- Try multiple runs with different initial assignments
- Consider that current solution may be near-optimal

## When to Use TripleLoop

**DO use when**:
- Problem is small (&lt;100 objects per container)
- Stuck at local optimum with all simpler moves
- Solution quality is critical
- Have time budget (offline optimization)
- Graph partitioning or complex rearrangement problem

**DO NOT use when**:
- Problem is medium/large (&gt;100 objects)
- Production environment with time constraints
- Simpler moves are still finding improvements
- Just starting optimization (try simple moves first)

## Related Move Types

**Simpler alternatives (try first)**:
- [Single](../basic/single) - O(N × C)
- [Swap](../swap/) - O(N²)
- [SingleEndChain](../chain/single-end-chain) - O(N² × C)

**Similar complexity**:
- [KLSearch](kl-search) - Also expensive, for graph partitioning

**When to use each**:
1. **First**: Single, Swap
2. **If stuck**: SingleEndChain
3. **If still stuck and small**: TripleLoop
4. **For graph problems**: KLSearch

## Source Code

- Thrift definition: [`interface/thrift/SolverSpecs.thrift:559`](https://github.com/facebookincubator/rebalancer/blob/main/interface/thrift/SolverSpecs.thrift#L559)
- Implementation: [`solver/moves/TripleLoopMoveType.h`](https://github.com/facebookincubator/rebalancer/blob/main/solver/moves/TripleLoopMoveType.h)
- Tests: [`solver/moves/tests/`](https://github.com/facebookincubator/rebalancer/tree/main/solver/moves/tests/)

## Next Steps

- Try [SingleEndChain](../chain/single-end-chain) before TripleLoop (less expensive)
- Consider [KLSearch](kl-search) for graph partitioning problems
- Review [Performance Guide](../../solvers/performance) for optimization strategies
- See [Move Types Overview](../) for choosing appropriate move types
