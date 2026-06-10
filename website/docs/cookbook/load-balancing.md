---
sidebar_position: 1
---

# Load Balancing Tasks Across Servers

This is one of the simplest use cases: distributing tasks evenly across servers to balance load.

## Problem Description

A microservice has 100 worker processes that need to run on 10 physical hosts. Each worker has different CPU and memory requirements. The current placement is imbalanced - some hosts are at 90% CPU while others are at 20%. You want to redistribute workers to balance load.

## Problem Modeling

### Rebalancer Terms

- **Objects**: The 100 worker tasks
- **Containers**: The 10 hosts
- **Dimensions**:
  - `cpu_usage` (object dimension): CPU used by each task
  - `memory_usage` (object dimension): Memory used by each task
- **Scope**: `host` (compare and balance across all hosts)
- **Goal**: Balance both CPU and memory across hosts

## Complete Solution

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/load_balancing/basic_load_balancing.py start=solution_start end=solution_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/load_balancing/basic_load_balancing.cpp start=solution_start end=solution_end
```

</TabItem>
</Tabs>

## Explanation

### Why These Choices?

1. **BalanceSpec**: Well-suited for load balancing - minimizes variance across hosts
2. **LEGACY formula**: Combines max and squares for balanced distribution
3. **fixAverageToInitial=True**: Maintains total load (doesn't require adding capacity)
4. **Equal weights (1.0)**: CPU and memory equally important
5. **LocalSearchSolver**: Scales well to 100+ objects, finds good solutions quickly

### What Happens?

1. Solver calculates current CPU/memory distribution
2. Identifies imbalanced hosts (some overloaded, some empty)
3. Iteratively moves tasks to balance load
4. Stops when no improving moves found or time limit reached

## Variations

### Add Capacity Constraints

Ensure hosts don't exceed their capacity:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/load_balancing/with_capacity_constraints.py start=variation_start end=variation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/load_balancing/with_capacity_constraints.cpp start=variation_start end=variation_end
```

</TabItem>
</Tabs>

### Minimize Movement

Prefer solutions that move fewer tasks:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/load_balancing/with_minimize_movement.py start=variation_start end=variation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/load_balancing/with_minimize_movement.cpp start=variation_start end=variation_end
```

</TabItem>
</Tabs>

### Rack-Aware Balancing

Balance at both host and rack levels:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/load_balancing/rack_aware_balancing.py start=variation_start end=variation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/load_balancing/rack_aware_balancing.cpp start=variation_start end=variation_end
```

</TabItem>
</Tabs>

### Use Optimal Solver

For smaller problems (&lt;50 objects), get provably optimal solution:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/load_balancing/with_optimal_solver.py start=variation_start end=variation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/load_balancing/with_optimal_solver.cpp start=variation_start end=variation_end
```

</TabItem>
</Tabs>

## Performance Notes

- **LocalSearchSolver**: Handles 1000+ objects easily, &lt;10s solve time
- **OptimalSolver**: Best for &lt;100 objects, may take minutes for larger problems
- **Memory**: Each object requires ~100 bytes, containers ~50 bytes

## Next Steps

- Add disaster recovery constraints: [Disaster Recovery Placement](disaster-recovery)
- Handle multi-tenant scenarios: [Multi-Tenant Allocation](multi-tenant)
- Minimize disruption: [MinimizeMovementSpec](../reference/movement/minimize-movement)

## Related Goals and Constraints

- [BalanceSpec](../reference/balance-optimize/balance) - Balance resources across scope items
- [CapacitySpec](../reference/capacity) - Limit resource usage
- [MinimizeMovementSpec](../reference/movement/minimize-movement) - Reduce number of moves
