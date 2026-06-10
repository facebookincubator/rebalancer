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

#include "algopt/rebalancer/interface/ProblemSolverFactory.h"

#include "algopt/rebalancer/interface/ProblemSolver.h"

#include <folly/system/HardwareConcurrency.h>

namespace facebook::rebalancer::interface {

constexpr std::string_view kDefaultThreadPoolNamePrefix = "CPUThreadPool";

std::unique_ptr<ProblemSolver> ProblemSolverFactory::makeProblemSolver(
    std::string serviceName,
    std::string serviceScope,
    bool canExecuteAsync) {
  return makeProblemSolver(
      ProblemSolver::makeCPUThreadPoolExecutor(
          kDefaultThreadPoolNamePrefix,
          /*numThreads=*/folly::available_concurrency()),
      std::move(serviceName),
      std::move(serviceScope),
      canExecuteAsync);
}

std::unique_ptr<ProblemSolver> ProblemSolverFactory::makeProblemSolver(
    std::shared_ptr<folly::ThreadPoolExecutor> executor,
    std::string serviceName,
    std::string serviceScope,
    bool canExecuteAsync) {
  return std::make_unique<ProblemSolver>(
      executor,
      std::move(serviceName),
      std::move(serviceScope),
      /*prepareProblemOnly=*/false,
      canExecuteAsync);
}

} // namespace facebook::rebalancer::interface
