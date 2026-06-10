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

#include <algorithm>
#include <limits>
#include <optional>

namespace facebook::rebalancer::entities {

struct ObjectDimensionValueStats {
  bool observe(const double value) {
    if (value < 0) {
      hasNegative = true;
    } else if (value == 0.0) {
      hasZero = true;
    } else if (value > 0) {
      minPositive_ = std::min(value, minPositive_);
    }
    maxValue = std::max(value, maxValue);
    return value == 0.0;
  }

  std::optional<double> getMinPositive() const {
    return minPositive_ == std::numeric_limits<double>::infinity()
        ? std::nullopt
        : std::make_optional(minPositive_);
  }

  bool hasNegative = false;
  bool hasZero = false;
  double maxValue = std::numeric_limits<double>::lowest();

 private:
  double minPositive_ = std::numeric_limits<double>::infinity();
};

} // namespace facebook::rebalancer::entities
