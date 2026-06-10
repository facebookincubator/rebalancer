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

#include "algopt/rebalancer/algopt_common/ValueSortedMap.h"

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>

namespace facebook::algopt::benchmarks {

BENCHMARK(ValueSortedMapAssignKeyIsSame) {
  constexpr int kElements = static_cast<int>(1e9);
  struct compare {
    bool operator()(
        const std::pair<std::string, int>& lhs,
        const std::pair<std::string, int>& rhs) const {
      if (lhs.second != rhs.second) {
        return lhs.second > rhs.second;
      }
      return lhs.first < rhs.first;
    }
  };
  ValueSortedMap<std::string, int, compare> vs;
  for (const auto i : folly::irange(kElements)) {
    vs.assign("a", i);
  }
}

BENCHMARK(ValueSortedMapAssignKeyIsDifferent) {
  constexpr int kElements = static_cast<int>(100e6);
  struct compare {
    bool operator()(
        const std::pair<std::string, int>& lhs,
        const std::pair<std::string, int>& rhs) const {
      if (lhs.second != rhs.second) {
        return lhs.second > rhs.second;
      }
      return lhs.first < rhs.first;
    }
  };
  ValueSortedMap<std::string, int, compare> vs;
  for (const auto i : folly::irange(kElements)) {
    vs.assign(fmt::format("object {}", i), i);
  }
}

BENCHMARK(ValueSortedMapAssignKeyIsDifferentVsStdSort) {
  constexpr int kElements = static_cast<int>(100e6);
  std::vector<std::pair<std::string, int>> v;
  v.reserve(kElements);
  for (const auto i : folly::irange(kElements)) {
    v.emplace_back(fmt::format("object {}", i), i);
  }
  std::sort(v.begin(), v.end(), [](const auto& lhs, const auto& rhs) {
    if (lhs.second != rhs.second) {
      return lhs.second > rhs.second;
    }
    return lhs.first < rhs.first;
  });
}

} // namespace facebook::algopt::benchmarks

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);
  folly::runBenchmarks();
  return 0;
}
