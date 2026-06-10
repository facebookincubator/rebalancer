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

#include "algopt/rebalancer/entities/builders/RoutingConfigsBuilder.h"

#include "algopt/rebalancer/entities/builders/BuilderUtils.h"

namespace facebook::rebalancer::entities {

namespace {
constexpr static std::string_view kMakeRoutingConfigId{"makeRoutingConfigId"};
}

RoutingConfigId RoutingConfigsBuilder::makeRoutingConfigId(
    const std::string& routingConfigName) {
  return idBuilder_.makeIdFromName(
      routingConfigName,
      routingConfigNameToId_,
      routingConfigIdToData_,
      kMakeRoutingConfigId);
}

folly::coro::Task<RoutingConfigsBuilderResult> RoutingConfigsBuilder::build()
    const {
  RoutingConfigsBuilderResult result;
  co_await routingConfigIdToData_.waitAndReadFromEach(
      [&](RoutingConfigId id, const RoutingConfigData& data) {
        result.routingConfigIdToConfig.emplace(id, data.routingConfig);
      });

  result.routingConfigNameToId = *routingConfigNameToId_.rlock();

  co_return result;
}

folly::coro::Task<std::string> RoutingConfigsBuilder::summarize() const {
  co_return summarizeNames("RoutingConfigs", *routingConfigNameToId_.rlock());
}

} // namespace facebook::rebalancer::entities
