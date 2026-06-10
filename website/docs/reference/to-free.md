---
sidebar_position: 15
---

# ToFreeSpec

**Type**: Goal or Constraint

Drain or evacuate objects from specified containers.

## Overview

`ToFreeSpec` removes all objects (or reduces utilization to zero for a dimension) from specified containers. This is essential for decommissioning hosts, performing maintenance, or draining overloaded containers.

**Use this when**: You need to empty specific containers completely, either for maintenance, decommissioning, or rebalancing.

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/to_free/to_free_spec_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/to_free/to_free_spec_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Unique identifier for this spec |
| `containers` | list&lt;string&gt; | Yes | - | Container IDs to drain/evacuate |
| `dimension` | string | No | `{object}_count` | Dimension to minimize (default: object count) |
| `formula` | ToFreeSpecFormula | No | MINIMIZE_TOTAL_UTILIZATION | How to measure freedom |

## Formula Options

| Formula | Behavior | Use Case |
|---------|----------|----------|
| **MINIMIZE_TOTAL_UTILIZATION** | Minimize total utilization across all specified containers | Drain as much as possible from all containers |
| **MINIMIZE_OCCUPIED_CONTAINERS** | Minimize number of containers with any utilization | Fully drain some containers, may leave others |

## Common Usage Patterns

### 1. Drain Hosts for Maintenance

Remove all objects from hosts under maintenance:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/to_free/to_free_spec_examples.py start=drain_maintenance_start end=drain_maintenance_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/to_free/to_free_spec_examples.cpp start=drain_maintenance_start end=drain_maintenance_end
```

</TabItem>
</Tabs>

### 2. Decommission Old Hardware

Empty old hosts before removing from cluster:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/to_free/to_free_spec_examples.py start=decommission_old_hardware_start end=decommission_old_hardware_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/to_free/to_free_spec_examples.cpp start=decommission_old_hardware_start end=decommission_old_hardware_end
```

</TabItem>
</Tabs>

### 3. Drain Specific Resource Type

Reduce specific dimension to zero (not all objects):

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/to_free/to_free_spec_examples.py start=drain_specific_resource_start end=drain_specific_resource_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/to_free/to_free_spec_examples.cpp start=drain_specific_resource_start end=drain_specific_resource_end
```

</TabItem>
</Tabs>

### 4. Gradual Draining (Goal, not Constraint)

Soft goal to drain, but don't fail if impossible:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/to_free/to_free_spec_examples.py start=gradual_draining_start end=gradual_draining_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/to_free/to_free_spec_examples.cpp start=gradual_draining_start end=gradual_draining_end
```

</TabItem>
</Tabs>

### 5. Drain Prioritization

Use formula to control draining strategy:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/to_free/to_free_spec_examples.py start=drain_prioritization_start end=drain_prioritization_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/to_free/to_free_spec_examples.cpp start=drain_prioritization_start end=drain_prioritization_end
```

</TabItem>
</Tabs>

### 6. Drain by Priority

Drain high-priority containers first:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/to_free/to_free_spec_examples.py start=drain_by_priority_start end=drain_by_priority_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/to_free/to_free_spec_examples.cpp start=drain_by_priority_start end=drain_by_priority_end
```

</TabItem>
</Tabs>

## How It Works

**As Constraint**:
- Ensures all specified containers have zero utilization for the dimension
- Uses MINIMIZE_TOTAL_UTILIZATION formula (ignores formula parameter)
- Fails if objects cannot be placed elsewhere

**As Goal**:
- Minimizes utilization according to formula
- Best effort - may not achieve complete draining
- Won't fail if infeasible

With **MINIMIZE_TOTAL_UTILIZATION**:
```
objective = sum(utilization(container, dimension) for container in containers)
```

With **MINIMIZE_OCCUPIED_CONTAINERS**:
```
objective = count(containers where utilization > 0)
```

## Constraint vs Goal

### Use as Constraint When:
- **Must** drain containers (failure is acceptable)
- Decommissioning or hard maintenance deadline
- Have verified capacity exists elsewhere

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/to_free/to_free_spec_examples.py start=constraint_usage_start end=constraint_usage_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/to_free/to_free_spec_examples.cpp start=constraint_usage_start end=constraint_usage_end
```

</TabItem>
</Tabs>

### Use as Goal When:
- **Want** to drain, but not critical
- Not sure if capacity exists
- Willing to accept partial draining

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/to_free/to_free_spec_examples.py start=goal_usage_start end=goal_usage_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/to_free/to_free_spec_examples.cpp start=goal_usage_start end=goal_usage_end
```

</TabItem>
</Tabs>

## Formula Comparison Example

