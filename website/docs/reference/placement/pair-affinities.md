---
sidebar_position: 14
---

# PairAffinitiesSpec

**Type**: Goal only

Encourage pairs of objects to be assigned to the same container (colocation).

## Overview

`PairAffinitiesSpec` creates affinity between pairs of objects, encouraging them to be placed on the same container. Each pair has an affinity score - higher scores create stronger incentive for colocation. This is useful when certain objects benefit from being near each other (low latency, data locality, coordinated workloads).

**Use this when**: You want specific pairs of objects colocated (e.g., frontend-backend services, cache-app pairs, coordinated microservices).

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/placement/pair_affinities_spec_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/placement/pair_affinities_spec_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Unique identifier for this goal |
| `scope` | string | Yes | - | Scope level (e.g., `"host"`, `"rack"`) |
| `affinities` | list&lt;PairAffinity&gt; | Yes | - | List of object pairs with affinity scores |
| `limit` | double | No | 1.0 | Normalization factor for affinity scores |

### PairAffinity Structure

| Field | Type | Description |
|-------|------|-------------|
| `pair` | ObjectPair | Pair of objects |
| `affinity` | double | Affinity score (higher = stronger preference for colocation) |

### ObjectPair Structure

| Field | Type | Description |
|-------|------|-------------|
| `object1` | string | First object ID |
| `object2` | string | Second object ID |

## How It Works

For each pair:
- If both objects on **same container**: Contributes `-affinity` to objective (reward)
- If objects on **different containers**: Contributes `0` (no penalty/reward)

**Goal**: Minimize objective → maximize total affinity achieved → colocate pairs

**Normalization**: The `limit` parameter normalizes scores. With `limit=1.0`, affinity scores are used as-is.

## Affinity Score Guidelines

| Score | Strength | Use Case |
|-------|----------|----------|
| 1-5 | **Weak** | Nice-to-have colocation |
| 5-15 | **Moderate** | Important colocation (reduce latency) |
| 15-50 | **Strong** | Critical colocation (tight coupling) |
| 50+ | **Very strong** | Must colocate (consider constraint instead) |

## Common Usage Patterns

### 1. Service Tier Colocation

Colocate frontend with corresponding backend instances:

```python
# Each frontend pairs with its backend
pairs = []
for i in range(20):
    pairs.append(
        PairAffinity(
            pair=ObjectPair(
                object1=f"frontend_{i}",
                object2=f"backend_{i}",
            ),
            affinity=15.0,  # Strong affinity
        )
    )

solver.add_goal(
        GoalSpec(
            pairAffinitiesSpec=PairAffinitiesSpec(
        name="tier-colocation",
        scope="host",
        affinities=pairs,
    )
        ),
    weight=3.0
)
```

### 2. Cache-App Colocation

Colocate application with its cache for low latency:

```python
# App instances prefer their dedicated cache
cache_pairs = [
    PairAffinity(
        pair=ObjectPair(object1="app_0", object2="cache_0"),
        affinity=20.0,
    ),
    PairAffinity(
        pair=ObjectPair(object1="app_1", object2="cache_1"),
        affinity=20.0,
    ),
    # ... more pairs
]

solver.add_goal(
        GoalSpec(
            pairAffinitiesSpec=PairAffinitiesSpec(
        name="cache-locality",
        scope="host",
        affinities=cache_pairs,
    )
        ),
    weight=4.0  # High priority
)
```

### 3. Coordinated Microservices

Colocate microservices that communicate frequently:

```python
# Services that work together
coordination_pairs = [
    PairAffinity(
        pair=ObjectPair(object1="order_service", object2="payment_service"),
        affinity=25.0,  # Frequent communication
    ),
    PairAffinity(
        pair=ObjectPair(object1="order_service", object2="inventory_service"),
        affinity=25.0,
    ),
    PairAffinity(
        pair=ObjectPair(object1="user_service", object2="session_service"),
        affinity=30.0,  # Very frequent
    ),
]

solver.add_goal(
        GoalSpec(
            pairAffinitiesSpec=PairAffinitiesSpec(
        name="microservice-coordination",
        scope="host",
        affinities=coordination_pairs,
    )
        ),
    weight=2.5
)
```

