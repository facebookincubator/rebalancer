---
sidebar_position: 4
---

# Multi-Objective Optimization

Balance multiple conflicting objectives with weights and priority tuples.

## Problem Description

You have:
- **100 storage nodes** to rebalance
- **Multiple conflicting goals**: balance CPU, balance memory, minimize movement
- **Trade-offs**: Better balance requires more movement
- **Challenge**: How to prioritize and combine objectives?

## Real-World Scenario

A storage cluster has become imbalanced over time. You need to rebalance across three dimensions:
1. **CPU utilization**: Currently 20-95% across nodes
2. **Memory utilization**: Currently 30-90% across nodes
3. **Data movement**: Prefer minimal disruption

These goals conflict - achieving balanced distribution requires moving many objects. You must decide: Is 80% balance with minimal movement better than 95% balance with massive movement?

## Problem Modeling

### Rebalancer Terms

- **Objects**: 100 storage nodes
- **Containers**: Host servers
- **Dimensions**:
  - `cpu_usage`: CPU consumed by each node
  - `memory_usage`: Memory consumed by each node
  - `data_size`: Data size for movement cost
- **Goals**:
  - Balance CPU (high priority)
  - Balance memory (medium priority)
  - Minimize movement (low priority)

## Solution Approaches

### Approach 1: Weighted Sum (Simple)

Combine goals with different weights:

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/multi_objective/weighted_sum.py start=solution_start end=solution_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/multi_objective/weighted_sum.cpp start=solution_start end=solution_end
```

</TabItem>
</Tabs>

**Pros**:
- Simple to understand
- Easy to tune weights
- Smooth trade-offs

**Cons**:
- Hard to predict exact balance vs. movement trade-off
- No guarantees on individual objective quality
- Weights don't directly map to acceptable ranges

### Approach 2: Lexicographic Tuples (Priority Levels)

Optimize objectives in strict priority order:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/multi_objective/lexicographic_tuples.py start=solution_start end=solution_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/multi_objective/lexicographic_tuples.cpp start=solution_start end=solution_end
```

</TabItem>
</Tabs>

**Pros**:
- Clear priority levels
- Guarantees: won't sacrifice higher priority for lower
- Predictable behavior

**Cons**:
- Less smooth trade-offs
- May ignore lower priorities entirely
- Can be more rigid than desired

### Approach 3: Hybrid (Tuples + Weights)

Combine both approaches for fine control:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/multi_objective/hybrid_approach.py start=solution_start end=solution_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/multi_objective/hybrid_approach.cpp start=solution_start end=solution_end
```

</TabItem>
</Tabs>

**Pros**:
- Best of both worlds
- Flexible and predictable
- Fine-grained control

**Cons**:
- More complex to configure
- Requires understanding both mechanisms

## Complete Example

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/multi_objective/complete_example.py start=solution_start end=solution_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/multi_objective/complete_example.cpp start=solution_start end=solution_end
```

</TabItem>
</Tabs>

## Weight Tuning Guidelines

### Finding the Right Weights

Iterative process:

1. **Start with equal weights** (all 1.0)
2. **Measure outcomes**: Which goal is under-optimized?
3. **Increase weight** of under-optimized goal by 2-5×
4. **Re-solve and compare**: Better balance?
5. **Repeat** until satisfied

### Weight Ratio Rules of Thumb

| Ratio | Interpretation | Use Case |
|-------|---------------|----------|
| 1:1:1 | All equal | Balanced priorities |
| 10:5:1 | Strong hierarchy | Clear priority order |
| 10:1:1 | Dominant goal | One critical objective |
| 10:10:1 | Two critical, one nice-to-have | Common pattern |

### When to Use Tuples Instead

Use tuples when:
- Must guarantee minimum quality on high-priority goal
- Willing to completely ignore low-priority goals if needed
- Have true priority levels (e.g., correctness > performance > cost)

Use weights when:
- Want smooth trade-offs
- All goals somewhat important
- Willing to accept partial satisfaction of all goals

