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

#include "algopt/rebalancer/algopt_common/Precision.h"

#include <cmath>

namespace facebook::algopt {

int PrecisionImpl::compare(
    double value1,
    double value2,
    const algopt::common::thrift::PrecisionTolerances& tolerances) {
  const double absoluteEpsilonValue = *tolerances.absolute();
  const double relativeEpsilonValue = *tolerances.relative();
  auto diff = value1 - value2;
  if (fabs(diff) < absoluteEpsilonValue) {
    return 0;
  }
  if (fabs(diff) * 2 / (fabs(value1) + fabs(value2)) < relativeEpsilonValue) {
    return 0;
  }
  return diff > 0 ? 1 : -1;
}

double PrecisionImpl::trim(
    double value,
    const algopt::common::thrift::PrecisionTolerances& tolerances) {
  return isZero(fabs(value), tolerances) ? 0 : value;
}

double PrecisionImpl::snap(double value, double tolerance) {
  if (std::abs(std::round(value) - value) < tolerance) {
    return std::round(value);
  }
  return value;
}

bool PrecisionImpl::isInteger(
    double value,
    const algopt::common::thrift::PrecisionTolerances& tolerances) {
  return compare(round(value), value, tolerances) == 0;
}

bool PrecisionImpl::isZeroOrOne(
    double value,
    const algopt::common::thrift::PrecisionTolerances& tolerances) {
  return compare(value, 0, tolerances) == 0 ||
      compare(value, 1, tolerances) == 0;
}

bool PrecisionImpl::isZero(
    double value,
    const algopt::common::thrift::PrecisionTolerances& tolerances) {
  return std::abs(value) < *tolerances.absolute();
}

bool PrecisionImpl::isStrictlyGtZero(
    double val,
    const algopt::common::thrift::PrecisionTolerances& tolerances) {
  return compare(val, 0, tolerances) == 1;
}

bool PrecisionImpl::isstrictlyGreater(
    double val1,
    double val2,
    const algopt::common::thrift::PrecisionTolerances& tolerances) {
  return compare(val1, val2, tolerances) == 1;
}

bool PrecisionImpl::isEqual(
    double val1,
    double val2,
    const algopt::common::thrift::PrecisionTolerances& tolerances) {
  return compare(val1, val2, tolerances) == 0;
}

bool PrecisionImpl::isStrictlyLesser(
    double val1,
    double val2,
    const algopt::common::thrift::PrecisionTolerances& tolerances) {
  return compare(val1, val2, tolerances) == -1;
}

bool PrecisionImpl::isLesserOrEqual(
    double val1,
    double val2,
    const algopt::common::thrift::PrecisionTolerances& tolerances) {
  return compare(val1, val2, tolerances) <= 0;
}

bool PrecisionImpl::isGreaterOrEqual(
    double val1,
    double val2,
    const algopt::common::thrift::PrecisionTolerances& tolerances) {
  return compare(val1, val2, tolerances) >= 0;
}

} // namespace facebook::algopt
