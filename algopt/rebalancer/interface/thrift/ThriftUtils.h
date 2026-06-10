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

#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"

namespace facebook::rebalancer::interface::thriftUtils {

inline interface::RoutingLatencyMetricInfo makeRoutingLatencyMetric(
    interface::RoutingLatencyMetric type,
    std::optional<double> percentile = std::nullopt) {
  interface::RoutingLatencyMetricInfo metric;
  metric.type() = type;
  metric.percentile().from_optional(std::move(percentile));
  return metric;
}

inline std::string toString(const interface::RoutingLatencyMetricInfo& metric) {
  switch (*metric.type()) {
    case interface::RoutingLatencyMetric::AVG:
      return apache::thrift::util::enumNameSafe(*metric.type());
    case interface::RoutingLatencyMetric::MAX:
      return apache::thrift::util::enumNameSafe(*metric.type());
    case interface::RoutingLatencyMetric::P99:
      return apache::thrift::util::enumNameSafe(*metric.type());
    case interface::RoutingLatencyMetric::PERCENTILE:
      return fmt::format("P{}", *metric.percentile());
  }

  throw std::runtime_error(
      fmt::format(
          "Unknown RoutingLatencyMetric {}",
          apache::thrift::util::enumNameSafe(*metric.type())));
}

} // namespace facebook::rebalancer::interface::thriftUtils
