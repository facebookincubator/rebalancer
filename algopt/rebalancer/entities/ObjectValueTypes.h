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

#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/entities/Map.h"
#include "algopt/rebalancer/entities/Partition.h"
#include "algopt/rebalancer/entities/thrift/gen-cpp2/Entities_types.h"

#include <folly/container/F14Map.h>
#include <folly/container/MapUtil.h>
#include <folly/Range.h>

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

namespace facebook::rebalancer::entities {

// ObjectValues is the per-object value source for one dimension: a small,
// cheap-to-copy handle whose copies share the same immutable backing, so it can
// be stored and passed by value without duplicating per-object values or
// pinning callers to a concrete representation.
//
// The backing is a std::variant of two alternatives:
//   - ObjectBacked: an explicit per-object map.
//   - GroupBacked: a per-group map plus the disjoint Partition that assigns
//     objects to groups. This keeps a dimension whose values are defined per
//     group compact -- O(groups) rather than O(objects) -- instead of expanding
//     it into a per-object map.
//
// Each alternative is a self-contained value type that implements the full set
// of per-backing operations. ObjectValues just dispatches to the active
// alternative through std::visit, so adding a third backing is a compile error
// until every operation handles it -- there is no implicit "if not object, then
// group" assumption anywhere.
class ObjectValues {
 private:
  // Explicit per-object map. All stats are derived from the map itself.
  struct ObjectBacked {
    std::shared_ptr<const ObjectIdToDoubleMap> map;

    double getObjectValue(ObjectId objectId) const {
      return map->getValue(objectId);
    }

    double defaultValue() const {
      return map->getDefaultValue();
    }

    size_t totalObjectCount() const {
      return map->totalSize();
    }

    size_t nonDefaultCount() const {
      return map->nonDefaultSize();
    }

    double sum() const {
      double nonDefaultSum = 0;
      map->forEachNonDefault(
          [&](ObjectId, double val) { nonDefaultSum += val; });
      return nonDefaultSum +
          map->getDefaultValue() * (map->totalSize() - map->nonDefaultSize());
    }

    template <typename F>
    void forEachNonDefault(F&& fn) const {
      map->forEachNonDefault(std::forward<F>(fn));
    }

    std::shared_ptr<const ObjectIdToDoubleMap> asMapOrNull() const {
      return map;
    }

    template <typename ObjectFn, typename GroupFn>
    decltype(auto) visit(ObjectFn&& objectFn, GroupFn&&) const {
      return objectFn(*map);
    }
  };

  // Per-group map plus the disjoint partition that assigns objects to groups.
  // Iteration walks only the entries present in `groupValues`; like the object
  // map, callers are expected not to store default-valued entries, and runtime
  // is proportional to the size of the provided group map.
  struct GroupBacked {
    std::shared_ptr<const GroupIdToDoubleMap> groupValues;
    std::shared_ptr<const Partition> partition;
    PartitionId partitionId;
    double defaultValue_ = 0;
    size_t totalObjects = 0;

    double getObjectValue(ObjectId objectId) const {
      const auto* groupIds =
          folly::get_ptr(partition->getObjectIdToGroupIds(), objectId);
      if (groupIds == nullptr || groupIds->empty()) {
        return defaultValue_;
      }
      // The ObjectValues ctor rejects non-disjoint partitions, so an object
      // belongs to at most one group in this backing partition.
      return folly::get_default(*groupValues, groupIds->front(), defaultValue_);
    }

    double defaultValue() const {
      return defaultValue_;
    }

    size_t totalObjectCount() const {
      return totalObjects;
    }

    size_t nonDefaultCount() const {
      size_t count = 0;
      for (const auto& [groupId, value] : *groupValues) {
        count += partition->getObjectIds(groupId).size();
      }
      return count;
    }

    double sum() const {
      double total = 0;
      size_t coveredObjectCount = 0;
      for (const auto& [groupId, groupValue] : *groupValues) {
        const size_t groupSize = partition->getObjectIds(groupId).size();
        total += groupValue * groupSize;
        coveredObjectCount += groupSize;
      }
      total += defaultValue_ * (totalObjects - coveredObjectCount);
      return total;
    }

    template <typename F>
    void forEachNonDefault(F&& fn) const {
      for (const auto& [groupId, groupValue] : *groupValues) {
        for (const auto objectId : partition->getObjectIds(groupId)) {
          fn(objectId, groupValue);
        }
      }
    }

    static std::shared_ptr<const ObjectIdToDoubleMap> asMapOrNull() {
      return nullptr;
    }

    template <typename ObjectFn, typename GroupFn>
    decltype(auto) visit(ObjectFn&&, GroupFn&& groupFn) const {
      return groupFn(partitionId, *groupValues);
    }
  };

