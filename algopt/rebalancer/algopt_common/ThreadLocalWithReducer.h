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

#include <folly/container/irange.h>
#include <folly/Likely.h>
#include <folly/Portability.h>
#include <folly/ScopeGuard.h>
#include <folly/SharedMutex.h>
#include <folly/ThreadLocal.h>

#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace facebook::rebalancer {

/**
 * A thread-local object that reduces into a master thread's object when
 * requested.
 *
 * This is useful when you want to accumulate some data from different threads
 * and then combine this data into a single object. The reduction is done
 * lazily, i.e. only when the master thread's object is requested.
 *
 * This class provides an interface to get the object owned by the current
 * thread, and an interface to get the master thread's object, which will be
 * reduced into by other threads' objects.
 */
template <class T>
class ThreadLocalWithReducer {
 public:
  template <typename R>
  explicit constexpr ThreadLocalWithReducer(R&& reducer) noexcept
      : ThreadLocalWithReducer(reducer, []() -> T { return {}; }) {}

  /// @param constructor The constructor function. Used to create new objects.
  ///                    Objects must have a move method and a default
  ///                    constructor.
  template <
      typename R,
      typename F,
      std::enable_if_t<folly::is_invocable_r_v<T, F>, int> = 0>
  explicit ThreadLocalWithReducer(R&& reducer, F&& constructor)
      : masterThreadId_{std::this_thread::get_id()},
        reducer_{std::forward<R>(reducer)},
        constructor_{std::forward<F>(constructor)} {
    std::ignore = makeTlp();
  }

  ~ThreadLocalWithReducer() = default;

  ThreadLocalWithReducer(ThreadLocalWithReducer&& that) noexcept
      : masterThreadId_{std::exchange(that.masterThreadId_, {})},
        reducer_{std::exchange(that.reducer_, {})},
        constructor_{std::exchange(that.constructor_, {})},
        objects_{std::exchange(that.objects_, {})},
        tlp_{std::exchange(that.tlp_, {})} {}

  ThreadLocalWithReducer& operator=(ThreadLocalWithReducer&& that) noexcept {
    assert(this != &that);
    masterThreadId_ = std::exchange(that.masterThreadId_, {});
    reducer_ = std::exchange(that.reducer_, {});
    constructor_ = std::exchange(that.constructor_, {});
    objects_ = std::exchange(that.objects_, {});
    tlp_ = std::exchange(that.tlp_, {});
    return *this;
  }

  /// Returns a reference to the object owned by the current thread.
  FOLLY_ERASE T& getLocal() const {
    const auto ptr = tlp_.get();
    return FOLLY_LIKELY(!!ptr) ? *ptr : *makeTlp();
  }

  /// Returns a pointer to the object owned by the current thread.
  T* operator->() const {
    return getLocal();
  }

  /// Returns a pointer to the object owned by the current thread.
  T& operator*() const {
    return *getLocal();
  }

  /// Returns a reference to the object owned by the master thread.
  /// All objects owned by other threads are reduced into the master thread's
  /// and then zeroed out.
  T& getMaster() const {
    if (std::this_thread::get_id() != masterThreadId_) {
      throw std::runtime_error(
          "ThreadLocalWithReducer::getMaster() can only be called from the master thread!");
    }

    // Reduce into the first object, which is, by definition, the one owned by
    // the master thread.
    for (const auto i : folly::irange(1, objects_.size())) {
      reducer_(*objects_[0], *objects_[i]);
      // Zero out the other thread's copy. This allows us to safely query the
      // master thread and then continue aggregating without doing
      // double-counting.
      *objects_[i] = std::move(constructor_());
    }
    return *objects_[0];
  }

  /// Zeros out all objects owned by all threads.
  void clear() {
    if (std::this_thread::get_id() != masterThreadId_) {
      throw std::runtime_error(
          "ThreadLocalWithReducer::clear() can only be called from the master thread!");
    }

    for (auto& x : objects_) {
      *x = std::move(constructor_());
    }
  }

 private:
  // non-copyable
  ThreadLocalWithReducer(const ThreadLocalWithReducer&) = delete;
  ThreadLocalWithReducer& operator=(const ThreadLocalWithReducer&) = delete;

  // Creates a new object for the current thread and returns a pointer to it.
  FOLLY_NOINLINE T* makeTlp() const {
    auto new_object = std::make_shared<T>();
    *new_object = std::move(constructor_());

    {
      std::lock_guard<std::mutex> lock(mutex_);
      objects_.push_back(new_object);
    }

    tlp_.reset(new_object);

    return new_object.get();
  }

  /// The thread id of the thread that created this object. Used to ensure that
  /// only the master thread can call `getMaster()` and `clear()`.
  std::thread::id masterThreadId_;
  /// The reducer function. Used to reduce objects owned by other threads into
  /// the master thread's object.
  std::function<void(T&, const T&)> reducer_;
  /// The constructor function. Used to create new objects.
  std::function<T()> constructor_;
  /// Contains all objects owned by all threads.
  mutable std::vector<std::shared_ptr<T>> objects_;
  /// Points to the object owned by the current thread.
  mutable folly::ThreadLocalPtr<T> tlp_;
  // Protects writes to `objects_` in makeTlp(). Reads in getMaster() and
  // clear() are intentionally unlocked — they rely on the masterThreadId_
  // check instead. GUARDED_BY is therefore omitted: it would fire on those
  // legitimate unlocked reads and require suppressions that could hide future
  // real races.
  mutable std::mutex mutex_{};
};

} // namespace facebook::rebalancer
