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

#include "algopt/lp/detail/generic/impl/GenericExpressionImpl.h"

#include "algopt/lp/detail/generic/impl/GenericRelationImpl.h"
#include "algopt/lp/detail/generic/impl/GenericVariableImpl.h"
#include "algopt/lp/generic/Expression.h"
#include "algopt/lp/generic/Operators.h"

#include <fmt/format.h>
#include <folly/container/F14Set.h>
#include <folly/logging/xlog.h>
#include <folly/MapUtil.h>
#include <folly/String.h>

#include <map>
#include <memory>
#include <stdexcept>
#include <string>

namespace facebook::algopt::lp::detail {

namespace {

// helper function to create image
void addVariableTerm(
    GenericExpressionImpl& imageExpr,
    const ConstGenericVarPtr& var,
    double coeff,
    RealizationRule realizationRule,
    bool include) {
  auto isValNearZero = [&var] { return std::abs(var->getValue()) < EPSILON; };
  if (include) {
    // the variable will be included in the image
    imageExpr.add(var, coeff);
    if (realizationRule.allowedTermsRule ==
            RealizationRule::AllowedTerms::FULLY_CONSTRAINED &&
        !isValNearZero()) {
      imageExpr.add(-coeff * var->getValue());
    }
  } else {
    // the variable will not be included in the image but will be substituted
    // or ignored depending on realizationRules
    if (realizationRule.remainingTermsRule ==
            RealizationRule::RemainingTerms::SUBSTITUTE_CURR_VALUE &&
        !isValNearZero()) {
      imageExpr.add(coeff * var->getValue());
    }
  }
}

} // namespace

RealizationRule::RealizationRule(
    RemainingTerms remainingRule,
    ConstantTerm constantRule,
    AllowedTerms allowedRule)
    : allowedTermsRule(allowedRule),
      remainingTermsRule(remainingRule),
      constantTermRule(constantRule) {}

RealizationRule::RealizationRule(
    AllowedTerms allowedRule,
    ConstantTerm constantRule,
    RemainingTerms remainingRule)
    : allowedTermsRule(allowedRule),
      remainingTermsRule(remainingRule),
      constantTermRule(constantRule) {}

RealizationRule::RealizationRule(ConstantTerm constantRule)
    : constantTermRule(constantRule) {}

GenericExpressionImpl::GenericExpressionImpl(double constant)
    : constant_(constant) {}

GenericExpressionImpl::GenericExpressionImpl(
    double coeff,
    std::shared_ptr<const GenericVariableImpl> variable)
    : constant_(0) {
  // only create an expression if needed
  if (std::abs(coeff) > EPSILON) {
    linearCoeff_.emplace(variable, coeff);
  }
}

std::shared_ptr<ExpressionImpl> GenericExpressionImpl::clone() const {
  return std::make_shared<GenericExpressionImpl>(*this);
}

void GenericExpressionImpl::add(double constant) {
  constant_ += constant;
}

void GenericExpressionImpl::add(
    std::shared_ptr<const ExpressionImpl> expression) {
  auto genericExpr =
      std::static_pointer_cast<const GenericExpressionImpl>(expression);
  if (!genericExpr) {
    throw std::runtime_error(
        "Can only add GenericExpressionImpls to a GenericExpressionImpl");
  }
  for (const auto& [var, delta] : genericExpr->linearCoeff_) {
    add(var, delta);
  }
  if (genericExpr->quadraticCoeff_) {
    for (const auto& [varPair, delta] : *genericExpr->quadraticCoeff_) {
      add(varPair, delta);
    }
  }
  constant_ += genericExpr->constant_;
}

void GenericExpressionImpl::multiply(double constant) {
  partitionIndex_ = std::nullopt;
  if (std::abs(constant) < EPSILON) {
    linearCoeff_.clear();
    if (quadraticCoeff_) {
      quadraticCoeff_->clear();
    }
    constant_ = 0;
  } else {
    for (auto& [var, coeff] : linearCoeff_) {
      coeff *= constant;
    }
    if (quadraticCoeff_) {
      for (auto& [varPair, coeff] : *quadraticCoeff_) {
        coeff *= constant;
      }
    }
    constant_ *= constant;
  }
}

ConstGenericVarPtrPair GenericExpressionImpl::makeOrderedPair(
    ConstGenericVarPtr x,
    ConstGenericVarPtr y) {
  return x < y ? std::make_pair(x, y) : std::make_pair(y, x);
}

void GenericExpressionImpl::multiply(
    std::shared_ptr<const ExpressionImpl> expression) {
  partitionIndex_ = std::nullopt;
  auto genericExpr =
      std::static_pointer_cast<const GenericExpressionImpl>(expression);
  if (!genericExpr) {
    throw std::runtime_error(
        "Can only add GenericExpressionImpls to a GenericExpressionImpl");
  } else if (isConstant()) {
    auto origConstant = constant_;
    overwrite(genericExpr);
    multiply(origConstant);
  } else if (genericExpr->isQuadratic()) {
    throw std::runtime_error(
        "Multiplication of a quadratic expression by non constant is not supported");
  } else {
    // multiply two linear expressions e1 * e2
    // Step 1: generate the quadratic terms: e1.vars * e2.vars
    quadraticCoeff_.emplace();
    for (auto& [var1, coef1] : linearCoeff_) {
      for (auto& [var2, coef2] : genericExpr->linearCoeff_) {
        // ensure that variable pairs are ordered
        auto varPair = makeOrderedPair(var1, var2);
        auto& coef = (*quadraticCoeff_)[varPair];
        coef += coef1 * coef2;
      }
    }
    // Step 2: generate the linear terms corresponding to e1.vars * e2.const
    for (auto& [_, coef] : linearCoeff_) {
      coef *= genericExpr->constant_;
    }
    // Step 3: generate the linear terms corresponding to e1.const * e2.vars
    for (const auto& [var, coef] : genericExpr->linearCoeff_) {
      linearCoeff_[var] += constant_ * coef;
    }
    // update the constant term as e1.const * e2.const
    constant_ *= genericExpr->constant_;

    // sparsify resulting linear and quadratic terms
    auto sparsify = [](auto& container) {
      for (auto iter = container.begin(); iter != container.end();) {
        auto coef = iter->second;
        if (std::abs(coef) < EPSILON) {
          iter = container.erase(iter);
        } else {
          iter++;
        }
      }
    };
    sparsify(linearCoeff_);
    sparsify(*quadraticCoeff_);
  }
}

std::shared_ptr<RelationImpl> GenericExpressionImpl::makeEqualZeroRelation()
    const {
  auto expr = std::make_shared<GenericExpressionImpl>(*this);
  return std::make_shared<GenericRelationImpl>(
      expr, Relation::ConstantType::EQ_ZERO);
}

std::shared_ptr<RelationImpl> GenericExpressionImpl::makeLessEqualZeroRelation()
    const {
  auto expr = std::make_shared<GenericExpressionImpl>(*this);
  return std::make_shared<GenericRelationImpl>(
      expr, Relation::ConstantType::LE_ZERO);
}

std::shared_ptr<RelationImpl>
GenericExpressionImpl::makeGreaterEqualZeroRelation() const {
  auto expr = std::make_shared<GenericExpressionImpl>(*this);
  return std::make_shared<GenericRelationImpl>(
      expr, Relation::ConstantType::GE_ZERO);
}

double GenericExpressionImpl::getValue() const {
  double exprValue = constant_;
  for (const auto& [var, coeff] : linearCoeff_) {
    exprValue += var->getValue() * coeff;
  }
  if (quadraticCoeff_) {
    for (const auto& [varPair, coeff] : *quadraticCoeff_) {
      exprValue +=
          varPair.first->getValue() * varPair.second->getValue() * coeff;
    }
  }
  return exprValue;
}

double GenericExpressionImpl::computeValue() const {
  double exprValue = constant_;
  for (const auto& [var, coeff] : linearCoeff_) {
    exprValue += var->computeValue() * coeff;
  }
  if (quadraticCoeff_) {
    for (const auto& [varPair, coeff] : *quadraticCoeff_) {
      exprValue += varPair.first->computeValue() *
          varPair.second->computeValue() * coeff;
    }
  }
  return exprValue;
}

std::string GenericExpressionImpl::digest(bool showValues, size_t limit) const {
  std::vector<std::string> desc;
  desc.push_back(fmt::to_string(constant_));
  // digest will be used for debug/unit tests, so better to ensure that
  // vars are ordered by name
  auto comp = [](ConstGenericVarPtr a, ConstGenericVarPtr b) {
    return a->getName() < b->getName();
  };
  auto compPair = [&](const ConstGenericVarPtrPair& a,
                      const ConstGenericVarPtrPair& b) {
    return a.first != b.first ? comp(a.first, b.first)
                              : comp(a.second, b.second);
  };

  // 1. add linear terms to description
  const std::map<ConstGenericVarPtr, double, decltype(comp)> orderedVars(
      linearCoeff_.begin(), linearCoeff_.end(), comp);
  bool allVarsHaveValue = true;
  for (const auto& [var, coeff] : orderedVars) {
    if (desc.size() >= limit) {
      desc.push_back(
          fmt::format(" ... {} more ... ", linearCoeff_.size() - limit));
      break;
    }
    auto value = showValues && var->hasValue()
        ? fmt::format("[{}]", var->getValue())
        : "";
    desc.push_back(fmt::format("({} · {}{})", coeff, var->getName(), value));
    allVarsHaveValue = allVarsHaveValue && var->hasValue();
  }

  // 2. add quadratic terms to description
  if (quadraticCoeff_) {
    const std::map<ConstGenericVarPtrPair, double, decltype(compPair)>
        orderedVarsQuadratic(
            quadraticCoeff_->begin(), quadraticCoeff_->end(), compPair);
    for (const auto& [varPair, coeff] : orderedVarsQuadratic) {
      if (desc.size() >= limit) {
        desc.push_back(
            fmt::format(" ... {} more ... ", quadraticCoeff_->size() - limit));
        break;
      }
      auto var1 = varPair.first;
      auto var2 = varPair.second;
      if (showValues) {
        desc.push_back(
            fmt::format(
                "({} · {}{} · {}{})",
                coeff,
                var1->getName(),
                var1->hasValue() ? fmt::format("[{}]", var1->getValue()) : "",
                var2->getName(),
                var2->hasValue() ? fmt::format("[{}]", var2->getValue()) : ""));
      } else {
        desc.push_back(
            fmt::format(
                "({} · {} · {})", coeff, var1->getName(), var2->getName()));
      }
      allVarsHaveValue =
          allVarsHaveValue && var1->hasValue() && var2->hasValue();
    }
  }

  const std::string exprValStr =
      showValues && allVarsHaveValue ? fmt::format(" [{}]", getValue()) : "";
  return folly::join(" + ", desc) + exprValStr;
}

void GenericExpressionImpl::print() const {
  XLOG(INFO) << digest();
}

void GenericExpressionImpl::add(ConstGenericVarPtr var, double delta) {
  partitionIndex_ = std::nullopt;
  auto& value = linearCoeff_[var];
  value += delta;
  // only retain nonzero variables to preserve sparsity
  if (std::abs(value) < EPSILON) {
    linearCoeff_.erase(var);
  }
}

void GenericExpressionImpl::add(
    const ConstGenericVarPtrPair& varPair,
    double delta) {
  partitionIndex_ = std::nullopt;
  if (!quadraticCoeff_) {
    quadraticCoeff_.emplace();
  }
  auto& value = (*quadraticCoeff_)[varPair];
  value += delta;
  // only retain nonzero variables to preserve sparsity
  if (std::abs(value) < EPSILON) {
    quadraticCoeff_->erase(varPair);
  }
}

std::optional<PartitionId> GenericExpressionImpl::getPartitionId() const {
  std::optional<PartitionId> partitionId;
  auto belongsToDifferentPartition = [&](ConstGenericVarPtr var) {
    if (partitionId) {
      return var->getPartitionId() != *partitionId;
    } else {
      partitionId = var->getPartitionId();
      return false;
    }
  };
  for (const auto& [var, _] : linearCoeff_) {
    if (belongsToDifferentPartition(var)) {
      return std::nullopt;
    }
  }
  if (quadraticCoeff_) {
    for (const auto& [varPair, _] : *quadraticCoeff_) {
      if (belongsToDifferentPartition(varPair.first) ||
          belongsToDifferentPartition(varPair.second)) {
        return std::nullopt;
      }
    }
  }
  return partitionId;
}

bool GenericExpressionImpl::isConstant() const {
  return linearCoeff_.empty() && !isQuadratic();
}

bool GenericExpressionImpl::isQuadratic() const {
  if (quadraticCoeff_ && !quadraticCoeff_->empty()) {
    return true;
  }
  return false;
}

double GenericExpressionImpl::getConstant() const {
  return constant_;
}

const folly::F14FastMap<ConstGenericVarPtr, double>&
GenericExpressionImpl::getLinearCoeffMap() const {
  return linearCoeff_;
}

const std::optional<folly::F14FastMap<ConstGenericVarPtrPair, double>>&
GenericExpressionImpl::getQuadraticCoeffMap() const {
  return quadraticCoeff_;
}

const folly::F14FastMap<ConstGenericVarPtr, double>&
GenericExpressionImpl::getCoeffMap() const {
  if (isQuadratic()) {
    throw std::runtime_error(
        "getCoeffMap is unsupported for quadratic expressions");
  }
  return linearCoeff_;
}

Expression GenericExpressionImpl::realize(
    const folly::F14FastMap<ConstGenericVarPtr, Variable>& oldVarsToNew) const {
  Expression expr(constant_);
  for (auto& [var, coeff] : linearCoeff_) {
    expr += coeff * oldVarsToNew.at(var);
  }
  if (quadraticCoeff_) {
    for (auto& [varPair, coeff] : *quadraticCoeff_) {
      Expression quadComp(coeff);
      quadComp *= oldVarsToNew.at(varPair.first);
      quadComp *= oldVarsToNew.at(varPair.second);
      expr += quadComp;
    }
  }
  return expr;
}

void GenericExpressionImpl::setupPartitionIndex() {
  if (!partitionIndex_) {
    partitionIndex_.emplace();
    // the partition index is used as a cache for image(..) function calls
    // which are only supported on linear expressions
    for (const auto& [var, coeff] : linearCoeff_) {
      partitionIndex_.value()[var->getPartitionId()][var] = coeff;
    }
  }
}

std::tuple<int, int, int> GenericExpressionImpl::getNumCoeffParity() const {
  int numPosCoefs = 0;
  int numNegCoefs = 0;
  int numZeroCoefs = 0;
  auto countCoefficients = [&](double coef) {
    if (std::abs(coef) < EPSILON) {
      // terms with near zero coeff are discarded during expression building
      // so this number should be usually zero
      numZeroCoefs++;
    } else if (coef < 0) {
      numNegCoefs++;
    } else {
      numPosCoefs++;
    }
  };
  for (const auto& [_, coef] : linearCoeff_) {
    countCoefficients(coef);
  }
  if (quadraticCoeff_) {
    for (const auto& [_, coef] : *quadraticCoeff_) {
      countCoefficients(coef);
    }
  }
  return {numPosCoefs, numNegCoefs, numZeroCoefs};
}

Expression GenericExpressionImpl::image(
    const folly::F14FastSet<PartitionId>& allowedPartitions,
    RealizationRule realizationRule) const {
  if (isQuadratic()) {
    throw std::runtime_error(
        "image() functions are not supported for quadratic expressions");
  }
  auto imageExpr = std::make_shared<GenericExpressionImpl>(0);
  if (!partitionIndex_) {
    XLOG(WARNING)
        << "Partition index is not built. Taking images of large expressions may be slow";
    for (auto& [var, coeff] : linearCoeff_) {
      // partitionId must be set for all decomposable variables
      const bool include = allowedPartitions.contains(var->getPartitionId());
      addVariableTerm(*imageExpr, var, coeff, realizationRule, include);
    }
  } else {
    for (auto& partitionId : allowedPartitions) {
      if (auto coefmap = folly::get_ptr(*partitionIndex_, partitionId)) {
        for (auto& [var, coeff] : *coefmap) {
          // partitionId must be set for all decomposable variables
          addVariableTerm(*imageExpr, var, coeff, realizationRule, true);
        }
      }
    }
    const int numRemainingTerms =
        getCoeffMap().size() - imageExpr->getCoeffMap().size();
    if (numRemainingTerms > 0 &&
        realizationRule.remainingTermsRule ==
            RealizationRule::RemainingTerms::SUBSTITUTE_CURR_VALUE) {
      auto allTermsValue = getValue() - getConstant();
      auto selectedTermsValue =
          imageExpr->getValue() - imageExpr->getConstant();
      auto remainingTermsValue = allTermsValue - selectedTermsValue;
      imageExpr->add(remainingTermsValue);
    }
  }
  if (realizationRule.constantTermRule ==
      RealizationRule::ConstantTerm::RETAIN) {
    imageExpr->add(constant_);
  }
  return Expression(imageExpr);
}

Expression GenericExpressionImpl::image(
    RealizationRule::ConstantTerm constantRule) const {
  auto imageExpr = Expression(clone());
  if (constantRule == RealizationRule::ConstantTerm::DROP) {
    imageExpr -= getConstant();
  }
  return imageExpr;
}

void GenericExpressionImpl::overwrite(
    const std::shared_ptr<const GenericExpressionImpl>& expr) {
  constant_ = expr->constant_;
  linearCoeff_ = expr->linearCoeff_;
  if (expr->quadraticCoeff_) {
    quadraticCoeff_ = *expr->quadraticCoeff_;
  }
}

thrift::GenericExpression GenericExpressionImpl::toThrift(
    const folly::F14FastMap<ConstGenericVarPtr, int64_t>& varToId) const {
  thrift::GenericExpression expr;
  expr.constant() = constant_;
  for (auto& [var, coef] : linearCoeff_) {
    expr.linearCoeffs()->emplace(varToId.at(var), coef);
  }
  if (quadraticCoeff_) {
    for (auto& [varPair, coef] : *quadraticCoeff_) {
      auto& quadraticCoeffs = *expr.quadraticCoeffs();
      auto id1 = varToId.at(varPair.first);
      auto id2 = varToId.at(varPair.second);
      quadraticCoeffs[id1][id2] = coef;
    }
  }
  return expr;
}

std::shared_ptr<GenericExpressionImpl> GenericExpressionImpl::fromThrift(
    thrift::GenericExpression expr,
    const folly::F14FastMap<int64_t, ConstGenericVarPtr>& idToVar) {
  // add constant term
  auto newExpr = std::make_shared<GenericExpressionImpl>(*expr.constant());
  // add linear terms
  for (auto& [varId, coeff] : expr.linearCoeffs().value()) {
    const auto& var = idToVar.at(varId);
    newExpr->add(var, coeff);
  }
  // add quadratic terms
  if (!expr.quadraticCoeffs().value().empty()) {
    for (auto& [firstVarId, coefMap] : expr.quadraticCoeffs().value()) {
      const auto& firstVar = idToVar.at(firstVarId);
      for (auto& [secondVarId, coeff] : coefMap) {
        const auto& secondVar = idToVar.at(secondVarId);
        auto varPair = makeOrderedPair(firstVar, secondVar);
        newExpr->add(varPair, coeff);
      }
    }
  }
  return newExpr;
}

} // namespace facebook::algopt::lp::detail
