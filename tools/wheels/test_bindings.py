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
#
# Wheel-level Python binding tests.
#
# Imports exclusively from the public ``rebalancer`` package (the installed
# wheel), not from internal ``algopt.*`` paths.  Exercises:
#   - the nanobind extension loads and ProblemSolver can be constructed
#   - ProblemSolver.ping() returns a sentinel value
#   - TypedDict spec shims produce plain dicts (JSON serializable)
#   - a real capacity-constrained solve converges to the expected assignment
#   - get_library_path() returns an existing file path
#
# Run standalone: python test_bindings.py
# Run in CI:       python -m pytest test_bindings.py -v (or python test_bindings.py)

import os
import unittest
from collections import Counter

from rebalancer import get_library_path, ProblemSolver
from rebalancer.specs import (
    CapacitySpec,
    ConstraintSpec,
    KLSearchMoveTypeSpec,
    LocalSearchSolverSpec,
    MinimizeMovementSpec,
    MoveTypeSpec,
    SingleMoveTypeSpec,
    SolverSpec,
    SwapMoveTypeSpec,
    TripleLoopMoveTypeSpec,
)


def _local_search_solver() -> SolverSpec:
    return SolverSpec(
        localSearchSolverSpec=LocalSearchSolverSpec(
            moveTypeList=[
                MoveTypeSpec(singleMoveTypeSpec=SingleMoveTypeSpec()),
                MoveTypeSpec(swapMoveTypeSpec=SwapMoveTypeSpec()),
                MoveTypeSpec(tripleLoopMoveTypeSpec=TripleLoopMoveTypeSpec()),
                MoveTypeSpec(klSearchMoveTypeSpec=KLSearchMoveTypeSpec()),
            ]
        )
    )


class TestLibraryPath(unittest.TestCase):
    def test_library_file_exists(self) -> None:
        path = get_library_path()
        self.assertIsInstance(path, str)
        self.assertTrue(
            os.path.isfile(path),
            f"bundled library not found at: {path}",
        )


class TestProblemSolverConstruction(unittest.TestCase):
    def test_ping(self) -> None:
        ps = ProblemSolver(service_name="rebalancer", service_scope="tests")
        # ping() returns a fixed sentinel (4) confirming the C++ bridge is alive.
        self.assertEqual(4, ps.ping())

    def test_method_chaining_returns_self(self) -> None:
        ps = ProblemSolver(service_name="rebalancer", service_scope="tests")
        result = (
            ps.set_object_name("task")
            .set_container_name("host")
            .set_assignment({"host0": ["task0"], "host1": []})
        )
        self.assertIsInstance(result, ProblemSolver)


class TestSpecShims(unittest.TestCase):
    def test_specs_are_plain_dicts(self) -> None:
        # The binding deserializes spec dicts via JSON; confirm the TypedDict
        # shims produce plain dicts (not custom objects) so the round-trip works.
        spec = ConstraintSpec(
            capacitySpec=CapacitySpec(name="mem", scope="host", dimension="memory")
        )
        self.assertIsInstance(spec, dict)
        self.assertIn("capacitySpec", spec)

    def test_solver_spec_is_dict(self) -> None:
        spec = _local_search_solver()
        self.assertIsInstance(spec, dict)
        self.assertIn("localSearchSolverSpec", spec)


class TestCapacitySolve(unittest.TestCase):
    """Two hosts, four tasks, each needing 10 GB.  Each host has 20 GB cap.
    host0 starts overcommitted (3 tasks), host1 undercommitted (1 task).
    The solver must rebalance to 2 tasks per host."""

    def test_rebalance_to_equal_load(self) -> None:
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
            .add_solver(_local_search_solver())
        )

        solution = solver.solve()
        assignment = solution["assignment"]

        # Every task is assigned to some host.
        self.assertCountEqual(
            ["task0", "task1", "task2", "task3"],
            assignment.keys(),
        )

        tasks_per_host: Counter[str] = Counter(assignment.values())
        # Capacity is equal so solver must split 4 tasks across 2 hosts 2-2.
        self.assertEqual(2, tasks_per_host["host0"])
        self.assertEqual(2, tasks_per_host["host1"])


class TestNoMoveSolve(unittest.TestCase):
    """With no constraints or goals, solve() on a trivial problem should
    return the original assignment unchanged."""

    def test_trivial_solve_returns_assignment(self) -> None:
        solver = ProblemSolver(service_name="rebalancer", service_scope="tests")
        (
            solver.set_object_name("item")
            .set_container_name("bin")
            .set_assignment({"bin0": ["item0", "item1"], "bin1": ["item2"]})
            .add_solver(_local_search_solver())
        )
        solution = solver.solve()
        self.assertIn("assignment", solution)
        assignment = solution["assignment"]
        self.assertEqual(3, len(assignment))
        for item in ["item0", "item1", "item2"]:
            self.assertIn(item, assignment)


if __name__ == "__main__":
    unittest.main(verbosity=2)
