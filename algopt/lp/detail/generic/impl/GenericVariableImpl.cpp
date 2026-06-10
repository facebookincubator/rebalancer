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

#include "algopt/lp/detail/generic/impl/GenericVariableImpl.h"

#include "algopt/lp/detail/generic/impl/GenericExpressionImpl.h"

#include <fmt/core.h>
#include <fmt/format.h>
#include <folly/lang/Assume.h>

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

namespace facebook::algopt::lp::detail {

namespace {

thrift::VariableType toThriftType(GenericVariableImpl::Type type) {
  switch (type) {
    case GenericVariableImpl::Type::BINARY:
      return thrift::VariableType::BINARY;
    case GenericVariableImpl::Type::CONTINUOUS:
      return thrift::VariableType::CONTINUOUS;
    case GenericVariableImpl::Type::INTEGER:
      return thrift::VariableType::INTEGER;
    case GenericVariableImpl::Type::SEMI_CONTINUOUS:
      return thrift::VariableType::SEMI_CONTINUOUS;
    case GenericVariableImpl::Type::SEMI_INTEGER:
      return thrift::VariableType::SEMI_INTEGER;
    default:
      throw std::runtime_error("unexpected variable type");
  }
}

GenericVariableImpl::Type fromThriftType(thrift::VariableType type) {
  switch (type) {
    case thrift::VariableType::BINARY:
      return GenericVariableImpl::Type::BINARY;
    case thrift::VariableType::CONTINUOUS:
      return GenericVariableImpl::Type::CONTINUOUS;
    case thrift::VariableType::INTEGER:
      return GenericVariableImpl::Type::INTEGER;
    case thrift::VariableType::SEMI_CONTINUOUS:
      return GenericVariableImpl::Type::SEMI_CONTINUOUS;
    case thrift::VariableType::SEMI_INTEGER:
      return GenericVariableImpl::Type::SEMI_INTEGER;
    default:
      throw std::runtime_error("unexpected variable type");
  }
}

std::string toString(GenericVariableImpl::Type type) {
  switch (type) {
    case GenericVariableImpl::Type::BINARY:
      return "BINARY";
    case GenericVariableImpl::Type::CONTINUOUS:
      return "CONTINUOUS";
    case GenericVariableImpl::Type::INTEGER:
      return "INTEGER";
    case GenericVariableImpl::Type::SEMI_CONTINUOUS:
      return "SEMI_CONTINUOUS";
    case GenericVariableImpl::Type::SEMI_INTEGER:
      return "SEMI_INTEGER";
    default:
      throw std::runtime_error("unexpected variable type");
  }
}

double getMaxValue(GenericVariableImpl::Type type) {
  return type == GenericVariableImpl::Type::BINARY ? 1 : INFINITY;
}

double getMinValue(GenericVariableImpl::Type type) {
  return type == GenericVariableImpl::Type::BINARY ? 0 : -INFINITY;
}

} // namespace

GenericVariableImpl::GenericVariableImpl(const std::string& name, Type type)
    : name_(name) {
  type_ = type;
}

double GenericVariableImpl::getLB() const {
  return lb_.value_or(getMinValue(type_));
}

double GenericVariableImpl::getUB() const {
  return ub_.value_or(getMaxValue(type_));
}

double GenericVariableImpl::getThreshold() const {
  return threshold_.value_or(0);
}

void GenericVariableImpl::setLB(double lb) {
  lb_ = lb;
}

void GenericVariableImpl::setUB(double ub) {
  ub_ = ub;
}

void GenericVariableImpl::setThreshold(double threshold) {
  if (!hasThreshold()) {
    throw std::runtime_error(
        fmt::format(
            "cannot set threshold on a {} variable", fmt::underlying(type_)));
  }
  threshold_ = threshold;
}

void GenericVariableImpl::setLB(double lb) const {
  lb_ = lb;
}

void GenericVariableImpl::setUB(double ub) const {
  ub_ = ub;
}

void GenericVariableImpl::setThreshold(double threshold) const {
  if (!hasThreshold()) {
    throw std::runtime_error(
        fmt::format(
            "cannot set threshold on a {} variable", fmt::underlying(type_)));
  }
  threshold_ = threshold;
}

void GenericVariableImpl::updateLB(double lb) const {
  if (lb_) {
    // choose the tighter lower bound
    lb = std::max(lb, *lb_);
  }
  setLB(lb);
}

void GenericVariableImpl::updateUB(double ub) const {
  if (ub_) {
    // choose the tighter upper bound
    ub = std::min(ub, *ub_);
  }
  setUB(ub);
}

void GenericVariableImpl::updateThreshold(double threshold) const {
  if (!hasThreshold()) {
    throw std::runtime_error(
        fmt::format(
            "cannot set threshold on a {} variable", fmt::underlying(type_)));
  }
  if (threshold_) {
    // choose the tighter lower bound
    threshold = std::max(threshold, *threshold_);
  }
  setThreshold(threshold);
}

void GenericVariableImpl::setPartition(int partitionId) {
  partitionId_ = PartitionId(partitionId);
}

void GenericVariableImpl::clearValue() const {
  value_ = std::nullopt;
}

void GenericVariableImpl::setValue(double value) const {
  value_ = value;
}

bool GenericVariableImpl::hasValue() const {
  return value_.has_value();
}

double GenericVariableImpl::getValue() const {
  if (value_) {
    return *value_;
  }
  throw std::runtime_error(fmt::format("value is unset for {}", digest()));
}

double GenericVariableImpl::computeValue() const {
  auto varType = getType();
  const bool isBinaryOrInteger =
      (varType == GenericVariableImpl::Type::BINARY ||
       varType == GenericVariableImpl::Type::INTEGER);
  auto varValue = getValue();
  return isBinaryOrInteger ? round(varValue) : varValue;
}

PartitionId GenericVariableImpl::getPartitionId() const {
  if (partitionId_) {
    return *partitionId_;
  }
  throw std::runtime_error(
      fmt::format("partitionId is unset for {}", digest()));
}

bool GenericVariableImpl::hasPartitionId() const {
  return partitionId_.has_value();
}

GenericVariableImpl::Type GenericVariableImpl::getType() const {
  return type_;
}

const std::string& GenericVariableImpl::getName() const {
  return name_;
}

std::shared_ptr<ExpressionImpl> GenericVariableImpl::makeExpression(
    double coeff) const {
  return std::make_shared<GenericExpressionImpl>(coeff, shared_from_this());
}

void GenericVariableImpl::setInitialValue(double initialValue) const {
  initialValue_ = initialValue;
}
void GenericVariableImpl::clearInitialValue() const {
  initialValue_ = std::nullopt;
}

std::optional<double> GenericVariableImpl::getInitialValue() const {
  return initialValue_;
}

Variable GenericVariableImpl::initVar(Problem& p) const {
  switch (type_) {
    case Type::BINARY:
      return (p.makeBoolVar)(name_);
    case Type::INTEGER:
      return (p.makeIntVar)(name_);
    case Type::SEMI_CONTINUOUS:
      return (p.makeSemiContVar)(name_, *threshold_);
    case Type::SEMI_INTEGER:
      return (p.makeSemiIntVar)(name_, *threshold_);
    case Type::CONTINUOUS:
      return (p.makeVar)(name_);
    case Type::UNSET:
      throw std::runtime_error("Variable type was not set!");
  }
  folly::assume_unreachable();
}

Variable GenericVariableImpl::realize(Problem& p) const {
  auto var = initVar(p);

  // save other auxiliary info
  if (lb_) {
    var.setLB(*lb_);
  }
  if (ub_) {
    var.setUB(*ub_);
  }
  if (partitionId_) {
    var.setPartition(*partitionId_);
  }
  if (threshold_) {
    var.setThreshold(*threshold_);
  }
  if (initialValue_) {
    p.addStartValue(var, *initialValue_);
  }
  return var;
}

thrift::GenericVariable GenericVariableImpl::toThrift() const {
  thrift::GenericVariable thriftVar;
  thriftVar.name() = name_;
  thriftVar.type() = toThriftType(type_);
  if (lb_) {
    thriftVar.lowerBound() = *lb_;
  }
  if (ub_) {
    thriftVar.upperBound() = *ub_;
  }
  if (partitionId_) {
    thriftVar.partitionId() = *partitionId_;
  }
  if (initialValue_) {
    thriftVar.initialValue() = *initialValue_;
  }
  if (threshold_) {
    thriftVar.threshold() = *threshold_;
  }
  return thriftVar;
}

std::shared_ptr<GenericVariableImpl> GenericVariableImpl::fromThrift(
    const thrift::GenericVariable& var) {
  auto genericVar = std::make_shared<GenericVariableImpl>(
      *var.name(), fromThriftType(*var.type()));

  if (auto ub = var.upperBound()) {
    genericVar->setUB(*ub);
  }
  if (auto lb = var.lowerBound()) {
    genericVar->setLB(*lb);
  }
  if (auto partitionId = var.partitionId()) {
    genericVar->setPartition(*partitionId);
  }
  if (auto initialValue = var.initialValue()) {
    genericVar->setInitialValue(*initialValue);
  }
  if (auto threshold = var.threshold()) {
    genericVar->setThreshold(*threshold);
  }
  return genericVar;
}

std::string GenericVariableImpl::digest() const {
  return fmt::format(
      "{} [{}{}] ({} <lb: {} ub: {}> partition: {})",
      name_,
      initialValue_ ? fmt::format("{} → ", *initialValue_) : "",
      value_ ? fmt::to_string(*value_) : "",
      toString(type_),
      lb_ ? fmt::to_string(*lb_) : "",
      ub_ ? fmt::to_string(*ub_) : "",
      partitionId_ ? *partitionId_ : -1);
}

} // namespace facebook::algopt::lp::detail
