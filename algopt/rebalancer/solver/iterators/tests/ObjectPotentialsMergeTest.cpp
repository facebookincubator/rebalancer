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

#include "algopt/rebalancer/solver/iterators/ObjectPotentialsMerge.h"
#include "algopt/rebalancer/solver/iterators/StlWrapper.h"
#include "algopt/rebalancer/solver/tests/IdConverterTestUtils.h"

#include <gtest/gtest.h>

using namespace std;
namespace facebook::rebalancer::packer::tests {

TEST(ObjectPotentialsMergeTest, Descending) {
  auto container = makeObjectPotentialsMergeContainer(
      vector<AbstractContainer<ObjectPotential>>{
          makeStlWrapperContainer(
              vector<ObjectPotential>{
                  ObjectPotential{.objectId = object(10), .potential = 1000},
                  ObjectPotential{.objectId = object(13), .potential = 900},
                  ObjectPotential{.objectId = object(17), .potential = 800}}),
          makeStlWrapperContainer(
              vector<ObjectPotential>{
                  ObjectPotential{.objectId = object(12), .potential = 1700},
                  ObjectPotential{.objectId = object(14), .potential = 1200},
                  ObjectPotential{.objectId = object(15), .potential = 1100}}),
          makeStlWrapperContainer(
              vector<ObjectPotential>{
                  ObjectPotential{.objectId = object(11), .potential = 2000},
                  ObjectPotential{.objectId = object(10), .potential = 1500},
                  ObjectPotential{.objectId = object(16), .potential = 950},
                  ObjectPotential{.objectId = object(15), .potential = 700}})},
      true);
  const vector<ObjectPotential> output(container.begin(), container.end());
  const vector<ObjectPotential> expected = {
      ObjectPotential{.objectId = object(11), .potential = 2000},
      ObjectPotential{.objectId = object(12), .potential = 1700},
      ObjectPotential{.objectId = object(10), .potential = 1500},
      ObjectPotential{.objectId = object(14), .potential = 1200},
      ObjectPotential{.objectId = object(15), .potential = 1100},
      ObjectPotential{.objectId = object(16), .potential = 950},
      ObjectPotential{.objectId = object(13), .potential = 900},
      ObjectPotential{.objectId = object(17), .potential = 800}};
  EXPECT_EQ(expected, output);
}

TEST(ObjectPotentialsMergeTest, Ascending) {
  auto container = makeObjectPotentialsMergeContainer(
      vector<AbstractContainer<ObjectPotential>>{
          makeStlWrapperContainer(
              vector<ObjectPotential>{
                  ObjectPotential{.objectId = object(17), .potential = 800},
                  ObjectPotential{.objectId = object(13), .potential = 900},
                  ObjectPotential{.objectId = object(10), .potential = 1000}}),
          makeStlWrapperContainer(
              vector<ObjectPotential>{
                  ObjectPotential{.objectId = object(15), .potential = 1100},
                  ObjectPotential{.objectId = object(14), .potential = 1200},
                  ObjectPotential{.objectId = object(12), .potential = 1700}}),
          makeStlWrapperContainer(
              vector<ObjectPotential>{
                  ObjectPotential{.objectId = object(15), .potential = 700},
                  ObjectPotential{.objectId = object(16), .potential = 950},
                  ObjectPotential{.objectId = object(10), .potential = 1500},
                  ObjectPotential{.objectId = object(11), .potential = 2000}})},
      false);
  const vector<ObjectPotential> output(container.begin(), container.end());
  const vector<ObjectPotential> expected = {
      ObjectPotential{.objectId = object(15), .potential = 700},
      ObjectPotential{.objectId = object(17), .potential = 800},
      ObjectPotential{.objectId = object(13), .potential = 900},
      ObjectPotential{.objectId = object(16), .potential = 950},
      ObjectPotential{.objectId = object(10), .potential = 1000},
      ObjectPotential{.objectId = object(14), .potential = 1200},
      ObjectPotential{.objectId = object(12), .potential = 1700},
      ObjectPotential{.objectId = object(11), .potential = 2000}};
  EXPECT_EQ(expected, output);
}

} // namespace facebook::rebalancer::packer::tests