### 4. Replica Anti-Affinity

Use **negative** affinity to separate replicas:

```python
# Separate replicas for fault tolerance
replica_separation = [
    PairAffinity(
        pair=ObjectPair(object1="service_replica_0", object2="service_replica_1"),
        affinity=-10.0,  # Negative = avoid colocation
    ),
    PairAffinity(
        pair=ObjectPair(object1="service_replica_0", object2="service_replica_2"),
        affinity=-10.0,
    ),
    PairAffinity(
        pair=ObjectPair(object1="service_replica_1", object2="service_replica_2"),
        affinity=-10.0,
    ),
]

solver.add_goal(
        GoalSpec(
            pairAffinitiesSpec=PairAffinitiesSpec(
        name="replica-anti-affinity",
        scope="host",
        affinities=replica_separation,
    )
        ),
    weight=3.0
)
```

### 5. Weighted Affinity by Communication Volume

Affinity scores based on observed traffic:

```python
def calculate_pair_affinities(traffic_matrix):
    """Calculate affinities from observed traffic between services.

    Args:
        traffic_matrix: Dict[(service1, service2)] -> requests/sec

    Returns:
        List of PairAffinity objects
    """
    affinities = []

    for (svc1, svc2), traffic_rps in traffic_matrix.items():
        # Higher traffic = stronger affinity
        # 1000 rps -> affinity 10, 10000 rps -> affinity 100
        affinity_score = min(100.0, traffic_rps / 100.0)

        if affinity_score > 1.0:  # Only meaningful affinities
            affinities.append(
                PairAffinity(
                    pair=ObjectPair(object1=svc1, object2=svc2),
                    affinity=affinity_score,
                )
            )

    return affinities

# Use measured traffic data
traffic_data = get_service_traffic_matrix()
affinities = calculate_pair_affinities(traffic_data)

solver.add_goal(
        GoalSpec(
            pairAffinitiesSpec=PairAffinitiesSpec(
        name="traffic-based-affinity",
        scope="host",
        affinities=affinities,
    )
        ),
    weight=2.0
)
```

### 6. Rack-Level Colocation

Colocate at rack level (looser than host level):

```python
# Colocate pairs at rack level (same rack, not necessarily same host)
rack_pairs = [
    PairAffinity(
        pair=ObjectPair(object1="service_a_0", object2="service_b_0"),
        affinity=10.0,
    ),
    # ... more pairs
]

solver.add_goal(
        GoalSpec(
            pairAffinitiesSpec=PairAffinitiesSpec(
        name="rack-level-colocation",
        scope="rack",  # Rack scope, not host
        affinities=rack_pairs,
    )
        ),
    weight=1.5
)
```

### 7. Asymmetric Pairs with Multiple Affinities

Create complex affinity patterns:

```python
# Service A needs to be near both B and C
affinities = [
    # A-B affinity
    PairAffinity(
        pair=ObjectPair(object1="service_a", object2="service_b"),
        affinity=20.0,
    ),
    # A-C affinity
    PairAffinity(
        pair=ObjectPair(object1="service_a", object2="service_c"),
        affinity=15.0,
    ),
    # B-C weak anti-affinity (they don't need each other)
    PairAffinity(
        pair=ObjectPair(object1="service_b", object2="service_c"),
        affinity=-5.0,
    ),
]

solver.add_goal(
        GoalSpec(
            pairAffinitiesSpec=PairAffinitiesSpec(
        name="complex-affinity",
        scope="host",
        affinities=affinities,
    )
        ),
    weight=2.5
)
```

## Performance Considerations

- **Impact**: O(number of affinity pairs specified)
- **Solver compatibility**: Works with both Local Search and Optimal solvers
- **Scaling**: Linear with pair count
- **Best practice**: Only specify pairs with meaningful affinity (avoid O(n²) all-pairs)

