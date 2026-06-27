---
sidebar_position: 1
---

# Solver Overview

Rebalancer provides multiple solver algorithms, each with different trade-offs between solution quality, speed, and scalability. This guide helps you choose the right solver for your problem.

## Available Solvers

| Solver | Type | Optimality | Speed | Scale | Best For |
|--------|------|------------|-------|-------|----------|
| **Local Search** | Heuristic | Local optimum | Fast | 1M+ objects | Large problems, quick results |
| **Optimal (HiGHS)** | Exact | Global optimum* | Medium | &lt;10K objects | Smaller problems, open source |
| **Optimal (Gurobi)** | Exact | Global optimum* | Fast | &lt;10K objects | Medium problems, best MIP performance |
| **Optimal (XPRESS)** | Exact | Global optimum* | Fast | &lt;10K objects | Medium problems, community license |

\* *Given enough time and within time/memory limits*

## Quick Decision Guide

### Choose Local Search If:
- ✅ You have >1,000 objects
- ✅ You need results in seconds
- ✅ A "good enough" solution is acceptable
- ✅ You're rebalancing frequently (online rebalancing)
- ✅ You have complex goals/constraints

### Choose Optimal Solver If:
- ✅ You have &lt;1,000 objects
- ✅ You need provably optimal solutions
- ✅ You can wait minutes/hours
- ✅ Solution quality is more important than speed
- ✅ You're doing one-time optimization

## Solver Comparison

### Local Search

**How it works**: Starts with current assignment and iteratively makes improving moves (swap objects, move objects, etc.) until no better moves found.

**Pros**:
- Extremely fast (seconds for 1M objects)
- Scales to very large problems
- Memory efficient
- Handles any goal/constraint combination
- Configurable time/move limits

**Cons**:
- No optimality guarantee (finds local optimum)
- Quality depends on initial assignment
- May get stuck in local optima
- Results can vary between runs

**Performance**:
- 100 objects: &lt;1 second
- 1,000 objects: 1-5 seconds
- 10,000 objects: 5-30 seconds
- 100,000 objects: 30-300 seconds
- 1,000,000 objects: 5-30 minutes

### Optimal Solver (MIP-based)

**How it works**: Formulates problem as Mixed Integer Programming (MIP) model and uses commercial/open-source MIP solvers (HiGHS, Gurobi, or XPRESS) to find optimal solution.

**Pros**:
- Provably optimal solutions (within tolerance)
- Guarantees feasibility if solution exists
- Can provide optimality gap (how close to optimal)
- Deterministic results

**Cons**:
- Exponential worst-case complexity
- Doesn't scale to large problems (>10K objects)
- Can be very slow or run out of memory
- Requires external solver (HiGHS/Gurobi/XPRESS)

**Performance** (varies by solver):
- 10 objects: &lt;1 second
- 100 objects: 1-10 seconds
- 1,000 objects: 10-300 seconds
- 10,000 objects: Minutes to hours (may timeout)

## Solver Selection Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/overview_examples.py start=solver_selection_start end=solver_selection_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/overview_examples.cpp start=solver_selection_start end=solver_selection_end
```

</TabItem>
</Tabs>

## When Each Solver Works Best

### Local Search Scenarios

**Production rebalancing**:
- Continuous rebalancing of live systems
- Need quick results for operational decisions
- Large fleets (1000s of servers, 100,000s of tasks)

**Quick iterations**:
- Experimenting with different goals/constraints
- Prototyping solutions
- Answering "what if" questions quickly

**Complex problems**:
- Many competing goals
- Complex constraint interactions

### Optimal Solver Scenarios

**Capacity planning**:
- One-time placement decisions
- Long-term planning where optimality matters
- Small clusters with critical workloads

**Validation**:
- Verify Local Search results
- Understand true optimal solution
- Compare heuristic quality

**Small problems**:
- &lt;10,000 objects and containers
- Simple goal/constraint structure
- Have time to wait for optimal solution

## Next Steps

- **Learn Local Search**: [Local Search Solver Guide](local-search)
- **Learn Optimal Solvers**: [Optimal Solver Guide](optimal)
- **Performance Tuning**: [Performance Guide](performance)
- **Advanced Strategies**: [Solver Strategies](strategies)

## Related Documentation

- [Getting Started: Build Your First Model](../getting-started/first-model) - Basic solver usage
