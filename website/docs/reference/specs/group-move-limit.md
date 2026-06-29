import useBaseUrl from '@docusaurus/useBaseUrl';

# Group Move Limit

**Type**: [Constraint](#constraint-only)

Limit how many objects of the same group are allowed to move. For example, cap how
many tasks of a job may be reassigned in a single rebalance, so a job is never
disrupted too much at once.

:::note Container scope only
GroupMoveLimit currently supports only the **container** scope, so every scope item
referred to below is a container.
:::

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Descriptive name for logging/debugging |
| `partitionName` | string | Yes | - | Partition whose groups' moves are limited (e.g. `"group"`) |
| `limit` | [Limit](../common/limit) | No | ABSOLUTE, `globalLimit` 1 | Max number of moves allowed per group (see [Limit](#limit)) |
| `sourceScopeItemsAffectingLimitFilter` | [Filter](../common/filter) | No | all scope items | Only moves *out of* these scope items count toward a group's limit (see [Which moves count](#which-moves-count)) |
| `destinationScopeItemsAffectingLimitFilter` | [Filter](../common/filter) | No | all scope items | Only moves *into* these scope items count toward a group's limit (see [Which moves count](#which-moves-count)) |
| `dimension` | string | No | object count | What a move's size is measured by; defaults to the number of objects moved |

## Example

All examples share this setup: 4 tasks split into two groups (`group0` =
`task0`, `task1`; `group1` = `task2`, `task3`), placed across 3 hosts.

**Partition:**

<img src={useBaseUrl('/img/reference/group-move-limit/partition-setup.png')} alt="Partition: group0 holds task0 and task1; group1 holds task2 and task3" />

**Initial assignment:** `host0` holds `task0`, `task1`, `task2`; `host1` holds
`task3`; `host2` is empty. `group0` prefers `host1` and `group1` prefers `host2`
(expressed with an AssignmentAffinities goal), so without any limit every task
would move toward its preferred host.

<img src={useBaseUrl('/img/reference/group-move-limit/assignment-setup.png')} alt="Initial assignment: host0 holds three tasks, host1 holds one, host2 is empty" />

*(The diagrams number tasks and containers from 1, so Task 1 = `task0`, Container 1
= `host0`, and so on.)*

A GroupMoveLimit constraint with a global limit of 1 lets each group move at most
one of its objects, so each group can satisfy only part of its preference:

```cpp
solver.setObjectName("task");
solver.setContainerName("host");

solver.setAssignment(std::map<std::string, std::vector<std::string>>{
    {"host0", {"task0", "task1", "task2"}},
    {"host1", {"task3"}},
    {"host2", {}},
});

// Two groups of two tasks each.
solver.addPartition(
    "group",
    std::map<std::string, std::string>{
        {"task0", "group0"}, {"task1", "group0"},
        {"task2", "group1"}, {"task3", "group1"}});

// (group0 prefers host1 and group1 prefers host2 via an AssignmentAffinities goal.)

// Each group may move at most one of its objects.
GroupMoveLimitSpec groupMoveLimit;
groupMoveLimit.partitionName() = "group";

Limit limit;
limit.type() = LimitType::ABSOLUTE;
limit.globalLimit() = 1;
groupMoveLimit.limit() = limit;

solver.addConstraint(groupMoveLimit);
```

There are several valid solutions; each leaves each group having moved at most once:

<img src={useBaseUrl('/img/reference/group-move-limit/example-1.png')} alt="Two valid solutions, each with every group having moved at most once" />

## Constraint only

GroupMoveLimit can only be used as a **constraint**; there is no goal form. Add it
with `solver.addConstraint`. As with any constraint, if the initial assignment
already satisfies it the final one will too, and if it is initially broken the
general [constraint policy](../constraint-policy) applies.

## Limit

The `limit` ([`Limit`](../common/limit)) caps the number of moves per group; the
default is an absolute global limit of `1`. Use `groupLimits` to set a different
cap per group---for example, allow `group0` up to 2 moves while forbidding `group1`
from moving at all ([Example 2](#example-2)). With a `dimension`, the limit bounds
the summed dimension of the moved objects rather than their count.

## Which moves count

By default every move of a group's object counts toward that group's limit. The two
filters narrow which moves are counted, by the move's **source** and **destination**
scope items:

- `sourceScopeItemsAffectingLimitFilter` --- a move counts only if it leaves a
  scope item selected by this filter.
- `destinationScopeItemsAffectingLimitFilter` --- a move counts only if it enters a
  scope item selected by this filter.

A move counts toward a group's limit only when **both** conditions hold. Moves that
fall outside the filters are always allowed, regardless of the limit
([Example 3](#example-3)).

## More Examples {#examples}

All examples build on the [shared setup](#example) above. Each links to its
runnable unit test.

### Example 2: per-group limit {#example-2}

A GroupMoveLimit with `groupLimits` of 2 for `group0` and 0 for `group1`: `group0`
may move freely toward its preferred host while `group1` is pinned in place. Only
`group0` ends up moving.
([source](https://github.com/facebook/rebalancer/blob/main/algopt/rebalancer/interface/tests/GroupMoveLimitSpecTest.cpp#L115-L137))

<img src={useBaseUrl('/img/reference/group-move-limit/example-2.png')} alt="Per-group limit: only group0 moved" />

### Example 3: filtering which moves count {#example-3}

A global limit of `0` (normally pinning everything in place), but with
`sourceScopeItemsAffectingLimitFilter` whitelisting `{host0, host2}` and
`destinationScopeItemsAffectingLimitFilter` blacklisting `{host1}`. `group0` can
still move to `host1` because moves *into* `host1` do not count; and `task3` can
move out of `host1` because moves *out of* `host1` (not in the source whitelist) do
not count either.
([source](https://github.com/facebook/rebalancer/blob/main/algopt/rebalancer/interface/tests/GroupMoveLimitSpecTest.cpp#L139-L170))

<img src={useBaseUrl('/img/reference/group-move-limit/example-3.png')} alt="Filtering which moves count: moves to or from host1 are unaffected by the limit" />

## Source

- Thrift definition: [`interface/thrift/ProblemSpecs.thrift`](https://github.com/facebook/rebalancer/blob/main/algopt/rebalancer/interface/thrift/ProblemSpecs.thrift) (`GroupMoveLimitSpec`)
- SpecBuilder: [`materializer/spec_builder/GroupMoveLimitSpecBuilder.cpp`](https://github.com/facebook/rebalancer/blob/main/algopt/rebalancer/materializer/spec_builder/GroupMoveLimitSpecBuilder.cpp)---the code that defines this spec's behavior
- Tests and runnable examples: [`interface/tests/GroupMoveLimitSpecTest.cpp`](https://github.com/facebook/rebalancer/blob/main/algopt/rebalancer/interface/tests/GroupMoveLimitSpecTest.cpp)---the unit tests the snippets on this page are drawn from
