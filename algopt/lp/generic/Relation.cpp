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

#include "algopt/lp/generic/Relation.h"

namespace facebook::algopt::lp {

RelationImpl::~RelationImpl() = default;

Relation::Relation(double value, ConstantType type)
    : constantValue_(value), constantType_(type) {}

Relation::Relation(std::shared_ptr<RelationImpl> relation)
    : relation_(relation) {}

bool Relation::isConstant() const {
  return !relation_;
}

double Relation::getConstantValue() const {
  return constantValue_;
}

Relation::ConstantType Relation::getConstantType() const {
  return constantType_;
}

std::shared_ptr<const RelationImpl> Relation::get() const {
  return relation_;
}

} // namespace facebook::algopt::lp
