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

#include "algopt/rebalancer/entities/Map.h"

#include <folly/concurrency/ConcurrentHashMap.h>
#include <folly/container/F14Map.h>
#include <folly/coro/SharedPromise.h>
#include <folly/coro/Task.h>
#include <folly/Synchronized.h>

namespace facebook::rebalancer::materializer {

// forward declarations
template <class Value>
class SingleEntryCache;

template <class Key, class Value>
class Cache {
  using SingleEntryCacheCachePtr = std::shared_ptr<SingleEntryCache<Value>>;

 public:
  template <class Function>
  const Value& getSavedOrCompute(const Key& key, const Function& onMiss) {
    auto insertedValuePtr = checkIfExists(key);
    if (insertedValuePtr) {
      return insertedValuePtr->getSavedOrCompute(onMiss);
    }

    return insertNew(key)->getSavedOrCompute(onMiss);
  }

  // returns the saved value, or throws if key has not been previously seen
  const Value& at(const Key& key) const {
    auto insertedValuePtr = checkIfExists(key);
    if (!insertedValuePtr) {
      throw std::runtime_error(fmt::format("key {} does not exist", key));
    }

    return insertedValuePtr->getSaved();
  }

  bool contains(const Key& key) const {
    // a key is considered to be present if and only if it has been fully
    // constructed (i.e., the value has been 'saved' in the underlying
    // SingleEntryCache)
    auto insertedValuePtr = checkIfExists(key);
    if (!insertedValuePtr) {
      return false;
    }

    return insertedValuePtr->isSaved();
  }

  void clear() {
    cache_.wlock()->clear();
  }

  int size() {
    return cache_.rlock()->size();
  }

  void erase(const Key& key) {
    cache_.wlock()->erase(key);
  }

 private:
  SingleEntryCacheCachePtr checkIfExists(const Key& key) const {
    auto rLockedCache = cache_.rlock();
    auto it = rLockedCache->find(key);
    if (it != rLockedCache->end()) {
      return it->second;
    }
    return nullptr;
  }

  SingleEntryCacheCachePtr insertNew(const Key& key) {
    auto wLockedCache = cache_.wlock();
    auto [it, _] = wLockedCache->try_emplace(
        key, std::make_shared<SingleEntryCache<Value>>());
    return it->second;
  }

 private:
  // The choice of folly::Synchronized instead of folly::ConcurrentHashMap is
  // deliberate since experiments (D42219620) show that for our use
  // of case of write-once-read-potentially-many-times per key and no
  // erase/update, a) there isn't a very clear advantage when using
  // folly::concurrentHashMap with multiple threads, b) since this cache can
  // potentially be used when running on one thread, that performance is
  // important as well, and for this case folly::Synchronized is much faster
  folly::Synchronized<
      entities::Map<Key, std::shared_ptr<SingleEntryCache<Value>>>>
      cache_;
};

template <class Value>
class SingleEntryCache {
 public:
  SingleEntryCache() = default;
  ~SingleEntryCache() = default;
  SingleEntryCache(SingleEntryCache&& other) = default;
  SingleEntryCache& operator=(SingleEntryCache&& other) = default;
  SingleEntryCache(const SingleEntryCache& other) {
    std::scoped_lock<std::mutex> writeLock(other.writeMutex);
    entry_ = other.entry_;
    saved_ = other.saved_.load();
  }

  SingleEntryCache& operator=(const SingleEntryCache& other) {
    std::scoped_lock<std::mutex> writeLock(other.writeMutex);
    entry_ = other.entry_;
    saved_ = other.saved_.load();
    return *this;
  }

  template <class Function>
  const Value& getSavedOrCompute(const Function& onMiss) {
    if (saved_) {
      return entry_;
    }

    const std::scoped_lock<std::mutex> computationLock(writeMutex);
    // Note: the check below is important to ensure that onMiss() is computed
    // exactly once
    if (saved_) {
      return entry_;
    }

    entry_ = onMiss();
    saved_ = true;
    return entry_;
  }

  // returns the saved entry, or throws if key has no entry associated with it
  const Value& getSaved() const {
    if (saved_) {
      return entry_;
    }

    // it's possible that another thread has called getSavedOrCompute() on the
    // same key, so try to get the mutex and if still no value is saved,
    // throw
    std::scoped_lock<std::mutex> computationLock(writeMutex);
    if (!saved_) {
      throw std::runtime_error(
          "unexpected error: no entry found in SingleEntryCache. getSaved() should only be used when you are certain the entry will be saved before getSaved() function is complete");
    }

    return entry_;
  }

  bool isSaved() const {
    return saved_;
  }

 private:
  Value entry_;
  std::atomic<bool> saved_{false};
  // Protects writes to `entry_` and `saved_`. Reads of `entry_` on the fast
  // path in getSavedOrCompute() and getSaved() are intentionally unlocked —
  // they are gated on the atomic `saved_` check. GUARDED_BY is therefore
  // omitted: it would fire on those legitimate unlocked reads and require
  // suppressions that could hide future real races.
  mutable std::mutex writeMutex;
};

template <class Key, class Value>
class AsyncCache {
 public:
  template <class CoroFunction>
  folly::coro::Task<const Value&> getSavedOrCompute(
      const Key& key,
      CoroFunction&& onMiss) {
    // first, check if the key is already in the cache using a read lock
    auto existsEntryPtr = cache_.withRLock(
        [&](auto& rLockedCache) { return folly::get_ptr(rLockedCache, key); });
    if (existsEntryPtr) {
      co_await existsEntryPtr->promise.getFuture();
      co_return existsEntryPtr->value;
    }

    // key is not in the cache, so insert it. if inserting for the first time,
    // then this call is a producer
    auto [newEntryPtr, isProducer] = cache_.withWLock([&](auto& wLockedCache) {
      auto [it, inserted] = wLockedCache.try_emplace(key);
      return std::make_pair(&it->second, inserted);
    });

    if (isProducer) {
      try {
        newEntryPtr->value = co_await onMiss();
        newEntryPtr->promise.setValue();
      } catch (...) {
        newEntryPtr->promise.setException(
            folly::exception_wrapper{std::current_exception()});
        throw;
      }
      co_return newEntryPtr->value;
    }

    // not a producer, so wait for the producer to finish
    co_await newEntryPtr->promise.getFuture();
    co_return newEntryPtr->value;
  }

  size_t size() const {
    return cache_.rlock()->size();
  }

 private:
  struct Entry {
    Value value;
    folly::coro::SharedPromise<void> promise;
  };

  // use F14NodeMap for reference stability
  folly::Synchronized<folly::F14NodeMap<Key, Entry>> cache_;
};
} // namespace facebook::rebalancer::materializer
