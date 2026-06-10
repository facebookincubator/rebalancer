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
# CIBW_REPAIR_WHEEL_COMMAND_LINUX wrapper. auditwheel needs
# LD_LIBRARY_PATH to point at getdeps-installed shared libs so it can
# bundle them into the wheel.
#
# cibuildwheel passes {dest_dir} and {wheel} as $1 and $2.
set -euo pipefail
dest_dir="$1"
wheel="$2"

# patchelf 0.17.2 (bundled with older auditwheel/cibuildwheel) corrupts
# .init_array entries in shared libs when expanding .dynstr for SONAME
# renaming. Specifically: gflags/glog static initializers baked into
# libfolly.so crash with SIGSEGV after patchelf moves sections.
# patchelf 0.18.0+ fixes this. Install the PyPI patchelf package (which
# ships the binary) to ensure a fixed version is in PATH before auditwheel
# runs. auditwheel finds patchelf via shutil.which, so this overrides
# whatever the container has. The >=0.18 release is pre-release on PyPI
# so --pre is required to find it.
pip install --quiet --pre "patchelf>=0.18"
echo "patchelf version: $(patchelf --version)"

LD_LIBRARY_PATH="$(ls -d /tmp/fbcode_builder_getdeps-*/installed/*/lib \
                          /tmp/fbcode_builder_getdeps-*/installed/*/lib64 \
                       2>/dev/null | tr '\n' ':'):${LD_LIBRARY_PATH:-}" \
    auditwheel repair -w "$dest_dir" "$wheel"
