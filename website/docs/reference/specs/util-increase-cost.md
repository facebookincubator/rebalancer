import useBaseUrl from '@docusaurus/useBaseUrl';

# Util Increase Cost

**Type**: [Goal](#goal-only)

Encourage objects to land on under-utilized scope items by penalizing utilization
**above a lower bound**. A scope item below the lower bound can take on more load for
free, while pushing a scope item past the lower bound incurs a cost---so objects are
drawn toward the emptiest scope items. When several scope items are below the lower
bound, the choice between them is arbitrary.

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Descriptive name for logging/debugging |
| `scope` | string | Yes | - | Scope whose scope items are penalized (e.g. `"host"`) |
| `dimension` | string | Yes | - | Dimension whose relative utilization is penalized |
| `lowerBound` | double | Yes | - | Relative-utilization threshold; only utilization above it is penalized |
| `squares` | bool | No | false | Raise the penalty to a superlinear power (`1.1`, despite the name), penalizing higher utilization disproportionately |
| `filter` | [Filter](../common/filter) | No | all scope items | Which scope items count toward the goal |

## Example

An example use: drain a host while filling the emptiest hosts first. Three hosts have
a capacity of 10 and a lower bound of `0.5`. `host0` is at 0.2, `host1` at 0.3, and
`host2` at 0.5 (exactly the bound). We drain `host0`; its task should land on `host1`
(which has room below the bound) rather than `host2` (already at the bound, so any
addition is penalized).
([source](https://github.com/facebook/rebalancer/blob/main/algopt/rebalancer/interface/tests/UtilIncreaseCostTest.cpp#L62-L89))

*(The diagram labels these from 1: Container 1 = `host0`, Task 1 = `task0`, and so
on.)*

**Initial assignment:**

<img src={useBaseUrl('/img/reference/util-increase-cost/example-1-initial.png')} alt="Initial assignment: host0 at 0.2, host1 at 0.3, host2 at 0.5 (the lower bound)" />

```cpp
solver.setObjectName("task");
solver.setContainerName("host");

solver.setAssignment(std::map<std::string, std::vector<std::string>>{
    {"host0", {"task0"}},  // util 0.2
    {"host1", {"task1"}},  // util 0.3
    {"host2", {"task2"}},  // util 0.5 == lower bound
});

// cpu load per task; each host has a capacity of 10.
solver.addObjectDimension(
    "cpu", std::map<std::string, double>{{"task0", 2}, {"task1", 3}, {"task2", 5}});
solver.addContainerDimension(
    "cpu", std::map<std::string, double>{{"host0", 10}, {"host1", 10}, {"host2", 10}});

// Drain host0.
ToFreeSpec toFree;
toFree.containers() = {"host0"};
solver.addConstraint(toFree);

// Penalize utilization above the 0.5 lower bound.
UtilIncreaseCostSpec utilIncreaseCost;
utilIncreaseCost.scope() = "host";
utilIncreaseCost.dimension() = "cpu";
utilIncreaseCost.lowerBound() = 0.5;
utilIncreaseCost.squares() = true;
solver.addGoal(utilIncreaseCost);
```

`task0` moves to `host1`, bringing it to exactly the lower bound (0.5) at no cost.
Moving it to `host2` instead would push that host above the bound and incur a
penalty.

**Final assignment:**

<img src={useBaseUrl('/img/reference/util-increase-cost/example-1-final.png')} alt="Final assignment: host0 drained, its task moved to host1 which reaches the 0.5 lower bound" />

## Goal only

UtilIncreaseCost can only be used as a **goal**; there is no constraint form. Its
penalty competes with other goals, so it nudges placement toward under-utilized scope
items rather than enforcing a hard rule.

## Larger-capacity bias with `squares`

When every candidate scope item is already above the lower bound, the `squares`
formula biases placement toward **larger-capacity** scope items, which is often not
what you want. The reason: relative utilization is `absoluteUtil / capacity`, so
adding an object raises a bigger scope item's relative utilization by less than it
raises a smaller one's. The squared penalty therefore grows more slowly on the larger
scope item, making it look cheaper---so load piles onto the biggest scope items. If
you need balanced relative utilization instead, use [Balance](balance) with its
[`IDEAL` formula](balance#formula), which compares scope items by relative
utilization directly and does not over-load the larger ones.

## More Examples {#examples}

Both examples drain `host0`, which starts with 10 tasks (each `cpu` = 1) while
`host1` and `host2` start empty.

### Example 2: multiple scope items below the lower bound {#example-2}

With a lower bound of `0.5` and a capacity of 100 per host, both `host1` and `host2`
stay below the bound no matter how the 10 tasks are split, so there is no penalty to
distinguish them---the tasks are placed across the two arbitrarily.
([source](https://github.com/facebook/rebalancer/blob/main/algopt/rebalancer/interface/tests/UtilIncreaseCostTest.cpp#L110-L158))

**Initial assignment:**

<img src={useBaseUrl('/img/reference/util-increase-cost/example-2-initial.png')} alt="Example 2 initial: 10 tasks on host0, host1 and host2 empty" />

**Final assignment:**

<img src={useBaseUrl('/img/reference/util-increase-cost/example-2-final.png')} alt="Example 2 final: the 10 tasks spread arbitrarily across host1 and host2" />

### Example 3: all scope items above the lower bound (larger-capacity bias) {#example-3}

With a lower bound of `0`, every host is above the bound as soon as it holds a task.
`host1` has a far larger capacity than `host2`, so under the `squares` formula it
absorbs the smaller relative-utilization increase, and all 10 tasks move there---an
illustration of the [larger-capacity bias](#larger-capacity-bias-with-squares).
([source](https://github.com/facebook/rebalancer/blob/main/algopt/rebalancer/interface/tests/UtilIncreaseCostTest.cpp#L160-L205))

**Initial assignment:**

<img src={useBaseUrl('/img/reference/util-increase-cost/example-3-initial.png')} alt="Example 3 initial: 10 tasks on host0; host1 has very large capacity, host2 small" />

**Final assignment:**

<img src={useBaseUrl('/img/reference/util-increase-cost/example-3-final.png')} alt="Example 3 final: all tasks moved to the larger-capacity host1" />

## Source

- Thrift definition: [`interface/thrift/ProblemSpecs.thrift`](https://github.com/facebook/rebalancer/blob/main/algopt/rebalancer/interface/thrift/ProblemSpecs.thrift) (`UtilIncreaseCostSpec`)
- SpecBuilder: [`materializer/spec_builder/UtilIncreaseCostSpecBuilder.cpp`](https://github.com/facebook/rebalancer/blob/main/algopt/rebalancer/materializer/spec_builder/UtilIncreaseCostSpecBuilder.cpp)---the code that defines this spec's behavior
- Tests and runnable examples: [`interface/tests/UtilIncreaseCostTest.cpp`](https://github.com/facebook/rebalancer/blob/main/algopt/rebalancer/interface/tests/UtilIncreaseCostTest.cpp)---the unit tests the snippets on this page are drawn from
