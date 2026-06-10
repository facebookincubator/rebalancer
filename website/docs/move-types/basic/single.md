---
sidebar_position: 1
---

# Single

**Move Type**: Basic
**Complexity**: O(objects × containers)

Move one object at a time to any destination container. This is the most fundamental move type and should be included in almost every Local Search configuration.

## Overview

`Single` evaluates moving each object from the hot container to every possible destination container. It explores all single-object moves in parallel using multi-threading.

**Use when**:
- Always - this is the foundation of Local Search
- Problems with soft constraints or unconstrained
- Initial solver configuration

**Avoid when**:
- Need faster convergence (use `SingleFast` instead)
- Single-threaded environment required (use `SingleGreedy`)

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| (none) | - | - | - | Single move type has no configuration parameters |

## How It Works

Given a **hot container** (the container contributing most to the objective):

1. **Select object**: Pick one object from the hot container (the "hot object")
2. **Select destination**: Pick a different container (the "destination container")
3. **Evaluate**: Test moving the hot object to the destination container
4. **Repeat**: Try all combinations of (hot object, destination container)
5. **Apply best**: Apply the move that improves the objective most

All moves are evaluated **in parallel** to benefit from multi-threading.

### Visual Example

```
Before:                          After (if move applied):
┌─────────────┐                 ┌─────────────┐
│ Hot         │                 │ Hot         │
│ Container   │                 │ Container   │
│  • obj1     │  ──move obj1──> │  • obj2     │
│  • obj2     │                 │  • obj3     │
│  • obj3     │                 └─────────────┘
└─────────────┘                 ┌─────────────┐
┌─────────────┐                 │ Dest        │
│ Dest        │                 │ Container   │
│ Container   │                 │  • obj4     │
│  • obj4     │                 │  • obj1  ←  │
└─────────────┘                 └─────────────┘
```

## Complexity

**Moves evaluated per iteration**: O(N × C)

Where:
- N = number of objects in hot container
- C = number of destination containers

**Example**:
- Hot container has 100 objects
- System has 50 containers
- Moves evaluated = 100 × 50 = 5,000 moves

**Parallelization**: All moves evaluated in parallel using available CPU cores

## Usage Patterns

### Basic Configuration

Most common usage - include Single in move type list:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_examples.py start=basic_configuration_start end=basic_configuration_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_examples.cpp start=basic_configuration_start end=basic_configuration_end
```

</TabItem>
</Tabs>

### Combined with Other Move Types

Typical pattern - Single as foundation, other types for special cases:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_examples.py start=combined_with_others_start end=combined_with_others_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_examples.cpp start=combined_with_others_start end=combined_with_others_end
```

</TabItem>
</Tabs>

### Load Balancing

Use Single for straightforward load balancing:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/single_examples.py start=load_balancing_start end=load_balancing_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/single_examples.cpp start=load_balancing_start end=load_balancing_end
```

</TabItem>
</Tabs>

## Performance Characteristics

### Scalability

| Problem Size | Objects/Container | Containers | Typical Time per Iteration |
|--------------|-------------------|------------|---------------------------|
| Small | 10 | 10 | &lt;1ms |
| Medium | 100 | 100 | 10-50ms |
| Large | 1,000 | 1,000 | 0.5-2s |
| Very Large | 10,000 | 10,000 | 10-60s |

### Multi-threading

Single move type benefits significantly from multi-threading:
- All object-destination pairs evaluated in parallel
- Scales well up to 8-16 cores
- CPU-bound (evaluation is fast, parallelization helps)

### Memory Usage

- Minimal memory overhead beyond problem state
- Each move evaluation is independent (no temporary state)
- Safe for large problems (100K+ objects)

## Comparison with Variants

| Move Type | Speed | Thoroughness | Use Case |
|-----------|-------|--------------|----------|
| **Single** | Medium | Complete | Default choice, thorough search |
| [SingleFast](single-fast) | Fast | Partial | Faster convergence, may miss optimal |
| [SingleGreedy](single-greedy) | Fastest | Greedy | Single-threaded, speed critical |
| [SingleRandomBatches](single-random-batches) | Fast | Sampled | Parallel batching for speed |

**Recommendation**: Start with `Single`. Only switch to variants if:
- Speed is critical → `SingleFast` or `SingleGreedy`
- Problem is extremely large → `SingleRandomBatches` with sampling

## Troubleshooting

### Problem: Single moves too slow

**Diagnosis**: Large problem (many objects or containers)

**Solutions**:
- Switch to [SingleFast](single-fast) for early termination
- Use [SingleRandomStratified](../basic/single-random-stratified) with sampling
- Reduce problem size by filtering containers/objects
- Check if moves are being evaluated inefficiently

### Problem: Not finding good solutions

**Diagnosis**: Single moves alone may not escape local optima

**Solutions**:
- Add [Swap](../swap/) for capacity-constrained problems
- Add [SingleEndChain](../chain/single-end-chain) for 2-move sequences
- Try multiple solver runs with different random seeds
- Improve initial assignment quality

### Problem: Getting stuck in local optimum quickly

**Diagnosis**: Simple moves can't improve objective further

**Solutions**:
- Add more powerful move types (Swap, Chain, TripleLoop)
- Check if constraints are blocking improvements
- Verify initial assignment isn't extremely broken
- Review objective function for issues

## Related Move Types

**Variants**:
- [SingleFast](single-fast) - Faster with early termination
- [SingleGreedy](single-greedy) - Greedy single-threaded variant
- [SingleRandomBatches](single-random-batches) - Parallel batched variant
- [SingleRandomStratified](../basic/single-random-stratified) - Sampled destinations
- [SingleColdestStratified](../basic/single-coldest-stratified) - Target cold containers

**Complementary**:
- [Swap](../swap/) - For capacity-constrained problems
- [SingleEndChain](../chain/single-end-chain) - For 2-move sequences
- [FixedDest](../fixed/fixed-dest) - When destination is predetermined

## Source Code

- Thrift definition: [`interface/thrift/SolverSpecs.thrift:511`](https://github.com/facebookincubator/rebalancer/blob/main/interface/thrift/SolverSpecs.thrift#L511)
- Implementation: [`solver/moves/SingleMoveType.h`](https://github.com/facebookincubator/rebalancer/blob/main/solver/moves/SingleMoveType.h)
- Tests: [`solver/moves/tests/SingleMoveTypeTest.cpp`](https://github.com/facebookincubator/rebalancer/tree/main/solver/moves/tests/)

## Next Steps

- Learn about [SingleFast](single-fast) for faster convergence
- See [Swap](../swap/) for capacity-constrained problems
- Review [Move Types Overview](../) for choosing move types
- Check [Local Search Solver](../../solvers/local-search) for using move types