```python
# Scenario: Drain 3 hosts with 10 objects each

# MINIMIZE_TOTAL_UTILIZATION
# Result: Moves 10 objects from each host (30 total)
# All 3 hosts partially drained (e.g., 0, 0, 30 objects)

# MINIMIZE_OCCUPIED_CONTAINERS
# Result: Fully drains 1-2 hosts, may leave others
# (e.g., 0, 0, 30 objects or 0, 15, 15 objects)
```

## Performance Considerations

- **Impact**: Minimal overhead
- **Solver compatibility**: Works with both Local Search and Optimal solvers
- **Scaling**: O(number of containers to drain)
- **Infeasibility risk**: High if insufficient capacity elsewhere

## Common Pitfalls

### 1. Insufficient Capacity

**Problem**: Can't drain because other hosts lack capacity.

```python
# BAD: Trying to drain 50% of cluster
solver.add_constraint(
        ConstraintSpec(
            toFreeSpec=ToFreeSpec(containers=half_of_all_hosts)
        )
)
# Remaining hosts don't have capacity!
```

**Solution**: Verify capacity exists, or use as goal instead of constraint.

### 2. Draining Current Assignments

**Problem**: Draining containers that currently have many objects without checking feasibility.

```python
# Check capacity first
remaining_capacity = calculate_remaining_capacity(
    all_hosts, hosts_to_drain
)
total_to_move = calculate_total_load(hosts_to_drain)

if remaining_capacity < total_to_move:
    print("Warning: Insufficient capacity!")
    # Use goal instead, or add hosts
```

### 3. Wrong Dimension

**Problem**: Draining wrong dimension.

```python
# BAD: Want to drain storage, but drain all objects
solver.add_constraint(
        ConstraintSpec(
            toFreeSpec=ToFreeSpec(
        containers=storage_hosts,
        # Missing dimension parameter!
    )
        )
)
# Drains ALL objects, not just storage
```

**Solution**: Specify dimension:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/to_free/to_free_spec_examples.py start=pitfall_wrong_dimension_good_start end=pitfall_wrong_dimension_good_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/to_free/to_free_spec_examples.cpp start=pitfall_wrong_dimension_good_start end=pitfall_wrong_dimension_good_end
```

</TabItem>
</Tabs>

### 4. Conflicting with AvoidMoving

**Problem**: Can't drain because objects are pinned.

```python
solver.add_constraint(
        ConstraintSpec(
            avoidMovingSpec=AvoidMovingSpec(objects=["obj_on_host1"])
        )
)
solver.add_constraint(
        ConstraintSpec(
            toFreeSpec=ToFreeSpec(containers=["host1"])
        )
)
# Contradiction!
```

**Solution**: Remove pins for objects on containers being drained.

### 5. Using Wrong Formula

**Problem**: Using MINIMIZE_OCCUPIED when you want total draining.

```python
# BAD: May only drain some containers
solver.add_constraint(
        ConstraintSpec(
            toFreeSpec=ToFreeSpec(
        containers=[f"host{i}" for i in range(10)],
        formula=ToFreeSpecFormula.MINIMIZE_OCCUPIED_CONTAINERS,
    )
        )
)
# May drain only 5 hosts, leave 5 with objects!
```

**Solution**: Use MINIMIZE_TOTAL_UTILIZATION for constraints.

## Combining with Other Specs

### With CapacitySpec

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/to_free/to_free_spec_examples.py start=constraint_usage_start end=constraint_usage_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/to_free/to_free_spec_examples.cpp start=constraint_usage_start end=constraint_usage_end
```

</TabItem>
</Tabs>

### With NonAcceptingSpec

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/to_free/to_free_spec_examples.py start=combining_capacity_start end=combining_capacity_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/to_free/to_free_spec_examples.cpp start=combining_capacity_start end=combining_capacity_end
```

</TabItem>
</Tabs>

## Verification Example

Verify draining succeeded:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/to_free/to_free_spec_examples.py start=combining_capacity_start end=combining_capacity_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/to_free/to_free_spec_examples.cpp start=combining_capacity_start end=combining_capacity_end
```

</TabItem>
</Tabs>

## Incremental Draining

Drain hosts gradually over multiple rounds:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/to_free/to_free_spec_examples.py start=combining_non_accepting_start end=combining_non_accepting_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/to_free/to_free_spec_examples.cpp start=combining_non_accepting_start end=combining_non_accepting_end
```

</TabItem>
</Tabs>

## Related Specs

- [NonAcceptingSpec](placement/non-accepting) - Block new placements (complementary)
- [AvoidMovingSpec](movement/avoid-moving) - Prevent moving objects (may conflict)
- [CapacitySpec](capacity) - Ensure receiving hosts have capacity
- [MinimizeContainersSpec](balance-optimize/minimize-containers) - Minimize containers used (related goal)

## Source Code

- Thrift definition: `interface/thrift/ProblemSpecs.thrift:1075`
- Implementation: `solver/constraints/ToFree.cpp`
