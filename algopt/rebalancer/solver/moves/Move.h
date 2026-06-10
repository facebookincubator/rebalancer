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

#include "algopt/rebalancer/entities/Universe.h"
#include "algopt/rebalancer/solver/utils/ChangeSet.h"

namespace facebook::rebalancer {

class Move {
 public:
  Move(
      entities::ObjectId object,
      entities::ContainerId srcContainer,
      entities::ContainerId dstContainer);

  entities::ObjectId getObject() const;
  entities::ContainerId getSourceContainer() const;
  entities::ContainerId getDestinationContainer() const;
  ChangeSet getChangeSet() const;
  std::string toString(const entities::Universe& universe) const;
  bool operator==(const Move& other) const;
  std::size_t getHashValue() const;

  // returns 0 if moves are identical, -1 if hash of lhs is smaller, otherwise
  // returns 1
  static int compare(const Move& lhs, const Move& rhs);

 private:
  entities::ObjectId object_;
  entities::ContainerId srcContainer_;
  entities::ContainerId dstContainer_;
  std::size_t hashValue_;
};

} // namespace facebook::rebalancer
