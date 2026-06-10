---
sidebar_position: 4
---

# Goals vs Constraints

Understanding the difference between goals and constraints is crucial for modeling problems effectively in Rebalancer.

## The Core Difference

| Aspect | Goals | Constraints |
|--------|-------|-------------|
| **Nature** | Soft objectives | Hard requirements |
| **Optimization** | Minimized by solver | Must be satisfied |
| **Violation** | Acceptable (traded off) | Not acceptable |
| **Value** | Lower is better | Must be ≤ 0 |
| **Multiple** | Combined with weights | All must hold |

## Goals: What to Optimize

**Goals** are objectives you want to minimize. They're "soft" - Rebalancer does its best but may not achieve perfect satisfaction.

### Example: Balance CPU

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
from rebalancer.specs import BalanceSpec, GoalSpec

solver.add_goal(
    GoalSpec(
        balanceSpec=BalanceSpec(
            name="balance-cpu",
            scope="host",
            dimension="cpu_usage",
            formula="LEGACY",
        )
    ),
    weight=1.0,
)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
BalanceSpec balanceCpu;
balanceCpu.name() = "balance-cpu";
balanceCpu.scope() = "host";
balanceCpu.dimension() = "cpu_usage";
balanceCpu.formula() = BalanceSpecFormula::LEGACY;
solver.addGoal(balanceCpu, 1.0);
```

</TabItem>
</Tabs>

This says: "Try to balance CPU usage across hosts." If perfect balance is impossible (due to other constraints or conflicting goals), Rebalancer will get as close as possible.

### Common Goals

- **Balance**: Distribute a resource evenly
- **MinimizeMovement**: Prefer solutions that move fewer objects
- **UtilIncreaseCost**: Prefer moving to less-utilized containers
- **Assignment Affinities**: Prefer certain objects on certain containers

## Constraints: What Must Be True

**Constraints** are hard requirements that must be satisfied. If a solution violates a constraint, it's invalid (or the constraint is treated as a very high-priority goal).

### Example: Memory Capacity

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
from rebalancer.specs import CapacitySpec, ConstraintSpec, Limit

solver.add_constraint(
    ConstraintSpec(
        capacitySpec=CapacitySpec(
            name="memory-capacity",
            scope="host",
            dimension="memory_usage",
            limit=Limit(type="ABSOLUTE", globalLimit=64.0),  # GB
        )
    )
)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
CapacitySpec memCapacity;
memCapacity.name() = "memory-capacity";
memCapacity.scope() = "host";
memCapacity.dimension() = "memory_usage";
memCapacity.limit() = Limit();
memCapacity.limit()->globalLimit() = 64.0;  // GB
solver.addConstraint(memCapacity);
```

</TabItem>
</Tabs>

This says: "Memory usage must not exceed 64GB per host." This is non-negotiable.

### Common Constraints

- **Capacity**: Resource usage must not exceed limits
- **AvoidAssignments**: Certain object-container pairs forbidden
- **AvoidMoving**: Certain objects must not move
- **NonAccepting**: Certain containers accept no new objects

## Both Goal and Constraint

Many specs can be used as either goal or constraint:

### As a Constraint

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
from rebalancer.specs import CapacitySpec, ConstraintSpec

solver.add_constraint(
    ConstraintSpec(capacitySpec=CapacitySpec(...))
)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
CapacitySpec capacitySpec;
// ... configure spec ...
solver.addConstraint(capacitySpec);
```

</TabItem>
</Tabs>

Meaning: "Capacity MUST not be exceeded."

### As a Goal

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
from rebalancer.specs import CapacitySpec, GoalSpec

solver.add_goal(
    GoalSpec(capacitySpec=CapacitySpec(...)),
    weight=100.0,
)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
CapacitySpec capacitySpec;
// ... configure spec ...
solver.addGoal(capacitySpec, 100.0);
```

</TabItem>
</Tabs>

Meaning: "Try hard to respect capacity, but violate if necessary to satisfy higher-priority goals."

## When to Use Each

### Use a Constraint When:
- The requirement is non-negotiable
- Violation would make the solution unacceptable
- It's a physical or business constraint

Examples:
- "Database shards must fit in host memory"
- "Task cannot run on host it's excluded from"
- "Reserved capacity must be respected"

### Use a Goal When:
- You want something but can compromise
- It's an optimization objective
- You're willing to trade it off against other goals

Examples:
- "Prefer balanced load (but don't require it)"
- "Minimize data movement (but move if needed)"
- "Prefer keeping objects on current hosts"

## Broken Constraints

What if the initial assignment violates a constraint?

### Default Behavior

Rebalancer treats broken constraints as high-priority goals:

```
goal_value = invalidCost * max(0, constraint_value) + invalidState * step(constraint_value)
```

Where:
- `invalidCost` = 100 (default)
- `invalidState` = 10000 (default)
- `step(x)` = 1 if x > 0, else 0

