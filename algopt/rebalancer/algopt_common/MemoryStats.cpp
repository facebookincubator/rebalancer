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

#include "algopt/rebalancer/algopt_common/MemoryStats.h"

#include <cinttypes>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>

namespace facebook::algopt {

MemoryStats MemoryStats::get() {
  std::ifstream meminfo("/proc/meminfo");
  if (!meminfo.is_open()) {
    throw std::runtime_error("Failed to open /proc/meminfo");
  }
  std::string line;
  int64_t memTotalKb = 0;
  int64_t memAvailableKb = 0;
  while (std::getline(meminfo, line)) {
    if (line.starts_with("MemTotal:")) {
      std::sscanf(line.c_str(), "MemTotal: %" SCNd64 " kB", &memTotalKb);
    } else if (line.starts_with("MemAvailable:")) {
      std::sscanf(
          line.c_str(), "MemAvailable: %" SCNd64 " kB", &memAvailableKb);
    }
  }
  return MemoryStats{
      .freeMemoryBytes = memAvailableKb * 1024,
      .usedMemoryBytes = (memTotalKb - memAvailableKb) * 1024,
  };
}

} // namespace facebook::algopt
