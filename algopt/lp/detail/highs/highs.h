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

#include <stdexcept>
#include <string>

#ifdef REBALANCER_USE_HIGHS

// clang-format off
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#include <Highs.h>
#pragma GCC diagnostic pop
// clang-format on

#include <fmt/core.h>
#include <folly/logging/xlog.h>

#define REBALANCER_HIGHS_CALL(x)                            \
  do {                                                      \
    const auto _restat_ = (x);                              \
    if (_restat_ == HighsStatus::kError) [[unlikely]] {     \
      throw HiGHSError(fmt::format("HiGHS Error: {}", #x)); \
    }                                                       \
  } while (false)

#define REBALANCER_HIGHS_NON_THROWING_CALL(x)           \
  do {                                                  \
    const auto _restat_ = (x);                          \
    if (_restat_ == HighsStatus::kError) [[unlikely]] { \
      XLOG(ERR) << fmt::format("HiGHS Error: {}", #x);  \
    }                                                   \
  } while (false)

#endif

namespace facebook::algopt::lp::detail {

// Define a custom exception class inheriting from std::exception
class HiGHSError : public std::exception {
 private:
  std::string message;

 public:
  // Constructor to initialize the error message
  explicit HiGHSError(const std::string& msg);

  // Override the what() method to return the custom error message
  const char* what() const noexcept override;
};

} // namespace facebook::algopt::lp::detail
