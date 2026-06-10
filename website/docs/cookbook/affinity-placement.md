---
sidebar_position: 7
---

# Affinity and Anti-Affinity Placement

Place objects with preferences for specific containers or proximity to other objects.

## Problem Description

You have:
- **100 microservice instances** across **20 hosts**
- Some services need **GPU hosts**, some need **high-memory hosts**
- **Related services** should be **colocated** for low latency
- **Conflicting services** should be **separated** for fault isolation
- **Goal**: Place services respecting hardware affinities and service relationships

## Real-World Scenario

A microservices platform runs various workloads with different requirements:

1. **GPU workloads**: ML training jobs need GPU-enabled hosts
2. **Memory-intensive**: Data processing needs high-memory hosts
3. **Service dependencies**: Frontend should be near backend for low latency
4. **Anti-affinity**: Replicas of same service should be on different hosts for availability

## Problem Modeling

### Rebalancer Terms

- **Objects**: 100 microservice instances
- **Containers**: 20 hosts with different hardware profiles
- **Dimensions**:
  - `cpu_usage`: CPU consumed by each service
  - `memory_usage`: Memory consumed by each service
- **Affinities**:
  - Hardware affinity: GPU workloads → GPU hosts
  - Service affinity: Frontend → near backend
  - Anti-affinity: Replicas → different hosts
- **Goals**:
  - Maximize affinity satisfaction
  - Balance load across hosts

## Complete Solution

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/affinity_placement/basic_affinity_placement.py start=solution_start end=solution_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/affinity_placement/basic_affinity_placement.cpp start=solution_start end=solution_end
```

</TabItem>
</Tabs>

## Explanation

### Why These Choices?

1. **AssignmentAffinitiesSpec**: Soft preferences for specific placements
2. **GroupCountSpec MAX=1**: Hard anti-affinity constraint (replicas on different hosts)
3. **Affinity weights**: Higher for critical requirements (GPU=5.0), lower for nice-to-have (frontend-backend=2.0)
4. **CapacitySpec**: Ensures placements respect hardware limits
5. **BalanceSpec**: Secondary goal to distribute load evenly

### What Happens?

1. Solver identifies affinity violations in random initial placement
2. Moves GPU workloads to GPU hosts (high priority)
3. Moves memory-intensive workloads to high-memory hosts
4. Attempts to colocate frontend with backend
5. Ensures anti-affinity (no two replicas of same service on one host)
6. Balances load across all hosts

## Variations

### Rack Affinity (Locality)

Prefer instances on same rack for low latency:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/affinity_placement/rack_affinity.py start=variation_start end=variation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/affinity_placement/rack_affinity.cpp start=variation_start end=variation_end
```

</TabItem>
</Tabs>

### Negative Affinity (Avoid Specific Hosts)

Use AvoidAssignmentsSpec for hard incompatibility:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/affinity_placement/negative_affinity.py start=variation_start end=variation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/affinity_placement/negative_affinity.cpp start=variation_start end=variation_end
```

</TabItem>
</Tabs>

### Weighted Affinity Scores

Different affinity strengths for different relationships:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/affinity_placement/weighted_affinity.py start=variation_start end=variation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/affinity_placement/weighted_affinity.cpp start=variation_start end=variation_end
```

</TabItem>
</Tabs>

### Zone Anti-Affinity

Spread replicas across availability zones:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/affinity_placement/zone_anti_affinity.py start=variation_start end=variation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/affinity_placement/zone_anti_affinity.cpp start=variation_start end=variation_end
```

</TabItem>
</Tabs>

## Affinity Weight Guidelines

| Affinity Strength | Weight | Use Case |
|------------------|--------|----------|
| **Critical** | 10.0 - 100.0 | Hardware requirements (GPU, memory) |
| **Strong** | 5.0 - 10.0 | Performance-critical colocation |
| **Medium** | 2.0 - 5.0 | Preferred but not required |
| **Weak** | 0.5 - 2.0 | Nice to have, minor benefit |

## Performance Notes

- **AssignmentAffinitiesSpec**: Moderate overhead, scales with number of affinities
- **LocalSearchSolver**: Handles 100+ instances with affinities efficiently
- **Many affinities**: Consider consolidating or using higher-level constructs
- **Scaling**: O(number of affinity pairs)

## Common Issues

### Conflicting Affinities

**Problem**: Incompatible affinity requirements.

```python
from rebalancer.specs import AssignmentAffinity

# Can't satisfy both: object on host A AND host B
AssignmentAffinity(objectName="obj1", scopeItemName="hostA", affinity=10.0)
AssignmentAffinity(objectName="obj1", scopeItemName="hostB", affinity=10.0)
```

**Solution**: Use affinity for preference, constraint for requirements.

### Affinity vs Anti-Affinity Conflict

**Problem**: Want colocation but also need anti-affinity.

```python
# Want frontend near backend
# But also want frontend replicas spread
```

**Solution**: Use rack-level affinity, host-level anti-affinity:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
from rebalancer.specs import (
    ColocateGroupsSpec,
    ConstraintSpec,
    GoalSpec,
    GroupCountSpec,
    Limit,
)

# Colocate on same rack
solver.add_goal(
    GoalSpec(
        colocateGroupsSpec=ColocateGroupsSpec(
            name="colocate-on-rack",
            scope="rack",
            partitionName="service",
        )
    ),
    weight=1.0,
)

# Spread across hosts
solver.add_constraint(
    ConstraintSpec(
        groupCountSpec=GroupCountSpec(
            name="spread-on-hosts",
            scope="host",
            partitionName="service",
            bound="MAX",
            limit=Limit(type="ABSOLUTE", globalLimit=1.0),
        )
    )
)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Colocate on same rack
solver.addGoal(colocateSpec);  // scope="rack"

// Spread across hosts
solver.addConstraint(groupCountSpec);  // scope="host", MAX, limit=1
```

</TabItem>
</Tabs>

### Too Many Affinities

**Problem**: Thousands of affinity pairs slow down solver.

**Solution**: Use higher-level constructs:

```python
# Instead of individual affinities for all instances:
# Use GroupAssignmentAffinitiesSpec or partition-based specs
```

### Affinity Not Satisfied

**Problem**: Solver ignores affinity preferences.

**Solution**: Increase affinity weight or verify no conflicts:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
from rebalancer.specs import AssignmentAffinitiesSpec, GoalSpec

# Increase weight
solver.add_goal(
    GoalSpec(
        assignmentAffinitiesSpec=AssignmentAffinitiesSpec(
            name="affinity",
            scope="host",
            affinities=[],  # ...your AssignmentAffinity entries...
        )
    ),
    weight=10.0,  # Higher
)

# Check for capacity conflicts
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Increase weight
solver.addGoal(affinitySpec, 10.0);  // Higher

// Check for capacity conflicts
```

</TabItem>
</Tabs>

## Next Steps

- Handle gradual migration with affinity: [Gradual Migration](gradual-migration)
- Colocate related services: [Colocation Patterns](colocation)
- Multi-tenant affinity policies: [Multi-Tenant Allocation](multi-tenant)

## Related Goals and Constraints

- [AssignmentAffinitiesSpec](../reference/placement/assignment-affinities) - Affinity preferences
- [GroupCountSpec](../reference/groups/group-count) - Anti-affinity constraints
- [AvoidAssignmentsSpec](../reference/placement/avoid-assignments) - Hard placement blocks
- [ColocateGroupsSpec](../reference/groups/colocate-groups) - Group-level colocation
