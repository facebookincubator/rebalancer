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

#include "algopt/lp/environment/GurobiEnvironment.h"

#ifdef REBALANCER_USE_GUROBI

#include <stdexcept>

namespace facebook::algopt::lp {

std::shared_ptr<GurobiEnvironment> GurobiEnvironment::self_ = nullptr;

GurobiEnvironment::GurobiEnvironment() : env_(/*empty=*/true) {}

bool GurobiEnvironment::isInitialized() {
  return self_ != nullptr;
}

void GurobiEnvironment::make(
    const std::string& name,
    const std::string& app_name,
    const std::string& expiration,
    const std::string& key) {
  if (self_) {
    throw std::runtime_error("GurobiEnvironment already initialized");
  }

  // Create a temporary environment and initialize it fully before assigning to
  // self_
  auto temp_env = std::shared_ptr<GurobiEnvironment>(new GurobiEnvironment);

  temp_env->env_.set(GRB_IntParam_OutputFlag, 1);
  temp_env->env_.set("GURO_PAR_ISVNAME", name);
  temp_env->env_.set("GURO_PAR_ISVAPPNAME", app_name);
  temp_env->env_.set("GURO_PAR_ISVEXPIRATION", expiration);
  temp_env->env_.set("GURO_PAR_ISVKEY", key);
  temp_env->env_.start();

  // Only assign to self_ after the environment is fully initialized
  // This ensures isInitialized() only returns true when the environment is
  // ready to use
  self_ = std::move(temp_env);
}

void GurobiEnvironment::hideOutput() {
  self_->getEnv().set(GRB_IntParam_OutputFlag, 0);
}

void GurobiEnvironment::showOutput() {
  self_->getEnv().set(GRB_IntParam_OutputFlag, 1);
}

std::shared_ptr<GurobiEnvironment> GurobiEnvironment::get() {
  if (!self_) {
    throw std::runtime_error(notInitializedMsg.data());
  }
  return self_;
}

GRBEnv& GurobiEnvironment::getEnv() {
  if (!self_) {
    throw std::runtime_error(notInitializedMsg.data());
  }
  return env_;
}

} // namespace facebook::algopt::lp

#endif
