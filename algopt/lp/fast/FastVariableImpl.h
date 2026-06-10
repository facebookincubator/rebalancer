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

#include "algopt/lp/fast/FastTypes.h"
#include "algopt/lp/generic/Variable.h"

#include <cstdint>
#include <memory>

namespace facebook::algopt::lp {

class FastVariableImpl : public VariableImpl {
 public:
  FastVariableImpl(uint64_t variableId, InnerVariable* innerVariable);

  void setLB(double lb) override;
  void setUB(double ub) override;
  void setThreshold(double threshold) override;
  bool hasThreshold() const override;

  double getValue() const override;

  std::shared_ptr<ExpressionImpl> makeExpression(double coeff) const override;

  uint64_t getVariableId() const;

 private:
  uint64_t variableId_;
  InnerVariable* innerVariable_;
};

} // namespace facebook::algopt::lp
