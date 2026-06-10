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

} // namespace facebook::algopt
