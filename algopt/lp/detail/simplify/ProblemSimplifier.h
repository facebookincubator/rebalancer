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

#include "algopt/lp/detail/generic/impl/GenericProblemImpl.h"
#include "algopt/lp/generic/Problem.h"

#include <folly/container/F14Map.h>
#include <folly/MapUtil.h>

namespace facebook::algopt::lp::detail {

class ProblemSimplifier : public GenericProblemImpl {
 public:
  explicit ProblemSimplifier(const std::function<Problem()>& factory);

  void solve(bool useSolverSpecificMultiObjectiveSolveIfAvailable) override;
  void simplify();
};

} // namespace facebook::algopt::lp::detail
