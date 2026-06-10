#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# pyre-strict

"""End-to-end smoke test for the nanobind Python bindings.

Mirrors the simplest case from interface/fb/py_client/tests/capacity_test.py
but constructs Thrift specs via the ``rebalancer.specs`` TypedDict shims (which
just produce dict[str, Any] values shaped like the Thrift JSON wire format),
so it builds in OSS too and exercises the typed surface that ships in the
wheel.

Note: ``from rebalancer import ProblemSolver`` (the __init__.py re-export that
OSS wheel users see) is only exercised by GitHub Actions CI, since the internal
test target doesn't carry the oss_root __init__.py dep.
"""

import unittest
from collections import Counter

from algopt.rebalancer.python._rebalancer import ProblemSolver
from rebalancer.specs import (
    CapacitySpec,
    ConstraintSpec,
    KLSearchMoveTypeSpec,
    LocalSearchSolverSpec,
    MoveTypeSpec,
    SingleMoveTypeSpec,
    SolverSpec,
    SwapMoveTypeSpec,
    TripleLoopMoveTypeSpec,
)


class SmokeTest(unittest.TestCase):
    def test_specs_are_plain_dicts(self) -> None:
        # The binding deserializes spec dicts via JSON; verify the TypedDict
        # shims produce plain dicts at runtime so the round-trip works.
        spec = ConstraintSpec(
            capacitySpec=CapacitySpec(name="x", scope="host", dimension="mem")
        )
        self.assertIsInstance(spec, dict)
        self.assertIn("capacitySpec", spec)

    def test_ping(self) -> None:
        ps = ProblemSolver(service_name="rebalancer", service_scope="tests")
        self.assertEqual(4, ps.ping())

    def test_initially_broken_capacity_constraint(self) -> None:
        # 2 hosts, 4 tasks, each host has 20 GB capacity, each task needs 10 GB.
        # host0 starts with 3 tasks (overcommitted), host1 with 1.
        # Solver should rebalance to 2 each.
        solver = ProblemSolver(service_name="rebalancer", service_scope="tests")
        (
            solver.set_object_name("task")
            .set_container_name("host")
            .should_use_dynamic_object_ordering(True)
            .set_assignment(
                {
                    "host0": ["task0", "task1", "task2"],
                    "host1": ["task3"],
                }
            )
            .add_object_dimension(
                "memory",
                {"task0": 10.0, "task1": 10.0, "task2": 10.0, "task3": 10.0},
            )
            .add_container_dimension(
                "memory", container_to_value={}, default_value=20.0
            )
            .add_constraint(
                ConstraintSpec(
                    capacitySpec=CapacitySpec(
                        name="memory_capacity",
                        scope="host",
                        dimension="memory",
                    )
                )
            )
            .add_solver(
                SolverSpec(
                    localSearchSolverSpec=LocalSearchSolverSpec(
                        moveTypeList=[
                            MoveTypeSpec(singleMoveTypeSpec=SingleMoveTypeSpec()),
                            MoveTypeSpec(swapMoveTypeSpec=SwapMoveTypeSpec()),
                            MoveTypeSpec(
                                tripleLoopMoveTypeSpec=TripleLoopMoveTypeSpec()
                            ),
                            MoveTypeSpec(klSearchMoveTypeSpec=KLSearchMoveTypeSpec()),
                        ]
                    )
                )
            )
        )

        solution = solver.solve()
        tasks_in_host: Counter[str] = Counter()
        for host in solution["assignment"].values():
            tasks_in_host[host] += 1

        self.assertEqual(2, tasks_in_host["host0"])
        self.assertEqual(2, tasks_in_host["host1"])


if __name__ == "__main__":
    unittest.main()
