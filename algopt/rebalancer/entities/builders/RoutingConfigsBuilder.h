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

#include "algopt/rebalancer/entities/builders/AsyncValueMap.h"
#include "algopt/rebalancer/entities/builders/IdBuilder.h"
#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/entities/RoutingConfig.h"

#include <folly/container/F14Map.h>
#include <folly/coro/Collect.h>
#include <folly/coro/Task.h>
#include <folly/futures/SharedPromise.h>

namespace facebook::rebalancer::entities {

struct RoutingConfigData {
  std::shared_ptr<entities::RoutingConfig> routingConfig;
};

struct RoutingConfigsBuilderResult {
  Map<std::string, RoutingConfigId> routingConfigNameToId;
  Map<RoutingConfigId, std::shared_ptr<RoutingConfig>> routingConfigIdToConfig;
};

class RoutingConfigsBuilder {
 public:
  explicit RoutingConfigsBuilder(IdBuilder& idBuilder)
      : idBuilder_(idBuilder) {}

  folly::coro::Task<void> add(
      RoutingConfigId routingConfigId,
      RoutingConfigData routingConfigData) {
    co_return routingConfigIdToData_.set(
        routingConfigId, std::move(routingConfigData));
  }

  folly::coro::Task<std::shared_ptr<const RoutingConfigData>> get(
      RoutingConfigId routingConfigId) const {
    co_return co_await routingConfigIdToData_.get(routingConfigId);
  }

  RoutingConfigId makeRoutingConfigId(const std::string& routingConfigName);

  RoutingConfigId getRoutingConfigId(
      const std::string& routingConfigName) const {
    return routingConfigNameToId_.rlock()->at(routingConfigName);
  }

  folly::coro::Task<RoutingConfigsBuilderResult> build() const;

  folly::coro::Task<std::string> summarize() const;

 private:
  IdBuilder& idBuilder_;
  AsyncValueMap<RoutingConfigId, entities::RoutingConfigData>
      routingConfigIdToData_;
  folly::Synchronized<Map<std::string, RoutingConfigId>> routingConfigNameToId_;
};

} // namespace facebook::rebalancer::entities
