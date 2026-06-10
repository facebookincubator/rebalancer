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

#include "algopt/lp/detail/polyglot/LpProblemBinding.h"

#include <folly/container/irange.h>

namespace facebook::algopt::lp::binding {

Expression sumExpressions(const std::vector<Expression>& expressions) noexcept {
  if (expressions.empty()) {
    return Expression(0.0);
  }

  lp::Expression result = expressions[0].getImpl();
  for (const auto i : folly::irange(1, expressions.size())) {
    result += expressions[i].getImpl();
  }
  return Expression(std::move(result));
}

Expression sumVariables(const std::vector<Variable>& variables) noexcept {
  if (variables.empty()) {
    return Expression(0.0);
  }

  lp::Expression result = variables[0].getImpl();
  for (const auto i : folly::irange(1, variables.size())) {
    result += variables[i].getImpl();
  }
  return Expression(std::move(result));
}

} // namespace facebook::algopt::lp::binding
