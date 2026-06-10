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

#include <utility>

namespace facebook::algopt {

/// NonOwningPtr is a pointer-like class that does not own the memory it points
/// to. It is useful for passing around pointers to objects that are owned by
/// another class. It is similar to std::shared_ptr, but does not have reference
/// counting. It is also similar to std::unique_ptr, but does not own the memory
/// it points to. Used primarily for code readability.
template <typename T>
class NonOwningPtr {
 public:
  NonOwningPtr(T* ptr = nullptr) : ptr_(ptr) {}

  /// Return existing pointer and set NonOwningPtr to nullptr.
  T* release() {
    T* temp = ptr_;
    ptr_ = nullptr;
    return temp;
  }

  /// Set NonOwningPtr to point to a new pointer, if one is provided; otherwise,
  /// set to nullptr.
  void reset(T* ptr = nullptr) {
    ptr_ = ptr;
  }

  /// Return pointer held by NonOwningPtr.
  T* get() const {
    return ptr_;
  }

  /// Deference pointer held by NonOwningPtr.
  T& operator*() const {
    assert(ptr_ != nullptr);
    return *ptr_;
  }

  /// Dereference pointer held by NonOwningPtr.
  T* operator->() const {
    assert(ptr_ != nullptr);
    return ptr_;
  }

 private:
  T* ptr_ = nullptr;
};

/// Create a NonOwningPtr from a class and its constructor arguments.
template <typename T, typename... Args>
NonOwningPtr<T> makeNonOwningPtr(Args&&... args) {
  return NonOwningPtr<T>(new T(std::forward<Args>(args)...));
}

} // namespace facebook::algopt
