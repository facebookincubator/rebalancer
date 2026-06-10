---
sidebar_position: 19
---

# MaximizeAllocationSpec

**Type**: Goal only

Maximize utilization on a specified set of containers.

## Overview

`MaximizeAllocationSpec` creates incentive to pack objects onto specific containers (filtered by the spec), maximizing their utilization before using other containers. This is useful for preferring certain hosts, consolidating onto cheaper hardware, or ensuring specific containers are well-utilized.

**Use this when**: You want to preferentially fill specific containers (e.g., pack onto cheap hosts first, utilize premium hosts fully, consolidate onto specific racks).

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/maximize_allocation_spec_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/maximize_allocation_spec_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Unique identifier for this goal |
| `scope` | string | Yes | - | Scope level (e.g., `"host"`, `"rack"`) |
| `dimension` | string | Yes | - | Object dimension to maximize |
| `filter` | Filter | No | all | Containers to maximize utilization on |

## How It Works

For containers matching the filter:
```
objective = sum(dimension values on filtered containers)
```

**Goal**: Maximize the sum → pack more objects onto filtered containers

**Example**:
- Budget hosts: [host1, host2]
- host1 has 50 GB, host2 has 30 GB → total = 80 GB
- Solver tries to increase this total by moving more data to these hosts

## Common Usage Patterns

### 1. Prefer Cheap Hosts

Fill budget-tier hosts before using premium hosts:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/maximize_allocation_spec_examples.py start=prefer_cheap_hosts_start end=prefer_cheap_hosts_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/maximize_allocation_spec_examples.cpp start=prefer_cheap_hosts_start end=prefer_cheap_hosts_end
```

</TabItem>
</Tabs>

**Result**: Budget hosts fill up before premium hosts are used.

### 2. Data Locality

Maximize data on hosts with local storage:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/maximize_allocation_spec_examples.py start=data_locality_start end=data_locality_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/maximize_allocation_spec_examples.cpp start=data_locality_start end=data_locality_end
```

</TabItem>
</Tabs>

### 3. Rack Consolidation

Consolidate workload onto specific racks:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/maximize_allocation_spec_examples.py start=rack_consolidation_start end=rack_consolidation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/maximize_allocation_spec_examples.cpp start=rack_consolidation_start end=rack_consolidation_end
```

</TabItem>
</Tabs>

### 4. New Infrastructure Preference

Prefer new hosts over old hosts:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/maximize_allocation_spec_examples.py start=new_infrastructure_preference_start end=new_infrastructure_preference_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/maximize_allocation_spec_examples.cpp start=new_infrastructure_preference_start end=new_infrastructure_preference_end
```

</TabItem>
</Tabs>

### 5. Zone Preference

Prefer specific availability zones:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/maximize_allocation_spec_examples.py start=zone_preference_start end=zone_preference_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/maximize_allocation_spec_examples.cpp start=zone_preference_start end=zone_preference_end
```

</TabItem>
</Tabs>

### 6. Energy Efficiency

Pack onto energy-efficient hosts:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/maximize_allocation_spec_examples.py start=energy_efficiency_start end=energy_efficiency_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/maximize_allocation_spec_examples.cpp start=energy_efficiency_start end=energy_efficiency_end
```

</TabItem>
</Tabs>

### 7. Tiered Storage Strategy

Fill fast storage before slow storage:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/maximize_allocation_spec_examples.py start=tiered_storage_strategy_start end=tiered_storage_strategy_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/maximize_allocation_spec_examples.cpp start=tiered_storage_strategy_start end=tiered_storage_strategy_end
```

</TabItem>
</Tabs>

## Weight Guidelines

Choose weight based on preference strength:

| Weight | Preference | Use Case |
|--------|-----------|----------|
| 0.5-1.0 | **Weak** | Slight preference, easily overridden |
| 1.0-3.0 | **Moderate** | Clear preference |
| 3.0-5.0 | **Strong** | Strong preference (primary goal) |
| 5.0+ | **Very strong** | Near-mandatory (consider constraint) |

**Balance**: Higher weight relative to balance/capacity goals = stronger consolidation.

## Performance Considerations

- **Impact**: Minimal - simple summation
- **Solver compatibility**: Works with both Local Search and Optimal solvers
- **Scaling**: O(number of filtered containers)

## Common Pitfalls

### 1. No Filter Specified

**Problem**: Without filter, maximizes allocation everywhere (no useful effect).

```python
# BAD: No filter = maximize everywhere (no preference)
solver.add_goal(
        GoalSpec(
            maximizeAllocationSpec=MaximizeAllocationSpec(
        name="maximize-all",
        scope="host",
        dimension="cpu_usage",
        # Missing filter!
    )
        ),
    weight=2.0
)
# Just fills hosts randomly, no useful behavior
```

**Solution**: Always specify filter for specific containers:

```python
# GOOD: Filter specified
solver.add_goal(
        GoalSpec(
            maximizeAllocationSpec=MaximizeAllocationSpec(
        name="maximize-specific",
        scope="host",
        dimension="cpu_usage",
        filter=Filter(items=target_hosts),
    )
        ),
    weight=2.0
)
```

### 2. Conflicting with Balance

**Problem**: Maximize allocation conflicts with balance goal.

```python
# BAD: Trying to both pack AND balance
solver.add_goal(
        GoalSpec(
            maximizeAllocationSpec=MaximizeAllocationSpec(
        filter=Filter(items=subset_hosts),
    )
        ),
    weight=10.0  # Very high
)

