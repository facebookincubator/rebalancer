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

#include "algopt/lp/factory/ProblemFactory.h"

#include "algopt/lp/detail/simplify/ProblemSimplifier.h"
#include "algopt/lp/fast/FastProblemImpl.h"
#ifdef REBALANCER_USE_GUROBI
#include "algopt/lp/detail/gurobi/GurobiProblem.h"
#endif
#ifdef REBALANCER_USE_XPRESS
#include "algopt/lp/detail/xpress/XpressProblem.h"
#endif
#ifdef REBALANCER_USE_HIGHS
#include "algopt/lp/detail/highs/HiGHSProblem.h"
#endif

namespace facebook::algopt::lp {

Problem ProblemFactory::makeXpressProblem() {
#ifdef REBALANCER_USE_XPRESS
#ifndef REBALANCER_OSS_BUILD
  lazyLoadXpress();
#else
  if (!XpressEnvironment::isInitialized()) {
    throw std::runtime_error(XpressEnvironment::notInitializedMsg.data());
  }
#endif
  return Problem(std::make_unique<detail::XpressProblem>());
#else
  throw std::runtime_error("XPRESS is not available in this build");
#endif
}

Problem ProblemFactory::makeGenericProblem(
    const std::function<Problem()>& factory) {
  return Problem(std::make_unique<detail::GenericProblemImpl>(factory));
}

Problem ProblemFactory::makeSimplifiedProblem(
    const std::function<Problem()>& factory) {
  return Problem(std::make_unique<detail::ProblemSimplifier>(factory));
}

Problem ProblemFactory::makeGurobiProblem() {
#ifdef REBALANCER_USE_GUROBI
#ifndef REBALANCER_OSS_BUILD
  // Always call lazyLoadGurobi() - it's thread-safe via std::call_once
  // and will be a no-op if already initialized
  lazyLoadGurobi();
#else
  if (!GurobiEnvironment::isInitialized()) {
    throw std::runtime_error(GurobiEnvironment::notInitializedMsg.data());
  }
#endif
  return Problem(std::make_unique<detail::GurobiProblem>());
#else
  throw std::runtime_error("Gurobi is not available in this build");
#endif
}

Problem ProblemFactory::makeFastProblem() {
  return Problem(std::make_unique<FastProblemImpl>());
}

Problem ProblemFactory::makeHiGHSProblem() {
#ifdef REBALANCER_USE_HIGHS
  return Problem(std::make_unique<detail::HiGHSProblem>());
#else
  throw std::runtime_error("HiGHS is not available in this build");
#endif
}

} // namespace facebook::algopt::lp
