---
sidebar_position: 12
---

# MinimizeContainersSpec

**Type**: Goal only

Minimize the number of containers used (bin packing / consolidation).

## Overview

`MinimizeContainersSpec` reduces the number of containers that have objects assigned to them. This is the core goal for bin packing, server consolidation, and cost reduction problems where you want to pack objects onto as few containers as possible.

**Use this when**: Consolidating VMs onto fewer hosts, packing shards onto fewer servers, or freeing up resources for decommissioning.

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_containers_spec_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_containers_spec_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Unique identifier for this goal |
| `scope` | string | Yes | - | Scope level (e.g., `"host"`, `"rack"`) |
| `dimension` | string | Yes | - | Object dimension to count (typically a count=1.0 dimension) |
| `formula` | MinimizeContainersFormula | No | MINIMIZE_OCCUPIED_CONTAINERS | Which formula to use (see below) |
| `containerCosts` | map&lt;string, double&gt; | No | empty | Per-container costs (prefer freeing expensive containers) |
| `maxFreeLimit` | int | No | none | Maximum number of containers to free (safety limit) |

### MinimizeContainersFormula Enum

| Value | Description | Use Case |
|-------|-------------|----------|
| `MINIMIZE_OCCUPIED_CONTAINERS` | **Count non-empty containers** | Standard bin packing (default) |
| `MINIMIZE_TOTAL_UTILIZATION` | **Sum of all utilization** | Minimize total load across all containers |

## How It Works

### MINIMIZE_OCCUPIED_CONTAINERS (Default)

Counts number of containers with at least one object:

```
cost = number of containers with objects > 0
```

**Example**: 10 hosts, 5 with VMs, 5 empty
- Cost = 5 (number of non-empty hosts)
- Goal: Pack VMs onto fewer hosts to reduce cost

### MINIMIZE_TOTAL_UTILIZATION

Sums utilization across all containers:

```
cost = sum over all containers: (dimension_sum / capacity)
```

**Example**: 3 hosts with [50%, 30%, 0%] utilization
- Cost = 0.5 + 0.3 + 0 = 0.8
- Goal: Reduce total utilization (may spread load differently)

**Note**: MINIMIZE_OCCUPIED_CONTAINERS is almost always what you want for consolidation.

## Common Usage Patterns

### 1. Basic VM Consolidation

Pack VMs onto fewest hosts:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_containers_spec_examples.py start=basic_vm_consolidation_start end=basic_vm_consolidation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_containers_spec_examples.cpp start=basic_vm_consolidation_start end=basic_vm_consolidation_end
```

</TabItem>
</Tabs>

### 2. Consolidation with Capacity Constraints

Ensure capacity limits respected during consolidation:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_containers_spec_examples.py start=consolidation_capacity_start end=consolidation_capacity_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_containers_spec_examples.cpp start=consolidation_capacity_start end=consolidation_capacity_end
```

</TabItem>
</Tabs>

### 3. Weighted Container Costs

Prefer freeing expensive or problematic containers:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_containers_spec_examples.py start=weighted_costs_start end=weighted_costs_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_containers_spec_examples.cpp start=weighted_costs_start end=weighted_costs_end
```

</TabItem>
</Tabs>

**How it works**: Empty containers contribute their cost to the objective, so solver prefers freeing high-cost containers.

### 4. Safety Limit on Freeing

Prevent freeing too many containers at once:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_containers_spec_examples.py start=safety_limit_start end=safety_limit_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_containers_spec_examples.cpp start=safety_limit_start end=safety_limit_end
```

</TabItem>
</Tabs>

### 5. Rack-Level Consolidation

Consolidate entire racks:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_containers_spec_examples.py start=rack_consolidation_start end=rack_consolidation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_containers_spec_examples.cpp start=rack_consolidation_start end=rack_consolidation_end
```

</TabItem>
</Tabs>

### 6. Incremental Consolidation Over Rounds

Gradually consolidate over multiple rounds:

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_containers_spec_examples.py start=incremental_consolidation_start end=incremental_consolidation_end
```

## Weight Guidelines

Choose weight based on consolidation priority:

| Weight | Priority | Use Case |
|--------|----------|----------|
| 1-3 | **Low** | Consolidate if easy, but not critical |
| 5-10 | **Medium** | Important consolidation goal |
| 10-20 | **High** | Primary objective (bin packing) |
| 20+ | **Critical** | Must consolidate aggressively |

**Relative to balance**: Typically use 2-5× higher weight than balance goals to prioritize consolidation.

## Combining with ToFreeSpec

