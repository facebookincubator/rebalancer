#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
# Licensed under the Apache License, Version 2.0

"""Fetch and display GitHub Actions failure logs for the rebalancer project.

Usage:
    # Show recent failed runs
    python3 tools/fetch_gha_logs.py

    # Show logs for a specific run ID
    python3 tools/fetch_gha_logs.py 12345678

    # Fetch logs for failed workflows from a pull request
    python3 tools/fetch_gha_logs.py --pr 42

    # Fetch logs for failed workflows from the latest CI run on the current branch
    python3 tools/fetch_gha_logs.py --latest

    # Same, but for a specific branch
    python3 tools/fetch_gha_logs.py --latest --branch main

    # Only check a specific workflow
    python3 tools/fetch_gha_logs.py --latest --workflow mac

    # Save extracted errors to a file (useful for pasting into Claude Code)
    python3 tools/fetch_gha_logs.py --latest --output /tmp/gha_errors.txt

Requires: gh CLI authenticated with GitHub.
On Meta devservers, automatically uses HTTPS_PROXY=http://fwdproxy:8080.
Use --proxy to override.
"""

import argparse
import json
import os
import platform
import shutil
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path

REPO = "facebookincubator/rebalancer"
META_PROXY = "http://fwdproxy:8080"


def _is_meta_infra() -> bool:
    """Return True if running on Meta infrastructure (devserver or similar)."""
    hostname = platform.node().lower()
    return "facebook" in hostname or ".meta." in hostname or hostname.endswith(".meta")


# Patterns that indicate the interesting part of a build failure
ERROR_PATTERNS = [
    "error:",
    "FAILED",
    "fatal error",
    "undefined reference",
    "no matching function",
    "cannot find",
    "CMake Error",
    "ninja: build stopped",
    "FAILED: CMakeFiles",
    "collect2: error",
    "ld: error",
    "TEST.*FAILED",
    "SEGFAULT",
    "Assertion failed",
]


def _check_gh_version() -> bool:
    """Return True if `gh --version` output indicates the GitHub CLI from github.com."""
    try:
        result = subprocess.run(
            ["gh", "--version"], capture_output=True, text=True, timeout=10
        )
        return result.returncode == 0 and "github.com" in result.stdout
    except (FileNotFoundError, subprocess.TimeoutExpired):
        return False


def ensure_gh_cli() -> None:
    """Ensure the GitHub CLI is installed and is the correct version.

    If `gh` is missing or not the GitHub CLI from github.com, attempts to
    install it via `feature install github_cli` and re-verifies.
    """
    if _check_gh_version():
        return

    if shutil.which("gh") is None:
        print("gh CLI not found on PATH.", file=sys.stderr)
    else:
        print(
            "gh CLI found but does not appear to be the GitHub CLI from github.com.",
            file=sys.stderr,
        )

    print("Attempting: feature install github_cli ...", file=sys.stderr)
    subprocess.run(["feature", "install", "github_cli"])

    if not _check_gh_version():
        print(
            "Error: gh CLI is still not available after install.\n"
            "Please run 'feature install github_cli' manually and ensure "
            "'gh --version' mentions github.com.",
            file=sys.stderr,
        )
        sys.exit(1)

    print("gh CLI installed successfully.", file=sys.stderr)


def gh_api(endpoint: str, proxy: str | None) -> dict | list | bytes:
    """Call the GitHub API via gh CLI, returning parsed JSON."""
    env = os.environ.copy()
    if proxy:
        env["HTTPS_PROXY"] = proxy
        env["HTTP_PROXY"] = proxy

    cmd = ["gh", "api", endpoint]
    result = subprocess.run(cmd, capture_output=True, env=env)

    if result.returncode != 0:
        stderr = result.stderr.decode()
        print(f"Error calling gh api {endpoint}:\n{stderr}", file=sys.stderr)
        sys.exit(1)

    # Try JSON first, fall back to raw bytes (for log downloads)
    try:
        return json.loads(result.stdout)
    except (json.JSONDecodeError, UnicodeDecodeError):
        return result.stdout


def gh_api_raw(endpoint: str, proxy: str | None) -> bytes | None:
    """Call the GitHub API via gh CLI, returning raw bytes (or None on error)."""
    env = os.environ.copy()
    if proxy:
        env["HTTPS_PROXY"] = proxy
        env["HTTP_PROXY"] = proxy

    cmd = ["gh", "api", endpoint]
    result = subprocess.run(cmd, capture_output=True, env=env)

    if result.returncode != 0:
        stderr = result.stderr.decode()
        print(f"Warning: gh api {endpoint} failed:\n{stderr}", file=sys.stderr)
        return None

    return result.stdout