## Common Pitfalls

### 1. Too Many Pairs

**Problem**: Specifying all possible pairs creates overhead.

```python
# BAD: All pairs (n² complexity)
affinities = []
for obj1 in all_objects:  # 1000 objects
    for obj2 in all_objects:
        if obj1 != obj2:
            affinities.append(
                PairAffinity(
                    pair=ObjectPair(object1=obj1, object2=obj2),
                    affinity=1.0,
                )
            )
# Result: 1,000,000 pairs!
```

**Solution**: Only specify meaningful pairs:

```python
# GOOD: Only important pairs
affinities = []
for i in range(num_services):
    # Only frontend-backend pairs
    affinities.append(
        PairAffinity(
            pair=ObjectPair(
                object1=f"frontend_{i}",
                object2=f"backend_{i}",
            ),
            affinity=10.0,
        )
    )
# Result: ~100 pairs
```

### 2. Conflicting with Capacity

**Problem**: Affinity wants colocation but capacity prevents it.

```python
# BAD: Want 10 large objects on same host, but capacity only fits 3
large_object_pairs = [
    PairAffinity(
        pair=ObjectPair(object1=f"large_{i}", object2=f"large_{j}"),
        affinity=50.0,
    )
    for i in range(10) for j in range(i+1, 10)
]
# Can't satisfy all affinities due to capacity
```

**Solution**: Verify capacity sufficient for desired colocation:

```python
# Check if colocation is feasible
total_size = sum(object_sizes[f"large_{i}"] for i in range(10))
host_capacity = 100.0

if total_size > host_capacity:
    print(f"Warning: Cannot colocate all large objects (need {total_size}, have {host_capacity})")
    # Reduce affinity or accept partial colocation
```

### 3. Wrong Scope

**Problem**: Pairs defined at wrong scope level.

```python
# BAD: Want host-level colocation but specified rack scope
solver.add_goal(
        GoalSpec(
            pairAffinitiesSpec=PairAffinitiesSpec(
        scope="rack",  # Pairs on same rack (but maybe different hosts)
        affinities=tight_coupling_pairs,  # Want same host!
    )
        ),
    weight=10.0
)
```

**Solution**: Match scope to desired colocation level:

```python
# GOOD: Scope matches intent
solver.add_goal(
        GoalSpec(
            pairAffinitiesSpec=PairAffinitiesSpec(
        scope="host",  # Pairs on same host
        affinities=tight_coupling_pairs,
    )
        ),
    weight=10.0
)
```

### 4. Symmetric vs Asymmetric Pairs

**Problem**: Adding both (A, B) and (B, A) doubles the affinity.

```python
# BAD: Double-counting affinity
affinities = [
    PairAffinity(pair=ObjectPair(object1="A", object2="B"), affinity=10.0),
    PairAffinity(pair=ObjectPair(object1="B", object2="A"), affinity=10.0),
    # Effective affinity is 20.0!
]
```

**Solution**: Only specify each pair once:

```python
# GOOD: Each pair once
affinities = [
    PairAffinity(pair=ObjectPair(object1="A", object2="B"), affinity=10.0),
    # Don't add (B, A)
]
```

**Note**: Pairs are symmetric - (A, B) and (B, A) are equivalent.

## Combining with Other Specs

### With ColocateGroupsSpec

Use both for group-level and pair-level colocation:

```python
# Colocate entire service groups
solver.add_goal(
        GoalSpec(
            colocateGroupsSpec=ColocateGroupsSpec(
        name="service-groups",
        partitionName="service_type",
        # ...
    )
        ),
    weight=2.0
)

# Plus specific pair affinities
solver.add_goal(
        GoalSpec(
            pairAffinitiesSpec=PairAffinitiesSpec(
        name="tight-pairs",
        scope="host",
        affinities=critical_pairs,
    )
        ),
    weight=3.0  # Higher weight = pairs more important
)
```

### With BalanceSpec

Balance load while respecting affinities:

