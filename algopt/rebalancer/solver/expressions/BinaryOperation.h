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

#include "algopt/rebalancer/solver/expressions/Expression.h"
#include "algopt/rebalancer/solver/if/gen-cpp2/packer_types.h"
#include "algopt/rebalancer/solver/utils/BoundConstraints.h"

namespace facebook::rebalancer {

class BinaryOperation : public Expression {
 public:
  explicit BinaryOperation(
      std::shared_ptr<Expression> expr1,
      std::shared_ptr<Expression> expr2,
      std::shared_ptr<const entities::Universe> universe);

  void updateEquivalenceSets(EquivalenceSets& equivSets) const override;

 protected:
  virtual std::vector<double> bound_candidates(
      Context& context,
      const BoundConstraints& bc) const = 0;

  std::shared_ptr<Expression> child1st;
  std::shared_ptr<Expression> child2nd;

 private:
  virtual Bounds innerLowerAndUpperBounds(
      Context& context,
      const BoundConstraints& bc) const override;
};

} // namespace facebook::rebalancer
