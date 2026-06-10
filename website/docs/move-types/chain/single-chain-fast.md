---
sidebar_position: 2
---

# SingleChainFast

**Move Type**: Chain
**Complexity**: O(1) to O(objects² × containers) with early termination

Like [SingleChain](single-chain) but stops as soon as ANY improving chain is found. Much faster convergence but may miss better moves.

## Overview

`SingleChainFast` evaluates **2-object chain moves** with **early exit**: stops as soon as it finds a chain that improves the objective. Like [SingleChain](single-chain), the hot container is in the **middle** (gives one, receives another), but the search terminates early.

**Use when**:
- Need faster chain moves than [SingleChain](single-chain)
- Speed more important than solution quality
- Hot container capacity-constrained (must receive back)
- [SingleChain](single-chain) too slow

**Avoid when**:
- Solution quality critical (use [SingleChain](single-chain))
- Hot should empty (use [SingleEndChain](single-end-chain))
- Problem is small (overhead not worth it)
- Need deterministic results

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_chain_fast_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_chain_fast_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `partitionNameToExploreFastChainsWithinObjectGroup` | string | No | null | Restrict replacement objects to same partition |
| `specialFastColdContainer` | string | No | null | Fixed destination for hot object |

### Parameter Details

**partitionNameToExploreFastChainsWithinObjectGroup**:
- Restricts which objects can replace the hot object
- Only objects in same partition group will be considered
- Useful for type-based constraints

**specialFastColdContainer**:
- Forces hot object to move to specific container
- Reduces search space significantly
- Useful when destination is known

## How It Works

Given a **hot container** (most broken):

1. **Select hot object**: Pick object from hot container
2. **Select destination**: Pick other container 2 for hot object
3. **Select source**: Pick other container 1 (source of replacement)
4. **Select replacement**: Pick object from other container 1
5. **Evaluate chain**: Test 2-move chain in parallel (multi-threaded)
6. **Early exit**: If chain improves objective → **STOP and apply it**
7. **Continue**: If not, try next combination
8. **Worst case**: If no improving chain found, try all combinations

### Visual Example

```
Hot Container (1000 objects):
  obj1 → Try chain with replObj1 from Container A: improvement! ✓ STOP HERE

SingleChainFast returns after evaluating just 1 chain!

Comparison with SingleChain:
- SingleChainFast:  Tries ~1-100 chains, finds improvement, stops
- SingleChain:      Tries 1,000,000 chains (1000² × 1 containers), picks best
```

### Comparison with SingleChain

| Aspect | SingleChain | SingleChainFast |
|--------|-------------|-----------------|
| **Evaluation** | All combinations | Stop at first improvement |
| **Typical chains evaluated** | Millions | 1-100 |
| **Quality** | Best chain | First improving chain |
| **Speed** | Slow | Much faster |
| **Parallelism** | No | Yes (multi-threaded) |

## Complexity

**Best case**: O(1) - First chain tried improves objective

**Average case**: O(N × C / k) where k = improvement frequency

**Worst case**: O(N² × C) - No improving chains found (same as SingleChain)

Where:
- N = number of objects in hot container
- C = number of other containers
- k = average chains tried before finding improvement

**Example - Medium problem**:
- Hot container: 1,000 objects
- System: 100 containers
- Typical run: Finds improvement in **10-100 chains** (vs 100M for SingleChain)
- **Speedup**: 1,000-10,000x

## Usage Patterns

### Fast Chain Moves

Default usage for speed:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_chain_fast_examples.py start=fast_chains_start end=fast_chains_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_chain_fast_examples.cpp start=fast_chains_start end=fast_chains_end
```

</TabItem>
</Tabs>

### Type-Restricted Fast Chains

Only replace with same type, with early exit:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_chain_fast_examples.py start=restricted_start end=restricted_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_chain_fast_examples.cpp start=restricted_start end=restricted_end
```

</TabItem>
</Tabs>

### Fixed Destination Fast

When you know destination and want speed:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_chain_fast_examples.py start=fixed_dest_start end=fixed_dest_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_chain_fast_examples.cpp start=fixed_dest_start end=fixed_dest_end
```

</TabItem>
</Tabs>

### Multi-Stage Strategy

Fast chains first, then thorough if needed:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_chain_fast_examples.py start=multistage_start end=multistage_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_chain_fast_examples.cpp start=multistage_start end=multistage_end
```

</TabItem>
</Tabs>

## Performance Characteristics

### Speed vs Quality

| Metric | SingleChainFast | SingleChain |
|--------|-----------------|-------------|
| **Speed** | Very Fast | Slow |
| **Chains evaluated** | 1-100 | 100K-100M |
| **Solution quality** | 60-80% optimal | 95-100% optimal |
| **Iteration time** | &lt;1s | 10-60s |
| **Parallelism** | Yes (multi-threaded) | No |

