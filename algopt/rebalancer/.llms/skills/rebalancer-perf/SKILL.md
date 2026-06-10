---
name: rebalancer-perf
description: >
  Performance testing CLI for the rebalancer solver (algopt/rebalancer/).
  Use when benchmarking solver performance, comparing commits for regressions,
  profiling solver execution, running A/B tests, or querying the rebalancer_runs
  Scuba table. Covers local, Skycastle (distributed), and ServiceLab (isolated
  hardware) execution environments. Also handles replaying saved solver configs
  and managing predefined run collections like the pareto golden set.

  Only for the rebalancer project — not for general ServiceLab benchmarks,
  non-rebalancer services, or functional correctness testing.
metadata:
  oncalls:
    - algopt
  strict: true
---

# rebalancer_perf — Performance Testing CLI

A Click-based CLI for comparing rebalancer solver performance across commits. It replays
saved solver instances (identified by run IDs from the `rebalancer_runs` Scuba table)
on different code versions and collects timing/quality metrics.

## Invocation

Use this invocation for all examples (abbreviated as `$PERF`):

```bash
PERF="buck2 run @fbcode//mode/opt fbcode//algopt/rebalancer/fb/tools:rebalancer_perf --"
$PERF <subcommand> [options]
```

Always build with `@fbcode//mode/opt` — debug builds are dramatically slower and produce
meaningless performance numbers. The `@fbcode//` prefix ensures the mode resolves
correctly regardless of your working directory.

Run IDs vary by service — examples:
- `8eb027dd-582a-4383-9cb3-0e7fb1d5b344-allServerTypes-phase-1-out-of-1-` (reservation_allocator)
- `_spm_generated_.multifeed.ris.fb_shorts.prod.atg.1_atg_1773783058642` (shard_manager)
- `144e002e-d7ef-4941-8623-afbeb12e1c30_ADS_DI_PERF_BGM_TEST_traffic_object_solve_62bb` (global_shard_placement)

## Planning: Determine Run IDs Up Front

Resolve run IDs during planning, before executing any commands. Once run IDs,
execution environment, and commits are decided, the command is fully determined.

Gather one of these from the user:

- **Explicit run IDs** — from Scuba, a teammate, or a previous run. Ask: "Which run
  IDs do you want to test against?"
- **A collection name** — a predefined set of representative instances. Run
  `$PERF collections --verbose` to show available options and help the user pick one. The
  `pareto` collection is the golden run set used for broad validation.
- **A Scuba query** — filter by service, solver type, or custom SQL. Guide the user on
  filters: "Which service? How many runs?"
- **A CSV file** — must have columns: `runId`, `serviceName`, `solverType`.

If the user's change targets a specific service (e.g., `shard_manager`), suggest filtering
run IDs by that service so the comparison is relevant. If it's a broad solver change,
suggest the `pareto` collection for comprehensive coverage.

Top services by volume: `shard_manager`, `reservation_allocator`, `dscp/dataplacer`,
`dataplacer_grouping`, `elastic_reservation_allocator`, `global_shard_placement`.

## Choosing an Execution Environment

| Scenario | Use | Why |
|----------|-----|-----|
| Quick functional check or comparison with 1-2 run IDs | `local` | Fast iteration, runs sequentially on the dev machine |
| Accurate performance measurement for a diff | `servicelab` | Runs on isolated hardware with controlled conditions — the only way to get reliable perf numbers |
| Broad validation across many run IDs (e.g., the `pareto` golden set) | `skycastle` | Parallelizes work across shards so you can cover dozens or hundreds of run IDs in reasonable time |

**Skycastle shard guidance**: Set `--shards` equal to the number of run IDs — each shard
processes one run ID, so matching them gives the fastest completion. For example, if the
`pareto` collection has 40 run IDs, use `--shards 40`. Skycastle runs on shared
infrastructure, so timing numbers are noisier than ServiceLab — use it for functional
validation and catching large regressions, not precise perf measurement.

## Sapling Revset Syntax

This repo uses Sapling (`sl`), not git. Commit references use Sapling revsets:
- `.` — current commit (equivalent to git `HEAD`)
- `.^` — parent of current commit (equivalent to git `HEAD~1`)
- `.^^` or `.~2` — grandparent

