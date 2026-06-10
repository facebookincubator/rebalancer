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

#include "algopt/rebalancer/solver/expressions/Expression.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"
#include "algopt/rebalancer/solver/expressions/Orchestrator.h"
#include "algopt/rebalancer/solver/expressions/TopToBottomEvaluator.h"
#include "algopt/rebalancer/solver/tests/IdConverterTestUtils.h"
#include "algopt/rebalancer/solver/utils/Assignment.h"

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>
#include <folly/logging/xlog.h>

/* using override */
using namespace std;
/* using override */
using namespace facebook::rebalancer;

int sums = 1000000;
int sums_with_vars = 10;

ExprPtr maxexp;
Orchestrator orchestrator;

BENCHMARK(MaxNode, iters) {
  for ([[maybe_unused]] const auto _ : folly::irange(iters)) {
    Context context;
    ChangeSet changes;
    changes.insert(
        Change(packer::tests::object(0), packer::tests::container(0), -1));
    context.changes() = std::move(changes);
    orchestrator.evaluate(maxexp.get(), context);
  }
}

int main(int argc, char** argv) {
  std::shared_ptr<const entities::Universe> universe =
      std::make_shared<const entities::Universe>();
  const folly::Init init(&argc, &argv);

  Assignment assignment;
  assignment.setOn(packer::tests::object(0), packer::tests::container(0));
  auto var = variable(
      packer::tests::object(0),
      packer::tests::container(0),
      universe,
      assignment);
  std::vector<ExprPtr> exprs;
  exprs.reserve(sums);
  for (const auto i : folly::irange(sums)) {
    exprs.push_back(
        0.6 +
        (i < sums_with_vars ? (sums_with_vars - i) * var
                            : const_expr(0, universe)));
  }

  maxexp = max(exprs, std::move(universe));
  orchestrator.init({maxexp.get()}, AffectedByChangeDecisionData(1, 1));
  Context context;

  maxexp->fullApply(TopToBottomEvaluator(context), assignment);
  XLOG(INFO) << "Starting";

  folly::runBenchmarks();

  return 0;
}