def _summarize_run(run: dict) -> dict:
    """Extract the fields we care about from a raw workflow run."""
    return {
        "id": run["id"],
        "name": run["name"],
        "status": run.get("status"),
        "conclusion": run.get("conclusion"),
        "created_at": run["created_at"],
        "head_branch": run["head_branch"],
        "html_url": run["html_url"],
    }


def list_failed_runs(
    proxy: str | None, workflow: str | None = None, limit: int = 10
) -> list[dict]:
    """List recent failed or cancelled workflow runs."""
    # The GitHub API only accepts a single `status` value, so query each
    # conclusion separately to keep server-side filtering — otherwise a busy
    # repo's recent successes would push older failures out of the window.
    runs: list[dict] = []
    seen_ids: set[int] = set()
    for status in ("failure", "cancelled"):
        data = gh_api(f"repos/{REPO}/actions/runs?status={status}&per_page=50", proxy)
        for r in data.get("workflow_runs", []):
            if r["id"] not in seen_ids:
                seen_ids.add(r["id"])
                runs.append(r)

    runs.sort(key=lambda r: r["created_at"], reverse=True)

    if workflow:
        runs = [r for r in runs if workflow.lower() in r["name"].lower()]
    else:
        runs = [r for r in runs if "windows" not in r["name"].lower()]

    return [_summarize_run(r) for r in runs[:limit]]


def list_latest_runs(
    proxy: str | None,
    workflow: str | None = None,
    branch: str | None = None,
    limit: int = 50,
) -> list[dict]:
    """List recent workflow runs (any status), optionally filtered by branch."""
    params = f"per_page={limit}"
    if branch:
        params += f"&branch={branch}"
    data = gh_api(f"repos/{REPO}/actions/runs?{params}", proxy)
    runs = data.get("workflow_runs", [])

    if workflow:
        runs = [r for r in runs if workflow.lower() in r["name"].lower()]
    else:
        runs = [r for r in runs if "windows" not in r["name"].lower()]

    return [_summarize_run(r) for r in runs]


def get_latest_per_workflow(runs: list[dict]) -> list[dict]:
    """Return the most recent run for each distinct workflow name."""
    seen: set[str] = set()
    result = []
    for run in runs:
        name = run["name"]
        if name not in seen:
            seen.add(name)
            result.append(run)
    return result


def get_failed_jobs(run_id: int, proxy: str | None) -> list[dict]:
    """Get failed jobs for a specific run."""
    data = gh_api(f"repos/{REPO}/actions/runs/{run_id}/jobs", proxy)
    jobs = data.get("jobs", [])

    result = []
    for job in jobs:
        if job.get("conclusion") in ("failure", "cancelled"):
            failed_steps = [
                {"name": s["name"], "conclusion": s["conclusion"]}
                for s in job.get("steps", [])
                if s.get("conclusion") in ("failure", "cancelled")
            ]
            result.append(
                {
                    "id": job["id"],
                    "name": job["name"],
                    "conclusion": job["conclusion"],
                    "failed_steps": failed_steps,
                }
            )
    return result


def download_logs(
    run_id: int,
    proxy: str | None,
    base_dir: Path | None = None,
    label: str | None = None,
) -> Path | None:
    """Download and extract logs for a run, returning the extraction directory.

    Returns None if the logs API returned an error (e.g., 404 because the
    run hasn't finished or its logs have expired).

    Args:
        run_id: The GitHub Actions run ID.
        proxy: HTTPS proxy URL or None.
        base_dir: If provided, extract into a subdirectory of this path
            instead of creating a new temporary directory.
        label: Subdirectory name when base_dir is provided (defaults to run_id).
    """
    log_bytes = gh_api_raw(f"repos/{REPO}/actions/runs/{run_id}/logs", proxy)
    if log_bytes is None:
        return None

    if base_dir is not None:
        extract_dir = base_dir / (label or str(run_id))
    else:
        extract_dir = Path(tempfile.mkdtemp(prefix="gha_logs_")) / "logs"

    zip_path = extract_dir.parent / f"logs_{run_id}.zip"
    extract_dir.mkdir(parents=True, exist_ok=True)
    zip_path.write_bytes(log_bytes)

    with zipfile.ZipFile(zip_path) as zf:
        zf.extractall(extract_dir)

    zip_path.unlink()
    return extract_dir