This means Rebalancer will try very hard to fix the constraint, but won't refuse to solve.

### Customizing Broken Constraint Handling

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
from rebalancer.specs import CapacitySpec, ConstraintSpec

solver.add_constraint(
    ConstraintSpec(capacitySpec=CapacitySpec(...)),
    invalid_cost=500.0,  # Custom penalty
    invalid_state=50000.0,  # Custom fixed cost
)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
CapacitySpec capacitySpec;
// ... configure spec ...
solver.addConstraint(capacitySpec, 500.0, 50000.0);
```

</TabItem>
</Tabs>

Higher values make Rebalancer try harder to fix the constraint.

### Checking for Broken Constraints

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
solution = solver.solve()

# A constraint is "broken" when its summary reports any broken count.
broken = solution["finalConstraint"]["brokenCount"]
if broken > 0:
    print(f"Warning: {broken} constraint(s) violated in solution")
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
auto solution = solver.solve();

if (!solution.profile().feasible()) {
    std::cout << "Warning: Some constraints are violated in solution\n";
}
```

</TabItem>
</Tabs>

## Goal Priorities

When you have multiple goals, their relative importance matters.

### Method 1: Weights

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
solver.add_goal(balance_spec, weight=1.0)
solver.add_goal(movement_spec, weight=0.1)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
solver.addGoal(balanceSpec, 1.0);
solver.addGoal(movementSpec, 0.1);
```

</TabItem>
</Tabs>

Rebalancer minimizes: `1.0 * balance + 0.1 * movement`

Higher weight = more important. In this example, balance is 10x more important than movement.

### Method 2: Goal Boundaries

For strict priority ordering:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
# Priority 1: Balance (most important)
solver.add_goal(balance_spec, weight=1.0)
solver.add_goal_boundary()  # End priority 1

# Priority 2: Minimize movement (less important)
solver.add_goal(movement_spec, weight=1.0)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Priority 1: Balance (most important)
solver.addGoal(balanceSpec, 1.0);
solver.addGoalBoundary();  // End priority 1

// Priority 2: Minimize movement (less important)
solver.addGoal(movementSpec, 1.0);
```

</TabItem>
</Tabs>

Rebalancer finds the solution with minimal balance, then among those, minimal movement. This creates a lexicographic tuple: `(balance, movement)`.

Comparison: `(a, b) < (A, B)` if `a < A` OR (`a == A` AND `b < B`)

### When to Use Goal Boundaries

Use tuples when:
- You want strict priority ordering
- No amount of improvement in Goal B justifies worsening Goal A
- Example: "Minimize capacity violations first, then balance load"

Use weights when:
- Goals can be traded off
- You can quantify relative importance
- Example: "Balance is 10x more important than movement"

## Practical Example

### Database Shard Rebalancing

**Requirements**:
1. Shards MUST fit in host memory (hard)
2. Load SHOULD be balanced (soft)
3. Movement SHOULD be minimized (soft, low priority)

**Solution**:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
from rebalancer.specs import (
    BalanceSpec,
    CapacitySpec,
    ConstraintSpec,
    GoalSpec,
    MinimizeMovementSpec,
)

# Hard constraint: capacity
solver.add_constraint(
    ConstraintSpec(
        capacitySpec=CapacitySpec(
            name="memory-capacity", scope="host", dimension="memory"
        )
    )
)

# High priority: balance load
solver.add_goal(
    GoalSpec(
        balanceSpec=BalanceSpec(
            name="balance-load", scope="host", dimension="query_load"
        )
    ),
    weight=100.0,
)

# Low priority: minimize movement
solver.add_goal(
    GoalSpec(
        minimizeMovementSpec=MinimizeMovementSpec(
            name="min-moves", scope="host", dimension="query_load"
        )
    ),
    weight=1.0,
)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Hard constraint: capacity
CapacitySpec memCapacity;
memCapacity.name() = "memory-capacity";
memCapacity.scope() = "host";
memCapacity.dimension() = "memory";
solver.addConstraint(memCapacity);

// High priority: balance load
BalanceSpec balanceLoad;
balanceLoad.name() = "balance-load";
balanceLoad.scope() = "host";
balanceLoad.dimension() = "query_load";
solver.addGoal(balanceLoad, 100.0);

// Low priority: minimize movement
MinimizeMovementSpec minMoves;
minMoves.name() = "min-moves";
solver.addGoal(minMoves, 1.0);
```

</TabItem>
</Tabs>

This says:
1. Never exceed memory (constraint)
2. Balance load (important goal, weight 100)
3. Minimize movement (less important, weight 1)

If balancing requires movement, Rebalancer will move objects (10:1 tradeoff).

## Next Steps

- Learn about [Scopes and Partitions](scopes-and-partitions) to structure constraints/goals
- Explore [Dimensions](dimensions) to understand what you can constrain/optimize
- See [Cookbook Examples](../cookbook/) for real-world problem modeling
- Browse [Reference](../reference/) for all available goals and constraints
