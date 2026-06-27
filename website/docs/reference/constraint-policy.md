---
sidebar_position: 2
---

# Constraint policy

A constraint defines what makes an assignment valid. The **constraint policy**
controls how strictly a constraint is enforced, and in particular what Rebalancer
does when one is *initially broken* (the starting assignment already violates it).
It applies to every constraint spec.

Set the default policy for all constraints with `solver.setConstraintPolicy(...)`,
or override it per constraint via `solver.addConstraint(spec, policy)`. The
default is `DEFAULT`.

## Policies

| Policy | Behavior |
|--------|----------|
| `DEFAULT` | If the constraint holds initially, keep it as a hard constraint. If it is already broken, turn "fix it" into a high-priority goal while keeping "do not make it worse" as a hard constraint. |
| `HARD` | Always a hard constraint. If the solver cannot fully satisfy it, that is an error rather than a best-effort result. |
| `SOFT` | Never a hard constraint. The fix-it goal (below) penalizes its violation, so the solver minimizes it but may leave it violated, or even let it get worse. |

`DEFAULT` exists so that a constraint that is already violated does not make the
whole problem infeasible: instead of failing, Rebalancer fixes the violation as
much as it can while guaranteeing it never gets worse.

## Handling a broken constraint

Under `DEFAULT` (when the constraint is initially broken) and under `SOFT`,
Rebalancer adds a **fix-it goal** that minimizes how much the constraint is
violated, by adding the following penalty term to the objective:

```
penalty = invalidCost  * (amount by which the constraint is violated)
        + invalidState * step(violation)
```

`step(violation)` is 1 while the constraint is violated and 0 once it is
satisfied. So `invalidCost` (default 100) rewards *incrementally* reducing the
violation, and `invalidState` (default 10000) rewards *fully* fixing it. Set
`invalidState` to 0 to drop the second term.

By default the fix-it goal goes into tuple position 0 (the highest priority), so
fixing a broken constraint takes precedence over everything else. If you organize
goals into [tuples](goal-priorities) and want the fix to compete at a lower
priority instead, set `tuplePosIfBroken` to that position.

Under `DEFAULT`, a broken constraint additionally keeps a **"do not make it
worse" hard constraint**: its violation may not exceed its initial level, so an
already-broken constraint is never made worse while the fix-it goal works it down.
`SOFT` does not add this, so a soft constraint may get worse. The solver does so
only when worsening it helps a higher- or equal-priority goal enough to improve
the overall objective.

## Tuning parameters

Three parameters tune the fix-it goal above:

| Parameter | Default | Description |
|-----------|---------|-------------|
| `invalidCost` | 100 | Weight for incrementally reducing a broken constraint's violation. |
| `invalidState` | 10000 | Weight for fully fixing a broken constraint (the second term above; set to 0 to disable). |
| `tuplePosIfBroken` | 0 | Tuple position for the fix-it goal. |

To set them globally, pass a `ConstraintParams` (the struct holding these three
fields) to `solver.setDefaultConstraintParams(...)`. To override them for a single
constraint, pass them as individual arguments to `addConstraint`:

```cpp
solver.addConstraint(
    spec,
    ConstraintPolicy::DEFAULT,
    /*invalidCost=*/100,
    /*invalidState=*/10000,
    /*tuplePosIfBroken=*/0);
```

## Source

- Thrift definitions: [`interface/thrift/Types.thrift`](https://github.com/facebookincubator/rebalancer/blob/main/algopt/rebalancer/interface/thrift/Types.thrift) (`ConstraintPolicy`, `ConstraintParams`)
- API: [`interface/ProblemSolver.h`](https://github.com/facebookincubator/rebalancer/blob/main/algopt/rebalancer/interface/ProblemSolver.h) (`setConstraintPolicy`, `setDefaultConstraintParams`, `addConstraint`)
