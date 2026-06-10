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

#include "algopt/rebalancer/algopt_common/Utils.h"

#include <folly/container/F14Map.h>
#include <folly/coro/Task.h>
#include <folly/futures/SharedPromise.h>
#include <folly/Synchronized.h>

namespace facebook::rebalancer {

/**
AsyncValueMap is a thread-safe map which supports the following operations:

1. registerKey(key) - registers a key in the map. This is explicitly registering
the intent that the users plans to set the value for it, perhaps later. Calling
get(key) with just a registered key will suspend until the value is set.

2. set(key, value) - sets the value for a key. Once a value is set, get(key)
will return the value.

3. get(key) - returns a shared pointer to the value w.r.t. 'key'. If 'key' was
neither registered nor set() prior to calling get(), it will throw.

4. waitAndReadFromEach() - wait for all the keys that were registered/set and
read each value via const reference. The map is NOT destroyed after a call,
so subsequent operations are still valid.

The map internally stores the value using a SharedPromise and returns a shared
pointer to it when get(key) is called.

It allows concurrent set() and get() operations if the key was previously
registered. If get() is called before set(), the coroutine suspends until
the value becomes available in the sharedPromise--again, only if the key was
previously registered. If not registered, it will throw.
*/
template <typename Key, typename Value>
class AsyncValueMap {
  using ValuePtr = std::shared_ptr<Value>;
  using ConstValuePtr = std::shared_ptr<const Value>;
  using Promise = folly::SharedPromise<ValuePtr>;

 public:
  // returns 'true' if 'key' was not previously registered, 'false' otherwise
  template <typename K>
  bool registerKey(K&& key) {
    return keyToPromise_.withWLock([&](auto& lockedKeyToPromise) {
      auto [_, isNew] = lockedKeyToPromise.try_emplace(std::forward<K>(key));
      return isNew;
    });
  }

  template <typename K, typename V>
  void set(K&& key, V&& value) {
    keyToPromise_.withWLock([&](auto& lockedKeyToPromise) {
      auto [it, isNew] = lockedKeyToPromise.try_emplace(std::forward<K>(key));
      if (!isNew && it->second.isFulfilled()) [[unlikely]] {
        throw std::runtime_error(
            fmt::format(
                "Unexpected call to set() on previously fulfilled key '{}'",
                it->first));
      }
      it->second.setValue(std::make_shared<Value>(std::forward<V>(value)));
    });
  }

  folly::coro::Task<ConstValuePtr> get(const Key& key) const {
    // extract out the semifuture while holding the lock
    auto semiFuture = keyToPromise_.withRLock([&key](auto& lockedKeyToPromise) {
      const auto promisePtr = folly::get_ptr(lockedKeyToPromise, key);
      if (!promisePtr) [[unlikely]] {
        throw std::runtime_error(
            fmt::format(
                "Unexpected call to get() w.r.t a key '{}' that was neither explicitly registered nor set",
                key));
      }
      return promisePtr->getSemiFuture();
    });

    // release the lock and await on the semifuture
    co_return co_await std::move(semiFuture);
  }

  template <typename Func>
  folly::coro::Task<void> waitAndReadFromEach(Func&& func) const {
    // get the semifutures while holding the read lock
    std::vector<std::pair<Key, folly::SemiFuture<ValuePtr>>> keySemiFuturePairs;
    keyToPromise_.withRLock([&keySemiFuturePairs](auto& lockedKeyToPromise) {
      keySemiFuturePairs.reserve(lockedKeyToPromise.size());
      for (const auto& [key, promise] : lockedKeyToPromise) {
        keySemiFuturePairs.emplace_back(key, promise.getSemiFuture());
      }
    });

    // release the lock and await on the semifutures
    for (auto&& [key, semiFuture] : keySemiFuturePairs) {
      auto valuePtr = co_await std::move(semiFuture);
      func(key, std::as_const(*valuePtr));
    }
  }

  std::size_t size() const {
    return keyToPromise_.withRLock(
        [](auto& lockedKeyToPromise) { return lockedKeyToPromise.size(); });
  }

  std::vector<Key> keys() const {
    return keyToPromise_.withRLock([](auto& lockedKeyToPromise) {
      return algopt::utils::getMapKeys(lockedKeyToPromise);
    });
  }

 private:
  folly::Synchronized<folly::F14FastMap<Key, Promise>> keyToPromise_;
};

} // namespace facebook::rebalancer
