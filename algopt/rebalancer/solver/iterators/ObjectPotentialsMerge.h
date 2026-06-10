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

#include "algopt/rebalancer/solver/expressions/Expression.h"
#include "algopt/rebalancer/solver/iterators/Abstract.h"

#include <queue>
#include <utility>
#include <vector>

namespace facebook::rebalancer {

class ObjectPotentialsMergeIterator
    : public AbstractIteratorImpl<ObjectPotential> {
 public:
  ObjectPotentialsMergeIterator();

  ObjectPotentialsMergeIterator(
      std::vector<std::pair<
          AbstractIterator<ObjectPotential>,
          AbstractIterator<ObjectPotential>>> inputs,
      bool descending);

  void operator++() override;

  ObjectPotential operator*() const override;

  std::shared_ptr<AbstractIteratorImpl> clone() const override;

 private:
  bool equal(const AbstractIteratorImpl& other) const override;

 private:
  struct ObjectPotentialWithIndex {
    ObjectPotential objectPotential;
    int index;

    bool operator==(const ObjectPotentialWithIndex& other) const;
  };

  static std::function<
      bool(const ObjectPotentialWithIndex&, const ObjectPotentialWithIndex&)>
  makeCompareFunction(bool descending);

  std::vector<std::pair<
      AbstractIterator<ObjectPotential>,
      AbstractIterator<ObjectPotential>>>
      inputs_;
  bool descending_;
  std::priority_queue<
      ObjectPotentialWithIndex,
      std::vector<ObjectPotentialWithIndex>,
      std::function<bool(ObjectPotentialWithIndex, ObjectPotentialWithIndex)>>
      sortedHeads_;
  PackerSet<entities::ObjectId> objectIdsSeen_;
};

// Combines multiple sorted collections of object potentials into one:
// - Duplicate objects are dropped. The potential of an object in the output is
//   the best (smallest if descending = false, largest if descending = true)
//   potential seen for that object among all input collections.
// - Objects in resuling collection are sorted by potential.
// - Objects in input collections must be sorted by potential.
AbstractContainer<ObjectPotential> makeObjectPotentialsMergeContainer(
    const std::vector<AbstractContainer<ObjectPotential>>& inputs,
    bool descending);

} // namespace facebook::rebalancer