Use both for targeted consolidation:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_containers_spec_examples.py start=incremental_consolidation_start end=incremental_consolidation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_containers_spec_examples.cpp start=combining_to_free_start end=combining_to_free_end
```

</TabItem>
</Tabs>

**Difference**:
- `ToFreeSpec`: "These specific containers MUST be empty"
- `MinimizeContainersSpec`: "Free up as many containers as possible"

## Performance Considerations

- **Impact**: Moderate - adds global optimization pressure
- **Solver compatibility**: Works with both Local Search and Optimal solvers
- **Convergence**: May require more solver time than simple balance
- **Scaling**: Linear with container count

**Optimization tip**: Use with LocalSearchSolver for large problems (&gt;1000 objects). OptimalSolver can find true minimum for smaller problems (&lt;100 objects).

## Common Pitfalls

### 1. Infeasible Consolidation

**Problem**: Total capacity insufficient for target consolidation.

```python
# BAD: Try to pack 1000 VMs onto 5 hosts, but total capacity only fits 500
solver.add_goal(
        GoalSpec(
            minimizeContainersSpec=MinimizeContainersSpec(...)
        ),
    weight=100.0  # Very high weight
)
# Solver will fail or violate capacity constraints
```

**Solution**: Verify consolidation is feasible:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_containers_spec_examples.py start=infeasibility_check_start end=infeasibility_check_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_containers_spec_examples.cpp start=infeasibility_check_start end=infeasibility_check_end
```

</TabItem>
</Tabs>

### 2. Overloaded Packed Containers

**Problem**: Consolidation creates overloaded containers.

```python
# BAD: Only minimize containers, ignore balance
solver.add_goal(
        GoalSpec(
            minimizeContainersSpec=MinimizeContainersSpec(...)
        ),
    weight=20.0
)
# Result: Few hosts, but some severely overloaded
```

**Solution**: Add balance goal with moderate weight:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_containers_spec_examples.py start=consolidation_balance_fix_start end=consolidation_balance_fix_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_containers_spec_examples.cpp start=consolidation_balance_fix_start end=consolidation_balance_fix_end
```

</TabItem>
</Tabs>

### 3. Forgetting Count Dimension

**Problem**: No count dimension, spec doesn't work.

```python
# BAD: dimension refers to non-existent dimension
solver.add_goal(
        GoalSpec(
            minimizeContainersSpec=MinimizeContainersSpec(
        dimension="count",  # Never added this dimension!
    )
        ),
    weight=10.0
)
```

**Solution**: Always add count dimension:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_containers_spec_examples.py start=count_dimension_start end=count_dimension_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_containers_spec_examples.cpp start=count_dimension_start end=count_dimension_end
```

</TabItem>
</Tabs>

### 4. Wrong Formula

**Problem**: Using MINIMIZE_TOTAL_UTILIZATION when you want to minimize container count.

```python
# BAD: Wrong formula for bin packing
solver.add_goal(
        GoalSpec(
            minimizeContainersSpec=MinimizeContainersSpec(
        formula=MinimizeContainersFormula.MINIMIZE_TOTAL_UTILIZATION,
    )
        ),
    weight=10.0
)
# This minimizes total utilization, NOT container count!
```

**Solution**: Use default MINIMIZE_OCCUPIED_CONTAINERS:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_containers_spec_examples.py start=correct_formula_start end=correct_formula_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_containers_spec_examples.cpp start=correct_formula_start end=correct_formula_end
```

</TabItem>
</Tabs>

## Verification Example

Verify consolidation achieved target:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_containers_spec_examples.py start=verify_consolidation_start end=verify_consolidation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_containers_spec_examples.cpp start=verify_consolidation_start end=verify_consolidation_end
```

</TabItem>
</Tabs>

## Cost Savings Calculation

Estimate cost savings from consolidation:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_containers_spec_examples.py start=calculate_savings_start end=calculate_savings_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/minimize_containers_spec_examples.cpp start=calculate_savings_start end=calculate_savings_end
```

</TabItem>
</Tabs>

## Related Specs

- [ToFreeSpec](../to-free) - Explicitly drain specific containers
- [BalanceSpec](../balance) - Balance load on remaining containers
- [CapacitySpec](../capacity) - Ensure capacity constraints during consolidation
- [NonAcceptingSpec](../placement/non-accepting) - Gradual draining (block new placements)

## See Also

- [Bin Packing Cookbook Example](../../cookbook/bin-packing) - Complete consolidation example

## Source Code

- Thrift definition: `interface/thrift/ProblemSpecs.thrift` (MinimizeContainersSpec)
- Implementation: `solver/goals/MinimizeContainers.cpp`
