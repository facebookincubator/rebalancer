import useBaseUrl from '@docusaurus/useBaseUrl';

# Minimize Movement

**Type**: [Goal or Constraint](#goal-vs-constraint)

Minimize, or hard-limit, how many objects move between scope items. For example,
discourage churn so Rebalancer only relocates objects when the gain is worth it, or
cap the number of moves a single rebalance may perform.

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Descriptive name for logging/debugging |
| `scope` | string | Yes | - | Scope across whose scope items movement is counted (e.g. `"host"`) |
| `dimension` | string | Yes | - | What a move costs; commonly the implicit `{object}_count` dimension, so cost is the number of moves |
| `magicScaling` | bool | No | true | When normalizing, multiply the result by `0.001` (ignored when `doNotNormalize` is true) (see [Normalization](#normalization)) |
| `doNotNormalize` | bool | No | false | Use the raw, un-normalized cost (the recommended setting); see [Normalization](#normalization) |
| `allowance` | double | No | 0 | Amount of movement (in `dimension`) that is free / allowed before any cost or limit applies (see [Allowance](#allowance)) |

## Example

An example use: only move objects when the benefit outweighs the cost. There are 3
hosts and 2 tasks, both initially on `host0`. Both prefer `host1` (an
AssignmentAffinities goal): `task0` with affinity 3, `task1` with affinity 10. A
MinimizeMovement goal makes each move cost 4.
([source](https://github.com/facebook/rebalancer/blob/main/algopt/rebalancer/interface/tests/MinimizeMovementTest.cpp#L28-L104))

**Initial assignment:**

<img src={useBaseUrl('/img/reference/minimize-movement/example-1-initial.png')} alt="Initial assignment: task0 and task1 both on host0" />

```cpp
solver.setObjectName("task");
solver.setContainerName("host");

solver.setAssignment(std::map<std::string, std::vector<std::string>>{
    {"host0", {"task0", "task1"}},
    {"host1", {}},
    {"host2", {}},
});

// Both tasks prefer host1: task0 with affinity 3, task1 with affinity 10.
auto pref = [](std::string task, std::string host, double affinity) {
  AssignmentAffinity entry;
  entry.objectName() = std::move(task);
  entry.scopeItemName() = std::move(host);
  entry.affinity() = affinity;
  return entry;
};

AssignmentAffinitiesSpec affinities;
affinities.scope() = "host";
affinities.affinities() = {
    pref("task0", "host1", 3),
    pref("task1", "host1", 10),
};
solver.addGoal(affinities);

// Each move costs 4 (the goal's weight); count moves with the task_count
// dimension, without normalization.
MinimizeMovementSpec minimizeMovement;
minimizeMovement.scope() = "host";
minimizeMovement.dimension() = "task_count";
minimizeMovement.doNotNormalize() = true;
solver.addGoal(minimizeMovement, 4);
```

`task1` moves to `host1` because its affinity gain (10) outweighs the move cost
(4), while `task0` stays put because its gain (3) is less than the cost (4).

**Final assignment:**

<img src={useBaseUrl('/img/reference/minimize-movement/example-1-final.png')} alt="Final assignment: task1 moved to host1, task0 stayed on host0" />

## Goal vs. constraint

**As a goal**, each unit of movement beyond the `allowance` adds cost, which
competes with the gains from other goals. Rebalancer performs a move only when it
has a net positive effect---so a move happens only if its benefit elsewhere offsets
its movement cost.

**As a constraint**, it becomes a hard cap: Rebalancer may perform at most
`allowance` worth of movement on top of the initial assignment. Set `allowance` to
the number of moves (or amount of `dimension`) you want to permit.

:::note Why it matters per solver
The [local search](../../solvers/overview) solver is greedy and only applies moves
that strictly improve the objective, so it already avoids needless moves even
without this spec. The [optimal (MIP) solver](../../solvers/overview), however, may
freely reshuffle objects among equally-optimal solutions; add a MinimizeMovement
goal (even with a tiny weight) to make it prefer the solution that moves the least.
:::

## Allowance

`allowance` is an amount of free movement, expressed in the spec's `dimension`.
As a **goal**, moves within the allowance incur no cost; only movement beyond it is
penalized. As a **constraint**, the allowance *is* the limit---the maximum amount of
movement permitted ([Example 5](#example-5)).

## Normalization

Normalization only affects the numeric value of the goal:

- **Non-normalized** (`doNotNormalize = true`, the recommended setting): the value
  equals the number of objects moved (or the summed `dimension` of the moved
  objects). `magicScaling` is ignored. Scale it with the goal's weight.
- **Normalized** (the current default): the same value, but divided by the number
  of scope items, and---if `magicScaling` is on---multiplied by `0.001`.

When the `dimension` is also defined on the scope items, the normalized cost of a
move additionally depends on the **destination scope item's capacity**: moving into
a larger-capacity scope item is cheaper. The full normalized formula is:

```
(magicScaling ? 0.001 : 1)
  * sum(objectDimension[move.object] / scopeItemDimension[move.destination] for move in moves)
  / scopeItemCount
```

[Example 2](#example-2) is Example 1 with normalization enabled (and asserts the
resulting values).

## More Examples {#examples}

All examples build on a similar setup to the [example above](#example). Each links
to its runnable unit test.

### Example 2: same cost per object, with normalization {#example-2}

Example 1 with normalization enabled. The per-move costs change, and both tasks
turn out worth moving to their preferred host. The unit test asserts the exact
values and explains how they are computed.
([source](https://github.com/facebook/rebalancer/blob/main/algopt/rebalancer/interface/tests/MinimizeMovementTest.cpp#L105-L182))

### Example 3: different cost per object, without normalization {#example-3}

Like Example 1, but each object has its own move cost, modeled with a `dimension`
that describes how costly each object is to move.
([source](https://github.com/facebook/rebalancer/blob/main/algopt/rebalancer/interface/tests/MinimizeMovementTest.cpp#L183-L264))

### Example 4: cost proportional to capacity, with normalization {#example-4}

Tasks have a cost proportional to their size (in GB), with normalization enabled,
so the cost of a move is inversely proportional to the **destination scope item's
capacity**. There are 3 hosts and 2 tasks, plus a constraint forcing `host0` to
empty. Rebalancer must move both tasks off `host0`, and picks `host2` over `host1`
because `host2`'s larger capacity means a smaller relative-utilization increase
(45% on `host2` vs. 60% on `host1`).
([source](https://github.com/facebook/rebalancer/blob/main/algopt/rebalancer/interface/tests/MinimizeMovementTest.cpp#L265-L342))

**Initial assignment:**

<img src={useBaseUrl('/img/reference/minimize-movement/example-4-initial.png')} alt="Example 4 initial assignment: two tasks on host0" />

**Final assignment:**

<img src={useBaseUrl('/img/reference/minimize-movement/example-4-final.png')} alt="Example 4 final assignment: both tasks moved to host2, the higher-capacity host" />

### Example 5: allowance as a hard limit {#example-5}

Used as a **constraint** rather than a goal. `host0` starts with 4 tasks and
`host1` is empty; all tasks prefer `host1` equally. A MinimizeMovement constraint
with an `allowance` of 3 caps the number of moves at 3. Rebalancer moves exactly 3
tasks---moving a fourth would improve the affinities goal further but would break
the constraint.
([source](https://github.com/facebook/rebalancer/blob/main/algopt/rebalancer/interface/tests/MinimizeMovementTest.cpp#L343-L401))

**Initial assignment:**

<img src={useBaseUrl('/img/reference/minimize-movement/example-5-initial.png')} alt="Example 5 initial assignment: four tasks on host0, host1 empty" />

**Final assignment:**

<img src={useBaseUrl('/img/reference/minimize-movement/example-5-final.png')} alt="Example 5 final assignment: three of the four tasks moved to host1, one stayed" />

## Source

- Thrift definition: [`interface/thrift/ProblemSpecs.thrift`](https://github.com/facebook/rebalancer/blob/main/algopt/rebalancer/interface/thrift/ProblemSpecs.thrift) (`MinimizeMovementSpec`)
- SpecBuilder: [`materializer/spec_builder/MinimizeMovementSpecBuilder.cpp`](https://github.com/facebook/rebalancer/blob/main/algopt/rebalancer/materializer/spec_builder/MinimizeMovementSpecBuilder.cpp)---the code that defines this spec's behavior
- Tests and runnable examples: [`interface/tests/MinimizeMovementTest.cpp`](https://github.com/facebook/rebalancer/blob/main/algopt/rebalancer/interface/tests/MinimizeMovementTest.cpp)---the unit tests the snippets on this page are drawn from