## Understanding the Priority/Tuple System

The tuple system organizes goals into strict priority levels for lexicographic optimization. This provides a middle ground between pure weighted sum (everything matters equally) and hard constraints (absolute requirements).

### How Tuples Work

**Priority Levels**:
- **Tuple 0**: Highest priority - solver optimizes these first
- **Tuple 1**: Second priority - optimized without worsening tuple 0
- **Tuple 2**: Third priority - optimized without worsening tuples 0-1
- **Tuple N**: Optimized without worsening any higher-priority tuples

**Key Properties**:
1. **Strict Ordering**: Lower tuple numbers = higher priority (0 is most important)
2. **Never Worsen**: Goals in tuple N will NEVER sacrifice goals in tuples 0 through N-1
3. **Within-Tuple Combination**: Goals in the same tuple are combined using weights
4. **Default Behavior**: If no tuple/priority specified, all goals go into tuple 0

### API Usage

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
from rebalancer.specs import BalanceSpec, GoalSpec, MinimizeMovementSpec

# Tuple 0 (highest priority): Balance CPU and memory
solver.add_goal(
    GoalSpec(balanceSpec=BalanceSpec(name="balance-cpu", scope="host", dimension="cpu")),
    weight=2.0,  # Within tuple 0, CPU is 2x more important than memory
    tuple_pos=0,  # Highest priority
)

solver.add_goal(
    GoalSpec(balanceSpec=BalanceSpec(name="balance-memory", scope="host", dimension="memory")),
    weight=1.0,
    tuple_pos=0,  # Same tuple as CPU
)

# Tuple 1 (second priority): Minimize movement
# This will NEVER worsen CPU or memory balance
solver.add_goal(
    GoalSpec(
        minimizeMovementSpec=MinimizeMovementSpec(
            name="minimize-movement",
            scope="host",
            dimension="data",
        )
    ),
    weight=1.0,
    tuple_pos=1,  # Lower priority
)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Tuple 0 (highest priority): Balance CPU and memory
solver.addGoal(
    balanceCpuSpec,
    2.0,  // weight: within tuple 0, CPU is 2x more important
    0     // tuplePos: highest priority
);

solver.addGoal(
    balanceMemorySpec,
    1.0,  // weight
    0     // tuplePos: same tuple as CPU
);

