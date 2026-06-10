---
sidebar_position: 15
---

# ScopeAffinitiesSpec

**Type**: Goal only

Create affinity between objects and specific containers based on object dimension values.

## Overview

`ScopeAffinitiesSpec` assigns affinity scores to containers (scope items), creating preference for placing objects with certain dimension values on those containers. The contribution to the objective is the product of the object's dimension value and the container's affinity score.

**Use this when**: You want objects with higher dimension values to preferentially go to certain containers (e.g., high-priority workloads to premium hosts, large objects to high-capacity hosts).

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/placement/scope_affinities_spec_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/placement/scope_affinities_spec_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Unique identifier for this goal |
| `scope` | string | Yes | - | Scope level (e.g., `"host"`, `"rack"`) |
| `dimension` | string | Yes | - | Object dimension name |
| `affinities` | map&lt;string, double&gt; | Yes | - | Affinity scores per container (scopeItem → score) |

## How It Works

For each object on each container:
```
contribution = object_dimension_value × container_affinity_score
```

**Total objective**: Sum of all contributions

**Goal**: Maximize total contribution → place high-value objects on high-affinity containers

**Example**:
- Object with `priority=5` on `premium_host_1` (affinity=10): contribution = 5 × 10 = 50
- Same object on `standard_host_1` (affinity=1): contribution = 5 × 1 = 5
- Solver prefers premium host (higher contribution)

## Affinity Score Guidelines

| Score | Preference | Use Case |
|-------|------------|----------|
| 0 | **Neutral** | No preference |
| 1-5 | **Weak** | Slight preference |
| 5-20 | **Moderate** | Clear preference |
| 20-100 | **Strong** | Strong preference |
| Negative | **Avoidance** | Discourage placement |

## Common Usage Patterns

### 1. Priority-Based Placement

Route high-priority workloads to premium hosts:

```python
# Object priority dimension (1-10)
priority_values = {
    "critical_workload": 10.0,  # High priority
    "important_workload": 7.0,
    "standard_workload": 3.0,
    "background_job": 1.0,  # Low priority
}
solver.add_object_dimension("priority", priority_values)

# Container affinities
host_affinities = {
    "premium_host_1": 10.0,  # Premium hosts attract high-priority
    "premium_host_2": 10.0,
    "standard_host_1": 5.0,  # Standard hosts moderately attractive
    "standard_host_2": 5.0,
    "budget_host_1": 1.0,  # Budget hosts least attractive
    "budget_host_2": 1.0,
}

solver.add_goal(
        GoalSpec(
            scopeAffinitiesSpec=ScopeAffinitiesSpec(
        name="priority-placement",
        scope="host",
        dimension="priority",
        affinities=host_affinities,
    )
        ),
    weight=3.0
)
```

**Result**: `critical_workload` (priority=10) strongly prefers `premium_host_1` (contribution=100) over `budget_host_1` (contribution=10).

### 2. Size-Based Placement

Route large objects to high-capacity hosts:

```python
# Object sizes
sizes = {
    "large_db_1": 500.0,  # 500 GB
    "large_db_2": 400.0,
    "medium_service": 50.0,
    "small_service": 10.0,
}
solver.add_object_dimension("data_size", sizes)

# High-capacity hosts more attractive for large objects
capacity_affinities = {
    "high_cap_host_1": 10.0,  # 2TB hosts
    "high_cap_host_2": 10.0,
    "medium_cap_host_1": 5.0,  # 500GB hosts
    "medium_cap_host_2": 5.0,
    "low_cap_host_1": 1.0,  # 100GB hosts
}

solver.add_goal(
        GoalSpec(
            scopeAffinitiesSpec=ScopeAffinitiesSpec(
        name="size-based-placement",
        scope="host",
        dimension="data_size",
        affinities=capacity_affinities,
    )
        ),
    weight=2.0
)
```

### 3. Performance Tier Matching

Match workload performance needs to host capabilities:

```python
# Workload performance requirements (IOPS)
iops_requirements = {
    "realtime_service": 10000.0,  # 10k IOPS needed
    "batch_processor": 1000.0,  # 1k IOPS needed
    "archive_job": 100.0,  # 100 IOPS needed
}
solver.add_object_dimension("required_iops", iops_requirements)

# Host performance tiers
perf_affinities = {
    "nvme_host_1": 100.0,  # NVMe hosts (high IOPS)
    "nvme_host_2": 100.0,
    "ssd_host_1": 10.0,  # SSD hosts (medium IOPS)
    "ssd_host_2": 10.0,
    "hdd_host_1": 1.0,  # HDD hosts (low IOPS)
}

solver.add_goal(
        GoalSpec(
            scopeAffinitiesSpec=ScopeAffinitiesSpec(
        name="perf-tier-matching",
        scope="host",
        dimension="required_iops",
        affinities=perf_affinities,
    )
        ),
    weight=4.0  # High weight - performance critical
)
```

### 4. Cost Optimization

Place expensive workloads on cheaper hosts:

