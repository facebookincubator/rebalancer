---
name: rebalancer
description: Debugging tool for rebalancer constraint violations and move failures using eval_move, list_server_moves, and query_mip CLIs.
metadata:
  oncalls:
    - algopt
  strict: true
---

# Rebalancer Debugging — eval_move CLI

A CLI for debugging why the rebalancer made (or didn't make) certain moves.
It talks to the Rebalancer Explorer backend to evaluate hypothetical moves
and inspect constraint/objective impacts.

`eval_move` works for **ALL** rebalancer run types — SM, RAS, and any other.

## What is Rebalancer? (30-second version)

- **Objects** (e.g. servers, shards) get placed into **containers** (e.g. reservations, hosts)
- **Constraints** = hard rules that cannot be broken (value 0 = OK, value > 0 = violated)
- **Objectives** = soft goals to minimize (lower = better)
- The solver tries to find the best assignment that satisfies all constraints
- A **move** changes which container an object is in
- When a move **violates a constraint**, it is blocked

## Invocation

Build all tools once (abbreviated as `$EVAL`, `$LSM`, `$QM` in examples):

```bash
EVAL=$(buck build @fbcode//mode/dev fbcode//algopt/rebalancer/fb/tools:eval_move --show-full-output 2>/dev/null | awk '{print $2}')
LSM=$(buck build @fbcode//mode/dev fbcode//algopt/rebalancer/fb/tools:list_server_moves --show-full-output 2>/dev/null | awk '{print $2}')
QM=$(buck build @fbcode//mode/dev fbcode//algopt/rebalancer/fb/tools:query_mip --show-full-output 2>/dev/null | awk '{print $2}')
DIAG=$(buck build @fbcode//mode/dev fbcode//algopt/rebalancer/fb/tools:diagnose_tree --show-full-output 2>/dev/null | awk '{print $2}')
```

### eval_move — Evaluate a hypothetical move
```bash
$EVAL --run-id <RUN_ID> --variable <OBJECT_NAME> --container <CONTAINER_NAME>
```

### list_server_moves — List solver moves involving a server
```bash
# List all solver moves to/from a server
$LSM --run-id <RUN_ID> --server <SERVER>

# Look up which server a shard is on (source + destination)
$LSM --run-id <RUN_ID> --shard <SHARD_NAME>

# Compute exact MOVED_DATA utilization for a server (solver moves only)
$LSM --run-id <RUN_ID> --compute-moved-data <SERVER>

# Compute MOVED_DATA including a proposed additional move
$LSM --run-id <RUN_ID> --compute-moved-data <SERVER> --proposed-shard <SHARD>
```

### query_mip — Query moves in progress on a server
```bash
# Find shards with has_move_in_progress=1 on a server (both src and dst directions)
$QM --run-id <RUN_ID> --server <SERVER>
```

### diagnose_tree — Walk the expression tree for blocking expressions
```bash
# Auto-detect blockers and walk their trees
$DIAG --run-id <RUN_ID> --variable <OBJECT_NAME> --container <CONTAINER_NAME>

# Walk a specific expression by name
$DIAG --run-id <RUN_ID> --variable <OBJECT_NAME> --container <CONTAINER_NAME> \
    --expression-name "per_server_max_move_"

# Control tree depth (default: 5)
$DIAG --run-id <RUN_ID> --variable <OBJECT_NAME> --container <CONTAINER_NAME> --max-depth 8
```

Use raw variable/container names exactly as shown in the Explorer UI.

## Specifying a Run

Every command needs a rebalancer run to inspect:

```bash
$EVAL --run-id <UUID> --variable <OBJECT_NAME> --container <CONTAINER_NAME>
```

Get the run ID from the Explorer URL, Scuba, or the user.

## How to Read Results

### Constraint Status

| Status | Meaning | Emoji |
|--------|---------|-------|
| `OK` | Satisfied before and after the move | ✅ |
| `BROKEN` | Was OK, now violated — **this blocks the move** | ❌ |
| `FIXED` | Was violated, now OK — move helps | ✅ |
| `STILL_BROKEN` | Violated before and after | ⚠️ |

A move is blocked if **any** constraint has status `BROKEN`.

### Constraint Values

- `0` = constraint is satisfied
- `> 0` = constraint is violated (the number = how much it's violated by)
- `Delta` = change caused by the move (positive = got worse)

### Objective Direction

- `improved` = value decreased (good — objectives are minimized)
- `regressed` = value increased (bad — objective is now violated)
- `unchanged` = no change

### Objective Values

- `0` = objective is satisfied (not violated)
- `> 0` = objective is violated
- `Delta > 0` = the move causes a new violation or worsens an existing one

**Important:** An objective delta of +1 means the objective becomes violated — it does NOT mean "one more move" or a count increase. Objectives follow the same value semantics as constraints: 0 = satisfied, >0 = violated.

### Important: Assignment Base Matters

When evaluating a move, the **base** determines what you're comparing against:

- **`--src-base INITIAL`** (default): Baseline is the assignment BEFORE the solver ran. Comparing INITIAL vs FINAL+move shows ALL changes including what the solver did. This matches the Explorer UI's default.
- **`--src-base FINAL`**: Baseline is the assignment AFTER the solver ran. Comparing FINAL vs FINAL+move shows ONLY the marginal impact of your move.

**Always use the default (INITIAL → FINAL)** to match what the Explorer shows. Use FINAL → FINAL only when you want to isolate the marginal effect of a single move.

### Verdicts

| Verdict | Meaning |
|---------|---------|
| `SAFE` | Move doesn't break any constraint and doesn't regress objectives |
| `BLOCKED_BY_CONSTRAINT` | Move violates one or more constraints |
| `OBJECTIVE_REGRESSION` | Constraints OK but objectives get worse |

## How to Read Tree Output

The `diagnose_tree` tool outputs a nested JSON tree for each blocking expression. Each node contains:

| Field | Meaning |
|-------|--------|
| `expressionType` | Computation type (e.g., `LinearSum`, `AnyPositive`, `ObjectLookup`) |
| `sourceValue` | Value at the source (baseline) assignment |
| `destinationValue` | Value at the destination (override) assignment |
| `delta` | `destinationValue - sourceValue` — the change caused by the move |
| `coefficient` | This node's weight in its parent's computation |
| `properties` | Metadata (scope item, dimension, container) — always report when present |
| `children` | Child nodes (only those with `|delta| > 1e-9` are included) |

**Blame path**: Starting from the root, follow the children with the largest `|delta|` at each level. This traces the primary contributor from the top-level expression down to the leaf-level physical entity.

For deeper understanding of how specs map to tree structures and how to derive construction rationale, see `references/diagnosis_guide.md`.

## Critical Concept: Constraint vs Objective Duality

Some expressions (like `per_server_max_move_`) appear as **BOTH** a constraint AND an objective.

- As a **constraint**: tracks the global total violation across all servers (e.g., baseline=21000 means 21000 violations system-wide)
- As an **objective** (tuple_index=0): tracks the marginal cost of YOUR specific move

**Why this matters:** A move can show `per_server_max_move_` as `STILL_BROKEN` on the constraint side (global violation unchanged) but `regressed` on the objective side (your move causes a violation, going from 0 to 1). Since tuple_index=0 objectives have strict lexicographic priority, any regression there blocks the move.

**Verdict = `OBJECTIVE_REGRESSION`** means: no constraint was newly broken, but a high-priority objective got worse. The solver treats this as "not worth doing."

## Hidden Factors: Moves In Progress and Avoid Assignments

**ALWAYS check for moves in progress and avoid assignments when debugging a violation.** These are pre-existing conditions that affect constraints and objectives but do **NOT** appear in the Rebalancer Explorer UI's move diff (INITIAL→FINAL). If the math from solver moves alone doesn't add up, these are the most likely explanation.

### Moves In Progress
- Shards with `has_move_in_progress=1` in the Explorer data table have an active move that hasn't completed yet.
- These contribute to `MOVED_DATA` utilization on both their source and destination servers.
- They are **not** included in `getMovesBetweenAssignmentsV2` (INITIAL→FINAL), so they are invisible when counting solver moves.
- To find them: query the Explorer data table for shards on a server with `has_move_in_progress >= 1`.

### Avoid Assignments
- Some objects have avoid-assignment directives that prevent them from being placed on specific containers.
- These can cause constraints or objectives to fire even when the move count appears under the limit.
- Like moves in progress, they are baked into the problem definition and not visible in the solver's move diff.

**When the violation math doesn't add up from solver moves alone, always check these hidden factors before concluding there's a bug.**

## How to Read Specs

The `eval_move` tool automatically fetches the **spec** (configuration) for each blocking expression. The `"specs"` section in the output contains the parsed spec JSON for each regressed objective and each `BROKEN` constraint.

**Always start with the spec when explaining a violation.** The spec tells you exactly what the expression is configured to enforce — its partition, dimension, limits, and filters. This is the source of truth.

### Spec Structure

An expression that is both an objective and a constraint will have both entries:
```json
{
  "specs": {
    "per_server_max_move_": {
      "objective": {
        "type": "objective",
        "weight": 1.0,
        "tuple_index": 0,
        "spec": { ... }
      },
      "constraint": {
        "type": "constraint",
        "invalid_cost": ...,
        "invalid_state": ...,
        "policy": "...",
        "spec": { ... }
      }
    }
  }
}
```

An expression that is only a constraint or only an objective will have a single entry:
```json
{
  "specs": {
    "BALANCE_LOADlower_bound_region_gbl_flash_storage": {
      "type": "constraint",
      "invalid_cost": ...,
      "invalid_state": ...,
      "policy": "...",
      "spec": { ... }
    }
  }
}
```

### Common Spec Types

For a **complete catalog of all 51 spec types** with their thrift definitions, enum values, and constraint-vs-goal usage, see `references/spec_catalog.md`.

| Spec Type | Key Fields | What It Enforces |
|-----------|------------|-----------------|
| `CapacitySpec` | `scope`, `dimension`, `definition`, `bound`, `limit` | Bounds utilization on scope items. The `definition` and `bound` fields control what is measured and in which direction — read them carefully. |
| `GroupMoveLimitSpec` | `partitionName`, `dimension`, `limit`, `sourceScopeItemsAffectingLimitFilter`, `destinationScopeItemsAffectingLimitFilter` | Limits how many objects can move within each group of a partition |
| `BalanceLoadSpec` | `scope`, `dimension`, `lowerBound`, `upperBound` | Keeps load balanced across scope items within bounds |

### CapacitySpec: `definition` and `bound` Fields

The `CapacitySpec` has two critical enum fields that determine its semantics. **Always read both** before interpreting the spec.

**`definition`** (what is measured):

| Value | Name | Meaning |
|-------|------|---------|
| 1 | `AFTER` | Utilization after all moves are applied |
| 2 | `DURING_AND_AFTER` | Utilization during and after moves |
| 3 | `DURING` | Utilization only during moves (transient state) |
| 4 | `DOUBLE_DURING_AND_AFTER` | Double utilization during and after |
| 5 | `DOUBLE_DURING` | Double utilization during moves |
| 6 | `NEW` | Utilization from newly assigned objects only |
| 7 | `OLD` | Utilization from objects in their original assignment |
| 8 | `MOVED_DATA` | Utilization from **only the data that moved** — affects **BOTH the source and destination** containers (not just the destination) |

**`bound`** (direction of the limit):

| Value | Name | Meaning |
|-------|------|---------|
| 1 | `MAX` | Upper bound — utilization must not **exceed** the limit |
| 2 | `MIN` | Lower bound — utilization must not fall **below** the limit |

**Example: `per_server_max_move_`**
- `definition=MOVED_DATA` + `bound=MAX` + `dimension=__shard___count__` + `scope=__server__`
- Meaning: "The total shard count of **moved data** on each server must not exceed the per-server limit"
- **Critical:** `MOVED_DATA` charges **both** the source and destination servers. Moving a shard from server A to server B increases the moved-data count on **both** A and B.
- **Critical:** `MOVED_DATA` includes **moves in progress** (pre-existing moves that haven't completed yet). These are shards with `has_move_in_progress=1` in the Explorer data table. They contribute to the moved-data count but do NOT appear in the INITIAL→FINAL move diff. Always check for moves in progress when the math doesn't add up.
- When debugging a violation, always check the **source server's** limit and existing move count — the violation often comes from the source side, not the destination.
- With `scopeItemLimits={"<server_hash_1>": 20, "<server_hash_2>": 19}`, each server has its own limit. The violation formula is `max(0, utilization - limit)` — the spec only breaks when utilization **strictly exceeds** the limit, not when it equals it.

### How to Use Specs in Explanations

When explaining a violation:
1. **Read the spec first** — identify the spec type, partition/scope, dimension, and limits
2. **Translate to plain language** — e.g., "This spec limits the number of shard moves per server group to X"
3. **Connect to the violation** — e.g., "Your move would exceed this limit on the target server's group"
4. **Then reference the values** — show the baseline → override → delta from the eval_move output

## Analysis Priority: Always Check Objectives First

When analyzing `eval_move` output, **ALWAYS check objectives before constraints**, regardless of the verdict. The solver uses lexicographic priority, so a tuple_index=0 objective regression is the **most important** blocker — even if there are also `BROKEN` constraints.

**Priority order for identifying the primary blocker:**
1. **tuple_index=0 objective regressions** (e.g. `per_server_max_move_`) — highest priority, always report first
2. **Higher tuple_index objective regressions** — next priority
3. **`BROKEN` constraints** — report after objectives

**Why this matters:** A move may show multiple `BROKEN` constraints AND an objective regression. Even if you fixed all the `BROKEN` constraints, the move would STILL be blocked by the tuple_index=0 objective regression. Reporting `BROKEN` constraints as the primary blocker is misleading — the objective regression is the dominant reason the solver rejects the move.

## Debugging Recipes

### Recipe 1: "Why is my move blocked?"

1. Get the run ID (from Explorer URL or ask the user)
2. Get the exact variable and container names (from Explorer UI or user)
3. Run:
   ```bash
   $EVAL --run-id $RUN_ID --variable "$VARIABLE" --container "$CONTAINER"
   ```
4. **Identify what's broken** (always check objectives first, regardless of verdict):
   - Look for objectives with `direction: regressed`. If any have `tuple_index: 0`, this is the **primary** blocker due to strict lexicographic priority.
   - Then look for constraints with status `BROKEN`.
   - If both exist, the objective regression is the primary reason, broken constraints are secondary.
   - If verdict is `SAFE` → the move is not blocked. **If the user is asking why the solver didn't make this move**, follow Recipe 4 ("Why didn't the solver make this SAFE move?").
5. **Check the spec of the broken expression** (in the `"specs"` section of the output):
   - Find the primary blocking expression's spec
   - Read the spec type, partition/scope, dimension, and limits to understand what it enforces
6. **MANDATORY: Deep investigation** — do NOT just state the spec limits and stop. You MUST run the companion tools to get actual numbers:
   - **If `per_server_max_move_` fires** → follow Recipe 2 in full (look up source server, list solver moves, query MIP, compute MOVED_DATA, build the math)
   - **If a capacity/balance constraint fires** → use `$LSM --shard` to find the source server, then investigate utilization on both sides
   - **For any blocking expression**, always present a complete accounting: `existing_utilization + proposed_change` vs `limit`, with concrete numbers
7. **Explain from the spec with concrete numbers**:
   - Translate the spec to plain language
   - **Show the actual math**: e.g., "Server X has 15 solver moves + 6 moves in progress = 21 MOVED_DATA. Limit is 20. Adding your move would make it 22, exceeding the limit by 2."
   - Reference the baseline → override → delta values as supporting evidence
   - Identify **which server** (source or destination) is the one causing the violation
8. Example explanation format: "The primary blocker is `per_server_max_move_` (tuple_index=0 objective). According to its spec, this is a `CapacitySpec` with `definition=MOVED_DATA` and `bound=MAX` on `scope=__server__` with `dimension=__shard___count__`. The source server [X] has limit=[N]. Solver moves involving this server: [M1] from + [M2] to = [M] total. Moves in progress: [P]. Total MOVED_DATA = [M+P] = [T]. Your proposed move would increase this to [T+1], which exceeds the limit of [N]. Additionally, 5 `BALANCE_LOAD` constraints would break (secondary)."
9. **If the blocking expression's value is still unclear after spec analysis**, follow Recipe 9 (Expression Tree Diagnosis) to walk the expression tree and trace the blame path to the leaf-level cause.

### Recipe 2: "Why does per_server_max_move_ fire?"

This is a move-limiting expression that uses a `CapacitySpec` with `definition=MOVED_DATA` and `bound=MAX`.

**What the spec means**: It's a `CapacitySpec` (not a `GroupMoveLimitSpec`) with:
- `definition=MOVED_DATA` — measures only the data that has been moved (not the total load)
- `bound=MAX` — upper bound (must not exceed the limit)
- `dimension=__shard___count__` — measured in shard count units
- `scope=__server__` — applied per server
- `scopeItemLimits` — per-server limits (e.g., 20 for a given server)

**Key insight**: `per_server_max_move_` is registered as BOTH a constraint AND an objective.
- As a **constraint**: tracks the global total violation across all servers (may already be non-zero)
- As an **objective** (tuple_index=0): tracks the marginal cost of a specific move
- A move can show the constraint as `STILL_BROKEN` (global, unchanged) but the objective as `regressed` (going from 0 to 1, meaning it becomes violated)
- Since tuple_index=0 has strict lexicographic priority, ANY regression (any new violation) blocks the move

**MOVED_DATA formula**: `MOVED_DATA = afterExpr + initialExpr - 2 * stayedExpr`. In practice, for shard count, this equals the total number of shards that moved to or from a server (solver moves + moves in progress).

**Debugging steps — MANDATORY deep investigation:**

You MUST run ALL of these steps. Do NOT stop at step 3 and guess — always get the actual numbers.

1. **Run `eval_move`** to get the verdict, specs, and the configured limit:
   ```bash
   $EVAL --run-id $RUN_ID --variable "$OBJECT" --container "$CONTAINER"
   ```
   Extract the `scopeItemLimits` for both source and destination servers from the spec.

2. **Look up the shard's source server** using `list_server_moves --shard`:
   ```bash
   $LSM --run-id $RUN_ID --shard "$OBJECT"
   ```
   This returns the shard's `src.__server__` and `dst.__server__`, giving you the source server hash.

3. **For BOTH source and destination servers**, run these three commands:

   a. **List solver moves** (moves in the INITIAL→FINAL diff):
      ```bash
      $LSM --run-id $RUN_ID --server <SERVER>
      ```
      This shows `moves_to_server`, `moves_from_server`, and `total_moves_involving_server`. These are the solver's planned moves.

   b. **Query moves in progress** (pre-existing moves not in the solver diff):
      ```bash
      $QM --run-id $RUN_ID --server <SERVER>
      ```
      This shows shards with `has_move_in_progress=1` both FROM and TO this server. These contribute to MOVED_DATA but are **invisible** in the INITIAL→FINAL move list.

   c. **Compute MOVED_DATA utilization** (exact accounting including the proposed move):
      ```bash
      $LSM --run-id $RUN_ID --compute-moved-data <SERVER> --proposed-shard "$OBJECT"
      ```
      This outputs `total_moved_data` (solver moves only) and `total_with_proposed` (solver + your move). Note: this does NOT include moves in progress — you must add those from step 3b.

4. **Build the complete math** for each server:
   ```
   Total MOVED_DATA = solver_moved_data + moves_in_progress_count + proposed_move
   Violation = max(0, Total_MOVED_DATA - scopeItemLimit)
   ```

   For each server, present a table like:
   | Component | Count | Source |
   |-----------|-------|--------|
   | Solver moves (from server) | X | list_server_moves |
   | Solver moves (to server) | Y | list_server_moves |
   | Moves in progress (from) | P1 | query_mip |
   | Moves in progress (to) | P2 | query_mip |
   | **Subtotal (existing)** | **X+Y+P1+P2** | |
   | Proposed move | +1 | your move |
   | **Total MOVED_DATA** | **X+Y+P1+P2+1** | |
   | **Limit** | **L** | scopeItemLimits |
   | **Over limit by** | **max(0, total-L)** | |

5. **Identify which server causes the violation**: The violation comes from whichever server's total MOVED_DATA exceeds its `scopeItemLimit`. It may be the source, destination, or both.

6. **Present the complete explanation** with all numbers:
   ```
   The primary blocker is `per_server_max_move_` (tuple_index=0 objective).

   Source server [SRC_HASH] (limit=[N]):
   - Solver moves: [X] from + [Y] to = [Z] total
   - Moves in progress: [P] shards
   - Current MOVED_DATA: [Z+P]
   - With proposed move: [Z+P+1]
   - Status: [OVER/UNDER] limit by [diff]

   Destination server [DST_HASH] (limit=[M]):
   - Solver moves: [A] from + [B] to = [C] total
   - Moves in progress: [Q] shards
   - Current MOVED_DATA: [C+Q]
   - With proposed move: [C+Q+1]
   - Status: [OVER/UNDER] limit by [diff]

   The violation comes from [SOURCE/DESTINATION/BOTH] server(s).
   ```

### Recipe 3: "What constraint is blocking the most moves?"

1. Identify the set of objects/containers you care about (from Explorer UI or user)
2. Run `eval_move` for each candidate:
   ```bash
   $EVAL --run-id $RUN_ID --variable "$OBJECT" --container "$CONTAINER"
   ```
3. Tally which constraint appears most often with status `BROKEN`
4. Explain what the dominant constraint means:
   - `rack_capacity_*` — rack is full
   - `subtype_*` — object type doesn't match what's needed
   - `per_server_max_move_*` — too many moves already
   - `failure_domain_*` — would put too many objects in one failure domain

### Recipe 4: "Why didn't the solver make this SAFE move?"

Sometimes `eval_move` with the default INITIAL → FINAL base returns `SAFE`, meaning the move is not blocked — yet the solver didn't include it. This happens because the solver is an optimizer: it only makes moves that **improve objectives**. A `SAFE` verdict means the move *wouldn't break anything*, but the solver may still skip it if the move provides no marginal benefit.

**IMPORTANT:** When the INITIAL → FINAL eval returns `SAFE` and the user is asking why the solver didn't make the move, you MUST proceed with these steps. Do NOT just report the move as SAFE and stop.

**Why INITIAL → FINAL can be misleading for SAFE moves:**
The INITIAL → FINAL comparison shows what would happen if this move were added on top of the solver's FINAL solution. `STILL_BROKEN` constraints with large negative deltas may appear to show huge improvements — but those improvements were already achieved by **other moves the solver made**. This move is being falsely credited for work that other moves did. To understand the true marginal impact, you MUST run the FINAL → FINAL comparison.

**Debugging steps:**

1. **Confirm SAFE with INITIAL → FINAL** (default eval_move):
   ```bash
   $EVAL --run-id $RUN_ID --variable "$OBJECT" --container "$CONTAINER"
   ```
   If verdict is not `SAFE`, use Recipe 1 or 2 instead.

2. **Run FINAL → FINAL to see the true marginal impact**:
   ```bash
   $EVAL --run-id $RUN_ID --variable "$OBJECT" --container "$CONTAINER" --src-base FINAL --dst-base FINAL
   ```
   This compares the solver's final assignment with vs without your move — isolating the pure marginal effect.

3. **Analyze the marginal impact**:
   - **Zero improved objectives**: The move provides no benefit. The solver has no reason to make it.
   - **Tiny improved objectives**: The improvement is too small to justify the move cost. The solver prioritizes moves with higher impact.
   - **Some improved + some regressed objectives**: The move has trade-offs. The solver found the trade-off wasn't worth it (e.g., improving a low-priority objective while worsening a higher-priority one).
   - **Improved objectives but still not made**: The solver may have hit its move budget or time limit before getting to this move.

4. **Present the explanation** comparing both perspectives:
   ```
   INITIAL → FINAL verdict: SAFE
   This comparison shows large improvements, but these are misleading — they
   were already achieved by other solver moves.

   FINAL → FINAL verdict: SAFE
   Marginal impact of this specific move on top of the solver's solution:
   - Improved objectives: [list, or "none"]
   - Regressed objectives: [list, or "none"]
   - Changed constraints: [list, or "none"]

   The solver did not make this move because [reason]:
   - "it provides zero marginal improvement to any objective"
   - "the marginal improvement is extremely small (delta = X) and not worth the move cost"
   - "it improves [obj_A] but regresses [obj_B] which has higher priority"
   - "the solver likely ran out of move budget or time before reaching this move"
   ```

**Common root causes for SAFE-but-not-made:**

| Scenario | Marginal Objectives | Marginal Constraints | Explanation |
|----------|--------------------|--------------------|-------------|
| Zero benefit | No change | No change | Move is neutral — solver has no reason to make it |
| Tiny benefit | Improved by < 0.01 | Minor improvement | Improvement too small to justify a move |
| Trade-off | Mixed improved/regressed | Mixed | Net effect is negative or neutral |
| Budget exhausted | Improved | No new violations | Solver ran out of moves or time; this move was lower priority |
| Precision truncation | No change (but expected) | Near-zero change | Improvement exists but falls below precision tolerance |

**Precision truncation:** The solver uses `1e-10` default precision. If improvements fall below this, they're treated as zero. Additionally, eval_move hides objectives with `|delta| < 1e-9`. See `references/precision_tolerance_guide.md` for the full precision comparison logic, verification commands, calculation templates, and recommended actions table.

**Key pattern: "zero objectives but constraint improvements visible"** — FINAL → FINAL shows 0 objectives but constraint deltas (e.g., small negative values) → sub-`1e-9` objective improvements are hidden. Recommend `1e-16` precision.

**MANDATORY:** When FINAL → FINAL shows near-zero improvement, compute the required precision tolerance using the formula in `references/precision_tolerance_guide.md`. Do NOT suggest arbitrary values.

**Diagnostic checklist:**
1. ☐ Check FINAL → FINAL marginal impact (step 2 above)
2. ☐ If zero objectives → check PrecisionTolerances truncation (recommend `1e-16`)
3. ☐ If tiny improvement → check MinCycleObjectiveImprovement thresholds
4. ☐ If real improvement → solver hit move budget or time limit
5. ☐ **MANDATORY: Verify with standalone solver** — if you diagnose precision truncation (checklist item 2), you MUST run the standalone solver with both default and `1e-16` precision to confirm the diagnosis empirically. Do NOT just report the precision hypothesis without testing it. See step 5 below.

**Step 5: Verify precision truncation with standalone solver**

When you diagnose precision truncation (zero objectives but constraint improvements visible), you MUST run the standalone solver to confirm:

```bash
# Build the standalone solver once (abbreviated as $SOLVER in examples)
SOLVER="buck2 run @fbcode//mode/opt fbcode//algopt/rebalancer/interface/standalone:standalone_solver --"

# Run with 1e-16 precision
$SOLVER --run-id $RUN_ID \
  --precision_tolerance_absolute 1e-16 \
  --precision_tolerance_relative 1e-16 \
  2>&1 | tee /tmp/solver_1e16.log

# Run with default precision for comparison
$SOLVER --run-id $RUN_ID \
  2>&1 | tee /tmp/solver_default.log

# Compare move counts
grep "moves applied" /tmp/solver_1e16.log /tmp/solver_default.log
```

**How to interpret results:**
- If `1e-16` produces significantly more moves → precision truncation confirmed. Report the actual move counts.
- If move counts are similar → precision is NOT the issue. Re-examine other causes (move budget, time limit, MinCycleObjectiveImprovement).

**IMPORTANT:** Always run BOTH solvers and report the actual move counts to the user. Do NOT skip this step or substitute it with theoretical analysis. The standalone solver is the ground truth.

Note: Large problems (hundreds of thousands of objects) can take up to 7200s to solve. Run both solver commands in parallel to save time. For quick verification, compare Stage-0 move counts.

See `references/precision_tolerance_guide.md` for the full precision comparison logic, calculation templates, and recommended actions table.

### Recipe 5: "Why is the solver not placing objects in all required regions?"

**Scenario**: A GROUP_ROUTING move type is configured to place objects (e.g., tenant replicas) in all demand regions, but the solver only places in a subset. The user expects placement in all regions except those where capacity is explicitly unavailable.

**Common root causes:**
1. **GROUP_ROUTING is all-or-nothing**: The `GroupRoutingMoveType` generates a single MoveSet containing moves to ALL required destinations. If ANY destination violates a hard constraint (e.g., capacity), the **entire MoveSet is rejected** — including valid placements to other regions.
2. **Non-accepting containers**: Some regions may be marked as non-accepting (`non_accepting_containers` constraint), which prevents any placement there.
3. **Capacity limits**: A region may have initial utilization exceeding its upper limit (e.g., 44% usage with 10% limit), so the solver can't place more objects there.

**Debugging steps:**
1. Run `eval_move` for a manual placement of one object to one missing region. If it shows constraint violations, identify which constraint blocks it.
2. Check if the blocked region's constraint is the reason GROUP_ROUTING rejects the entire batch.
3. Check if a SINGLE_FAST stage is configured as a fallback (stage1). If so, individual placements may still happen via that stage.
4. **Solution**: If GROUP_ROUTING fails due to one region, the solver falls back to whatever move types are configured in subsequent stages (e.g., SINGLE_FAST in stage1). If no subsequent stage is configured, no moves happen. Consider increasing capacity in the blocked region or adding SINGLE_FAST as a fallback stage.

### Recipe 6: "Explorer shows improvement but solver didn't make the move"

**Scenario**: A user manually tests a move in Rebalancer Explorer (FINAL → FINAL) and sees it improves soft threshold or other objectives. But the solver didn't make this move.

**Common root causes:**
1. **Precision truncation**: The improvement is too small for the solver to detect (below `1e-10` default precision). See Recipe 4 for detailed precision analysis.
2. **Missing stage configuration**: The solver stage that should evaluate this type of move may not be configured to optimize the relevant objective. Check the solver's stage configuration — the objective must be included in the stage's tuple position range.
3. **Sampling**: If the problem has many objects but the sampling size is small, the solver may never evaluate this particular move. Check `sample_size` vs `server_count`.
4. **Move type mismatch**: The stage may be configured with a move type (e.g., GROUP_ROUTING) that can't produce this specific single-object move.

**Debugging steps:**
1. Follow Recipe 4 (SAFE-but-not-made) to check marginal impact and precision.
2. Verify the stage configuration includes the relevant objective in its tuple position range.
3. Check if sampling might be excluding the object.

### Recipe 7: "Unexpected move — why did the solver make this move?"

**Scenario**: A user observes a move that seems unnecessary or counterproductive. They want to understand why the solver chose to make it.

**Common root causes:**
1. **Tiny objective improvement**: The move improves an objective by a very small amount (e.g., `1e-7`). The solver accepts any improving move, even tiny ones.
2. **Object not in expected partition**: An object may be unexpectedly included in a partition (e.g., due to a non-optional thrift field being initialized to empty string instead of being absent), causing a goal to evaluate it when it shouldn't.
3. **Floating-point artifacts**: Very small deltas (near machine epsilon) can cause the solver to accept moves that have no meaningful effect.

**Debugging steps:**
1. Use `eval_move` to evaluate the **reverse** move (undo) to see which objective regresses when the move is undone:
   ```bash
   $EVAL --run-id $RUN_ID --variable "$OBJECT" --container "$ORIGINAL_CONTAINER"
   ```
2. Identify the objective that improved. Check if the delta is very small (< `1e-6`).
3. If the improvement is in an unexpected goal (e.g., object shouldn't be part of that goal's partition), investigate the partition membership. Check if empty/default values are causing unexpected inclusion.
4. **Solution**: Fix the partition membership logic, or increase the solver's precision tolerance to ignore tiny improvements.

### Recipe 9: "Deep-diving into an expression value (Expression Tree Diagnosis)"

**When to use**: After `eval_move` identifies a blocking expression but the reason for its value is unclear from the spec alone. The expression tree reveals the internal computation hierarchy — which sub-expressions contribute, their coefficients, and their individual deltas.

**Debugging steps:**

1. **Run `eval_move`** to get the verdict, blocking expression name, AND its spec JSON:
   ```bash
   $EVAL --run-id $RUN_ID --variable "$OBJECT" --container "$CONTAINER"
   ```
   Note the blocking expression name and its spec.

2. **Run `diagnose_tree`** to get the expression tree:
   ```bash
   $DIAG --run-id $RUN_ID --variable "$OBJECT" --container "$CONTAINER" \
       --expression-name "<BLOCKING_EXPRESSION_NAME>"
   ```
   If you want all blockers, omit `--expression-name`.

3. **Follow the blame path**: Starting from the root node, at each level find the child with the largest `|delta|`. This traces the primary contributor through the tree.

4. **At each level, read `expressionType`** to understand the computation:
   - `LinearSum`: value = constant + Σ(coefficient × child). The child with the largest `|coefficient × child_delta|` is the primary contributor.
   - `AnyPositive`: the child that crossed from ≤0 to >0 caused the violation.
   - `Max`: the child that became the new maximum is the culprit.
   - `BinaryOperation`: compare both operands' deltas.
   - `Power`/`Square`: delta is amplified non-linearly.

5. **Correlate the tree with the spec**: Read the spec JSON fields (`scope`, `dimension`, `bound`, `definition`, `limits`, etc.) and use the generic materialization rules from `references/diagnosis_guide.md` to reason about WHY each tree node exists:
   - Root node type tells you the composition (`LinearSum` = summed violations, `AnyPositive` = conjunction)
   - Fan-out children correspond to scope items — check child `name` fields for scope item identifiers
   - Intermediate nodes implement the formula (`Power` = non-linear penalty, `BinaryOperation` = difference from limit)
   - Leaf `ObjectLookup` nodes correspond to the `dimension` and `scope` fields

6. **At leaf nodes**, `properties` and `name` reveal the physical entity (which scope item, dimension, container).

7. **Explain in plain language** connecting: spec intent → tree structure → leaf-level cause.
   ```
   The expression `balance_load_cpu` (BalanceLoadSpec, scope=region, dimension=cpu) has a
   LinearSum root summing per-region violations. The largest delta comes from region=us-east
   (child delta=+0.15). That child is a BinaryOperation(utilization - upperBound), where
   utilization increased from 0.82 to 0.97 (ObjectLookup leaf: your shard added +0.15 cpu
   to us-east). The upper bound is 0.90, so the violation is 0.97 - 0.90 = 0.07.
   ```

### Recipe 8: "Solver regression — suddenly making fewer moves"

**Scenario**: A solver that was working well suddenly produces significantly fewer moves. The regression correlates with a specific date or package version.

**Debugging steps:**
1. **Check the release history**: Look at the rebalancer package releases around the regression date (`rebalancer.packer` releases on Conveyor).
2. **Compare with a known-good version**: Re-run the solver with the previous package version:
   ```bash
   --packer-version <OLD_VERSION>
   ```
3. **Check config parsing**: Look at the solver logs for config file parsing failures. A code change may have broken the legacy config path (JSON files on disk) while the modern API path still works.
4. **Check for code path differences**: The rebalancer has a legacy path (JSON files) and a modern API path. Bug fixes to one may break the other. Check if the affected users are on the legacy path.
5. **Rollback**: If a regression is confirmed, roll back the package to the last known-good version and investigate the culprit diff.
## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| "Sandbox not loaded" / timeout | Run ID expired from Manifold | Find a fresh run ID from Explorer or Scuba |
| "Variable not found" | Object name format wrong | Use exact names from the Explorer UI |
| "Container not found" | Container name format wrong | Use exact names from the Explorer UI |
| Verdict is `OBJECTIVE_REGRESSION` not `BLOCKED_BY_CONSTRAINT` | Expression is both a constraint AND objective; the objective regresses | Check the objectives list, not just constraints. See "Constraint vs Objective Duality" section |
| `evaluate` shows SAFE but system says blocked | Different run or stale data | Verify run ID matches what the system used. Check `--src-base` and `--dst-base` |
| Slow sandbox loading | Large problem instance | Increase timeout: `--timeout 300` |
| Solver moves objects to over-capacity containers | Load is tiny relative to capacity, so floating-point comparison can't detect the overload (e.g., load=105 vs capacity=5.3e12) | Mark over-capacity containers as non-accepting in the problem setup |
| No moves despite unassigned objects and apparent improvements in Explorer | One object has extreme dimension values (e.g., CPU >1e15) dwarfing all others (1e5). Normalization makes smaller objects insignificant. | Remove the extreme object, or separate its goal into a different tuple. Sort by dimension in Explorer to find outliers. |
| Constraint appears violated in production but Explorer shows it improving | Constraint was **initially broken**. Default policy splits it into an objective (minimize violation) + constraint (don't worsen). The objective is improving but can't reach zero due to other constraints or time limits. | Check if the constraint has both an objective and constraint entry in Explorer. The initial violation is expected — look at the objective's improvement instead. See `ConstraintPolicy` in Types.thrift. |
| Solver proposes many moves internally but applies zero | Numerical instability — objective values span too many orders of magnitude (e.g., 1e16). MIP solver proposes moves but rebalancer rejects them as numerical artifacts. | Rescale dimension values so the coefficient range is < 1e9 (ideally < 1e6). Check solver logs for coefficient range warnings. |
| Balance spec `upperBound` seems ineffective | Average utilization is very low, so even a high relative bound maps to a low absolute threshold that many containers already exceed. E.g., avg=0.22, bound=1.9 → threshold=0.42. | Increase the bound further, or increase the move budget. Check actual average utilization to understand the threshold. |
| Problem declared infeasible intermittently | MIP solver (Gurobi/Xpress) is sensitive to seed and tolerance values. Tight tolerances can put the problem "on the edge". | Try different solver seeds, upgrade solver version (newer versions have better multi-objective handling), or relax tolerances slightly. |

## Naming Conventions

Variable and container names depend on the run type. **Always use the exact names shown in the Explorer UI.**

| Run Type | Object (variable) | Container | Example |
|----------|-------------------|-----------|---------|
| **SM** (shard manager) | `<shard_id>` | `<server_hash>` | `12345:0,1234500100-0,1234500100:1` → `-123456789` |
| **Other** | Varies | Varies | Check Explorer UI for exact names |

## Source

| Path | Purpose |
|------|---------|
| `algopt/rebalancer/fb/tools/eval_move.py` | Move evaluator (works for ALL run types) |
| `algopt/rebalancer/fb/tools/list_server_moves.py` | List solver moves to/from a server, lookup shard source/dest, compute MOVED_DATA |
| `algopt/rebalancer/fb/tools/query_mip.py` | Query shards with moves in progress (has_move_in_progress=1) on a server |
| `algopt/rebalancer/fb/tools/diagnose_tree.py` | Walk expression trees for blocking expressions via getTreeNodeV2 |
| `rebalancer/explorer/if/explorer.thrift` | Thrift API definition |

## References

| When to use | Reference |
|-------------|-----------|
| Plan for rebalancer debugging skill development. | [PLAN.md](references/PLAN.md) |