def extract_errors(log_dir: Path) -> dict[str, list[str]]:
    """Extract error lines from log files, grouped by log file name."""
    errors: dict[str, list[str]] = {}

    for log_file in sorted(log_dir.rglob("*.txt")):
        file_errors = []
        lines = log_file.read_text(errors="replace").splitlines()

        for i, line in enumerate(lines):
            if any(pat in line for pat in ERROR_PATTERNS):
                # Include context: 2 lines before, the error, and 5 lines after
                start = max(0, i - 2)
                end = min(len(lines), i + 6)
                context = lines[start:end]
                file_errors.append("\n".join(context))

        if file_errors:
            # Use relative name for readability
            name = log_file.relative_to(log_dir).as_posix()
            errors[name] = file_errors

    return errors


def format_errors(errors: dict[str, list[str]], max_per_file: int = 10) -> str:
    """Format extracted errors for display."""
    sections = []
    for filename, file_errors in errors.items():
        section_lines = [f"{'=' * 60}", f"FILE: {filename}", f"{'=' * 60}"]
        for j, err in enumerate(file_errors[:max_per_file]):
            section_lines.append(f"\n--- Error {j + 1} ---")
            section_lines.append(err)
        if len(file_errors) > max_per_file:
            section_lines.append(
                f"\n... and {len(file_errors) - max_per_file} more errors"
            )
        sections.append("\n".join(section_lines))

    return "\n\n".join(sections)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Fetch GitHub Actions failure logs for rebalancer"
    )
    parser.add_argument("run_id", nargs="?", type=int, help="Specific run ID to fetch")
    parser.add_argument(
        "--pr",
        type=int,
        help="Fetch logs for failing builds on a pull request (by PR number)",
    )
    parser.add_argument(
        "--latest",
        action="store_true",
        help="Fetch logs for the most recent failure per workflow (or single workflow with --workflow)",
    )
    parser.add_argument(
        "--workflow",
        type=str,
        help="Filter by workflow name (linux, mac, windows)",
    )
    parser.add_argument(
        "--branch",
        type=str,
        help="Filter runs by branch name (default: current git branch)",
    )
    parser.add_argument(
        "--proxy",
        type=str,
        default=None,
        help=f"HTTPS proxy (default: {META_PROXY} on Meta infra, none otherwise)",
    )
    parser.add_argument(
        "--output",
        type=str,
        help="Write extracted errors to this file",
    )
    parser.add_argument(
        "--raw",
        action="store_true",
        help="Print raw log content instead of extracted errors",
    )
    parser.add_argument(
        "--limit",
        type=int,
        default=10,
        help="Number of recent failures to show (default: 10)",
    )

    args = parser.parse_args()

    ensure_gh_cli()

    # Determine proxy: explicit flag > env var > Meta default (if on Meta infra)
    proxy = (
        args.proxy
        or os.environ.get("HTTPS_PROXY")
        or (META_PROXY if _is_meta_infra() else None)
    )

    # Each entry is (run_id, workflow_name or None)
    selected_runs: list[tuple[int, str | None]] = []

    if args.run_id is not None:
        selected_runs = [(args.run_id, None)]
    elif args.pr is not None:
        # Get the PR's head branch, then find failed runs on that branch
        print(f"Fetching PR #{args.pr} for {REPO}...")
        pr_data = gh_api(f"repos/{REPO}/pulls/{args.pr}", proxy)
        pr_branch = pr_data["head"]["ref"]
        pr_sha = pr_data["head"]["sha"]
        print(f"  PR branch: {pr_branch}  HEAD: {pr_sha[:12]}")

        print(f"Fetching CI runs for PR branch '{pr_branch}'...\n")
        runs = list_latest_runs(proxy, args.workflow, pr_branch)

        if not runs:
            print("No CI runs found for this PR.")
            sys.exit(0)

        latest = get_latest_per_workflow(runs)

        for r in latest:
            status = r["conclusion"] or r["status"]
            print(
                f"  {r['id']}  {r['name']:10s}  {status:12s}  "
                f"{r['created_at']}  ({r['head_branch']})"
            )

        failed = [r for r in latest if r["conclusion"] in ("failure", "cancelled")]

        if not failed:
            in_progress = [
                r for r in latest if r["status"] in ("queued", "in_progress")
            ]
            if in_progress:
                names = ", ".join(r["name"] for r in in_progress)
                print(f"\nNo failures — still running: {names}")
            else:
                print("\nNo failures in the latest runs for this PR.")
            sys.exit(0)

        selected_runs = [(r["id"], r["name"]) for r in failed]
        print(f"\nDownloading logs for {len(failed)} failed/cancelled workflow(s):")
        for r in failed:
            print(f"  {r['id']}  {r['name']}")
    elif args.latest:
        # Find the latest run per workflow and only download failures.
        # This avoids picking up stale failures from older runs or other branches.
        branch = args.branch
        if branch is None:
            # Auto-detect current git branch
            try:
                result = subprocess.run(
                    ["git", "rev-parse", "--abbrev-ref", "HEAD"],
                    capture_output=True,
                    text=True,
                    timeout=5,
                )
                if result.returncode == 0:
                    branch = result.stdout.strip()
            except (FileNotFoundError, subprocess.TimeoutExpired):
                pass

        branch_desc = f" on branch '{branch}'" if branch else ""
        print(f"Fetching latest CI runs for {REPO}{branch_desc}...\n")
        runs = list_latest_runs(proxy, args.workflow, branch)

        if not runs:
            print("No recent runs found.")
            sys.exit(0)

        latest = get_latest_per_workflow(runs)

        for r in latest:
            status = r["conclusion"] or r["status"]
            print(
                f"  {r['id']}  {r['name']:10s}  {status:12s}  "
                f"{r['created_at']}  ({r['head_branch']})"
            )

        failed = [r for r in latest if r["conclusion"] in ("failure", "cancelled")]

        if not failed:
            in_progress = [
                r for r in latest if r["status"] in ("queued", "in_progress")
            ]
            if in_progress:
                names = ", ".join(r["name"] for r in in_progress)
                print(f"\nNo failures — still running: {names}")
            else:
                print("\nNo failures in the latest runs.")
            sys.exit(0)

        selected_runs = [(r["id"], r["name"]) for r in failed]
        print(f"\nDownloading logs for {len(failed)} failed/cancelled workflow(s):")
        for r in failed:
            print(f"  {r['id']}  {r['name']}")
    else:
        # No --latest or run ID: just list recent failures without downloading
        print(f"Fetching recent failed/cancelled runs for {REPO}...\n")
        runs = list_failed_runs(proxy, args.workflow, args.limit)

        if not runs:
            print("No recent failures or cancellations found.")
            sys.exit(0)

        for i, run in enumerate(runs):
            status = run["conclusion"] or run["status"]
            print(
                f"  [{i + 1}] {run['id']}  {run['name']:10s}  {status:12s}  "
                f"{run['created_at']}  ({run['head_branch']})"
            )
            print(f"      {run['html_url']}")

        print("\nRe-run with a run ID or --latest to fetch logs.")
        sys.exit(0)

    # Create a shared temp directory when downloading multiple runs
    shared_dir: Path | None = None
    if len(selected_runs) > 1:
        shared_dir = Path(tempfile.mkdtemp(prefix="gha_logs_"))

    all_output_parts: list[str] = []
    last_log_dir: Path | None = None

    for run_id, workflow_name in selected_runs:
        # Show failed jobs
        print(f"\nFetching failed jobs for run {run_id}...")
        failed_jobs = get_failed_jobs(run_id, proxy)

        if failed_jobs:
            print(f"\nFailed jobs:")
            for job in failed_jobs:
                print(f"  - {job['name']} (job id: {job['id']})")
                for step in job["failed_steps"]:
                    print(f"    - Step: {step['name']}")
        else:
            print("No failed or cancelled jobs found (run may still be in progress).")

        # Download and extract logs
        print(f"\nDownloading logs for run {run_id}...")
        log_dir = download_logs(run_id, proxy, base_dir=shared_dir, label=workflow_name)
        if log_dir is None:
            print(
                f"Skipping run {run_id}: logs not available "
                "(run may still be in progress or logs may have expired)."
            )
            continue
        print(f"Logs extracted to: {log_dir}")
        last_log_dir = log_dir

        if args.raw:
            # Print all log files
            for log_file in sorted(log_dir.rglob("*.txt")):
                name = log_file.relative_to(log_dir).as_posix()
                print(f"\n{'=' * 60}")
                print(f"FILE: {name}")
                print(f"{'=' * 60}")
                print(log_file.read_text(errors="replace"))
        else:
            # Extract and display errors
            errors = extract_errors(log_dir)
            if errors:
                output = format_errors(errors)
                print(f"\n{output}")
                all_output_parts.append(output)
            else:
                print("\nNo obvious errors found in logs. Try --raw for full output.")
                print(f"Logs are at: {log_dir}")

    # Determine the top-level log directory to report
    log_root = shared_dir if shared_dir is not None else last_log_dir

    if args.output and all_output_parts:
        combined = "\n\n".join(all_output_parts)
        if log_root is not None:
            combined += f"\n\nLogs downloaded to: {log_root}\n"
        Path(args.output).write_text(combined)
        print(f"\nErrors written to: {args.output}")

    # Always print the log directory as the last line of output so that
    # callers (scripts, Claude Code, etc.) can reliably capture it.
    print(f"\nLog directory: {log_root}")


if __name__ == "__main__":
    main()
