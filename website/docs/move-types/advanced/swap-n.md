---
sidebar_position: 3
---

# SwapN

**Move Type**: Advanced
**Complexity**: O(N! × I) where N = concurrent objects, I = iterations
**Primary Use**: Satisfy exact-limit constraints by swapping N objects simultaneously

Swap **N objects simultaneously** to satisfy complex exact-limit constraints that simpler move types cannot handle.

## Overview

`SwapN` (also known as `SWAP_N`) is a highly specialized move type designed for problems with **exact-limit constraints** where you need to move N objects together to maintain balance. It was originally created for the Capacity Request Portal (CRP) problem.

Instead of moving objects one at a time, SwapN:
1. Selects N objects from a source set
2. Tries placing them in N different destination containers
3. Swaps them with N objects already in those containers
4. Evaluates if this simultaneous N-way swap improves the objective

This allows satisfying constraints like "exactly 3 requests per pod" that are impossible with single-object moves.

**Use when**:
- Have exact-limit constraints (MIN + MAX with same value)
- Simple moves get stuck due to constraints
- Need to move N objects simultaneously
- Know which objects and destinations to consider
- Very specific capacity allocation problems

**Avoid when**:
- No exact-limit constraints
- Simple moves work fine
- Don't have specific source objects
- Generic optimization (very expensive)
- N is large (&gt;5)

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/swap_n_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/swap_n_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `swapNConcurrentObjects` | int | No | 1 | Number of objects to swap simultaneously |
| `swapNSourceObjects` | list&lt;string&gt; | **Yes** | null | Source objects to consider |
| `swapNDestinationScope` | list&lt;list&lt;string&gt;&gt; | **Yes** | null | Grouped destination containers |
| `swapNIterations` | int | No | 1000000 | Max random iterations to try |

### Parameter Details

**swapNConcurrentObjects**:
- Number of objects to swap simultaneously (N)
- Example: 3 means swap 3 objects at once
- Higher N = exponentially more expensive
- Typically 2-5

**swapNSourceObjects**:
- List of object names to consider as sources
- Only these objects will be moved
- Example: ["request0", "request1", "request2", "request3"]
- Typically unassigned or pending objects

**swapNDestinationScope**:
- Nested list of container groups
- Outer list: groups (e.g., pods)
- Inner lists: containers in each group (e.g., racks per pod)
- Example: [["rack0", "rack1"], ["rack2", "rack3"]]
- Each group represents mutually exclusive destinations

**swapNIterations**:
- Maximum random sampling iterations
- Algorithm tries random combinations up to this limit
- Higher = better quality but slower
- Default 1M is usually sufficient

## How It Works

For each iteration (up to `swapNIterations`):

1. **Random selection**: Pick N random objects from `swapNSourceObjects`
2. **Random destinations**: Pick N random destination containers from `swapNDestinationScope` (one from each group if possible)
3. **Generate swap**: Create N-way swap moving selected objects to destinations
4. **Evaluate**: Test if this swap improves objective
5. **Track best**: Keep the best swap found so far
6. **Repeat**: Try different random combinations

### Visual Example

```
Problem: Allocate 6 requests to 2 pods, exactly 3 requests per pod

Source: unallocated = [req0, req1, req2, req3, req4, req5]

Destinations:
  Pod0: [rack0, rack1, rack2, ...]  (10 racks)
  Pod1: [rack10, rack11, rack12, ...]  (10 racks)

swapNConcurrentObjects = 3

Iteration 1:
  Select: [req0, req2, req4]
  Destinations: [rack1 (pod0), rack3 (pod0), rack11 (pod1)]  ✗ Invalid (2 in pod0, 1 in pod1)

Iteration 2:
  Select: [req1, req3, req5]
  Destinations: [rack0 (pod0), rack2 (pod0), rack4 (pod0)]  ✗ Invalid (all in pod0)

Iteration 3:
  Select: [req0, req1, req2]
  Destinations: [rack1 (pod0), rack11 (pod1), rack12 (pod1)]  ✗ Invalid (1 in pod0, 2 in pod1)

Iteration 100:
  Select: [req0, req2, req4]
  Destinations: [rack0 (pod0), rack2 (pod0), rack4 (pod0)]  ✓ Valid! (3 in pod0)

Then continue to place remaining 3 in pod1...
```

## Complexity

**Per iteration**: O(N!)

Where:
- N = `swapNConcurrentObjects`

**Total**: O(N! × I)

Where:
- I = iterations attempted (up to `swapNIterations`)

**Examples**:
- N=2, I=1000: ~2,000 combinations
- N=3, I=1000: ~6,000 combinations
- N=4, I=1000: ~24,000 combinations
- N=5, I=10000: ~1,200,000 combinations

⚠️ **Factorial growth**: N=6 is 720x more expensive than N=2!

## Usage Patterns

### Exact-Limit Allocation

