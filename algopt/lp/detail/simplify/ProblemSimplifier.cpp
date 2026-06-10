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

#include "algopt/lp/detail/simplify/ProblemSimplifier.h"

#include "algopt/lp/generic/Operators.h"
#include "algopt/rebalancer/algopt_common/Timer.h"

#include <folly/logging/xlog.h>

namespace facebook::algopt::lp::detail {

namespace {
double getSignedVarLb(const ConstGenericVarPtr& var, bool positiveCoef) {
  if (var->hasValue()) {
    return var->getValue();
  }
  return positiveCoef ? var->getLB() : var->getUB();
}
double getSignedVarUb(const ConstGenericVarPtr& var, bool positiveCoef) {
  if (var->hasValue()) {
    return var->getValue();
  }
  return positiveCoef ? var->getUB() : var->getLB();
}
std::pair<double, double> calculateLbAndUbForConstraint(
    const GenericRelationImpl& relation,
    double tolerance) {
  const auto& expr = relation.getExpr();
  double lb = expr->getConstant();
  double ub = expr->getConstant();
  for (const auto& [var, coef] : relation.getExpr()->getCoeffMap()) {
    if (std::abs(coef) < tolerance) {
      continue;
    }
    if (lb != -INFINITY) {
      auto signedVarLb = getSignedVarLb(var, coef > 0);
      if (signedVarLb != INFINITY && signedVarLb != -INFINITY) {
        lb += coef * signedVarLb;
      } else {
        lb = -INFINITY;
      }
    }
    if (ub != INFINITY) {
      auto signedVarUb = getSignedVarUb(var, coef > 0);
      if (signedVarUb != INFINITY && signedVarUb != -INFINITY) {
        ub += coef * signedVarUb;
      } else {
        ub = INFINITY;
      }
    }
    // Note that this cuts short calculation of ub if constraint is LEQ and
    // cuts short calculation of lb if constraint is GEQ. Those values are
    // not useful to proving fixed variables.
    if ((relation.isLeqZero() && lb == -INFINITY) ||
        (relation.isGeqZero() && ub == INFINITY) ||
        (lb == -INFINITY && ub == INFINITY)) {
      break;
    }
  }
  return std::make_pair(lb, ub);
}

int filterFixedVars(std::shared_ptr<GenericExpressionImpl>& expr) {
  auto coefs = expr->getCoeffMap();
  auto numFinalElements = coefs.size();
  for (const auto& [var, coef] : coefs) {
    if (var->hasValue()) {
      expr->add(coef * var->getValue());
      expr->add(var, -1.0 * coef);
      numFinalElements--;
    }
  }
  return numFinalElements;
}

} // namespace

ProblemSimplifier::ProblemSimplifier(const std::function<Problem()>& factory)
    : GenericProblemImpl(factory) {}

// Using bounds on variables we first compute the bounds (upper, lower)
// of a  constraint. Next we want to find constraints that are only satisfied if
// the variable assume the bound values. That is:
// - A "<= 0" constraint  with lb =  0, can only be satisfied if the variables
// assume their bound values.
// - Similarly a ">= 0" constraint with ub = 0 can only be satisfied if the
// variables assume their bound values.
void ProblemSimplifier::simplify() {
  const Timer timer(true);

  int numInitialElements = 0;
  int numFinalElements = 0;
  int numInitialVars = getVars().size();
  int numInitialConstraints = constraints_.rlock()->size();
  auto tolerance = 1e-8;

  constraints_.withWLock([&](auto& wlockedConstraints) {
    for (auto iter = wlockedConstraints.begin();
         iter != wlockedConstraints.end();) {
      const auto& constraint = *iter;
      const auto& relation = constraint->getRelation();
      const auto& expr = relation.getExpr();
      const auto& coefs = expr->getCoeffMap();

      const auto [lb, ub] = calculateLbAndUbForConstraint(relation, tolerance);

      numInitialElements += coefs.size();
      if ((relation.isLeqZero() || relation.isEqZero()) &&
          std::abs(lb) < tolerance) {
        for (const auto& [var, coef] : coefs) {
          if (std::abs(coef) < tolerance) {
            continue;
          }
          var->setValue(getSignedVarLb(var, coef > 0));
        }
        iter = wlockedConstraints.erase(iter);
      } else if (
          (relation.isGeqZero() || relation.isEqZero()) &&
          std::abs(ub) < tolerance) {
        for (const auto& [var, coef] : coefs) {
          if (std::abs(coef) < tolerance) {
            continue;
          }
          var->setValue(getSignedVarUb(var, coef > 0));
        }
        iter = wlockedConstraints.erase(iter);
      } else {
        iter++;
      }
    }

    for (auto iter = wlockedConstraints.begin();
         iter != wlockedConstraints.end();) {
      auto expr = (*iter)->getRelation().getExpr();
      auto numRemainingVars = filterFixedVars(expr);
      if (numRemainingVars == 0) {
        // all variables have been filtered out, erase this constraint
        iter = wlockedConstraints.erase(iter);
      } else {
        numFinalElements += numRemainingVars;
        iter++;
      }
    }
  });

  for (auto& objective : objectives_) {
    filterFixedVars(objective);
  }

  int numFinalVars = getVars().size();
  int numFinalConstraints = constraints_.rlock()->size();

  XLOG(INFO) << fmt::format(
      "Simplification took {:.2f} seconds, elements {} -> {} ({:.2f}%), "
      "constraints {} -> {} ({:.2f}%), free variables {} -> {} ({:.2f}%)",
      timer.getSeconds(),
      numInitialElements,
      numFinalElements,
      100.0 * (numFinalElements - numInitialElements) / numInitialElements,
      numInitialConstraints,
      numFinalConstraints,
      100.0 * (numFinalConstraints - numInitialConstraints) /
          numInitialConstraints,
      numInitialVars,
      numFinalVars,
      100.0 * (numFinalVars - numInitialVars) / numInitialVars);
}

void ProblemSimplifier::solve(
    bool useSolverSpecificMultiObjectiveSolveIfAvailable) {
  simplify();

  // Might simplify out all variables. Third-party solvers freak out in this
  // case. We can handle this appropriately here though.
  if (getVars().empty()) {
    for (auto& objective : objectives_) {
      auto result = thrift::ProblemResult();
      result.status() = thrift::ProblemStatus::OPTIMAL_FOUND;
      result.absoluteGap() = 0;
      result.relativeGap() = 0;
      result.bestBound() = objective->getValue();
      result.bestObjective() = objective->getValue();

      problemResultPerObjective_.push_back(result);
    }
    status_ = thrift::ProblemStatus::OPTIMAL_FOUND;
  } else {
    GenericProblemImpl::solve(useSolverSpecificMultiObjectiveSolveIfAvailable);
  }
}

} // namespace facebook::algopt::lp::detail
