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

#include "algopt/lp/fast/FastVariableImpl.h"

#include "algopt/lp/fast/FastExpressionImpl.h"

#include <stdexcept>

namespace facebook::algopt::lp {

FastVariableImpl::FastVariableImpl(
    uint64_t variableId,
    InnerVariable* innerVariable)
    : variableId_(variableId), innerVariable_(innerVariable) {
  type_ = innerVariable_->type;
}

void FastVariableImpl::setLB(double lb) {
  innerVariable_->lb = lb;
}

void FastVariableImpl::setUB(double ub) {
  innerVariable_->ub = ub;
}

void FastVariableImpl::setThreshold(double threshold) {
  innerVariable_->threshold = threshold;
}

bool FastVariableImpl::hasThreshold() const {
  return innerVariable_->threshold.has_value();
}

double FastVariableImpl::getValue() const {
  if (!innerVariable_->value.has_value()) {
    throw std::runtime_error(
        "Variable '" + innerVariable_->name +
        "' has no value set. FastProblemImpl does not support solving.");
  }
  return *innerVariable_->value;
}

std::shared_ptr<ExpressionImpl> FastVariableImpl::makeExpression(
    double coeff) const {
  return std::make_shared<FastExpressionImpl>(coeff, variableId_);
}

uint64_t FastVariableImpl::getVariableId() const {
  return variableId_;
}

} // namespace facebook::algopt::lp
