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

#include "algopt/rebalancer/treeprof/ThreadMemoryMonitor.h"

#include "algopt/rebalancer/algopt_common/Timer.h"

#include <folly/Memory.h>
#include <folly/memory/MallctlHelper.h>
#include <folly/memory/Malloc.h>

namespace facebook::algopt::treeprof {

thread_local uint64_t lastNet = 0;
thread_local double lastTime = 0;

uint64_t net() {
  uint64_t allocated;
  uint64_t deallocated;
  folly::mallctlRead("thread.allocated", &allocated);
  folly::mallctlRead("thread.deallocated", &deallocated);
  return allocated - deallocated;
}

int64_t ThreadMemoryMonitor::delta() {
  if (folly::usingJEMalloc()) {
    return net() - lastNet;
  }
  return 0;
}

uint64_t ThreadMemoryMonitor::peak() {
  if (!folly::usingJEMalloc()) {
    return 0;
  }
  uint64_t result = 0;
  folly::mallctlRead("thread.peak.read", &result);
  return result;
}

void ThreadMemoryMonitor::reset() {
  reset(algopt::seconds_since_epoch());
}

void ThreadMemoryMonitor::reset(double now) {
  if (folly::usingJEMalloc()) {
    lastNet = net();
    folly::mallctlCall("thread.peak.reset");
  }
  lastTime = now;
}

double ThreadMemoryMonitor::lastResetTime() {
  return lastTime;
}

} // namespace facebook::algopt::treeprof
