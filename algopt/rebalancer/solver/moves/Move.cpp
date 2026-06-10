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

#include "algopt/rebalancer/solver/moves/Move.h"

#include <sstream>

namespace facebook::rebalancer {

Move::Move(
    entities::ObjectId object,
    entities::ContainerId srcContainer,
    entities::ContainerId dstContainer)
    : object_(object),
      srcContainer_(srcContainer),
      dstContainer_(dstContainer),
      hashValue_(
          folly::hash::hash_combine(object, srcContainer, dstContainer)) {}

entities::ObjectId Move::getObject() const {
  return object_;
}

entities::ContainerId Move::getSourceContainer() const {
  return srcContainer_;
}

entities::ContainerId Move::getDestinationContainer() const {
  return dstContainer_;
}

ChangeSet Move::getChangeSet() const {
  ChangeSet changeSet;
  changeSet.insert(Change(object_, srcContainer_, -1));
  changeSet.insert(Change(object_, dstContainer_, 1));
  return changeSet;
}

std::string Move::toString(const entities::Universe& universe) const {
  std::stringstream ss;
  ss << fmt::format(
      "Object = {} (id: {}), Source Container = {} (id: {}), Destination Container = {} (id: {})",
      universe.getEntityName(object_),
      object_,
      universe.getEntityName(srcContainer_),
      srcContainer_,
      universe.getEntityName(dstContainer_),
      dstContainer_);
  return ss.str();
}

bool Move::operator==(const Move& other) const {
  return object_ == other.object_ && srcContainer_ == other.srcContainer_ &&
      dstContainer_ == other.dstContainer_;
}

/* static */
int Move::compare(const Move& lhs, const Move& rhs) {
  if (lhs == rhs) {
    return 0;
  }
  if (lhs.getHashValue() < rhs.getHashValue()) {
    return 1;
  }
  return -1;
}

std::size_t Move::getHashValue() const {
  return hashValue_;
}

} // namespace facebook::rebalancer
