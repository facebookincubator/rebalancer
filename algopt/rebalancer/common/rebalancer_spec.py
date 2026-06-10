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

from dataclasses import dataclass, field
from typing import Any

from algopt.rebalancer.common.formula_types import AssignmentFormula, ValueFormula
from algopt.rebalancer.common.utils import Holder


@dataclass
class ReBalancerAllocationSpec:
    hosts_to_be_allocated: set[str] = field(default_factory=set)
    name: str = ""
    weight_multiplier: float = 0.0
    constraint: bool = False
    # pyre-fixme[24]: Generic type `dict` expects 2 type parameters, use
    #  `typing.Dict[<key type>, <value type>]` to avoid runtime subscripting errors.
    container_fields: dict = field(default_factory=dict)

    allocation_container_formula: ValueFormula | None = None
    # If this field is not set, spec_utils will not add
    # any additional constraint or goal on this spec.
    # If it is set, a constraint or goal is added to drain
    # the given rack according to values evaluated by ValueFormula
    maximize_allocation_formula: ValueFormula | None = None


@dataclass
class ReBalancerSpec:
    apply_movesets: bool = False
    restrict_moving_object_only_once: bool = False
    only_swaps: bool = False
    move_all_on_container_as_group: bool = False
    move_all_on_container_as_group_to_empty_only: bool = False
    objects_match_serf_fields: list[str] = field(default_factory=list)
    containers_match_serf_fields: list[str] = field(default_factory=list)
    minimize_movement: list[Holder] = field(default_factory=list)
    goals: list[dict[str, Any]] = field(default_factory=list)
    constraints: list[dict[str, Any]] = field(default_factory=list)
    minimize_containers: list[Holder] = field(default_factory=list)
    require_to_move: list[Holder] = field(default_factory=list)
    require_all_swaps_contain: list[Holder] = field(default_factory=list)
    object_group_specs: list[Holder] = field(default_factory=list)
    placement_constraint_spec: Holder | None = None
    allocation_specs: list[ReBalancerAllocationSpec] = field(default_factory=list)
    limit_total_number_of_moves: int = 0
    subsample_max_movable_objects: int | None = None
    serf_scopes: Holder | None = None
    moves_in_progress_list: list[dict[str, str]] | None = None
    formulas_after_serf: list[AssignmentFormula | ValueFormula] = field(
        default_factory=list
    )
    formulas_after_counters: list[AssignmentFormula | ValueFormula] = field(
        default_factory=list
    )
    look_at_imbalance_goals_independently: bool = False
