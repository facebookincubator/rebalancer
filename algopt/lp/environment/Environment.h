// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <folly/CPortability.h>

#ifndef REBALANCER_OSS_BUILD
#include "algopt/lp/environment/fb/SetupEnvironments.h"
#endif

namespace facebook::algopt {

inline constexpr bool isOpenSourceBuild() {
#ifdef REBALANCER_OSS_BUILD
  return true;
#else
  return false;
#endif
}

inline constexpr bool isXpressAvailable() {
#ifdef REBALANCER_USE_XPRESS
  return true;
#else
  return false;
#endif
}

inline constexpr bool isGurobiAvailable() {
#ifdef REBALANCER_USE_GUROBI
  return true;
#else
  return false;
#endif
}

inline constexpr bool isHiGHSAvailable() {
#ifdef REBALANCER_USE_HIGHS
  return true;
#else
  return false;
#endif
}

inline constexpr bool isXpressOrGurobiAvailable() {
  return isXpressAvailable() || isGurobiAvailable();
}

inline constexpr bool isMipSolverAvailable() {
  return isGurobiAvailable() || isXpressAvailable() || isHiGHSAvailable();
}

inline constexpr bool isManifoldAvailable() {
#ifdef REBALANCER_OSS_BUILD
  return false;
#else
  return true;
#endif
}

// Runtime flags controlling native Xpress operator paths. Exposed through
// reference-returning accessors rather than plain mutable globals so they are
// not flagged as non-const global variables, while still retaining
// process-wide state via the function-local statics. FOLLY_EXPORT places the
// accessors on the dynamic symbol table so the dynamic linker dedups to a
// single instance of each flag at runtime. Only meaningful when
// isXpressAvailable() returns true. Default false preserves existing
// approximation behavior.
//
// Thread-safety: these flags are plain `bool` and are NOT synchronized. They
// are intended to be set once at program startup, before any solver use, and
// then only read during model construction. Mutating a flag from one thread
// while another thread reads it during a solve is an unsynchronized data race
// and is unsupported.
FOLLY_EXPORT inline bool& useXpressNativePwl() {
  static bool value = false;
  return value;
}
FOLLY_EXPORT inline bool& useXpressNativeQuadratic() {
  static bool value = false;
  return value;
}
FOLLY_EXPORT inline bool& useXpressNativeMax() {
  static bool value = false;
  return value;
}
FOLLY_EXPORT inline bool& useXpressIndicatorConstraints() {
  static bool value = false;
  return value;
}

} // namespace facebook::algopt
