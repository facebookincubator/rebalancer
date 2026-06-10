---
sidebar_position: 9
---

# Service Colocation

Colocate related services together for performance benefits like low latency and data locality.

## Problem Description

You have:
- **60 service instances** across **3 different services**
- **20 hosts** for deployment
- Services that benefit from **colocation** (frontend-backend, cache-app)
- **Goal**: Maximize colocation of related services while maintaining diversity

## Real-World Scenario

A microservices platform runs three tiers of services:
1. **Frontend service** (20 instances) - serves user requests
2. **API service** (20 instances) - handles business logic
3. **Cache service** (20 instances) - provides data caching

To minimize latency:
- Frontend and API instances should be **colocated** on the same hosts (reduce network hops)
- API and Cache instances should be **colocated** for fast cache access
- But each service needs **diversity** across hosts for fault tolerance

This creates competing constraints: colocation for performance vs spreading for reliability.

## Problem Modeling

### Rebalancer Terms

- **Objects**: 60 service instances (20 frontend, 20 API, 20 cache)
- **Containers**: 20 hosts
- **Partitions**:
  - `service_type`: Groups instances by service (frontend, API, cache)
  - `service_pair`: Groups instances into colocation pairs (frontend-API, API-cache)
- **Dimensions**:
  - `cpu_usage`: CPU consumed by each instance
  - `memory_usage`: Memory consumed by each instance
- **Goals**:
  - Colocate frontend with API instances
  - Colocate API with cache instances
  - Spread each service across multiple hosts for fault tolerance
- **Constraints**:
  - Host capacity limits
  - Max instances per host (don't overpack)

## Complete Solution

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/colocation/basic_colocation.py start=solution_start end=solution_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/colocation/basic_colocation.cpp start=solution_start end=solution_end
```

</TabItem>
</Tabs>

## Explanation

### Why These Choices?

1. **ColocateGroupsSpec with MAX bound**: Keeps paired services together on same host
2. **ColocateGroupsSpec with MIN bound**: Spreads each service across many hosts
3. **Partition by service_pair**: Defines which instances should be colocated
4. **Partition by service_type**: Enables per-service diversity constraints
5. **High colocation weight (5.0)**: Performance benefit is primary concern
6. **Lower spread weight (3.0)**: Secondary concern for fault tolerance
7. **GroupCountSpec constraint**: Prevents overconcentration of single service

### What Happens?

1. Solver identifies initial spread state (services distributed evenly)
2. Moves instances to colocate service pairs on same hosts
3. Balances competing goals (colocation vs diversity)
4. Ensures no host has too many instances of one service
5. Maintains capacity constraints and resource balance

## Variations

### Strict Colocation (Constraint)

Make colocation mandatory instead of a goal:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/colocation/strict_colocation_constraint.py start=variation_start end=variation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/colocation/strict_colocation_constraint.cpp start=variation_start end=variation_end
```

</TabItem>
</Tabs>

### Partial Colocation

Colocate only a subset of service pairs:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/colocation/partial_colocation.py start=variation_start end=variation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/colocation/partial_colocation.cpp start=variation_start end=variation_end
```

</TabItem>
</Tabs>

### Weighted Colocation Priority

Different colocation importance for different pairs:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/colocation/weighted_colocation_priority.py start=variation_start end=variation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/colocation/weighted_colocation_priority.cpp start=variation_start end=variation_end
```

</TabItem>
</Tabs>

### Anti-Colocation (Avoid Certain Pairs)

Prevent conflicting services from sharing hosts:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/colocation/anti_colocation.py start=variation_start end=variation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/colocation/anti_colocation.cpp start=variation_start end=variation_end
```

</TabItem>
</Tabs>

### Rack-Level Colocation

Colocate at rack level instead of host level:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/colocation/rack_level_colocation.py start=variation_start end=variation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/colocation/rack_level_colocation.cpp start=variation_start end=variation_end
```

</TabItem>
</Tabs>

### Dynamic Pair Definition

Define pairs based on runtime communication patterns:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/colocation/dynamic_pair_definition.py start=variation_start end=variation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/colocation/dynamic_pair_definition.cpp start=variation_start end=variation_end
```

</TabItem>
</Tabs>

## Performance Notes

- **ColocateGroupsSpec**: Moderate overhead, scales with partition size
- **LocalSearchSolver**: Handles 60+ instances easily, &lt;60s solve time
- **Multiple partitions**: Minimal overhead for 2-3 partitions
- **Scaling**: Linear with number of service pairs and diversity constraints

## Common Issues

### Conflicting Goals (Colocation vs Diversity)

**Problem**: Can't achieve both perfect colocation and perfect spread.

**Solution**: Tune weight ratio to prioritize:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/colocation/conflicting_goals_tuning.py start=variation_start end=variation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/colocation/conflicting_goals_tuning.cpp start=variation_start end=variation_end
```

</TabItem>
</Tabs>

### Capacity Constraints Prevent Colocation

**Problem**: Colocating services exceeds host capacity.

**Solution**: Ensure host capacity sufficient for colocated workloads:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/colocation/capacity_check.py start=variation_start end=variation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/colocation/capacity_check.cpp start=variation_start end=variation_end
```

</TabItem>
</Tabs>

### Incorrect Partition Definition

**Problem**: Pairs defined incorrectly, colocation doesn't work.

**Solution**: Verify partition structure:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/colocation/partition_definition.py start=variation_start end=variation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/colocation/partition_definition.cpp start=variation_start end=variation_end
```

</TabItem>
</Tabs>

### Weight Balance Wrong

**Problem**: Colocation weight too low, services don't colocate.

**Solution**: Increase colocation weight relative to other goals:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/colocation/weight_balance.py start=variation_start end=variation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/colocation/weight_balance.cpp start=variation_start end=variation_end
```

</TabItem>
</Tabs>

## Verification Example

Verify colocation achieved target:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/colocation/verify_colocation.py start=variation_start end=variation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/colocation/verify_colocation.cpp start=variation_start end=variation_end
```

</TabItem>
</Tabs>

## Latency Benefit Estimation

Estimate latency improvement from colocation:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/colocation/estimate_latency_benefit.py start=variation_start end=variation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/colocation/estimate_latency_benefit.cpp start=variation_start end=variation_end
```

</TabItem>
</Tabs>

## Next Steps

- Handle rack-level colocation: [Affinity Placement](affinity-placement)
- Add disaster recovery constraints: [Disaster Recovery](disaster-recovery)
- Balance competing objectives: [Multi-Objective Optimization](multi-objective)

## Related Goals and Constraints

- [ColocateGroupsSpec](../reference/groups/colocate-groups) - Main spec for colocation/spreading
- [GroupCountSpec](../reference/groups/group-count) - Diversity constraints
- [BalanceSpec](../reference/balance-optimize/balance) - Resource balance
- [CapacitySpec](../reference/capacity) - Capacity constraints
