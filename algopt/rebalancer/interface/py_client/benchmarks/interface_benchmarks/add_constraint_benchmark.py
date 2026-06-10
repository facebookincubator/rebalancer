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

"""
Benchmark: addConstraint using fromValue (linear scan) vs ConstraintSpecs (direct).

fromValue tries each union field in order until one matches, so cost grows with
field position. CapacitySpec is field #6, MinimizeMovementSpec is field #33.
"""

from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import ConstraintSpecs
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    CapacitySpec,
    MinimizeMovementSpec,
)
from windtunnel.benchmarks.python_benchmark_runner.benchmark import (
    main,
    register_benchmark,
)

FILE_PATH: str = "algopt/rebalancer/interface/py_client/benchmarks/interface_benchmarks/add_constraint_benchmark.py"

NUM_CONSTRAINTS: int = 10_000

CAPACITY_SPECS: list[CapacitySpec] = [
    CapacitySpec(name=f"cap_{i}", scope="host", dimension="mem")
    for i in range(NUM_CONSTRAINTS)
]
MINIMIZE_MOVEMENT_SPECS: list[MinimizeMovementSpec] = [
    MinimizeMovementSpec(name=f"mm_{i}", scope="host", dimension="mem")
    for i in range(NUM_CONSTRAINTS)
]
CAPACITY_UNIONS: list[ConstraintSpecs] = [
    ConstraintSpecs(capacitySpec=s) for s in CAPACITY_SPECS
]
MINIMIZE_MOVEMENT_UNIONS: list[ConstraintSpecs] = [
    ConstraintSpecs(minimizeMovementSpec=s) for s in MINIMIZE_MOVEMENT_SPECS
]


def _make_solver() -> ProblemSolver:
    solver = ProblemSolver(service_name="rebalancer", service_scope="bench")
    solver.setObjectName("task")
    solver.setContainerName("host")
    solver.setAssignment({"h1": ["t1"]})
    solver.addObjectDimension("mem", {"t1": 1.0})
    solver.addContainerDimension("mem", {"h1": 100.0})
    return solver


@register_benchmark(FILE_PATH)
def benchmark_fromValue_CapacitySpec_field6() -> None:
    solver = _make_solver()
    for i in range(NUM_CONSTRAINTS):
        solver.addConstraint(CAPACITY_SPECS[i])


@register_benchmark(FILE_PATH)
def benchmark_fromValue_MinimizeMovementSpec_field33() -> None:
    solver = _make_solver()
    for i in range(NUM_CONSTRAINTS):
        solver.addConstraint(MINIMIZE_MOVEMENT_SPECS[i])


@register_benchmark(FILE_PATH)
def benchmark_direct_CapacitySpec_field6() -> None:
    solver = _make_solver()
    for i in range(NUM_CONSTRAINTS):
        solver.addConstraint(CAPACITY_UNIONS[i])


@register_benchmark(FILE_PATH)
def benchmark_direct_MinimizeMovementSpec_field33() -> None:
    solver = _make_solver()
    for i in range(NUM_CONSTRAINTS):
        solver.addConstraint(MINIMIZE_MOVEMENT_UNIONS[i])


if __name__ == "__main__":
    main()
