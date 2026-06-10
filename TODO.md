# TODO

## Upstream dependency fixes

### Folly: `-mpclmul` applied unconditionally on non-x86 (Mac ARM)
`-mpclmul` enables the x86 PCLMULQDQ carry-less multiplication instruction,
used by folly's CRC32/checksum intrinsics. Two files in folly are involved:

- `CMakeLists.txt` (top-level, ~line 251): probes compiler support via
  `check_cxx_compiler_flag(-mpclmul COMPILER_HAS_M_PCLMUL)`
- `folly/hash/detail/CMakeLists.txt` (~lines 36–39): applies the flag when
  the probe passes:
  ```cmake
  if (COMPILER_HAS_M_PCLMUL)
    target_compile_options(folly_hash_detail_checksum_detail_obj PRIVATE -mpclmul)
  endif()
  ```

On Mac ARM (Apple Silicon), Clang accepts `-mpclmul` without a hard error
during the `check_cxx_compiler_flag` test compile, so the probe returns true.
When the flag is then used on real source files, Clang emits:
`warning: argument unused during compilation: '-mpclmul'`

Fix: guard the probe (or its application) behind an x86 architecture check,
e.g. in folly's top-level `CMakeLists.txt`:
```cmake
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|i[3-6]86|AMD64)$")
  check_cxx_compiler_flag(-mpclmul COMPILER_HAS_M_PCLMUL)
endif()
```

### fbthrift: pulls in google/benchmark, which fails to build under Clang 19+ in `-pedantic-errors` mode
`build/fbcode_builder/manifests/fbthrift` declares `benchmark` (google/benchmark
v1.8.0) as a dependency. Nothing the rebalancer wheel actually needs links
against it — it's only used for fbthrift's own internal microbenchmarks — but
getdeps builds the full transitive closure, so a benchmark build failure
breaks every downstream getdeps-based wheel/package build.

Symptom (cibuildwheel macOS, Homebrew LLVM clang 19):
```
benchmark-1.8.0/include/benchmark/benchmark.h:1408:30:
  error: '__COUNTER__' is a C2y extension [-Werror,-Wc2y-extensions]
```

Root cause: google/benchmark's CMake compiles its own targets with
`-Werror -pedantic -pedantic-errors` under `-std=c++11`. `__COUNTER__` is a
~20-year-old GCC/Clang/MSVC extension that the in-progress C2y draft is
finally standardizing; Clang 19 added `-Wc2y-extensions` to flag uses of
features from that future standard. Pre-Clang-19 (incl. AppleClang 15)
didn't have the diagnostic, so it compiled silently.

Workaround in our pipeline: pass `CXXFLAGS=-Wno-c2y-extensions` to the
whole getdeps dep build on macOS (committed in
`.github/workflows/wheels.yml`). This is fragile — it papers over the
fbthrift-side problem rather than fixing it.

Asks for the Thrift team:
1. **Make benchmark optional in fbthrift's manifest.** If a consumer doesn't
   need fbthrift's microbenchmarks, building google/benchmark and its dep
   tree is pure overhead and a fragility surface. A getdeps-level
   `[dependencies.fb=on]` / `[dependencies.test=on]` style gate, or a
   separate `fbthrift-bench` manifest, would let lean consumers skip it.
2. **Bump the bundled google/benchmark version.** v1.8.0 is from mid-2023;
   newer releases may have already silenced the `__COUNTER__` diagnostic
   or stopped using `-pedantic-errors` on `-std=c++11`. v1.9.x exists.
3. **Alternatively, upstream a patch to google/benchmark** that wraps the
   `__COUNTER__` use in `_Pragma("clang diagnostic ignored \"-Wc2y-extensions\"")`
   guards. This is the most defensible fix because every clang-19+ user
   building benchmark in pedantic mode hits this.

### Folly: add portable thread safety annotation macros
Folly provides only `FOLLY_ATTR_CLANG_NO_THREAD_SAFETY_ANALYSIS` (in
`folly/CppAttributes.h`) but no macros for the positive annotations like
`GUARDED_BY`, `REQUIRES`, `EXCLUDES`, etc. We define our own in
`algopt/rebalancer/common/ThreadAnnotations.h`. These should be upstreamed
to folly (e.g. as `folly/ThreadAnnotations.h`) so all folly consumers can
use them without each project reinventing the macros.

## Build warnings (our code)
