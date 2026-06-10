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
#include "algopt/lp/generic/Relation.h"

#include <vector>

namespace facebook::algopt::lp {

class FastRelationImpl : public RelationImpl {
 public:
  FastRelationImpl(
      std::vector<Term> terms,
      double constant,
      Relation::ConstantType sense);

  const std::vector<Term>& getTerms() const;
  double getConstant() const;
  Relation::ConstantType getSense() const;

 private:
  std::vector<Term> terms_;
  double constant_;
  Relation::ConstantType sense_;
};

} // namespace facebook::algopt::lp