 public:
  explicit ObjectValues(std::shared_ptr<const ObjectIdToDoubleMap> map) {
    if (!map) {
      throw std::invalid_argument("ObjectValues map cannot be null");
    }
    backing_ = ObjectBacked{std::move(map)};
  }

  ObjectValues(
      std::shared_ptr<const GroupIdToDoubleMap> groupValues,
      std::shared_ptr<const Partition> partition,
      double defaultValue,
      size_t totalObjects,
      PartitionId partitionId) {
    if (!groupValues) {
      throw std::invalid_argument("ObjectValues groupValues cannot be null");
    }
    if (!partition) {
      throw std::invalid_argument("ObjectValues partition cannot be null");
    }
    if (!partition->isDisjoint()) {
      throw std::invalid_argument(
          "ObjectValues group-backed partition must be disjoint");
    }
    backing_ = GroupBacked{
        .groupValues = std::move(groupValues),
        .partition = std::move(partition),
        .partitionId = partitionId,
        .defaultValue_ = defaultValue,
        .totalObjects = totalObjects};
  }

  ObjectValues(
      const thrift::ObjectValues& values,
      double defaultValue,
      size_t totalObjects,
      std::shared_ptr<const Partition> partition = nullptr);

  thrift::ObjectValues toThrift() const;

  double getObjectValue(ObjectId objectId) const {
    return std::visit(
        [&](const auto& backing) { return backing.getObjectValue(objectId); },
        backing_);
  }

  double defaultValue() const {
    return std::visit(
        [](const auto& backing) { return backing.defaultValue(); }, backing_);
  }

  size_t totalObjectCount() const {
    return std::visit(
        [](const auto& backing) { return backing.totalObjectCount(); },
        backing_);
  }

  size_t nonDefaultCount() const {
    return std::visit(
        [](const auto& backing) { return backing.nonDefaultCount(); },
        backing_);
  }

  double sum() const {
    return std::visit(
        [](const auto& backing) { return backing.sum(); }, backing_);
  }

  double sum(folly::Range<const ObjectId*> objectIds) const {
    double total = 0;
    for (const auto objectId : objectIds) {
      total += getObjectValue(objectId);
    }
    return total;
  }

  template <typename F>
  void forEachNonDefault(F&& fn) const {
    std::visit(
        [&](const auto& backing) { backing.forEachNonDefault(fn); }, backing_);
  }

  // Dispatches to objectFn(const ObjectIdToDoubleMap&) for object-backed
  // storage, or groupFn(PartitionId, const GroupIdToDoubleMap&) for
  // group-backed storage. Both callables must return the same type.
  template <typename ObjectFn, typename GroupFn>
  decltype(auto) visit(ObjectFn&& objectFn, GroupFn&& groupFn) const {
    return std::visit(
        [&](const auto& backing) { return backing.visit(objectFn, groupFn); },
        backing_);
  }

  // Compatibility bridge for older object-map-only callsites. New storage
  // consumers should use lookup/iteration helpers so alternate backing
  // representations do not leak through the API. Returns null for group-backed
  // storage, which has no single object map to hand out.
  std::shared_ptr<const ObjectIdToDoubleMap> asMapOrNull() const {
    return std::visit(
        [](const auto& backing) { return backing.asMapOrNull(); }, backing_);
  }

  // Returns values for only `groupId` in `partition`; all objects outside the
  // group take a 0 default. Same-partition group-backed storage returns compact
  // single-group storage; object-backed or cross-partition storage expands only
  // the requested group.
  ObjectValues sliceGroup(const Partition& partition, GroupId groupId) const {
    if (const auto* g = std::get_if<GroupBacked>(&backing_);
        g != nullptr && g->partition.get() == &partition) {
      auto singleGroupMap = std::make_shared<GroupIdToDoubleMap>();
      const auto groupValue =
          folly::get_default(*g->groupValues, groupId, g->defaultValue_);
      if (groupValue != 0.0) {
        singleGroupMap->emplace(groupId, groupValue);
      }
      return ObjectValues(
          std::move(singleGroupMap),
          g->partition,
          /*defaultValue=*/0.0,
          g->totalObjectCount(),
          g->partitionId);
    }

    return std::visit(
        [&](const auto& backing) {
          const auto& objectIds = partition.getObjectIds(groupId);
          auto objectValues = std::make_shared<ObjectIdToDoubleMap>(
              backing.totalObjectCount(),
              /*defaultValue=*/0.0,
              /*expectedNonDefaultSize=*/objectIds.size());
          for (const auto objectId : objectIds) {
            objectValues->emplace(objectId, backing.getObjectValue(objectId));
          }
          return ObjectValues(std::move(objectValues));
        },
        backing_);
  }

 private:
  std::variant<ObjectBacked, GroupBacked> backing_;
};

} // namespace facebook::rebalancer::entities
