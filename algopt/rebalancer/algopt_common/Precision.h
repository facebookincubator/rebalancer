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

#include "algopt/rebalancer/algopt_common/thrift/gen-cpp2/Types_types.h"

namespace facebook::algopt {

const inline double kEpsilon =
    *algopt::common::thrift::PrecisionTolerances().absolute();

class PrecisionImpl {
 public:
  static int compare(
      double value1,
      double value2,
      const algopt::common::thrift::PrecisionTolerances& tolerances);

  static double trim(
      double value,
      const algopt::common::thrift::PrecisionTolerances& tolerances);

  static double snap(double value, double tolerance);

  static bool isInteger(
      double value,
      const algopt::common::thrift::PrecisionTolerances& tolerances);

  static bool isZeroOrOne(
      double value,
      const algopt::common::thrift::PrecisionTolerances& tolerances);

  static bool isZero(
      double value,
      const algopt::common::thrift::PrecisionTolerances& tolerances);

  static bool isStrictlyGtZero(
      double val,
      const algopt::common::thrift::PrecisionTolerances& tolerances);

  static bool isstrictlyGreater(
      double val1,
      double val2,
      const algopt::common::thrift::PrecisionTolerances& tolerances);

  static bool isEqual(
      double val1,
      double val2,
      const algopt::common::thrift::PrecisionTolerances& tolerances);

  static bool isGreaterOrEqual(
      double val1,
      double val2,
      const algopt::common::thrift::PrecisionTolerances& tolerances);

  static bool isStrictlyLesser(
      double val1,
      double val2,
      const algopt::common::thrift::PrecisionTolerances& tolerances);

  static bool isLesserOrEqual(
      double val1,
      double val2,
      const algopt::common::thrift::PrecisionTolerances& tolerances);
};

class Precision {
 public:
  // Returns a -1, 0, 1, when the first argument is less than, equal to,
  // or greater than the second.
  static int compare(double value1, double value2) {
    return PrecisionImpl::compare(value1, value2, defaultTolerances);
  }

  // Returns 0 if value is close to 0, and value otherwise
  static double trim(double value) {
    return PrecisionImpl::trim(value, defaultTolerances);
  }

  // Snaps the value to the nearest integer if it is close, value otherwise
  static double snap(double value, double tolerance) {
    return PrecisionImpl::snap(value, tolerance);
  }

  // Returns true if value is an integer
  static bool isInteger(double value) {
    return PrecisionImpl::isInteger(value, defaultTolerances);
  }

  // Returns true if value is either 0 or 1
  static bool isZeroOrOne(double value) {
    return PrecisionImpl::isZeroOrOne(value, defaultTolerances);
  }

  static bool isZero(double value) {
    return PrecisionImpl::isZero(value, defaultTolerances);
  }

  // Returns true if value is strictly greater than 0
  static bool isStrictlyGtZero(double val) {
    return PrecisionImpl::isStrictlyGtZero(val, defaultTolerances);
  }

  // Returns true if val > val2 (syntax sugar for compare(val1, val2) == 1)
  static bool isstrictlyGreater(double val1, double val2) {
    return PrecisionImpl::isstrictlyGreater(val1, val2, defaultTolerances);
  }

  // Returns true if val1 == val2
  static bool isEqual(double val1, double val2) {
    return PrecisionImpl::isEqual(val1, val2, defaultTolerances);
  }

  // Returns true if val >= val2
  static bool isGreaterOrEqual(double val1, double val2) {
    return PrecisionImpl::isGreaterOrEqual(val1, val2, defaultTolerances);
  }

  // Returns true if val < val2
  static bool isStrictlyLesser(double val1, double val2) {
    return PrecisionImpl::isStrictlyLesser(val1, val2, defaultTolerances);
  }

  // Returns true if val <= val2
  static bool isLesserOrEqual(double val1, double val2) {
    return PrecisionImpl::isLesserOrEqual(val1, val2, defaultTolerances);
  }

 private:
  const static inline algopt::common::thrift::PrecisionTolerances
      defaultTolerances = algopt::common::thrift::PrecisionTolerances();
};

} // namespace facebook::algopt
