---
sidebar_position: 3
---

# Goal priorities

When you add more than one goal, Rebalancer needs to know their relative
importance. The objective is a **tuple** of buckets compared in order
(lexicographically), and within each bucket the goals are combined into a
**weighted sum**. By default every goal goes into a single bucket (tuple position
0), so the objective is just one weighted sum, set with [**weights**](#weights). To give some
goals strict priority over others, split them across buckets using
[**tuples**](#tuples-strict-priority).

## Weights

`solver.addGoal(spec, weight)` scales each goal by its weight and sums them into a
single objective to minimize. A larger weight means the goal matters more.

```cpp
solver.addGoal(balanceSpec, 1.0);
solver.addGoal(minimizeMovementSpec, 10.0); // 10x as important
```

A practical approach: start with equal weights, solve, inspect the result, and
scale a goal up or down until the trade-off looks right.

Weights cannot express a *strict* priority---even with a large weight gap, one
goal still trades off against another. For a strict order, use tuples.

## Tuples (strict priority)

A goal tuple is an ordered list of buckets compared **lexicographically**:
Rebalancer first minimizes bucket 0; among all solutions that tie on bucket 0 it
minimizes bucket 1; and so on.

Build a tuple by separating buckets with `addGoalBoundary()`. Weights still apply
*within* each bucket:

```cpp
solver.addGoal(goalA, 2.0);  // bucket 0
solver.addGoal(goalB, 1.0);  // bucket 0
solver.addGoalBoundary();    // end bucket 0, start bucket 1
solver.addGoal(goalC, 1.0);  // bucket 1 (strictly lower priority)
```

This builds the objective tuple `(2*goalA + goalB, goalC)`: Rebalancer first
minimizes the weighted sum `2*goalA + goalB`, and only among the assignments that
tie on it does it minimize `goalC`.

Or target a bucket directly with the tuple-position argument:

```cpp
solver.addGoal(goalA, 1.0, /*tuplePos=*/0);
solver.addGoal(goalC, 1.0, /*tuplePos=*/1);
```

Lexicographic order means tuples are compared left to right, moving on only when
earlier buckets tie:

```
(a, b, c) is smaller than (A, B, C) when
    a is smaller than A, or
    a == A and b is smaller than B, or
    a == A and b == B and c is smaller than C
```

## Source

- API: [`interface/ProblemSolver.h`](https://github.com/facebookincubator/rebalancer/blob/main/algopt/rebalancer/interface/ProblemSolver.h) (`addGoal`, `addGoalBoundary`)
