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

#include "algopt/lp/detail/gurobi/GurobiRelation.h"

#ifdef REBALANCER_USE_GUROBI

namespace facebook::algopt::lp::detail {

GurobiRelation::GurobiRelation(const GRBTempConstr& relation, bool isLinear)
    : relation_(relation), isLinear_(isLinear) {}

const GRBTempConstr& GurobiRelation::get() const {
  return relation_;
}

bool GurobiRelation::isLinear() const {
  return isLinear_;
}

} // namespace facebook::algopt::lp::detail

#endif
