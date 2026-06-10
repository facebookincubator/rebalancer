#!/usr/bin/env bash
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
# CIBW_TEST_COMMAND wrapper. On success, just runs the smoke test
# (`import rebalancer; rebalancer.get_library_path()`). On failure --
# specifically SIGSEGV during import -- runs diagnostic passes so we
# can see what's happening:
#   1. `python -X faulthandler` to catch Python-level signals with traceback
#   2. `LD_DEBUG=files` to log dlopen ordering and dependency resolution
#   3. `gdb -batch -ex run -ex bt` for a C-level backtrace at the crash site
#
# Linux wheel builds have been crashing at import with no diagnostic
# output; this script captures the data we need to root-cause.
set -u  # not -e: we want to continue past the first failure

PY_TEST='import rebalancer, os; print("version:", rebalancer.__version__); p = rebalancer.get_library_path(); print("lib:", p); assert os.path.isfile(p), f"missing library: {p}"'

echo "::group::Python binding tests"
# Run the full binding test suite against the installed wheel. test_bindings.py
# is alongside this script in tools/wheels/ and exercises the public rebalancer
# package: import, ping(), TypedDict spec shims, and a real capacity solve.
python "$(dirname "$0")/test_bindings.py" -v
binding_rc=$?
echo "::endgroup::"
if [[ $binding_rc -ne 0 ]]; then
    echo "::error::Python binding tests failed with exit code $binding_rc"
    exit $binding_rc
fi

echo "::group::Smoke test (with faulthandler)"
# faulthandler dumps a Python traceback on fatal signals before the process dies.
python -X faulthandler -c "$PY_TEST"
rc=$?
echo "::endgroup::"

if [[ $rc -eq 0 ]]; then
    exit 0
fi

echo "::error::Smoke test failed with exit code $rc; running diagnostics"

echo "::group::Wheel layout"
# Find site-packages WITHOUT importing rebalancer (which crashes).
sitelib="$(python -c 'import site; print(site.getsitepackages()[0])' 2>/dev/null || true)"
if [[ -n "$sitelib" ]]; then
    echo "=== site-packages native files ==="
    find "$sitelib" -maxdepth 4 -type f \( -name '*.so' -o -name '*.so.*' -o -name '*.dylib' \) \
        | grep -E 'rebalancer' | sort || true
    echo "=== librebalancer rpath (patchelf) ==="
    find "$sitelib" -name 'librebalancer*.so' -maxdepth 5 | while read -r lib; do
        echo "$lib:"
        patchelf --print-rpath "$lib" 2>/dev/null || true
        patchelf --print-needed "$lib" 2>/dev/null || true
    done
    echo "=== google:: symbols in librebalancer.so .dynsym ==="
    find "$sitelib" -name 'librebalancer*.so' -maxdepth 5 | while read -r lib; do
        echo "$lib:"
        nm -D "$lib" 2>/dev/null | grep -E 'LogMessage|google' | head -5 || echo "(none)"
    done
fi
echo "::endgroup::"

echo "::group::ldd / otool on bundled native libs"
if [[ -n "${sitelib:-}" ]]; then
    while read -r so; do
        echo "--- $so ---"
        if command -v ldd >/dev/null; then
            ldd "$so" 2>&1 || true
        fi
        if command -v otool >/dev/null; then
            otool -L "$so" 2>&1 || true
        fi
    done < <(find "$sitelib" -maxdepth 5 -type f \( -name '*.so' \) | grep rebalancer)
fi
echo "::endgroup::"

echo "::group::LD_DEBUG=files import (first 500 lines)"
LD_DEBUG=files python -c "import rebalancer" 2>&1 | head -500 || true
echo "::endgroup::"

echo "::group::LD_DEBUG=files import (last 300 lines)"
LD_DEBUG=files python -c "import rebalancer" 2>&1 | tail -300 || true
echo "::endgroup::"

echo "::group::All gflags init events"
LD_DEBUG=files python -c "import rebalancer" 2>&1 | grep -i gflags || true
echo "::endgroup::"

if command -v gdb >/dev/null; then
    echo "::group::gdb backtrace at crash"
    gdb -batch \
        -ex 'set confirm off' \
        -ex 'set pagination off' \
        -ex 'handle SIGSEGV stop print' \
        -ex 'run' \
        -ex 'bt full' \
        -ex 'info sharedlibrary' \
        -ex 'quit' \
        --args python -c "$PY_TEST" 2>&1 || true
    echo "::endgroup::"
else
    echo "gdb not available; skipping native backtrace"
fi

exit $rc