// Tuple 1 (second priority): Minimize movement
// This will NEVER worsen CPU or memory balance
solver.addGoal(
    minimizeMovementSpec,
    1.0,  // weight
    1     // tuplePos: lower priority
);
```

</TabItem>
</Tabs>

### How The Solver Processes Tuples

**Example Configuration**:
```
Tuple 0: balance-cpu (weight=2.0) + balance-memory (weight=1.0)
Tuple 1: minimize-movement (weight=1.0)
```

**Solver Process**:
1. **Phase 1 - Optimize Tuple 0**:
   - Find best combination of CPU and memory balance using 2:1 weighting
   - Example result: CPU imbalance = 100, Memory imbalance = 50
   - Combined tuple 0 value: (2.0 × 100) + (1.0 × 50) = 250

2. **Phase 2 - Optimize Tuple 1**:
   - Minimize data movement
   - **Hard Constraint**: Tuple 0 value must remain at 250 (or better)
   - Can only accept moves that don't worsen CPU or memory balance
   - Example: Move 10 TB while keeping tuple 0 optimal

**Result**: You get the best possible movement minimization while maintaining optimal resource balance.

## Debugging Multi-Objective Problems

### Inspecting Individual Goal Values

After solving, you can inspect how well each individual goal was satisfied:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
# After solving
solution = solver.solve()

# Access the final objective summary (Thrift-JSON dict shape).
final_objectives = solution["finalGlobalObjective"]

# Iterate through tuple levels (priority levels)
for tuple_idx, goal_tuple in enumerate(final_objectives["goals"]):
    print(f"\nTuple {tuple_idx} (priority level {tuple_idx}):")
    print(f"  Combined value: {goal_tuple['value']}")

    # Iterate through individual goals in this tuple
    for obj in goal_tuple["objs"]:
        print(f"  - {obj['name']}: {obj['value']} (weight: {obj['weight']})")

# Example output:
# Tuple 0 (priority level 0):
#   Combined value: 156.32
#   - balance-cpu: 123.45 (weight: 2.0)
#   - balance-memory: 67.89 (weight: 1.0)
# Tuple 1 (priority level 1):
#   Combined value: 234.56
#   - minimize-movement: 234.56 (weight: 1.0)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// After solving
auto solution = solver.solve();

// Access the final objective summary
auto& finalObjectives = solution.finalGlobalObjective;

// Iterate through tuple levels (priority levels)
for (size_t tupleIdx = 0; tupleIdx < finalObjectives.goals.size(); ++tupleIdx) {
    const auto& goalTuple = finalObjectives.goals[tupleIdx];
    std::cout << "\nTuple " << tupleIdx << " (priority level " << tupleIdx << "):\n";
    std::cout << "  Combined value: " << goalTuple.value << "\n";

    // Iterate through individual goals in this tuple
    for (const auto& obj : goalTuple.objs) {
        std::cout << "  - " << obj.name << ": " << obj.value
                  << " (weight: " << obj.weight << ")\n";
    }
}

// Example output:
// Tuple 0 (priority level 0):
//   Combined value: 156.32
//   - balance-cpu: 123.45 (weight: 2.0)
//   - balance-memory: 67.89 (weight: 1.0)
// Tuple 1 (priority level 1):
//   Combined value: 234.56
//   - minimize-movement: 234.56 (weight: 1.0)
```

</TabItem>
</Tabs>

**Structure**:
- `solution["finalGlobalObjective"]["goals"]` is a list of objective summaries, one per tuple/priority level
- Each objective summary dict has:
  - `value`: Combined value for all goals in this tuple
  - `objs`: List of single-objective summary dicts with individual goal details
- Each single-objective summary dict has:
  - `name`: Goal name
  - `value`: Goal value
  - `weight`: Goal weight

The `rebalancer.specs.AssignmentSolution` TypedDict mirrors this shape, so editors with type checking can autocomplete the keys.

### Comparing Solutions

Solve with different configurations and compare:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/cookbook/multi_objective/compare_approaches.py start=variation_start end=variation_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/cookbook/multi_objective/compare_approaches.cpp start=variation_start end=variation_end
```

</TabItem>
</Tabs>

## Common Pitfalls

### 1. Weights Too Close

**Problem**: Weights 1.0 vs 1.1 barely changes behavior.

**Solution**: Use ratios of 2× or more for noticeable effect.

### 2. Conflating Tuples and Weights

**Problem**: Using both but thinking weights work across tuples.

**Solution**: Weights only matter WITHIN a tuple level.

### 3. Ignoring Units

**Problem**: CPU (0-100) vs memory (0-64GB) have different scales.

**Solution**: Rebalancer auto-normalizes, but be aware of magnitudes.

### 4. Not Monitoring Trade-offs

**Problem**: Increasing one goal's weight hurts another.

**Solution**: Always measure all objectives after changing weights.

## Performance Notes

- **Multi-objective**: Slightly slower than single objective (&lt;10% overhead)
- **Tuples**: May take longer as solver optimizes each level sequentially
- **Weights**: Faster than tuples for similar outcomes

## Next Steps

- Minimize data movement specifically: [MinimizeMovementSpec](../reference/movement/minimize-movement)
- Handle tenant-specific objectives: [Multi-Tenant Allocation](multi-tenant)
- Balance at multiple scope levels: [Hierarchical Balancing](load-balancing#rack-aware-balancing)

## Related Goals

- [BalanceSpec](../reference/balance) - Balance resource utilization
- [MinimizeMovementSpec](../reference/movement/minimize-movement) - Minimize disruption
- Understanding priority levels (tuple system) - see below
