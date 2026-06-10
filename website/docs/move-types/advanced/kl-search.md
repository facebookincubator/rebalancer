---
sidebar_position: 1
---

# KLSearch

**Move Type**: Advanced
**Complexity**: O(n²) where n = objects in hot + cold containers
**Primary Use**: Escape local optima through sequential move exploration

Advanced move type based on the **Kernighan-Lin algorithm** that explores sequences of moves between two containers, accepting temporarily worsening moves to escape local optima.

## Overview

`KLSearch` (also known as `KL_SEARCH`) implements an algorithm inspired by the classic [Kernighan-Lin graph partitioning algorithm](https://en.wikipedia.org/wiki/Kernighan%E2%80%93Lin_algorithm). For a given hot (overloaded) container and cold container pair, it:

1. Sequentially picks moves in either direction with **best objective change**
2. **Accepts moves regardless of improvement** - can temporarily worsen objective
3. Tracks the sequence of all moves made
4. Returns the **prefix of moves** that achieves the **minimum objective**

This allows the solver to escape local optima by accepting temporarily worsening moves.

**Use when**:
- Stuck in local optima
- Simple move types plateaued
- Need sophisticated balancing between two containers
- Willing to invest computation for quality

**Avoid when**:
- Simple moves are working well
- Tight time constraints
- Very large containers (high complexity)
- Don't have identified hot/cold container pairs

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/kl_search_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/kl_search_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| *(none)* | - | - | - | No configuration parameters |

### Parameter Details

KLSearch has **no configurable parameters**. Simply include it in your move type list to enable Kernighan-Lin search.

The move type automatically:
- Identifies hot (worst) containers
- Pairs them with cold containers
- Explores move sequences between pairs
- Returns best sequence found

## How It Works

For each **hot container** paired with each **cold container**:

1. **Initialize**: Start with empty move sequence, no objects tried
2. **Iterate until all objects tried**:
   - **Hot → Cold**: Try moving each untried object from hot to cold
   - **Cold → Hot**: Try moving each untried object from cold to hot
   - **Pick best**: Select move with best objective change (even if negative)
   - **Add to sequence**: Append move to sequence, mark object as tried
3. **Find minimum**: Scan all prefixes of move sequence
4. **Return best prefix**: Return sequence up to point with minimum objective

### Visual Example

```
Initial State:
  Hot Container (overloaded):   [obj1:10, obj2:8, obj3:7]  Load: 25
  Cold Container (underloaded): [obj4:3, obj5:2]           Load: 5

Iteration 1:
  Try all hot→cold and cold→hot moves
  Best: obj1 hot→cold (reduces hot from 25 to 15)
  Moves: [obj1: hot→cold]
  Objective at step 1: 100

Iteration 2:
  Try remaining objects
  Best: obj4 cold→hot (worsens objective but best remaining)
  Moves: [obj1: hot→cold, obj4: cold→hot]
  Objective at step 2: 105 (worsened!)

Iteration 3:
  Try remaining objects
  Best: obj2 hot→cold (improves again)
  Moves: [obj1: hot→cold, obj4: cold→hot, obj2: hot→cold]
  Objective at step 3: 95 (best so far!)

...continue until all objects tried...

Final: Return prefix with minimum objective (step 3 in this example)
```

## Complexity

**Per container pair**: O(n²)

Where:
- n = total objects in hot container + cold container

**Worst case**:
- Hot container: 100 objects
- Cold container: 100 objects
- Total objects: 200
- **Iterations**: 200 (try each object once)
- **Evaluations per iteration**: ~200 (try remaining objects)
- **Total evaluations**: ~40,000

**Per iteration in solver**:
- Pairs explored: O(number of containers)
- Per pair: O(n²)

## Usage Patterns

### Basic Usage

Enable KL Search for advanced optimization:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/kl_search_examples.py start=basic_usage_start end=basic_usage_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/kl_search_examples.cpp start=basic_usage_start end=basic_usage_end
```

</TabItem>
</Tabs>

### Combined with Other Move Types

Use after simpler move types to refine solution:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/kl_search_examples.py start=combined_start end=combined_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/kl_search_examples.cpp start=combined_start end=combined_end
```

</TabItem>
</Tabs>

### For Balancing

Escape local optima when balancing load:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/kl_search_examples.py start=balancing_start end=balancing_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/kl_search_examples.cpp start=balancing_start end=balancing_end
```

</TabItem>
</Tabs>

## Performance Characteristics

### When Does It Help?

KLSearch helps when:
- **Local optima**: Simpler move types can't improve further
- **Complex dependencies**: Objects interact in non-obvious ways
- **Sophisticated balancing**: Need to carefully balance two containers
- **Quality over speed**: Willing to spend time for better solution
- **Plateau detection**: No improving single/swap moves found

KLSearch does NOT help when:
- **Simple moves working**: Still finding improvements with basic moves
- **Time constrained**: Can't afford O(n²) complexity
- **Very large containers**: 1000+ objects per container too slow
- **Initial placement**: More efficient to use greedy approaches first
- **No identified pairs**: Don't have clear hot/cold container pairs

### Complexity Trade-offs

| Container Size | Objects | Iterations | Evaluations | Time @100K/s |
|----------------|---------|------------|-------------|--------------|
| Small | 20 + 20 | 40 | ~1,600 | &lt;0.1s |
| Medium | 50 + 50 | 100 | ~10,000 | 0.1s |
| Large | 100 + 100 | 200 | ~40,000 | 0.4s |
| Very Large | 500 + 500 | 1000 | ~1,000,000 | 10s |

## Comparison with Alternatives

| Move Type | Accepts Worsening | Complexity | Use Case |
|-----------|-------------------|------------|----------|
| [Single](../basic/single) | No | O(n) | Initial optimization |
| [Swap](../swap/) | No | O(n²) | Direct swaps |
| [TripleLoop](triple-loop) | No | O(n³) | Complex 3-way swaps |
| **KLSearch** | **Yes** | O(n²) | **Escape local optima** |

## Troubleshooting

### Problem: Too slow

**Diagnosis**: Containers have many objects, causing O(n²) to be expensive

**Solutions**:
- Only use KL Search after simpler moves plateau
- Use time limits to prevent excessive computation
- Consider using only on smaller container pairs
- May not be suitable for very large problems

### Problem: Not finding improvements

**Diagnosis**: May not be a local optimum, or global optimum reached

**Solutions**:
- Verify simpler move types have plateaued
- Check if objective is already optimal
- Review constraints - may be blocking all moves
- Try with different container pairs

### Problem: Getting worse results

**Diagnosis**: Accepting too many worsening moves

**Solutions**:
- KL Search should improve overall, not worsen
- Check if base solution is good quality
- May need different objective weights
- Review constraints

## When to Use KLSearch

**DO use when**:
- Stuck in local optima
- Simple move types stopped improving
- Need sophisticated two-container balancing
- Quality more important than speed
- Have reasonable-sized containers (&lt;500 objects)

**DO NOT use when**:
- Simple moves still improving
- Very tight time budgets
- Very large containers (1000+ objects)
- Initial placement phase
- Don't need to escape local optima

## Related Move Types

**Alternatives for escaping local optima**:
- [TripleLoop](triple-loop) - More complex but different trade-offs
- [Swap](../swap/) - Simpler pairwise swaps

**Complementary move types**:
- [Single](../basic/single) - Use first for initial optimization
- [SingleGreedy](../basic/single-greedy) - Fast greedy approach first

## Source Code

- Thrift definition: [`interface/thrift/SolverSpecs.thrift:561`](https://github.com/facebookincubator/rebalancer/blob/main/interface/thrift/SolverSpecs.thrift#L561)
- Implementation: [`solver/moves/KLSearchMoveType.h`](https://github.com/facebookincubator/rebalancer/blob/main/solver/moves/KLSearchMoveType.h)
- Algorithm reference: [Kernighan-Lin Algorithm (Wikipedia)](https://en.wikipedia.org/wiki/Kernighan%E2%80%93Lin_algorithm)

## Next Steps

- Try [TripleLoop](triple-loop) for even more complex move sequences
- Review [Move Types Overview](../) for choosing move types
- Learn about [Local Search](../../solvers/local-search) solver configuration

## Notes

⚠️ **Complexity Warning**: KLSearch has O(n²) complexity per container pair. Use it strategically after simpler move types have plateaued, not as a first-resort optimization.

💡 **Kernighan-Lin Insight**: The key insight is accepting temporarily worsening moves. This allows escaping local optima that greedy approaches get stuck in.