```python
# Workload cost ($/hour)
workload_costs = {
    "expensive_ml_training": 50.0,  # $50/hour
    "standard_api": 5.0,  # $5/hour
    "cheap_batch": 0.5,  # $0.50/hour
}
solver.add_object_dimension("hourly_cost", workload_costs)

# Prefer placing expensive workloads on cheap hosts
# Use NEGATIVE affinities to minimize cost
host_cost_affinities = {
    "cheap_host_1": -10.0,  # Cheap hosts (negative = attracts expensive workloads)
    "cheap_host_2": -10.0,
    "expensive_host_1": -1.0,  # Expensive hosts (less attractive)
}

solver.add_goal(
        GoalSpec(
            scopeAffinitiesSpec=ScopeAffinitiesSpec(
        name="cost-optimization",
        scope="host",
        dimension="hourly_cost",
        affinities=host_cost_affinities,
    )
        ),
    weight=1.5
)
```

**Math**: Expensive workload (cost=50) on cheap host (affinity=-10) contributes 50 × -10 = -500 to objective. Minimizing objective prefers this placement.

### 5. Geographic Locality

Route user-facing services to geographically appropriate datacenters:

```python
# User traffic distribution (requests/sec per region)
traffic_by_region = {
    "us_east_service": {"us_east": 10000.0, "us_west": 1000.0, "eu": 100.0},
    "eu_service": {"us_east": 100.0, "us_west": 100.0, "eu": 10000.0},
}

# For each service, create affinity based on its primary region
for service, traffic in traffic_by_region.items():
    # Use traffic volume as dimension value
    primary_region = max(traffic, key=traffic.get)
    traffic_value = {service: traffic[primary_region]}
    solver.add_object_dimension(f"{service}_traffic", traffic_value)

    # Datacenter affinities (prefer local DC)
    dc_affinities = {
        f"{primary_region}_dc_1": 10.0,  # Local DC
        f"{primary_region}_dc_2": 10.0,
        # Other DCs have lower affinity
    }

    solver.add_goal(
        GoalSpec(
            scopeAffinitiesSpec=ScopeAffinitiesSpec(
            name=f"{service}-locality",
            scope="datacenter",
            dimension=f"{service}_traffic",
            affinities=dc_affinities,
        )
        ),
        weight=2.0
    )
```

### 6. Rack-Level Affinity

Prefer certain racks for specific workload types:

```python
# Workload network intensity
network_intensity = {
    "network_heavy_1": 100.0,  # High network usage
    "network_heavy_2": 100.0,
    "compute_heavy_1": 10.0,  # Low network usage
}
solver.add_object_dimension("network_gbps", network_intensity)

# Racks near network core more attractive for network-heavy workloads
rack_affinities = {
    "core_rack_1": 20.0,  # Near network core
    "core_rack_2": 20.0,
    "edge_rack_1": 5.0,  # Edge racks
    "edge_rack_2": 5.0,
}

solver.add_goal(
        GoalSpec(
            scopeAffinitiesSpec=ScopeAffinitiesSpec(
        name="network-proximity",
        scope="rack",
        dimension="network_gbps",
        affinities=rack_affinities,
    )
        ),
    weight=1.5
)
```

### 7. Dynamic Affinity from Metrics

Calculate affinities from observed performance:

```python
def calculate_dynamic_affinities(performance_db):
    """Calculate affinities based on historical performance.

    Args:
        performance_db: Database with host performance metrics

    Returns:
        Dict of host affinities
    """
    affinities = {}

    for host, metrics in performance_db.items():
        # Better performing hosts get higher affinity
        # Normalize latency: 10ms -> score 10, 100ms -> score 1
        latency_p95 = metrics['latency_p95_ms']
        affinity_score = max(1.0, 100.0 / latency_p95)
        affinities[host] = affinity_score

    return affinities

# Use dynamic affinities
perf_affinities = calculate_dynamic_affinities(performance_db)

solver.add_goal(
        GoalSpec(
            scopeAffinitiesSpec=ScopeAffinitiesSpec(
        name="performance-based",
        scope="host",
        dimension="latency_sensitivity",  # Object dimension
        affinities=perf_affinities,
    )
        ),
    weight=2.0
)
```

## Performance Considerations

- **Impact**: O(number of objects × number of containers with affinities)
- **Solver compatibility**: Works with both Local Search and Optimal solvers
- **Scaling**: Linear with object count
- **Best practice**: Only specify affinities for containers where it matters (sparse map)

## Common Pitfalls

### 1. Forgetting Dimension

**Problem**: Dimension not defined for objects.

```python
# BAD: Dimension "priority" not added
solver.add_goal(
    ScopeAffinitiesSpec(
        dimension="priority",  # But objects don't have this dimension!
        # ...
    ),
    weight=1.0
)
```

**Solution**: Always add object dimension first:

```python
# GOOD: Define dimension
priority_values = {obj: get_priority(obj) for obj in all_objects}
solver.add_object_dimension("priority", priority_values)

solver.add_goal(
        GoalSpec(
            scopeAffinitiesSpec=ScopeAffinitiesSpec(
        dimension="priority",
        # ...
    )
        ),
    weight=1.0
)
```

