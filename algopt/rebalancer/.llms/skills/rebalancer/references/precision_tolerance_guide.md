# Precision Tolerance Deep Dive

## eval_move Display Threshold Warning

**CRITICAL:** The `eval_move` tool has a `1e-9` display threshold for objectives (see `eval_move.py` line 113: `if abs(delta) > 1e-9`). Objectives with improvements smaller than `1e-9` are **completely filtered out** of the output. The tool also rounds all values to 6 decimal places (`round(value, 6)` on lines 107-109, 119-121).

This means:
- Objective improvements between `1e-16` and `1e-9` are **invisible** in eval_move output
- eval_move will report "0 objectives" even when real improvements exist
- Constraints use a different filter (`|delta| > 1e-9` OR `status != OK`), so constraint changes are more likely to be visible

**Key pattern:** If FINAL → FINAL shows 0 objectives but some constraint improvements, hidden sub-`1e-9` objective improvements almost certainly exist.

## Known Precision Values

| Precision | When to Use |
|-----------|------------|
| `1e-10` (default) | Normal operations |
| `1e-16` | When FINAL → FINAL shows 0 objectives but constraint improvements are visible. Enables the solver to detect sub-`1e-9` objective improvements that are hidden by eval_move's display threshold. |

## MANDATORY: Verify Precision Changes with Standalone Solver

**You MUST always run the standalone solver to verify a precision truncation diagnosis.** Do NOT just report the theoretical precision calculation — always back it up with actual solver runs comparing move counts. Use the standalone solver binary directly (not the perf tool, which requires commit comparison).

```bash
# Build the standalone solver once (abbreviated as $SOLVER)
SOLVER="buck2 run @fbcode//mode/opt fbcode//algopt/rebalancer/interface/standalone:standalone_solver --"

# Run with 1e-16 precision
$SOLVER --run-id <RUN_ID> \
  --precision_tolerance_absolute 1e-16 \
  --precision_tolerance_relative 1e-16 \
  2>&1 | tee /tmp/solver_1e16.log

# Run baseline (default precision) for comparison
$SOLVER --run-id <RUN_ID> \
  2>&1 | tee /tmp/solver_default.log

# Compare move counts
grep "moves applied" /tmp/solver_1e16.log /tmp/solver_default.log
```

**Run both commands in parallel** to save time — they are independent.

Note: Large problems (hundreds of thousands of objects) can take up to 7200s to solve. For quick verification, compare Stage-0 move counts — if the `1e-16` run makes significantly more moves, precision was the issue.

**How to interpret and report results:**
- If `1e-16` produces significantly more moves → precision truncation **confirmed**. Report exact move counts (e.g., "Default: X moves, 1e-16: Y moves, Z× increase").
- If move counts are similar → precision is **NOT** the issue. Re-examine other causes (move budget, time limit, MinCycleObjectiveImprovement).
- **Always report the actual numbers** from the solver runs to the user. Never substitute with theoretical analysis alone.

## Precision Tolerances

Defined in `algopt/rebalancer/algopt_common/thrift/Types.thrift`:
```thrift
struct PrecisionTolerances {
  1: double absolute = 1e-10;
  2: double relative = 1e-10;
}
```

## How Comparison Works

Implementation in `algopt/rebalancer/algopt_common/Precision.cpp`:
1. If `|value1 - value2| < absolute_epsilon` → values are treated as **equal**
2. If `|value1 - value2| * 2 / (|value1| + |value2|) < relative_epsilon` → values are treated as **equal**
3. Both checks must fail for values to be considered different

The solver treats two values as **equal** if EITHER threshold triggers:
- `abs_delta < absolute_tolerance` → treated as equal
- `rel_delta < relative_tolerance` → treated as equal

Both checks must **fail** for the solver to see the difference.

## MANDATORY: Calculate Required Precision for the Specific Move

When the FINAL → FINAL eval shows near-zero or zero improvement, you MUST compute what precision tolerance would be needed for the solver to recognize the improvement. Do NOT just suggest arbitrary values like `1e-12`.

For each objective or constraint that changed in the FINAL → FINAL eval, compute:

1. **Absolute delta**: `abs_delta = |override - baseline|`
2. **Relative delta**: `rel_delta = |override - baseline| * 2 / (|baseline| + |override|)`

For a move to be recognized:
- `absolute_tolerance` must be ≤ `abs_delta`
- `relative_tolerance` must be ≤ `rel_delta`

### Present the Calculation to the User

