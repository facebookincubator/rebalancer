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

#include "algopt/rebalancer/solver/expressions/Operators.h"
#include "algopt/rebalancer/solver/expressions/tests/ExpressionTestsBase.h"
#include "algopt/rebalancer/solver/expressions/tests/ExpressionUtils.h"
#include "algopt/rebalancer/solver/expressions/Variable.h"

#include <fmt/format.h>
#include <folly/container/irange.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::packer::tests {

class ObjectLookupTest : public ExpressionTestsBase {
 protected:
  ExprPtr makeLookup(
      std::shared_ptr<ObjectVector> vec,
      bool isStableStayed,
      const std::shared_ptr<const entities::Universe>& universe,
      const Assignment& assignment) {
    // only one container, so initial and full object vectors are same for
    // StableStayed
    auto containerPtr = std::make_shared<PackerSet<entities::ContainerId>>();
    containerPtr->insert({container(0)});
    return isStableStayed ? std::make_shared<StableStayed>(StableStayed(
                                vec, vec, containerPtr, universe, assignment))
                          : std::make_shared<ObjectLookup>(ObjectLookup(
                                vec, containerPtr, universe, assignment));
  }

  template <typename T>
  void lookupBasic() {
    setInitialAssignment(
        {{"container0", {"object0"}}, {"container1", {"object1"}}});
    const auto universe = buildUniverse();
    const Assignment assignment(
        universe->getContainers().getInitialAssignment());
    const PackerMap<entities::ObjectId, double> vector = {
        {object(0), 4}, {object(1), 3}};
    auto vec = makeObjectVector(vector, universe);
    auto lookup =
        makeLookup(vec, std::is_same<T, StableStayed>(), universe, assignment);

    EXPECT_EQ(4, apply(lookup, assignment));

    EXPECT_EQ(4, evaluate(lookup, {}, assignment));

    EXPECT_EQ(0, evaluate(lookup, {{object(0), container(1)}}, assignment));

    EXPECT_EQ(0, apply(lookup, Assignment()));

    EquivalenceSets equivalenceSets(*universe);
    // before : 1 equivalent set
    equivalenceSets.combine(
        std::vector<entities::ObjectId>{object(0), object(1)});

    const folly::F14FastSet<const void*> visited;
    // after : 2 equivalent sets
    updateEquivalenceSets(equivalenceSets, *lookup);
    EXPECT_EQ(equivalenceSets.size(), 2);
  }

  template <typename T>
  void lookupBoundsTests() {
    setInitialAssignment(
        {{"container0", {"object0", "object1"}},
         {"container1", {"object2", "object3"}}});

    const auto universe = buildUniverse();
    const Assignment assignment(
        universe->getContainers().getInitialAssignment());

    auto objectVector =
        makeObjectVector({{object(0), 4}, {object(1), 3}}, universe);
    auto lookup = makeLookup(
        objectVector, std::is_same<T, StableStayed>(), universe, assignment);
    EXPECT_EQ(7, apply(lookup, assignment));
    EXPECT_EQ(7, upper_bound(*lookup));
    EXPECT_EQ(0, lower_bound(*lookup));
    auto bc = BoundConstraints::Builder{}
                  .setDynamicContainers({container(1)})
                  .build();

    EXPECT_EQ(7, lower_bound(*lookup, bc));
    EXPECT_EQ(7, upper_bound(*lookup, bc));
  }
};

TEST_F(ObjectLookupTest, ObjectLookupBasic) {
  lookupBasic<ObjectLookup>();
}

TEST_F(ObjectLookupTest, StableStayedBasic) {
  lookupBasic<StableStayed>();
}

