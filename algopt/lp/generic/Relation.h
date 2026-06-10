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

#include <memory>

namespace facebook::algopt::lp {

class RelationImpl {
 public:
  RelationImpl() = default;
  RelationImpl(const RelationImpl&) = default;
  RelationImpl(RelationImpl&&) = default;
  RelationImpl& operator=(const RelationImpl&) = default;
  RelationImpl& operator=(RelationImpl&&) = default;
  virtual ~RelationImpl();
};

class Relation {
 public:
  enum ConstantType {
    EQ_ZERO,
    GE_ZERO,
    LE_ZERO,
  };

  Relation(double value, ConstantType type);
  explicit Relation(std::shared_ptr<RelationImpl> relation);

  bool isConstant() const;
  double getConstantValue() const;
  ConstantType getConstantType() const;

  std::shared_ptr<const RelationImpl> get() const;

 private:
  // When relation_ equals nullptr, the Relation object represents a constant
  // relation with value constantValue_ and type constantType_.
  double constantValue_ = 0;
  ConstantType constantType_ = EQ_ZERO;
  std::shared_ptr<RelationImpl> relation_;
};

} // namespace facebook::algopt::lp