```python
# Primary: Affinities
solver.add_goal(
        GoalSpec(
            pairAffinitiesSpec=PairAffinitiesSpec(
        name="service-affinity",
        affinities=service_pairs,
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

### With AssignmentAffinitiesSpec

Combine pair affinity with individual object preferences:

```python
# Pair affinity (frontend-backend)
solver.add_goal(
        GoalSpec(
            pairAffinitiesSpec=PairAffinitiesSpec(
        name="pair-affinity",
        affinities=frontend_backend_pairs,
    )
        ),
    weight=3.0
)

# Individual affinity (GPU workloads → GPU hosts)
solver.add_goal(
        GoalSpec(
            assignmentAffinitiesSpec=AssignmentAffinitiesSpec(
        name="hardware-affinity",
        affinities=gpu_affinities,
    )
        ),
    weight=2.0
)
```

## Verification Example

Verify pair colocation achieved:

```python
def verify_pair_colocation(solution, pair_affinities, threshold=0.7):
    """Verify pair colocation percentage.

    Args:
        solution: Solver solution
        pair_affinities: List of PairAffinity objects
        threshold: Minimum colocation rate (default 70%)

    Returns:
        True if threshold met
    """
    colocated = 0
    total = len(pair_affinities)

    for pair_aff in pair_affinities:
        obj1 = pair_aff.pair.object1
        obj2 = pair_aff.pair.object2

        # Find containers
        container1 = None
        container2 = None
        for container, objects in solution["assignment"].items():
            if obj1 in objects:
                container1 = container
            if obj2 in objects:
                container2 = container

        if container1 == container2:
            colocated += 1

    colocation_rate = colocated / total if total > 0 else 0

    if colocation_rate >= threshold:
        print(f"✅ Pair colocation: {colocation_rate*100:.1f}% ({colocated}/{total}) "
              f"- meets threshold {threshold*100:.0f}%")
        return True
    else:
        print(f"❌ Pair colocation: {colocation_rate*100:.1f}% ({colocated}/{total}) "
              f"- below threshold {threshold*100:.0f}%")
        return False
```

## Pair Affinity Builder

Helper to build pair affinities programmatically:

```python
class PairAffinityBuilder:
    """Helper to build PairAffinities systematically."""

    def __init__(self):
        self.affinities = []

    def add_pair(self, obj1, obj2, affinity):
        """Add a single pair."""
        self.affinities.append(
            PairAffinity(
                pair=ObjectPair(object1=obj1, object2=obj2),
                affinity=affinity,
            )
        )
        return self  # Chainable

    def add_matching_pairs(self, objects1, objects2, affinity):
        """Add pairs by index (obj1[i] pairs with obj2[i])."""
        for obj1, obj2 in zip(objects1, objects2):
            self.add_pair(obj1, obj2, affinity)
        return self

    def add_all_pairs(self, objects, affinity):
        """Add all combinations (use sparingly - O(n²))."""
        for i, obj1 in enumerate(objects):
            for obj2 in objects[i+1:]:
                self.add_pair(obj1, obj2, affinity)
        return self

    def add_star_pattern(self, center, satellites, affinity):
        """Add star pattern (center pairs with all satellites)."""
        for satellite in satellites:
            self.add_pair(center, satellite, affinity)
        return self

    def build(self):
        """Return list of PairAffinity objects."""
        return self.affinities

# Usage:
builder = PairAffinityBuilder()
builder.add_matching_pairs(frontends, backends, 15.0)
builder.add_star_pattern("orchestrator", workers, 10.0)
affinities = builder.build()
```

## Related Specs

- [ColocateGroupsSpec](../groups/colocate-groups) - Group-level colocation
- [AssignmentAffinitiesSpec](assignment-affinities) - Object→container affinity
- [ScopeAffinitiesSpec](scope-affinities) - Object→scope affinity

## Source Code

- Thrift definition: `interface/thrift/ProblemSpecs.thrift:950` (PairAffinity, PairAffinitiesSpec)
- Implementation: `solver/goals/PairAffinities.cpp`
