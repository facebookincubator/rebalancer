---
sidebar_position: 11
---

# AssignmentAffinitiesSpec

**Type**: Goal only

Assign scores to object→container assignments to express preferences and affinities.

## Overview

`AssignmentAffinitiesSpec` allows you to specify affinity scores for individual object→container pairs. Positive scores encourage assignments, negative scores discourage them. This provides fine-grained control over placement preferences based on any criteria.

**Use this when**: You need to express preferences for specific object→container pairs, such as GPU workloads preferring GPU hosts, locality preferences, or avoiding known incompatibilities.

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/placement/assignment_affinities_spec_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/placement/assignment_affinities_spec_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Unique identifier for this goal |
| `scope` | string | Yes | - | Scope level (e.g., `"host"`, `"rack"`) |
| `affinities` | list&lt;AssignmentAffinity&gt; | Yes | - | List of affinity scores |

### AssignmentAffinity Structure

| Field | Type | Description |
|-------|------|-------------|
| `object` | string | Object ID |
| `scopeItem` | string | Container ID (must match scope) |
| `score` | double | Affinity score (positive = prefer, negative = avoid) |

## Affinity Score Guidelines

| Score Range | Meaning | Use Case |
|------------|---------|----------|
| +10 to +100 | **Strong preference** | Critical affinities (GPU, locality) |
| +1 to +10 | **Moderate preference** | Nice-to-have affinities |
| +0.1 to +1 | **Weak preference** | Slight bias |
| 0 | **No preference** | Neutral (don't specify) |
| -0.1 to -1 | **Weak avoidance** | Minor incompatibility |
| -1 to -10 | **Moderate avoidance** | Known issues |
| -10 to -100 | **Strong avoidance** | Critical incompatibility |

**Note**: Use constraints (AvoidAssignmentsSpec) for absolute prohibitions.

## Common Usage Patterns

### 1. Hardware Affinity

Match workloads to compatible hardware:

```python
# GPU workloads to GPU hosts
gpu_hosts = ["host0", "host1", "host2"]
gpu_workloads = ["ml_train_0", "ml_train_1", "inference_0"]

affinities = []
for workload in gpu_workloads:
    for host in gpu_hosts:
        affinities.append(
            AssignmentAffinity(
                object=workload,
                scopeItem=host,
                score=20.0,  # Strong preference
            )
        )

solver.add_goal(
        GoalSpec(
            assignmentAffinitiesSpec=AssignmentAffinitiesSpec(
        name="gpu-hardware-affinity",
        scope="host",
        affinities=affinities,
    )
        ),
    weight=3.0
)
```

### 2. Data Locality

Prefer hosts with cached data:

```python
# Workloads prefer hosts with their data cached
cache_affinity_map = {
    "workload_0": {"host3": 50.0, "host7": 30.0},  # Has data on host3, host7
    "workload_1": {"host2": 40.0},
    "workload_2": {"host3": 60.0, "host8": 20.0},
}

affinities = []
for workload, host_scores in cache_affinity_map.items():
    for host, score in host_scores.items():
        affinities.append(
            AssignmentAffinity(
                object=workload,
                scopeItem=host,
                score=score,  # Score based on cache size/recency
            )
        )

solver.add_goal(
        GoalSpec(
            assignmentAffinitiesSpec=AssignmentAffinitiesSpec(
        name="data-locality",
        scope="host",
        affinities=affinities,
    )
        ),
    weight=2.0
)
```

### 3. Performance History

Avoid hosts with historical issues for specific workloads:

```python
# Workload had crashes on certain hosts - avoid them
problem_history = {
    "flaky_workload_0": ["host5", "host12"],  # Crashed here before
    "flaky_workload_1": ["host8"],
}

affinities = []
for workload, problem_hosts in problem_history.items():
    for host in problem_hosts:
        affinities.append(
            AssignmentAffinity(
                object=workload,
                scopeItem=host,
                score=-15.0,  # Strong avoidance
            )
        )

solver.add_goal(
        GoalSpec(
            assignmentAffinitiesSpec=AssignmentAffinitiesSpec(
        name="avoid-problem-hosts",
        scope="host",
        affinities=affinities,
    )
        ),
    weight=2.0
)
```

### 4. Gradual Migration

Use increasing scores to guide gradual migration:

```python
# Round 1: Slight preference for new datacenter
affinities_round1 = [
    AssignmentAffinity(object=obj, scopeItem=new_dc_host, score=1.0)
    for obj in all_objects
    for new_dc_host in new_datacenter_hosts
]

# Round 5: Strong preference for new datacenter
affinities_round5 = [
    AssignmentAffinity(object=obj, scopeItem=new_dc_host, score=10.0)
    for obj in all_objects
    for new_dc_host in new_datacenter_hosts
]

# Use affinities_round1 early, affinities_round5 later
```

### 5. Rack Affinity

Prefer specific racks for network locality:

```python
# Frontend services prefer racks close to load balancer
frontend_instances = ["frontend_0", "frontend_1", "frontend_2"]
close_racks = ["rack0", "rack1"]  # Near load balancer
far_racks = ["rack8", "rack9"]  # Far from load balancer

affinities = []

# Prefer close racks
for instance in frontend_instances:
    for rack in close_racks:
        affinities.append(
            AssignmentAffinity(
                object=instance,
                scopeItem=rack,
                score=5.0,  # Moderate preference
            )
        )

# Avoid far racks
for instance in frontend_instances:
    for rack in far_racks:
        affinities.append(
            AssignmentAffinity(
                object=instance,
                scopeItem=rack,
                score=-3.0,  # Slight avoidance
            )
        )

solver.add_goal(
        GoalSpec(
            assignmentAffinitiesSpec=AssignmentAffinitiesSpec(
        name="frontend-rack-locality",
        scope="rack",
        affinities=affinities,
    )
        ),
    weight=1.5
)
```

### 6. Multi-Tier Preferences

Different preference levels for different host types:

```python
# Workload preferences: Premium > Standard > Budget
workload_preferences = {
    "critical_service": {
        "premium_hosts": 10.0,
        "standard_hosts": 0.0,  # Neutral (can omit)
        "budget_hosts": -5.0,  # Avoid
    },
    "batch_job": {
        "premium_hosts": -2.0,  # Don't waste premium
        "standard_hosts": 5.0,  # Prefer
        "budget_hosts": 8.0,  # Best match
    },
}

host_tiers = {
    "premium_hosts": ["host0", "host1"],
    "standard_hosts": ["host2", "host3", "host4"],
    "budget_hosts": ["host5", "host6", "host7"],
}

affinities = []
for workload, tier_scores in workload_preferences.items():
    for tier, score in tier_scores.items():
        if score == 0:
            continue  # Skip neutral
        for host in host_tiers[tier]:
            affinities.append(
                AssignmentAffinity(
                    object=workload,
                    scopeItem=host,
                    score=score,
                )
            )

solver.add_goal(
        GoalSpec(
            assignmentAffinitiesSpec=AssignmentAffinitiesSpec(
        name="tier-preferences",
        scope="host",
        affinities=affinities,
    )
        ),
    weight=2.0
)
```

### 7. Dynamic Affinity from Metrics

Calculate affinities from runtime metrics:

```python
def calculate_affinities_from_metrics(metrics_db):
    """Calculate affinities based on observed performance.

    Args:
        metrics_db: Database with workload performance per host

    Returns:
        List of AssignmentAffinity objects
    """
    affinities = []

    for workload in all_workloads:
        # Get historical performance on each host
        perf_data = metrics_db.get_performance(workload)

        for host, metrics in perf_data.items():
            # Higher p95 latency = lower affinity
            # Scale: 100ms latency -> score 10, 1000ms -> score 0
            latency_p95 = metrics['latency_p95_ms']
            score = max(0, 10.0 - (latency_p95 / 100.0))

            if score > 1.0:  # Only add meaningful affinities
                affinities.append(
                    AssignmentAffinity(
                        object=workload,
                        scopeItem=host,
                        score=score,
                    )
                )

    return affinities

# Use dynamic affinities
dynamic_affinities = calculate_affinities_from_metrics(metrics_db)

solver.add_goal(
        GoalSpec(
            assignmentAffinitiesSpec=AssignmentAffinitiesSpec(
        name="performance-based-affinity",
        scope="host",
        affinities=dynamic_affinities,
    )
        ),
    weight=1.5
)
```

## Weight Selection Guide

Choose goal weight based on affinity importance:

| Weight | Affinity Strength | Use Case |
|--------|------------------|----------|
| 0.5-1.0 | **Weak hints** | Nice-to-have preferences |
| 1.0-3.0 | **Moderate** | Important but not critical |
| 3.0-5.0 | **Strong** | Critical affinities (GPU, etc.) |
| 5.0-10.0 | **Very strong** | Near-mandatory (consider constraint) |

**Rule of thumb**: If affinity must be satisfied, use a constraint (AvoidAssignmentsSpec) instead.

## Performance Considerations

- **Impact**: O(number of affinities specified)
- **Solver compatibility**: Works with both Local Search and Optimal solvers
- **Scaling**: Linear with affinity count
- **Sparse is better**: Only specify non-zero affinities

**Optimization**: Don't specify affinities for every object→container pair. Only specify where there's a meaningful preference (score ≠ 0).

## Common Pitfalls

### 1. Too Many Affinities

**Problem**: Specifying affinities for all pairs creates unnecessary overhead.

```python
# BAD: N×M affinities (all pairs)
affinities = []
for obj in all_objects:  # 1000 objects
    for host in all_hosts:  # 100 hosts
        affinities.append(
            AssignmentAffinity(object=obj, scopeItem=host, score=1.0)
        )
# Result: 100,000 affinities (slow!)
```

**Solution**: Only specify meaningful affinities:

```python
# GOOD: Sparse affinities (only preferences)
affinities = []
for obj in gpu_workloads:  # Only 10 GPU workloads
    for host in gpu_hosts:  # Only 5 GPU hosts
        affinities.append(
            AssignmentAffinity(object=obj, scopeItem=host, score=10.0)
        )
# Result: 50 affinities (fast!)
```

### 2. Conflicting with Constraints

**Problem**: Affinities conflict with hard constraints.

```python
# BAD: Affinity wants assignment that constraint blocks
solver.add_constraint(
        ConstraintSpec(
            avoidAssignmentsSpec=AvoidAssignmentsSpec(
        assignments=[
            AvoidAssignment(object="workload_0", scopeItems=["host1"])
        ]
    )
        )
)

solver.add_goal(
    AssignmentAffinitiesSpec(
        affinities=[
            AssignmentAffinity(
                object="workload_0",
                scopeItem="host1",
                score=100.0,  # Can't satisfy - constraint blocks!
            )
        ]
    ),
    weight=10.0
)
```

**Solution**: Ensure affinities align with constraints:

```python
# GOOD: Affinity respects constraints
solver.add_constraint(
        ConstraintSpec(
            avoidAssignmentsSpec=AvoidAssignmentsSpec(
        assignments=[
            AvoidAssignment(object="workload_0", scopeItems=["host1"])
        ]
    )
        )
)

solver.add_goal(
        GoalSpec(
            assignmentAffinitiesSpec=AssignmentAffinitiesSpec(
        affinities=[
            AssignmentAffinity(
                object="workload_0",
                scopeItem="host2",  # Different host
                score=100.0,
            )
        ]
    )
        ),
    weight=10.0
)
```

### 3. Score Scale Mismatch

**Problem**: Scores too small relative to weight, affinity has no effect.

```python
# BAD: Tiny scores
solver.add_goal(
        GoalSpec(
            assignmentAffinitiesSpec=AssignmentAffinitiesSpec(
        affinities=[
            AssignmentAffinity(object="obj", scopeItem="host", score=0.001)
        ],
    )
        ),
    weight=0.1  # 0.001 × 0.1 = 0.0001 (negligible)
)

solver.add_goal(
        GoalSpec(
            balanceSpec=BalanceSpec(...)
        ),
    weight=10.0  # Dominates
)
```

**Solution**: Scale scores and weights appropriately:

```python
# GOOD: Meaningful scores and weights
solver.add_goal(
        GoalSpec(
            assignmentAffinitiesSpec=AssignmentAffinitiesSpec(
        affinities=[
            AssignmentAffinity(object="obj", scopeItem="host", score=10.0)
        ],
    )
        ),
    weight=2.0  # 10.0 × 2.0 = 20.0 (meaningful)
)

solver.add_goal(
        GoalSpec(
            balanceSpec=BalanceSpec(...)
        ),
    weight=1.0
)
```

### 4. Wrong Scope Level

**Problem**: scopeItem doesn't match spec scope.

```python
# BAD: Spec scope is "rack" but scopeItem is a host
solver.add_goal(
        GoalSpec(
            assignmentAffinitiesSpec=AssignmentAffinitiesSpec(
        scope="rack",
        affinities=[
            AssignmentAffinity(
                object="workload",
                scopeItem="host3",  # This is a host, not a rack!
                score=10.0,
            )
        ],
    )
        ),
    weight=1.0
)
```

**Solution**: Match scopeItem to scope:

```python
# GOOD: Scope and scopeItem match
solver.add_goal(
        GoalSpec(
            assignmentAffinitiesSpec=AssignmentAffinitiesSpec(
        scope="rack",
        affinities=[
            AssignmentAffinity(
                object="workload",
                scopeItem="rack2",  # Matches scope="rack"
                score=10.0,
            )
        ],
    )
        ),
    weight=1.0
)
```

## Combining with Other Specs

### With AvoidAssignmentsSpec

Hard constraints for prohibitions, soft affinities for preferences:

```python
# MUST NOT run on these hosts (hard constraint)
solver.add_constraint(
        ConstraintSpec(
            avoidAssignmentsSpec=AvoidAssignmentsSpec(
        assignments=[
            AvoidAssignment(object="critical_workload", scopeItems=["broken_host"])
        ]
    )
        )
)

# PREFER to run on these hosts (soft goal)
solver.add_goal(
        GoalSpec(
            assignmentAffinitiesSpec=AssignmentAffinitiesSpec(
        affinities=[
            AssignmentAffinity(object="critical_workload", scopeItem="premium_host", score=10.0)
        ],
    )
        ),
    weight=2.0
)
```

### With BalanceSpec

Balance while respecting affinities:

```python
# Primary: Affinities
solver.add_goal(
        GoalSpec(
            assignmentAffinitiesSpec=AssignmentAffinitiesSpec(
        name="gpu-affinity",
        affinities=gpu_affinities,
    )
        ),
    weight=3.0
)

# Secondary: Balance (works around affinities)
solver.add_goal(
        GoalSpec(
            balanceSpec=BalanceSpec(
        name="balance-load",
        scope="host",
        dimension="cpu_usage",
    )
        ),
    weight=1.0
)
```

### With MinimizeMovementSpec

Respect affinities but minimize churn:

```python
# Affinities (pull toward preferred hosts)
solver.add_goal(
        GoalSpec(
            assignmentAffinitiesSpec=AssignmentAffinitiesSpec(
        name="locality",
        affinities=locality_affinities,
    )
        ),
    weight=2.0
)

# Movement cost (resist change)
solver.add_goal(
        GoalSpec(
            minimizeMovementSpec=MinimizeMovementSpec(
        name="minimize-churn",
        scope="host",
        dimension="data_size",
    )
        ),
    weight=1.5
)
```

## Verification Example

Verify affinities are mostly satisfied:

```python
def verify_affinities(solution, affinity_spec, threshold=0.7):
    """Verify affinity satisfaction rate.

    Args:
        solution: Solver solution
        affinity_spec: AssignmentAffinitiesSpec
        threshold: Minimum satisfaction rate (default 70%)

    Returns:
        True if threshold met
    """
    satisfied = 0
    total = 0

    for affinity in affinity_spec.affinities:
        total += 1
        obj = affinity.object
        preferred_container = affinity.scopeItem

        # Find where object ended up
        actual_container = None
        for container, objects in solution["assignment"].items():
            if obj in objects:
                actual_container = container
                break

        if actual_container == preferred_container:
            satisfied += 1

    satisfaction_rate = satisfied / total if total > 0 else 0

    if satisfaction_rate >= threshold:
        print(f"✅ Affinity satisfaction: {satisfaction_rate*100:.1f}% "
              f"({satisfied}/{total}) - meets threshold {threshold*100:.0f}%")
        return True
    else:
        print(f"❌ Affinity satisfaction: {satisfaction_rate*100:.1f}% "
              f"({satisfied}/{total}) - below threshold {threshold*100:.0f}%")
        return False
```

## Affinity Builder Helper

Helper function to build affinities programmatically:

```python
class AffinityBuilder:
    """Helper to build AssignmentAffinities systematically."""

    def __init__(self):
        self.affinities = []

    def add_affinity(self, object_id, container_id, score):
        """Add a single affinity."""
        self.affinities.append(
            AssignmentAffinity(
                object=object_id,
                scopeItem=container_id,
                score=score,
            )
        )
        return self  # Chainable

    def add_many_to_one(self, object_ids, container_id, score):
        """Add affinities for many objects to one container."""
        for obj in object_ids:
            self.add_affinity(obj, container_id, score)
        return self

    def add_one_to_many(self, object_id, container_ids, score):
        """Add affinities for one object to many containers."""
        for container in container_ids:
            self.add_affinity(object_id, container, score)
        return self

    def add_matrix(self, object_ids, container_ids, score):
        """Add affinities for all object×container pairs."""
        for obj in object_ids:
            for container in container_ids:
                self.add_affinity(obj, container, score)
        return self

    def build(self):
        """Return list of AssignmentAffinity objects."""
        return self.affinities

# Usage:
builder = AffinityBuilder()
builder.add_matrix(gpu_workloads, gpu_hosts, 10.0)
builder.add_one_to_many("critical_workload", premium_hosts, 15.0)
builder.add_many_to_one(batch_jobs, "budget_host", 5.0)

affinities = builder.build()
```

## Related Specs

- [AvoidAssignmentsSpec](avoid-assignments) - Hard prohibitions (constraints)
- [PairAffinitiesSpec](pair-affinities) - Affinity between pairs of objects
- [ScopeAffinitiesSpec](scope-affinities) - Affinity to scope types (e.g., all GPU hosts)

## Source Code

- Thrift definition: `interface/thrift/ProblemSpecs.thrift` (AssignmentAffinitiesSpec, AssignmentAffinity)
- Implementation: `solver/goals/AssignmentAffinities.cpp`