When omitted, `--commits` defaults to comparing the current commit (`.`) against its
public ancestor — which is usually the right thing for testing a local change.

## Mental Model

Every perf run answers three questions:

| Question | Config layer | Configured via |
|----------|-------------|----------------|
| **What** to compare? | Commits + run IDs | `--commits`, run ID source subcommand |
| **Where** to run? | Execution environment | Top-level command: `local`, `skycastle`, `servicelab` |
| **How** to configure? | Tracking, output, solver params | `--track`, `--output-dir`, `--num-repeat`, etc. |

- **Ask the user** for run IDs unless already clear from context. Suggest filtering by service for targeted changes or using `pareto` for broad changes, but let the user decide.
- **Ask** when environment choice matters (speed vs. accuracy) or change scope is unclear.
- **Proceed** when context makes the choice obvious (e.g., "benchmark my diff" → `servicelab`) or the user already specified run IDs + commits + environment.
## Discovery Protocol

Consult `--help` at any level for uncommon flags or when unsure:

```bash
$PERF --help
$PERF local --help
$PERF local run-scuba --help
```

## Command Structure

### Execution Commands

Each execution command has **four sub-subcommands** for choosing the run ID source:

| Run ID source command | Source |
|-----------------------|--------|
| `run-ids <ID>...` | Explicit IDs as arguments (also `--stdin` for piped input) |
| `run-collection <NAME>` | Predefined named collection |
| `run-scuba` | Dynamic Scuba query with filters |
| `run-csv <PATH>` | CSV file (`runId`, `serviceName`, `solverType` columns) |

#### `local` — Run on this machine

Build the standalone solver at the specified commits and replay each run ID on each
commit sequentially. Best for quick checks with a handful of run IDs.

```bash
# Quick functional check with 1 shard_manager run ID
$PERF local run-ids _spm_generated_.multifeed.ris.fb_shorts.prod.atg.1_atg_1773783058642

# Compare two commits on a few Scuba-sampled reservation_allocator runs
$PERF local run-scuba --size 5 --service reservation_allocator --commits abc123,def456
```

#### `skycastle` — Distribute across Skycastle workers

Parallelize across shards for broad validation. Not accurate for precise perf
measurement (shared infrastructure), but excellent for catching regressions across
many run IDs — especially the `pareto` golden set for bigger changes.

**Prerequisites**: Skycastle workers pull code remotely, so your changes must be
committed and cloud-synced before running:

```bash
sl commit -m "my changes"
sl cloud sync
```

```bash
# Validate a big solver change against the pareto golden set (~40 run IDs)
$PERF skycastle run-collection pareto --shards 40 --commits .^,.

# 50 Scuba-sampled runs — one shard per run ID for fastest completion
$PERF skycastle run-scuba --size 50 --shards 50
```

#### `servicelab` — A/B benchmark on isolated hardware

Create a ServiceLab experiment using `benchmark_rebalancer_dynamic_replay`. This is
the only environment that gives accurate, reproducible performance numbers — it
runs on dedicated isolated hardware with controlled conditions.

**Prerequisites**: Like Skycastle, ServiceLab runs remotely — commit and cloud-sync first
(`sl commit && sl cloud sync`).

```bash
# Accurate perf comparison for shard_manager and global_shard_placement instances
$PERF servicelab run-ids \
  _spm_generated_.multifeed.ris.fb_shorts.prod.atg.1_atg_1773783058642 \
  144e002e-d7ef-4941-8623-afbeb12e1c30_ADS_DI_PERF_BGM_TEST_traffic_object_solve_62bb \
  --priority HIGH --commits .^,.

# Collection-based benchmark
$PERF servicelab run-collection pareto
```

### Discovery Commands

#### `query` — Find run IDs from Scuba

Query the `rebalancer_runs` Scuba table and output run IDs in CSV or list format.

```bash
# Sample 10 recent run IDs
$PERF query --size 10

# Filter by service, output as plain list
$PERF query --service shard_manager --size 20 --format list

# Custom SQL query
$PERF query --sql "SELECT run_id, service_name, solver_type FROM rebalancer_runs \
  WHERE service_name = 'reservation_allocator' AND solver_type LIKE '%LocalSearch%' LIMIT 10"
```

