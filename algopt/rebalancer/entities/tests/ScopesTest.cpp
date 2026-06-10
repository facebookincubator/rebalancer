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

#include "algopt/rebalancer/algopt_common/TestUtils.h"
#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/entities/Scopes.h"

#include <gtest/gtest.h>

namespace facebook::rebalancer::entities {

TEST(ScopesTest, Constructor) {
  const auto scope1Id = ScopeId(1);
  const auto scope2Id = ScopeId(2);
  const auto scopeItem1Id = ScopeItemId(10);
  const auto scopeItem2Id = ScopeItemId(11);
  const auto container1Id = ContainerId(100);
  const auto container2Id = ContainerId(101);

  auto scope1 = std::make_unique<Scope>(
      std::make_shared<const Map<ScopeItemId, Set<ContainerId>>>(
          Map<ScopeItemId, Set<ContainerId>>{{scopeItem1Id, {container1Id}}}),
      Map<DimensionId, std::shared_ptr<const ScopeDimension>>{});
  auto scope2 = std::make_unique<Scope>(
      std::make_shared<const Map<ScopeItemId, Set<ContainerId>>>(
          Map<ScopeItemId, Set<ContainerId>>{
              {scopeItem2Id, {container1Id, container2Id}}}),
      Map<DimensionId, std::shared_ptr<const ScopeDimension>>{});

  Map<ScopeId, std::unique_ptr<Scope>> scopeIdToScope;
  scopeIdToScope.emplace(scope1Id, std::move(scope1));
  scopeIdToScope.emplace(scope2Id, std::move(scope2));

  const auto scopes = Scopes(std::move(scopeIdToScope));

  EXPECT_EQ(2, scopes.getScopeIds().size());

  const auto& s1 = scopes.getScope(scope1Id);
  EXPECT_EQ(
      std::set<ScopeItemId>({scopeItem1Id}),
      toSet<ScopeItemId>(s1.getScopeItemIds()));
  EXPECT_EQ(Set<ContainerId>({container1Id}), s1.getContainerIds(scopeItem1Id));

  const auto& s2 = scopes.getScope(scope2Id);
  EXPECT_EQ(
      std::set<ScopeItemId>({scopeItem2Id}),
      toSet<ScopeItemId>(s2.getScopeItemIds()));
  EXPECT_EQ(
      Set<ContainerId>({container1Id, container2Id}),
      s2.getContainerIds(scopeItem2Id));
}

TEST(ScopesTest, ToThrift) {
  const auto scopeId = ScopeId(0);
  const auto scopeItemId = ScopeItemId(1);
  const auto containerId = ContainerId(2);

  auto scope = std::make_unique<Scope>(
      std::make_shared<const Map<ScopeItemId, Set<ContainerId>>>(
          Map<ScopeItemId, Set<ContainerId>>{{scopeItemId, {containerId}}}),
      Map<DimensionId, std::shared_ptr<const ScopeDimension>>{});

  Map<ScopeId, std::unique_ptr<Scope>> scopeIdToScope;
  scopeIdToScope.emplace(scopeId, std::move(scope));

  const Scopes scopes(std::move(scopeIdToScope));

  auto thrift = scopes.toThrift();

  EXPECT_EQ(1, thrift.scopes()->size());
  EXPECT_EQ(1, thrift.scopes()->at(0).scopeItems()->size());
  EXPECT_EQ(std::vector<int>({2}), thrift.scopes()->at(0).scopeItems()->at(1));
}

TEST(ScopesTest, FromThrift) {
  thrift::Scope scopeThrift;
  scopeThrift.scopeItems() = {{1, {2}}};

  thrift::Scopes scopesThrift;
  scopesThrift.scopes() = {{0, scopeThrift}};

  const Scopes scopes(scopesThrift);

  EXPECT_EQ(
      std::vector<ScopeId>({ScopeId(0)}), toVec<ScopeId>(scopes.getScopeIds()));
  EXPECT_EQ(
      std::vector<ScopeItemId>({ScopeItemId(1)}),
      toVec<ScopeItemId>(scopes.getScope(ScopeId(0)).getScopeItemIds()));
}

} // namespace facebook::rebalancer::entities