Allocate requests with exact pod limits:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/swap_n_examples.py start=exact_limit_start end=exact_limit_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/swap_n_examples.cpp start=exact_limit_start end=exact_limit_end
```

</TabItem>
</Tabs>

### Capacity Request Portal

Original use case - allocate N hosts per request:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/swap_n_examples.py start=crp_start end=crp_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/swap_n_examples.cpp start=crp_start end=crp_end
```

</TabItem>
</Tabs>

### Limited Iterations

Control sampling for performance:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/swap_n_examples.py start=limited_iterations_start end=limited_iterations_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/swap_n_examples.cpp start=limited_iterations_start end=limited_iterations_end
```

</TabItem>
</Tabs>

## Performance Characteristics

### When Does It Help?

SwapN helps when:
- **Exact-limit constraints**: Have MIN + MAX with same value
- **Stuck simple moves**: Single/Swap move types can't make progress
- **Known sources**: Have specific objects to allocate
- **Small N**: Need to move 2-5 objects simultaneously
- **Specific problem structure**: Like CRP problem

SwapN does NOT help when:
- **No exact limits**: Using ranges (MIN ≠ MAX)
- **Simple moves work**: Other move types making progress
- **Large N**: N &gt; 5 (factorial explosion)
- **No source specification**: Don't know which objects
- **General optimization**: Too specialized and expensive

### Complexity Explosion

| N | Factorial | Iterations | Total Ops | Practical? |
|---|-----------|------------|-----------|------------|
| 2 | 2 | 1,000 | 2K | ✓ Fast |
| 3 | 6 | 1,000 | 6K | ✓ Good |
| 4 | 24 | 1,000 | 24K | ✓ OK |
| 5 | 120 | 10,000 | 1.2M | ⚠ Slow |
| 6 | 720 | 10,000 | 7.2M | ✗ Very slow |
| 10 | 3,628,800 | - | - | ✗ Impractical |

## Comparison with Alternatives

| Move Type | Simultaneous | Constraint Type | Use Case |
|-----------|--------------|-----------------|----------|
| [Single](../basic/single) | 1 object | General | Standard moves |
| [Swap](../swap/) | 2 objects | Pairwise | Object swaps |
| **SwapN** | **N objects** | **Exact-limit** | **CRP-like problems** |

## Troubleshooting

### Problem: No improving moves found

**Diagnosis**: Random sampling not finding valid combinations

**Solutions**:
- Increase `swapNIterations` (try 10M)
- Check if problem is actually solvable
- Verify source objects and destinations are correct
- May need different N value

### Problem: Too slow

**Diagnosis**: N too large or too many iterations

**Solutions**:
- Reduce `swapNConcurrentObjects` if possible
- Reduce `swapNIterations` (try 1K or 10K)
- Check if simpler move types can work
- Consider if problem structure allows smaller N

### Problem: Constraint violations

**Diagnosis**: Swaps violate constraints

**Solutions**:
- Verify exact-limit constraints are set correctly
- Check destination scope grouping is correct
- Review capacity constraints
- Ensure source objects are valid

### Problem: Wrong destinations

**Diagnosis**: `swapNDestinationScope` structure incorrect

**Solutions**:
- Verify outer list groups destinations correctly
- Check inner lists contain right containers
- Example: [["pod0_racks"], ["pod1_racks"]]
- Each inner list should represent mutually exclusive group

## When to Use SwapN

**DO use when**:
- Have exact-limit constraints (MIN = MAX)
- Simple moves stuck
- Know source objects
- N is small (2-5)
- Have CRP-like problem structure

**DO NOT use when**:
- Using constraint ranges (MIN ≠ MAX)
- Simple moves working
- N is large (&gt;5)
- Generic optimization needed
- Don't have specific sources/destinations

## Related Move Types

**Alternatives**:
- [Single](../basic/single) - For general moves
- [Swap](../swap/) - For 2-object swaps
- [KLSearch](kl-search) - For escaping local optima differently

**When to try alternatives first**:
- Always try Single and Swap first
- Only use SwapN when stuck on exact-limit constraints

## Source Code

- Thrift definition: [`interface/thrift/SolverSpecs.thrift:522`](https://github.com/facebookincubator/rebalancer/blob/main/interface/thrift/SolverSpecs.thrift#L522)
- Implementation: [`solver/moves/SwapNMoveType.h`](https://github.com/facebookincubator/rebalancer/blob/main/solver/moves/SwapNMoveType.h)
- Tests: [`interface/tests/SwapNMoveTypeTest.cpp`](https://github.com/facebookincubator/rebalancer/blob/main/interface/tests/SwapNMoveTypeTest.cpp)

## Next Steps

- Try [KLSearch](kl-search) for different local optima escape
- Review [Move Types Overview](../) for choosing move types
- Learn about exact-limit constraints in capacity specs

## Notes

⚠️ **Highly Specialized**: SwapN is designed for very specific problems with exact-limit constraints. It's expensive and should only be used when simpler move types fail.

⚠️ **Factorial Complexity**: Keep N ≤ 5. N=6 is 720x slower than N=2. N=10 is completely impractical.

💡 **Original Use Case**: Created for Capacity Request Portal (CRP) problem requiring "exactly N hosts per pod" allocation.