```
FINAL → FINAL delta for [expression_name]:
  baseline = [B], override = [O]
  absolute delta  = |O - B| = [abs_delta]
  relative delta  = |O - B| * 2 / (|B| + |O|) = [rel_delta]

Current precision tolerances (default):
  absolute = 1e-10
  relative = 1e-10

For this move to be recognized by the solver:
  absolute_tolerance must be ≤ [abs_delta]  → currently [PASSES/FAILS]
  relative_tolerance must be ≤ [rel_delta]  → currently [PASSES/FAILS]

Recommendation:
  [If both PASS]: Precision is not the issue — the solver can see the
    improvement but chose not to make the move for other reasons (e.g.,
    move budget, time limit, or cycle termination threshold).
  [If either FAILS]: Lower the failing tolerance. Set:
    --precision_tolerance_absolute [abs_delta / 10]
    --precision_tolerance_relative [rel_delta / 10]
    (Use 1/10th of the delta to provide a safety margin.)
  [If delta is exactly 0]: Precision is not the issue — the move
    genuinely has no impact on this expression.
```

### Example Calculation

```
FINAL → FINAL delta for some_capacity_constraint:
  baseline = 500.200, override = 500.150
  absolute delta  = |500.150 - 500.200| = 0.050
  relative delta  = 0.050 * 2 / (500.200 + 500.150) = 9.99e-5

Current precision tolerances (default):
  absolute = 1e-10
  relative = 1e-10

For this move to be recognized:
  absolute_tolerance must be ≤ 0.050      → 1e-10 ≤ 0.050 ✅ PASSES
  relative_tolerance must be ≤ 9.99e-5    → 1e-10 ≤ 9.99e-5 ✅ PASSES

Recommendation: Precision is not the issue — the solver can already detect
this improvement. The move was likely skipped because this is a constraint
improvement (not an objective improvement), and the solver only makes moves
that improve objectives. Check if there's a missing objective.
```

## How to Lower Precision Tolerance

1. **Via CLI flags** (for standalone solver runs):
   ```bash
   --precision_tolerance_absolute <abs_delta / 10> \
   --precision_tolerance_relative <rel_delta / 10>
   ```
   Use 1/10th of the computed delta values to provide a safety margin.

2. **Via Universe config** (for production runs):
   Set `PrecisionTolerances.absolute` and `PrecisionTolerances.relative` in the solver's Universe configuration to the computed values.

## Cycle Termination Thresholds (Separate from Precision)

The solver also has a **MinCycleObjectiveImprovement** threshold (defined in `algopt/rebalancer/interface/thrift/SolverSpecs.thrift`) that controls when the solver stops iterating. Even if precision detects an improvement, the solver may stop its search cycle if the improvement is too small:

```thrift
struct MinCycleObjectiveImprovementConfig {
  1: Types.Threshold defaultThreshold;
}

struct Threshold {
  1: optional double percent;    // e.g., 5.0 means 5% minimum improvement
  2: optional double absolute;   // e.g., 10.0 means minimum decrease of 10 units
}
```

- If `percent` is set: improvement must exceed `(decrease / |before|) * 100 > percent`
- If `absolute` is set: improvement must exceed `decrease > absolute`
- If both are set: **either** threshold being met is sufficient (OR semantics)
- Comparisons are **strict >**, not >=

If the solver is stopping early, check `MinCycleObjectiveImprovement` in the solver spec. Lowering or removing this threshold will allow the solver to continue searching for small improvements.

## Diagnostic Checklist: SAFE Move Not Made

1. ☐ Check FINAL → FINAL marginal impact
2. ☐ If marginal objectives are zero → check if PrecisionTolerances are truncating small differences
3. ☐ **If precision truncation suspected → MUST run standalone solver verification** (see "MANDATORY: Verify Precision Changes" above)
4. ☐ If marginal objectives show tiny improvement → check MinCycleObjectiveImprovement thresholds
5. ☐ If marginal objectives show real improvement → solver likely hit move budget or time limit

## Recommended Actions by Diagnosis

| Diagnosis | Recommended Action | Risk Level |
|-----------|-------------------|------------|
| **Zero marginal improvement** — no objectives change at all | Move genuinely has no benefit. **No action needed.** If the user believes it should help, the problem formulation may be missing an objective. | None |
| **Zero objectives but constraint improvements visible** — eval_move shows 0 objectives but constraints improve (e.g., small negative delta) | **Precision truncation** pattern. Recommend `1e-16` precision: `--precision_tolerance_absolute 1e-16 --precision_tolerance_relative 1e-16`. | Low |
| **Extremely small improvement** — objectives improve by < `1e-10` (below default PrecisionTolerances) | **Lower PrecisionTolerances** to `1e-16` so the solver can detect the improvement. | Low |
| **Small but detectable improvement** — objectives improve (e.g., delta `-0.001` to `-0.1`) but solver still skips | **Check MinCycleObjectiveImprovement** thresholds. Also check **time limits**. | Medium |
| **Meaningful improvement** — objectives clearly improve but move is not made | Solver likely hit **move budget** or **time limit**. Increase time limit or move budget. | Low |
| **Trade-off** — some objectives improve, others regress | Correct solver behavior. **Adjust objective weights** if the user wants the move to happen. | High |
