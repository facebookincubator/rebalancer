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

#include <fmt/core.h>

#include <exception>
#include <stdexcept>
#include <string>

namespace facebook {
namespace rebalancer {

/**
 Use this exception type for errors that are caused due to issues outside
 rebalancer codebase, such as a spec being used incorrectly, tupper ware move
 validation etc.

 For errors internal to rebalancer codebase, it is recommended to use
 std::runtime_error(...)
*/
struct RebalancerUserError : public std::runtime_error {
  std::string msg;

  explicit RebalancerUserError(const std::string& desc)
      : std::runtime_error("RebalancerUserError: " + desc) {}
};

} // namespace rebalancer
} // namespace facebook