TEST_F(ObjectLookupTest, LookupPartialApplyUsingObjects) {
  constexpr int objectCount = 100;
  constexpr int containerCount = 10;

  PackerMap<std::string, std::vector<std::string>> containerToObjects;
  for (const auto i : folly::irange(objectCount)) {
    const auto container = fmt::format("container{}", i % containerCount);
    containerToObjects[container].push_back(fmt::format("object{}", i));
  }

  setInitialAssignment(containerToObjects);

  const auto universe = buildUniverse();

  auto assignment =
      Assignment(universe->getContainers().getInitialAssignment());

  // there are 100 objects, but only 3 of them have non-default values
  auto objectVector = makeObjectVector(
      {{object(1), 10}, {object(3), 2}, {object(11), 4}}, 0, 100, universe);

  auto objectLookup = object_lookup(
      objectVector,
      std::make_shared<PackerSet<entities::ContainerId>>(
          PackerSet<entities::ContainerId>{container(1)}),
      universe,
      assignment);

  // after the initial apply, we expect objectLookup to evaluate to (10 + 4) =
  // 14, because of objects 1 and 11
  EXPECT_EQ(14, apply(objectLookup, assignment));
  EXPECT_TRUE(descendingChildPotentialsAsExpected(*objectLookup, {}));

  // make many changes so that we iterate using changes w.r.t. objects in
  // partial apply for objectLookup
  auto changes = ObjectToNewContainer{
      {object(1), container(2)},
      {object(3), container(4)},
      {object(4), container(1)},
      {object(5), container(1)},
      {object(6), container(1)},
      {object(7), container(1)}};

  // after the partial apply, we expect objectLookup to evaluate to 4. In
  // particular, the change w.r.t. object 3 should not be considered in the
  // apply for objectLookup
  EXPECT_EQ(4, applyChanges(objectLookup, changes, assignment));
  EXPECT_TRUE(descendingChildPotentialsAsExpected(*objectLookup, {}));
}

TEST_F(ObjectLookupTest, LookupBoundsTests) {
  lookupBoundsTests<ObjectLookup>();
}

TEST_F(ObjectLookupTest, StableStayedBoundsTests) {
  lookupBoundsTests<StableStayed>();
}

TEST_F(ObjectLookupTest, StableStayedEquivSets) {
  setInitialAssignment(
      {{"container0", {"object0", "object1"}},
       {"container1", {"object2", "object3"}}});

  const auto universe = buildUniverse();
  const PackerMap<entities::ObjectId, double> objectIdToVal = {
      {object(0), 4}, {object(1), 4}, {object(2), 4}, {object(3), 4}};
  auto vec = makeObjectVector(objectIdToVal, universe);
  auto containerPtr = std::make_shared<PackerSet<entities::ContainerId>>();
  containerPtr->insert({container(0), container(1)});
  const Assignment initialAssignment(
      universe->getContainers().getInitialAssignment());

  const PackerMap<entities::ObjectId, double> initialValCtr1 = {
      {object(0), 4}, {object(1), 4}, {object(2), 0}, {object(3), 0}};
  const PackerMap<entities::ObjectId, double> initialValCtr2 = {
      {object(0), 0}, {object(1), 0}, {object(2), 4}, {object(3), 4}};
  auto initialVec1 = makeObjectVector(initialValCtr1, universe);
  auto initialVec2 = makeObjectVector(initialValCtr2, universe);
  const Assignment assignment(universe->getContainers().getInitialAssignment());
  StableStayed stayed1(initialVec1, vec, containerPtr, universe, assignment);
  StableStayed stayed2(initialVec2, vec, containerPtr, universe, assignment);

  EquivalenceSets equivalenceSets(*universe);
  // before : 1 equivalent set
  equivalenceSets.combine(
      std::vector<entities::ObjectId>{
          object(0), object(1), object(2), object(3)});

  // after : 2 equivalent sets
  updateEquivalenceSets(equivalenceSets, stayed1);
  updateEquivalenceSets(equivalenceSets, stayed2);
  // equivalent sets are updated using vec and not initialVec1 or initialVec2
  // so the number of equivalent sets should still be 1
  EXPECT_EQ(equivalenceSets.size(), 1);

  ObjectLookup lookup(initialVec2, containerPtr, universe, assignment);
  // lookup on initial vec should split up the equivalent sets
  updateEquivalenceSets(equivalenceSets, lookup);
  EXPECT_EQ(equivalenceSets.size(), 2);
}

