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

from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver


def create_base_problem_solver(
    num_containers: int, num_objects: int, can_execute_async: bool = False
) -> ProblemSolver:
    solver = ProblemSolver(
        service_name="rebalancer",
        service_scope="benchmark",
        can_execute_async=can_execute_async,
    )
    solver.setObjectName("task")
    solver.setContainerName("host")

    base_objects_per_container = num_objects // num_containers
    remainder = num_objects % num_containers

    containerToObjects: dict[str, list[str]] = {}
    obj_idx = 0
    for container_idx in range(num_containers):
        objects_count = base_objects_per_container + (
            1 if container_idx < remainder else 0
        )
        containerToObjects[f"host_{container_idx}"] = [
            f"task_{obj_idx + i}" for i in range(objects_count)
        ]
        obj_idx += objects_count

    solver.setAssignment(containerToObjects)
    return solver


def create_scope_mapping(num_scope_items: int) -> dict[str, str]:
    return {f"host_{i}": f"scope_{i}" for i in range(num_scope_items)}


def create_dynamic_dimension_data(
    num_scope_items: int, num_objects: int
) -> dict[str, dict[str, float]]:
    all_object_names = [f"task_{i}" for i in range(num_objects)]
    all_values = [float(i % 100) for i in range(num_objects)]
    object_to_value = dict(zip(all_object_names, all_values))

    return {f"scope_{i}": object_to_value for i in range(num_scope_items)}


def create_partition_data(num_groups: int, num_objects: int) -> dict[str, list[str]]:
    base_objects_per_group = num_objects // num_groups
    remainder = num_objects % num_groups

    groupToObjects: dict[str, list[str]] = {}
    obj_idx = 0
    for group_idx in range(num_groups):
        objects_count = base_objects_per_group + (1 if group_idx < remainder else 0)
        groupToObjects[f"group_{group_idx}"] = [
            f"task_{obj_idx + i}" for i in range(objects_count)
        ]
        obj_idx += objects_count

    return groupToObjects
