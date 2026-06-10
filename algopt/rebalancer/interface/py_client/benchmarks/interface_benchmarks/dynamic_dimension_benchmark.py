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
from dataclasses import dataclass

from algopt.rebalancer.interface.py_client.benchmarks.interface_benchmarks.benchmark_utils import (
    create_base_problem_solver,
    create_dynamic_dimension_data,
    create_scope_mapping,
)
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from windtunnel.benchmarks.python_benchmark_runner.benchmark import (
    main,
    register_benchmark,
)


FILE_PATH: str = (
    "algopt/rebalancer/interface/py_client/benchmarks/dynamic_dimension_benchmark.py"
)

DIMENSION_NAME = "test_dimension"
SCOPE_NAME = "test_scope"
PARTITION_NAME = "test_partition"


@dataclass
class ProblemData:
    solver: ProblemSolver
    dimension: dict[str, dict[str, float]]


def setup_problem(
    num_scope_items: int,
    num_objects: int,
    can_execute_async: bool = False,
) -> ProblemData:
    solver = create_base_problem_solver(num_scope_items, num_objects, can_execute_async)
    solver.addScope(SCOPE_NAME, create_scope_mapping(num_scope_items))
    return ProblemData(
        solver, create_dynamic_dimension_data(num_scope_items, num_objects)
    )


DATA_400x360k: ProblemData = setup_problem(num_scope_items=400, num_objects=360000)
DATA_400x360k_ASYNC: ProblemData = setup_problem(
    num_scope_items=400, num_objects=360000, can_execute_async=True
)
DATA_100x90k: ProblemData = setup_problem(num_scope_items=100, num_objects=90000)
DATA_250x2m: ProblemData = setup_problem(250, 2000000)


@register_benchmark(FILE_PATH)
def benchmark_add_dynamic_dimension_100x90k() -> None:
    DATA_100x90k.solver.addDynamicObjectDimension(
        dimensionName=DIMENSION_NAME,
        scope=SCOPE_NAME,
        scopeItemToObjectToValue=DATA_100x90k.dimension,
        defaultValue=0.0,
    )


@register_benchmark(FILE_PATH)
def benchmark_add_dynamic_dimension_250x2m() -> None:
    DATA_250x2m.solver.addDynamicObjectDimension(
        dimensionName=DIMENSION_NAME,
        scope=SCOPE_NAME,
        scopeItemToObjectToValue=DATA_250x2m.dimension,
        defaultValue=0.0,
    )


@register_benchmark(FILE_PATH)
def benchmark_add_dynamic_dimension_250x2m_with_partition() -> None:
    DATA_250x2m.solver.addDynamicObjectDimensionByPartition(
        dimensionName=DIMENSION_NAME,
        scope=SCOPE_NAME,
        partitionName=PARTITION_NAME,
        scopeItemToGroupToValue=DATA_250x2m.dimension,
        defaultValue=0.0,
    )


@register_benchmark(FILE_PATH)
def benchmark_several_add_dynamic_dimension_400x360k_not_async() -> None:
    dimensionCount: int = 3
    for i in range(dimensionCount):
        DATA_400x360k.solver.addDynamicObjectDimension(
            dimensionName=f"{DIMENSION_NAME}_{i}",
            scope=SCOPE_NAME,
            scopeItemToObjectToValue=DATA_400x360k.dimension,
            defaultValue=0.0,
        )


@register_benchmark(FILE_PATH)
def benchmark_several_add_dynamic_dimension_400x360k_async() -> None:
    dimensionCount: int = 3
    for i in range(dimensionCount):
        DATA_400x360k_ASYNC.solver.addDynamicObjectDimension(
            dimensionName=f"{DIMENSION_NAME}_{i}",
            scope=SCOPE_NAME,
            scopeItemToObjectToValue=DATA_400x360k_ASYNC.dimension,
            defaultValue=0.0,
        )


if __name__ == "__main__":
    main()
