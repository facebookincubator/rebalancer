# Release Workflow Redesign

**Date:** 2026-06-12
**Status:** Approved

## Problem

The current `release.yml` has two flaws:

1. **Mixed-SHA artifacts.** `wait-for-builds` waits for each required workflow independently, with a per-workflow fallback to the "most recent successful ancestor run." Workflow A's artifacts may come from commit X while workflow B's come from commit Y — inconsistent.

2. **No stable release anchor.** The release always targets `github.sha` (the commit that triggered the workflow). A CI-fix commit after a failed build retriggers a release attempt at the new SHA rather than finding the first green build for the current version.

## Design

### Job graph

```
push → validate → find-release-sha → publish
```

### `validate` (unchanged)

Reads `version.txt`, runs `lint-version.py`, compares against the latest GitHub release tag via `gh release view`. Outputs `version` and `skip=true/false`. Fast (< 1 min); no SHA search logic.

### `find-release-sha` (new)

Runs only when `validate.skip == 'false'`.

**Step 1 — find `base_sha`.**
```sh
git log --format=%H -1 -- version.txt
```
This is the most recent commit that touched `version.txt`. It is the lower bound of the search range.

**Step 2 — build commit list.**
```sh
git log --reverse --format=%H <base_sha>^..HEAD
```
Produces all commits from `base_sha` through HEAD, oldest first. `base_sha` is included.

**Step 3 — scan oldest-first.**
For each SHA in the list, check every workflow in `$REQUIRED_WORKFLOWS`:
```sh
gh run list --workflow <wf> --commit <sha> --limit 1 --json status,conclusion
```
A SHA qualifies when every required workflow has `status=completed` and `conclusion=success`. Any workflow that is missing, in-progress, or failed disqualifies the SHA; the scan moves to the next commit.

**Step 4 — output or fail.**
The first qualifying SHA is output as `qualifying_sha`. If the list is exhausted with no qualifying SHA, the job exits 1 (release fails; re-trigger manually after builds pass).

No polling, no waiting, no fallback to ancestor runs.

**Outputs:** `qualifying_sha`

### `publish` (updated)

Consumes `qualifying_sha` from `find-release-sha` instead of `github.sha`. Artifact download uses `--commit $QUALIFYING_SHA` — guaranteed to have a successful run (verified by the scan). The ancestor-fallback logic in the download step is removed.

Steps:
1. Checkout at `qualifying_sha`
2. Download artifacts from `wheels.yml`, `sdist.yml`, `linux-sdk.yml`, `packaging_mac.yml` via `gh run download --commit qualifying_sha`
3. Publish wheels + sdist to TestPyPI
4. Create signed tag `v{version}` at `qualifying_sha`
5. Create GitHub Release with all artifacts

### What "simultaneously passed" means

The qualifying SHA is one where every required workflow has a completed-successful run recorded against that exact commit SHA. Runs that are in-progress or absent (e.g. not yet triggered) do not qualify — the SHA is skipped, not waited on.

### Failure mode

If no single SHA in `[base_sha, HEAD]` has all required workflows simultaneously green, `find-release-sha` fails. The developer re-triggers manually via `workflow_dispatch` once the necessary builds have passed.

### Removed complexity

- No 80-minute per-workflow polling loop
- No ancestor-fallback logic
- No mixed-SHA artifact downloads