### When Does It Help?

SingleChainFast helps when:
- **Speed critical**: Need fast convergence
- **Hot capacity-constrained**: Must receive object back
- **Frequent small improvements**: Easy to find some improvement quickly
- **Multi-core available**: Can leverage parallelism

SingleChainFast does NOT help when:
- **Quality critical**: Need best chain move
- **Rare improvements**: Takes long to find ANY improving chain
- **Hot should empty**: Use [SingleEndChain](single-end-chain) instead
- **Simple moves work**: Use [Single](../basic/single) or [SingleFast](../basic/single-fast)

## Comparison with Alternatives

| Move Type | Early Exit | Hot Role | Parallelism | Use Case |
|-----------|------------|----------|-------------|----------|
| [Single](../basic/single) | No | Gives only | No | General moves |
| [SingleFast](../basic/single-fast) | Yes (per object) | Gives only | No | Faster single moves |
| [SingleEndChain](single-end-chain) | No | End (gives) | No | 2-object chains (default) |
| [SingleChain](single-chain) | No | Middle (gives+receives) | No | Best chain quality |
| **SingleChainFast** | Yes (per chain) | Middle (gives+receives) | Yes | Fast chains |

**Decision tree**:
1. **Hot should empty?** → [SingleEndChain](single-end-chain)
2. **Hot must balance + need speed?** → **SingleChainFast**
3. **Hot must balance + need quality?** → [SingleChain](single-chain)
4. **Simple moves sufficient?** → [SingleFast](../basic/single-fast)

## Troubleshooting

### Problem: Making many small chain moves but not improving much

**Diagnosis**: Early exit finding local improvements, missing better global chains

**Solutions**:
- Switch to [SingleChain](single-chain) for better chain quality
- Use SingleChainFast for quick wins, then [SingleChain](single-chain) for refinement
- Combine with other move types
- Increase iteration limit to let fast chains make more sequential improvements

### Problem: Not finding any improving chains

**Diagnosis**: Already at local optimum OR improvements are rare

**Solutions**:
- This is expected at local optimum
- Try [SingleEndChain](single-end-chain) (different neighborhood)
- Check if simple moves ([Single](../basic/single)) work
- May need different move types ([Swap](../swap/))

### Problem: Solution quality worse than expected

**Diagnosis**: Early exit too aggressive, missing much better chains

**Solutions**:
- Use [SingleChain](single-chain) for better quality
- Run SingleChainFast multiple times
- Use as warm-start, then refine with [SingleChain](single-chain)
- May need different approach

### Problem: Still too slow

**Diagnosis**: Even with early exit, problem too large

**Solutions**:
- Set `specialFastColdContainer` to reduce search space
- Use `partitionNameToExploreFastChainsWithinObjectGroup` to filter
- Set strict `solveTime` limit
- Problem may be too large for chain moves
- Try simpler move types

## When to Use SingleChainFast

**DO use when**:
- Hot container capacity-constrained (must receive back)
- Need faster chain moves than [SingleChain](single-chain)
- Speed critical (&lt;10s response time needed)
- Have multiple CPU cores (leverage parallelism)
- Problem is medium-large (1K-10K objects)

**DO NOT use when**:
- Solution quality is critical → Use [SingleChain](single-chain)
- Hot should empty → Use [SingleEndChain](single-end-chain)
- Simple moves work → Use [SingleFast](../basic/single-fast)
- Need deterministic results

## Related Move Types

**Chain variants**:
- [SingleEndChain](single-end-chain) - **Default choice** for chains
- [SingleChain](single-chain) - Best quality, slower
- **SingleChainFast** - Fastest, early exit (this)

**Simpler alternatives**:
- [SingleFast](../basic/single-fast) - Faster single moves with early exit
- [Single](../basic/single) - Full single move exploration

**Use together**:
1. **First**: [SingleFast](../basic/single-fast)
2. **If stuck**: [SingleEndChain](single-end-chain) or **SingleChainFast**
3. **For quality**: [SingleChain](single-chain)

## Source Code

- Thrift definition: [`interface/thrift/SolverSpecs.thrift:573`](https://github.com/facebookincubator/rebalancer/blob/main/interface/thrift/SolverSpecs.thrift#L573)
- Implementation: [`solver/moves/SingleChainFastMoveType.h`](https://github.com/facebookincubator/rebalancer/blob/main/solver/moves/SingleChainFastMoveType.h)
- Tests: [`solver/moves/tests/`](https://github.com/facebookincubator/rebalancer/tree/main/solver/moves/tests/)

## Next Steps

- Try [SingleEndChain](single-end-chain) first - better default for chains
- Learn about [SingleChain](single-chain) for best chain quality
- Review [Move Types Overview](../) for choosing move types
- See [SingleFast](../basic/single-fast) for faster single moves
