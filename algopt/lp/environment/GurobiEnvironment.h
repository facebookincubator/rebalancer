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

#ifdef REBALANCER_USE_GUROBI

#include "algopt/lp/detail/gurobi/gurobi.h"

#include <memory>
#include <string_view>

namespace facebook::algopt::lp {

/// Manages Gurobi's environment. `GurobiEnvironment::make` must be called
/// before using Gurobi. The make call is not thread safe.
class GurobiEnvironment {
 public:
  static void make(
      const std::string& name,
      const std::string& app_name,
      const std::string& expiration,
      const std::string& key);

  static std::shared_ptr<GurobiEnvironment> get();
  static bool isInitialized();
  static constexpr std::string_view notInitializedMsg =
      "GurobiEnvironment is not initialized; please use GurobiEnvironment::make().";

  static void hideOutput();
  static void showOutput();

  GRBEnv& getEnv();

  GurobiEnvironment(const GurobiEnvironment&) = delete;
  GurobiEnvironment& operator=(const GurobiEnvironment&) = delete;
  GurobiEnvironment(GurobiEnvironment&&) = delete;
  GurobiEnvironment& operator=(GurobiEnvironment&&) = delete;

  ~GurobiEnvironment() = default;

 private:
  GurobiEnvironment();
  GRBEnv env_;

  static std::shared_ptr<GurobiEnvironment> self_; // NOLINT
};

} // namespace facebook::algopt::lp

#endif