#### `collections` — List predefined test collections

Show static and dynamic collections available for `run-collection` subcommands.
Use `--verbose` to see the individual run IDs in each collection.

```bash
$PERF collections
$PERF collections --verbose
```

### Analysis Commands

#### `logs` — Post-hoc analysis of local results

Parse solver log files from a previous `local` run and print comparison tables
showing metrics across commits.

```bash
$PERF logs /tmp/rebalancer_perf_local_abc123/ --baseline Control
```

### Config Commands

#### `go` — Replay a saved RunConfig

Re-execute a previously saved config JSON (from `--save-config`). Use `--override`
to tweak individual values without editing the file.

```bash
# Replay exactly
$PERF go saved_run.json

# Override shard count
$PERF go saved_run.json --override skycastle.shards=16
```

## Reading Results

**Local:** Results in `/tmp/rebalancer_perf_local_<hash>/` (or `--output-dir`). Parse with `$PERF logs <dir>` for a comparison table showing `time`, `evals_per_second`, `speedup` (>1.0 = faster), CPU/memory usage, and Strobelight icicle URLs. With `--track`, a Scuba URL is printed for collaborative analysis.

**Skycastle:** Results in `manifold://algopt_experiments/tree/perf_compare/{job_id}/` with `summary.csv`. Dashboard: `https://rebalancer-perf.nest.x2p.facebook.net/`

**ServiceLab:** Experiment link printed at submission. Review in ServiceLab UI with statistical significance testing (p-values, confidence intervals).

## Key Options

Consult `$PERF <subcommand> --help` for the full list.

**All execution commands:** `--commits` (Sapling revsets, default: `.` vs public ancestor), `--track` (Scuba experiment name), `--dry-run`, `--save-config <PATH>`, `--stdin` (pipe run IDs).

**Local:** `--monitors all|none|cpu,strobe`, `--num-repeat N` (variance analysis), `--solve-time`, `--output-dir`, `--clean` (buck2 clean between measurements).

**Skycastle:** `--shards N` (default 8; set equal to run ID count for fastest completion).

**ServiceLab:** `--priority LOW|MEDIUM|HIGH`.

**Scuba queries** (`run-scuba`, `query`): `--size N`, `--service`, `--solver-type`, `--window` (hours, default 72), `--sql` (custom query).

## Troubleshooting

| Symptom | Likely cause | Fix |
|---------|-------------|-----|
| "No run IDs found" from `run-scuba` or `query` | Lookback window too narrow, or service name typo | Increase `--window` (default 72h); verify service name with `query --size 1` without filters |
| Build fails at a commit | Commit doesn't compile in opt mode | Use `--dry-run` to verify config, then pick a known-good commit. Check `sl log` for the right hash |
| Wildly inconsistent local timing | Background processes or thermal throttling | Use `--num-repeat 3` for variance analysis, or switch to `servicelab` for isolated hardware |
| ServiceLab job stuck in queue | Low priority with high cluster load | Raise `--priority` to `HIGH`; check the ServiceLab UI link printed at submission |
| Skycastle shards fail | Run ID expired on Manifold (past retention) | Extend retention: `manifold updateExpiration rebalancer/flat/solver_run_{runID} -1` |
| "Commit not found" | Commit hash is ambiguous or not pulled | Use full hash from `sl log`; ensure the commit is on a pulled branch |
| Objective values differ between commits | Solver change affects solution quality | This may be intentional — compare objective alongside timing to assess the tradeoff |

## Source

| Path | Purpose |
|------|---------|
| `algopt/rebalancer/fb/tools/cli.py` | CLI entry point |
| `algopt/rebalancer/fb/tools/commands/` | Subcommand implementations |
| `algopt/rebalancer/fb/tools/lib/cli_options.py` | Shared Click option decorators |
| `algopt/rebalancer/fb/tools/lib/run_config.py` | `RunConfig` dataclass definitions |
| `algopt/rebalancer/fb/tools/lib/scuba_query.py` | Scuba query helpers and collection definitions |
