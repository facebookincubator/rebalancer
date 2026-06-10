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

#include "algopt/lp/detail/generic/impl/thrift/gen-cpp2/model_types.h"
#include "algopt/lp/generic/Problem.h"
#include "algopt/lp/generic/Variable.h"

#include <cstddef>
#include <memory>
#include <optional>

namespace facebook::algopt::lp::detail {

class PartitionId {
 public:
  explicit PartitionId(int id) : id_(id) {}
  operator size_t() const {
    return id_;
  }

  size_t asInt() const {
    return id_;
  }

 private:
  size_t id_;
};

class GenericVariableImpl
    : public VariableImpl,
      public std::enable_shared_from_this<GenericVariableImpl> {
 public:
  explicit GenericVariableImpl(const std::string& name, Type type);

  void setLB(double lb) override;
  void setUB(double ub) override;
  void setThreshold(double ub) override;

  // TODO: switch to type PartitionId in Variable interface
  void setPartition(int partitionId) override;
  void setValue(double value) const;
  void clearValue() const;
  void setInitialValue(double value) const;
  void clearInitialValue() const;

  // add const versions of setLB(), setUB() functions so that
  // they can be modified by const variable ptrs.
  // This exposes the virtual non-const versions to const objects
  void setLB(double lb) const;
  void setUB(double ub) const;
  void setThreshold(double threshold) const;
  // updates the existing upper/lower bound of a variable
  void updateLB(double lb) const;
  void updateUB(double ub) const;
  void updateThreshold(double threshold) const;

  double getLB() const;
  double getUB() const;
  double getThreshold() const;

  double getValue() const override;
  double computeValue() const;

  bool hasValue() const;
  std::optional<double> getInitialValue() const;
  PartitionId getPartitionId() const;
  bool hasPartitionId() const;
  Type getType() const;
  const std::string& getName() const;

  virtual std::shared_ptr<ExpressionImpl> makeExpression(
      double coeff) const override;

  // realize this variable into a given problem
  Variable realize(Problem& p) const;

  // util functions to read/write in thrift format
  thrift::GenericVariable toThrift() const;
  static std::shared_ptr<GenericVariableImpl> fromThrift(
      const thrift::GenericVariable& var);

  std::string digest() const;

 private:
  Variable initVar(Problem&) const;
  std::string name_;
  std::optional<PartitionId> partitionId_;
  // This is probably not ideal but because of how problems are created
  // GenericProblemImpl only has access to "const shared_ptr" of variables.
  // Making these fields mutable as these values need to be set internally
  mutable std::optional<double> threshold_;
  mutable std::optional<double> lb_;
  mutable std::optional<double> ub_;
  mutable std::optional<double> value_;
  mutable std::optional<double> initialValue_;
};

} // namespace facebook::algopt::lp::detail

// Define hash functions so that F14FastMap/Set can work with PartitionIds
namespace std {

template <>
struct hash<facebook::algopt::lp::detail::PartitionId> {
  size_t operator()(const facebook::algopt::lp::detail::PartitionId& k) const {
    return hash<size_t>{}(k);
  }
};

} // namespace std
