#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Bundle shared library dependencies into _artifacts/linux/lib/rebalancer/
# so the deb/rpm package is self-contained across Linux distros.
#
# Strategy: walk transitive deps of ALL .so files in lib/ via ldd. Deps
# that ldd finds INSIDE lib/ are getdeps-installed shared libs already
# placed there by fixup-dyn-deps — skip them. Deps resolved to system
# paths are copied to lib/rebalancer/.
#
# build_linux_sdk.sh runs this AFTER the patchelf pass that sets $ORIGIN
# rpaths, so ldd resolves getdeps shared libs to their artifact locations
# and _is_in_artifact() skips them correctly.
#
# The package installs /etc/ld.so.conf.d/rebalancer.conf pointing at
# /usr/local/lib/rebalancer/ and runs ldconfig in postinstall, so the
# loader finds the bundled system libs without any rpath patching of the
# getdeps shared libs in /usr/local/lib/.
import os
import re
import shutil
import subprocess
import sys

ARTIFACT_LIB = "_artifacts/linux/lib"
BUNDLE_DIR = os.path.join(ARTIFACT_LIB, "rebalancer")

# Never bundle — universally present glibc/kernel ABI libs.
_NEVER_BUNDLE = re.compile(
    r"^(linux-vdso|ld-linux|ld-musl|libc\.so|libm\.so|libpthread"
    r"|libdl\.so|librt\.so|libgcc_s)"
)


def _is_in_artifact(path: str) -> bool:
    """True if path is already inside _artifacts/linux/lib/ (getdeps shared lib)."""
    return os.path.abspath(path).startswith(os.path.abspath(ARTIFACT_LIB) + os.sep)


def _ldd_deps(lib_path: str) -> list[tuple[str, str]]:
    """Return resolved [(soname, path)] from ldd, skipping artifact and never-bundle libs."""
    try:
        out = subprocess.check_output(
            ["ldd", lib_path], stderr=subprocess.DEVNULL, text=True
        )
    except subprocess.CalledProcessError:
        return []
    resolved = []
    for line in out.splitlines():
        # Resolved: "    libfoo.so.1 => /path/to/libfoo.so.1 (0x...)"
        m = re.match(r"\s+(\S+)\s+=>\s+(/\S+)", line)
        if m:
            soname, path = m.group(1), m.group(2)
            if _NEVER_BUNDLE.match(soname) or not os.path.isfile(path):
                continue
            if _is_in_artifact(path):
                # Already in the artifact lib dir — getdeps shared lib, skip.
                continue
            resolved.append((soname, path))
    return resolved


def main() -> None:
    if not os.path.isdir(ARTIFACT_LIB):
        print(f"bundle_system_deps: {ARTIFACT_LIB} not found — skipping", flush=True)
        sys.exit(0)

    # Seed the queue with every real (non-symlink) .so in the artifact lib dir
    # so that transitive system deps of ALL shared libs are discovered.
    seed_libs = [
        os.path.join(ARTIFACT_LIB, f)
        for f in os.listdir(ARTIFACT_LIB)
        if f.endswith(".so") or ".so." in f
        if os.path.isfile(os.path.join(ARTIFACT_LIB, f))
        and not os.path.islink(os.path.join(ARTIFACT_LIB, f))
    ]
    if not seed_libs:
        print(
            "bundle_system_deps: no .so files found in artifact — skipping", flush=True
        )
        sys.exit(0)

    os.makedirs(BUNDLE_DIR, exist_ok=True)
    queue = list(seed_libs)
    visited: set[str] = set()

    while queue:
        lib = queue.pop()
        if lib in visited:
            continue
        visited.add(lib)

        for soname, path in _ldd_deps(lib):
            dest = os.path.join(BUNDLE_DIR, os.path.basename(path))
            if not os.path.exists(dest):
                shutil.copy2(path, dest)
                print(f"  bundled (system) {soname} from {path}", flush=True)
                queue.append(dest)

    bundled = sorted(os.listdir(BUNDLE_DIR))
    if bundled:
        print(f"\nBundled {len(bundled)} libs into {BUNDLE_DIR}/", flush=True)
        for f in bundled:
            print(f"  {f}", flush=True)
    else:
        print("No libs to bundle (all deps are core system libs)", flush=True)


if __name__ == "__main__":
    main()