### 2. Wrong Affinity Sign

**Problem**: Using positive affinities when you want to minimize.

```python
# BAD: Trying to minimize cost, but positive affinities
# Object cost=100 on host affinity=10 contributes +1000 (bad!)
solver.add_goal(
        GoalSpec(
            scopeAffinitiesSpec=ScopeAffinitiesSpec(
        dimension="cost",
        affinities={"cheap_host": 10.0},  # Wrong sign!
    )
        ),
    weight=1.0
)
```

**Solution**: Use negative affinities to attract high-cost items:

```python
# GOOD: Negative affinities for cost minimization
solver.add_goal(
        GoalSpec(
            scopeAffinitiesSpec=ScopeAffinitiesSpec(
        dimension="cost",
        affinities={"cheap_host": -10.0},  # Negative!
    )
        ),
    weight=1.0
)
```

### 3. Affinity Scale Mismatch

**Problem**: Affinity scores too small relative to dimension values.

```python
# BAD: Dimension values 1-1000, affinities 0.001
# Contributions tiny, goal has no effect
dimension_values = {obj: i*10 for i, obj in enumerate(objects)}  # 0-1000
affinities = {"host1": 0.001, "host2": 0.002}  # Too small!
```

**Solution**: Scale affinities to match dimension magnitude:

```python
# GOOD: Affinities scaled appropriately
dimension_values = {obj: i*10 for i, obj in enumerate(objects)}  # 0-1000
affinities = {"host1": 1.0, "host2": 2.0}  # Reasonable scale
```

### 4. Sparse Affinities Map

**Problem**: Not specifying affinities for all containers causes default behavior.

```python
# Incomplete: What about hosts without affinities?
affinities = {
    "premium_host_1": 10.0,
    "premium_host_2": 10.0,
    # Missing: standard_host_1, standard_host_2, ...
}
# Objects can go to unspecified hosts with 0 affinity
```

**Solution**: Explicitly set affinity for all relevant containers:

```python
# Complete: All hosts have explicit affinity
affinities = {
    "premium_host_1": 10.0,
    "premium_host_2": 10.0,
    "standard_host_1": 5.0,  # Explicitly set
    "standard_host_2": 5.0,
    "budget_host_1": 1.0,
    "budget_host_2": 1.0,
}
```

## Combining with Other Specs

### With AssignmentAffinitiesSpec

Combine dimension-based and explicit affinity:

```python
# Dimension-based: All high-priority → premium hosts
solver.add_goal(
        GoalSpec(
            scopeAffinitiesSpec=ScopeAffinitiesSpec(
        name="priority-affinity",
        dimension="priority",
        affinities=priority_host_affinities,
    )
        ),
    weight=2.0
)

# Explicit: Specific object → specific host
solver.add_goal(
        GoalSpec(
            assignmentAffinitiesSpec=AssignmentAffinitiesSpec(
        name="explicit-affinity",
        affinities=specific_assignments,
    )
        ),
    weight=3.0  # Higher weight = explicit wins
)
```

### With BalanceSpec

Prefer certain hosts but maintain balance:

```python
# Primary: Affinity-based placement
solver.add_goal(
        GoalSpec(
            scopeAffinitiesSpec=ScopeAffinitiesSpec(
        name="performance-affinity",
        dimension="perf_requirement",
        affinities=perf_host_affinities,
    )
        ),
    weight=2.0
)

# Secondary: Balance load
solver.add_goal(
        GoalSpec(
            balanceSpec=BalanceSpec(
        name="balance",
        scope="host",
        dimension="cpu_usage",
    )
        ),
    weight=1.0
)
```

## Verification Example

Verify affinity placement effectiveness:

```python
def verify_affinity_placement(solution, dimension_values, affinities):
    """Calculate average affinity score achieved.

    Args:
        solution: Solver solution
        dimension_values: Object dimension values
        affinities: Container affinity scores

    Returns:
        Average affinity score per object
    """
    total_contribution = 0.0
    total_dimension_value = 0.0

    for container, objects in solution["assignment"].items():
        container_affinity = affinities.get(container, 0.0)

        for obj in objects:
            obj_dimension = dimension_values.get(obj, 0.0)
            total_contribution += obj_dimension * container_affinity
            total_dimension_value += obj_dimension

    if total_dimension_value > 0:
        avg_affinity = total_contribution / total_dimension_value
        print(f"Average affinity score: {avg_affinity:.2f}")
        return avg_affinity
    else:
        return 0.0
```

## Related Specs

- [AssignmentAffinitiesSpec](assignment-affinities) - Explicit object→container affinity
- [PairAffinitiesSpec](pair-affinities) - Object pair affinity
- [UtilIncreaseCostSpec](../util-increase-cost) - Dimension-based cost function

## Source Code

- Thrift definition: `interface/thrift/ProblemSpecs.thrift:985` (ScopeAffinitiesSpec)
- Implementation: `solver/goals/ScopeAffinities.cpp`