TEST_F(ObjectLookupTest, isInteger) {
  setInitialAssignment(
      {{"container0", {"object0"}},
       {"container1", {"object1", "object2", "object3"}}});
  const auto universe = buildUniverse();
  const auto numObjects = getNumObjects();
  {
    auto objectVector = makeObjectVector(
        {{object(1), 10}, {object(3), 2}},
        /*defaultValue=*/0,
        numObjects,
        universe);
    const Assignment assignment(
        universe->getContainers().getInitialAssignment());
    auto objectLookup = object_lookup(
        objectVector,
        std::make_shared<PackerSet<entities::ContainerId>>(
            PackerSet<entities::ContainerId>{container(1)}),
        universe,
        assignment);
    Context intContext;
    EXPECT_TRUE(objectVector->is_integer(intContext));
    EXPECT_TRUE(objectLookup->is_integer(intContext));
  }

  {
    auto objectVector = makeObjectVector(
        {{object(1), 10}, {object(3), 2}},
        /*defaultValue=*/0.1,
        numObjects,
        universe);
    const Assignment assignment(
        universe->getContainers().getInitialAssignment());
    auto objectLookup = object_lookup(
        objectVector,
        std::make_shared<PackerSet<entities::ContainerId>>(
            PackerSet<entities::ContainerId>{container(1)}),
        universe,
        assignment);
    Context intContext;
    EXPECT_FALSE(objectVector->is_integer(intContext));
    EXPECT_FALSE(objectLookup->is_integer(intContext));
  }

  {
    auto objectVector = makeObjectVector(
        {{object(1), 10.2}, {object(3), 2}},
        /*defaultValue=*/0,
        numObjects,
        universe);
    const Assignment assignment(
        universe->getContainers().getInitialAssignment());
    auto objectLookup = object_lookup(
        objectVector,
        std::make_shared<PackerSet<entities::ContainerId>>(
            PackerSet<entities::ContainerId>{container(1)}),
        universe,
        assignment);
    Context intContext;
    EXPECT_FALSE(objectVector->is_integer(intContext));
    EXPECT_FALSE(objectLookup->is_integer(intContext));
  }

  {
    auto objectVector = makeObjectVector(
        {{object(0), 1}, {object(1), 10}, {object(2), 3}, {object(3), 2}},
        /*defaultValue=*/0.5,
        numObjects,
        universe);
    const Assignment assignment(
        universe->getContainers().getInitialAssignment());
    auto objectLookup = object_lookup(
        objectVector,
        std::make_shared<PackerSet<entities::ContainerId>>(
            PackerSet<entities::ContainerId>{container(1)}),
        universe,
        assignment);

    // although default value is 0.5, all the objects have integral values; so
    // expect is_integer to be true
    Context intContext;
    EXPECT_TRUE(objectVector->is_integer(intContext));
    EXPECT_TRUE(objectLookup->is_integer(intContext));
  }
}

TEST_F(ObjectLookupTest, VariableInitialValue) {
  setInitialAssignment(
      entities::Map<std::string, std::vector<std::string>>{
          {"container0", {"object0", "object1"}}, {"container1", {"object2"}}});
  const auto universe = buildUniverse();
  const Assignment assignment(universe->getContainers().getInitialAssignment());

  auto var0c0 =
      std::make_shared<Variable>(object(0), container(0), universe, assignment);
  EXPECT_EQ(1.0, var0c0->getInitialValue());

  auto var0c1 =
      std::make_shared<Variable>(object(0), container(1), universe, assignment);
  EXPECT_EQ(0.0, var0c1->getInitialValue());

  auto var2c1 =
      std::make_shared<Variable>(object(2), container(1), universe, assignment);
  EXPECT_EQ(1.0, var2c1->getInitialValue());
}

TEST_F(ObjectLookupTest, ObjectLookupInitialValue) {
  setInitialAssignment(
      entities::Map<std::string, std::vector<std::string>>{
          {"container0", {"object0", "object1"}}, {"container1", {"object2"}}});
  const auto universe = buildUniverse();
  const Assignment assignment(universe->getContainers().getInitialAssignment());

  auto objVec = makeObjectVector(
      {{object(0), 10}, {object(1), 5}, {object(2), 3}}, universe);

  auto lookupC0 = object_lookup(
      objVec,
      std::make_shared<PackerSet<entities::ContainerId>>(
          PackerSet<entities::ContainerId>{container(0)}),
      universe,
      assignment);

  EXPECT_EQ(15.0, lookupC0->getInitialValue());

  auto lookupC1 = object_lookup(
      objVec,
      std::make_shared<PackerSet<entities::ContainerId>>(
          PackerSet<entities::ContainerId>{container(1)}),
      universe,
      assignment);
  EXPECT_EQ(3.0, lookupC1->getInitialValue());

  auto lookupBoth = object_lookup(
      objVec,
      std::make_shared<PackerSet<entities::ContainerId>>(
          PackerSet<entities::ContainerId>{container(0), container(1)}),
      universe,
      assignment);
  EXPECT_EQ(18.0, lookupBoth->getInitialValue());
}

} // namespace facebook::rebalancer::packer::tests
