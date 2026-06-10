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

#include "algopt/lp/fast/FastRelationImpl.h"

namespace facebook::algopt::lp {

FastRelationImpl::FastRelationImpl(
    std::vector<Term> terms,
    double constant,
    Relation::ConstantType sense)
    : terms_(std::move(terms)), constant_(constant), sense_(sense) {}

const std::vector<Term>& FastRelationImpl::getTerms() const {
  return terms_;
}

double FastRelationImpl::getConstant() const {
  return constant_;
}

Relation::ConstantType FastRelationImpl::getSense() const {
  return sense_;
}

} // namespace facebook::algopt::lp
