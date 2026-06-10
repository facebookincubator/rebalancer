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

#include <folly/container/F14Map.h>
#include <folly/MicroLock.h>
#include <folly/Synchronized.h>
#include <folly/ThreadLocal.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>

namespace facebook::algopt {

// Thread-safe cache with at-most-once lazy computation per key.
// Indexed by a key type convertible to uint64_t, optimized for contiguous
// key ranges via chunked allocation. Each entry has its own lightweight lock
// for compute-once access, and each thread keeps a per-instance pointer to the
// most-recently-used chunk so repeated lookups in the same chunk avoid the
// shared chunk-map lock.
template <typename K, typename V, size_t ChunkBits = 12>
class SparseCache {
  static constexpr size_t kChunkSize = size_t{1} << ChunkBits;
  static constexpr uint64_t kChunkMask = kChunkSize - 1;

  struct Entry {
    folly::MicroLock mu;
    std::atomic<bool> saved{false};
    V value{};
  };
  using Chunk = std::array<Entry, kChunkSize>;

  struct CacheState {
    uint64_t id = std::numeric_limits<uint64_t>::max();
    Chunk* chunk = nullptr;
  };
  folly::Synchronized<folly::F14FastMap<uint64_t, std::unique_ptr<Chunk>>>
      chunks_;
  // Per-thread, per-instance cache of the most-recently-used chunk so the hot
  // path reads/updates it with no lock.
  folly::ThreadLocal<CacheState> cache_;

 public:
  template <class Function>
  const V& getSavedOrCompute(const K& key, const Function& onMiss) {
    const auto rawKey = static_cast<uint64_t>(key);
    const auto chunkId = rawKey >> ChunkBits;
    // Lambda to get or compute the value for an entry. Shared between fast and
    // slow paths to avoid code duplication.
    auto getOrCompute = [&](Entry& entry) -> const V& {
      const std::lock_guard<folly::MicroLock> lock(entry.mu);
      if (entry.saved.load(std::memory_order_relaxed)) {
        return entry.value;
      }
      entry.value = onMiss();
      entry.saved.store(true, std::memory_order_release);
      return entry.value;
    };

    // Fast path: cache hit, return from cache.
    if (chunkId == cache_->id) [[likely]] {
      return getOrCompute((*cache_->chunk)[rawKey & kChunkMask]);
    }

    // Slow path: cache miss, look up chunks and update cache if found.
    Chunk* chunkPtr = nullptr;
    {
      auto chunksGuard = chunks_.rlock();
      auto it = chunksGuard->find(chunkId);
      if (it != chunksGuard->end()) {
        chunkPtr = it->second.get();
      }
    }
    if (chunkPtr == nullptr) {
      auto chunksGuard = chunks_.wlock();
      auto& chunk = (*chunksGuard)[chunkId];
      if (!chunk) {
        chunk = std::make_unique<Chunk>();
      }
      chunkPtr = chunk.get();
    }
    cache_->id = chunkId;
    cache_->chunk = chunkPtr;
    return getOrCompute((*chunkPtr)[rawKey & kChunkMask]);
  }

  std::optional<V> atOrNullopt(const K& key) const {
    const auto rawKey = static_cast<uint64_t>(key);
    const auto chunkId = rawKey >> ChunkBits;
    // Lambda to get the value from an entry if saved. Shared between fast and
    // slow paths to avoid code duplication.
    auto getValueIfSaved = [&](const Entry& entry) -> std::optional<V> {
      if (!entry.saved.load(std::memory_order_acquire)) {
        return std::nullopt;
      }
      return entry.value;
    };

    // Fast path: cache hit, return from cache.
    if (chunkId == cache_->id) [[likely]] {
      return getValueIfSaved((*cache_->chunk)[rawKey & kChunkMask]);
    }

    // Slow path: cache miss, look up chunks and update cache if found.
    auto rLocked = chunks_.rlock();
    auto it = rLocked->find(chunkId);
    if (it == rLocked->end()) {
      return std::nullopt;
    }
    auto* chunk = it->second.get();
    cache_->id = chunkId;
    cache_->chunk = chunk;
    return getValueIfSaved((*chunk)[rawKey & kChunkMask]);
  }
};

} // namespace facebook::algopt
