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

#include "algopt/lp/environment/Environment.h" // NOLINT

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <stdexcept>

#define REBALANCER_EXPECT_RUNTIME_ERROR(code, message) \
  EXPECT_THAT(                                         \
      [&]() { code; }, testing::ThrowsMessage<std::runtime_error>(message))

#define REBALANCER_EXPECT_RUNTIME_ERROR_CONTAINS(code, substr) \
  EXPECT_THAT(                                                 \
      [&]() { code; },                                         \
      testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr(substr)));

#define REBALANCER_EXPECT_RUNTIME_ERROR_MATCHES(code, regex) \
  EXPECT_THAT(                                               \
      [&]() { code; },                                       \
      testing::ThrowsMessage<std::runtime_error>(            \
          testing::ContainsRegex(regex)));

template <typename T>
inline std::vector<T> toVec(const auto& x) {
  return std::vector<T>(x.begin(), x.end());
}

template <typename T>
inline std::set<T> toSet(const auto& x) {
  return std::set<T>(x.begin(), x.end());
}

template <typename T>
inline std::set<T> toSet(const std::initializer_list<T>& x) {
  return std::set<T>(x);
}