solver.add_goal(
        GoalSpec(
            balanceSpec=BalanceSpec(...)
        ),
    weight=10.0  # Also very high
)
# Conflicting: can't pack subset AND balance all
```

**Solution**: Use weights to prioritize:

```python
# GOOD: Clear priority
solver.add_goal(
        GoalSpec(
            maximizeAllocationSpec=MaximizeAllocationSpec(
        filter=Filter(items=subset_hosts),
    )
        ),
    weight=5.0  # Primary
)

solver.add_goal(
        GoalSpec(
            balanceSpec=BalanceSpec(...)
        ),
    weight=1.0  # Secondary (balance within remaining space)
)
```

### 3. Filtered Hosts Insufficient Capacity

**Problem**: Trying to pack onto hosts that don't have enough capacity.

```python
# BAD: Filtered hosts have 100 GB capacity, but trying to fit 500 GB
solver.add_goal(
        GoalSpec(
            maximizeAllocationSpec=MaximizeAllocationSpec(
        filter=Filter(items=small_hosts),  # Only 100 GB total
    )
        ),
    weight=10.0
)
# Can't fit everything, violates capacity constraints
```

**Solution**: Ensure filtered hosts have sufficient capacity:

```python
# Check capacity before setting goal
filtered_capacity = sum(capacity[host] for host in target_hosts)
total_needed = sum(object_sizes.values())

if filtered_capacity >= total_needed:
    solver.add_goal(
        GoalSpec(
            maximizeAllocationSpec=MaximizeAllocationSpec(...)
        ))
else:
    print(f"Warning: Filtered hosts only have {filtered_capacity}, need {total_needed}")
```

### 4. Wrong Dimension

**Problem**: Dimension doesn't exist.

```python
# BAD: Dimension "priority" not defined
solver.add_goal(
    MaximizeAllocationSpec(
        dimension="priority",  # Doesn't exist!
        filter=Filter(items=hosts),
    ),
    weight=2.0
)
```

**Solution**: Use existing dimension:

```python
# GOOD: Use defined dimension
solver.add_object_dimension("cpu_usage", cpu_values)

solver.add_goal(
        GoalSpec(
            maximizeAllocationSpec=MaximizeAllocationSpec(
        dimension="cpu_usage",
        filter=Filter(items=hosts),
    )
        ),
    weight=2.0
)
```

## Combining with Other Specs

### With MinimizeContainersSpec

Consolidate onto preferred hosts:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/maximize_allocation_spec_examples.py start=combining_minimize_containers_start end=combining_minimize_containers_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/maximize_allocation_spec_examples.cpp start=combining_minimize_containers_start end=combining_minimize_containers_end
```

</TabItem>
</Tabs>

### With BalanceSpec

Fill preferred hosts, balance the rest:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/maximize_allocation_spec_examples.py start=combining_balance_start end=combining_balance_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/maximize_allocation_spec_examples.cpp start=combining_balance_start end=combining_balance_end
```

</TabItem>
</Tabs>

### With CapacitySpec

Maximize within capacity limits:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/balance_optimize/maximize_allocation_spec_examples.py start=combining_capacity_start end=combining_capacity_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/balance_optimize/maximize_allocation_spec_examples.cpp start=combining_capacity_start end=combining_capacity_end
```

</TabItem>
</Tabs>

## Related Specs

- [MinimizeContainersSpec](minimize-containers) - Minimize containers used (consolidation)
- [BalanceSpec](balance) - Even distribution (opposite intent)
- [UtilIncreaseCostSpec](../util-increase-cost) - Non-linear utilization preference

## Source Code

- Thrift definition: `interface/thrift/ProblemSpecs.thrift:774` (MaximizeAllocationSpec)
- Implementation: `solver/goals/MaximizeAllocation.cpp`
