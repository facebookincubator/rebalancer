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

#include "algopt/rebalancer/solver/expressions/Log.h"

#include "algopt/rebalancer/solver/expressions/Transform.h"

namespace {
constexpr std::string_view type = "Log";
}

namespace facebook::rebalancer {

Log::Log(
    std::shared_ptr<Expression> expr,
    std::shared_ptr<const entities::Universe> universe)
    : Transform(std::move(expr), std::move(universe)) {
  setInitialValue(perform_transform(getOnlyChildRawPtr()->getInitialValue()));
}

const std::string_view& Log::getType() const {
  return type;
}

double Log::perform_transform(double val) const {
  return (val <= 0) ? -1 * std::numeric_limits<double>::max() : log(val);
}

bool Log::inner_is_integer(Context& /* not used */) {
  return false;
}

} // namespace facebook::rebalancer
