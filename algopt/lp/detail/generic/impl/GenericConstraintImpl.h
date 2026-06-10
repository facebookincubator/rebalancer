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

#include "algopt/lp/detail/generic/impl/GenericRelationImpl.h"
#include "algopt/lp/detail/generic/impl/GenericVariableImpl.h"
#include "algopt/lp/generic/Constraint.h"

namespace facebook::algopt::lp::detail {

class GenericRelationImpl;

class GenericConstraintImpl : public ConstraintImpl {
 public:
  explicit GenericConstraintImpl(
      const GenericRelationImpl& relation,
      const std::string& name);

  std::optional<PartitionId> getPartitionId() const;
  bool isComplicating() const;
  const GenericRelationImpl& getRelation() const;
  void setName(std::string);
  std::string getName() const;
  thrift::GenericConstraint toThrift(
      const folly::F14FastMap<ConstGenericVarPtr, int64_t>& varToId) const;
  static std::shared_ptr<const RelationImpl> fromThrift(
      const thrift::GenericConstraint& ctr,
      const folly::F14FastMap<int64_t, ConstGenericVarPtr>& idToVar);

 private:
  std::string name_;
  GenericRelationImpl relation_;
};

} // namespace facebook::algopt::lp::detail
