---
name: investigating-gha-failures
description: Use when GitHub Actions CI is failing, when investigating build or test failures on a branch or PR, when the user asks about CI status, or when you need to diagnose why a workflow failed, or when you need to examine GHA build logs (e.g. sccache stats, cache usage, build timing). Triggers on keywords like "CI", "GHA", "GitHub Actions", "build failure", "tests failing", "pipeline broken", "build logs", "sccache", "cache stats", "build performance".
---

# Investigating GHA Logs

## Overview

Use `tools/fetch_gha_logs.py` to fetch, extract, and display GitHub Actions logs. This script handles proxy setup, log downloading, and error/content extraction automatically.

**IMPORTANT**: Always prefer this script over raw `gh run view --log` or `gh api` calls for fetching GHA logs. The `gh run view --log` command often truncates output and drops steps, while this script downloads the full log zip and extracts complete content reliably.

## When to Use

- CI is red / builds are failing
- User asks "why is CI failing?" or "what's broken?"
- Need to check GHA status on current branch, another branch, or a PR
- Investigating build performance, cache hit rates, sccache stats, or build timing
- Need to read any content from GHA build logs (the `--raw` flag gives full logs)

## Quick Reference

| Goal | Command |
|------|---------|
| Latest failures on current branch | `python3 tools/fetch_gha_logs.py --latest` |
| Latest failures on a specific branch | `python3 tools/fetch_gha_logs.py --latest --branch main` |
| Failures for a PR | `python3 tools/fetch_gha_logs.py --pr 42` |
| Specific workflow only | `python3 tools/fetch_gha_logs.py --latest --workflow linux` |
| Specific run ID | `python3 tools/fetch_gha_logs.py 12345678` |
| Save errors to file | `python3 tools/fetch_gha_logs.py --latest --output /tmp/gha_errors.txt` |
| Full raw logs | `python3 tools/fetch_gha_logs.py --latest --raw` |
| Full raw logs for a specific run | `python3 tools/fetch_gha_logs.py 12345678 --raw` |
| List recent failures (no download) | `python3 tools/fetch_gha_logs.py` |

## Workflow for Failures

Run:
```
python3 tools/fetch_gha_logs.py --latest --output /tmp/gha_errors.txt
```
The file `/tmp/gha_errors.txt` will contain most build errors, test failures, linker errors, CMake issues, but also will include the location of the raw logs for all the most recent failing builds. Use these logs for additional context if you need them.

You can also request a particular pull request with
```
python3 tools/fetch_gha_logs.py --pr <N> --output /tmp/gha_errors.txt
```
and a particular run with
```
python3 tools/fetch_gha_logs.py <run id> --output /tmp/gha_errors.txt
```

Next, fix the identified issues in the source code.

## Workflow for Log Analysis (sccache stats, cache usage, timing, etc.)

Use `--raw` and pipe through grep to find what you need:
```bash
# Get sccache stats
python3 tools/fetch_gha_logs.py <run_id> --raw 2>&1 | grep -A 30 "sccache --show-stats"

# Get GHA cache list
python3 tools/fetch_gha_logs.py <run_id> --raw 2>&1 | grep -A 35 "gh cache list"

# Get cache restore results
python3 tools/fetch_gha_logs.py <run_id> --raw 2>&1 | grep "Cache restored"
```

Use `gh run list` or `gh api` to find run IDs and step timing metadata first, then use this script for actual log content.
