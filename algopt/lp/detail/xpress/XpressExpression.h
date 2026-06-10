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

#include "algopt/lp/environment/Environment.h" // NOLINT

#ifdef REBALANCER_USE_XPRESS

#include "algopt/lp/detail/xpress/xpress.h"
#include "algopt/lp/generic/Expression.h"

namespace facebook::algopt::lp::detail {

class XpressExpression : public ExpressionImpl {
 public:
  explicit XpressExpression(const dashoptimization::XPRBexpr& expression);

  std::shared_ptr<ExpressionImpl> clone() const override;

  void add(double constant) override;
  void add(std::shared_ptr<const ExpressionImpl> expression) override;

  void multiply(double constant) override;
  void multiply(std::shared_ptr<const ExpressionImpl> expression) override;

  std::shared_ptr<RelationImpl> makeEqualZeroRelation() const override;
  std::shared_ptr<RelationImpl> makeLessEqualZeroRelation() const override;
  std::shared_ptr<RelationImpl> makeGreaterEqualZeroRelation() const override;

  double getValue() const override;
  double computeValue() const override;

  void print() const override;

  const dashoptimization::XPRBexpr& get() const;

 private:
  static constexpr int kMaxBufferSize = 1000;

 private:
  template <typename T>
  void addBuffered(const T& term) {
    buffer_ += term;
    ++bufferSize_;

    if (bufferSize_ >= kMaxBufferSize) {
      flushBuffer();
    }
  }

  void flushBuffer() const;

 private:
  mutable dashoptimization::XPRBexpr expression_;
  mutable dashoptimization::XPRBexpr buffer_;
  mutable int bufferSize_ = 0;
};

} // namespace facebook::algopt::lp::detail

#endif
