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

from collections.abc import Callable, Iterable
from dataclasses import dataclass
from typing import Any, Optional, Union

from algopt.rebalancer.common.host_data import HostData
from algopt.rebalancer.common.rack_data import RackData
from algopt.rebalancer.common.state_holder import StateHolder


ConditionFunc = Callable[[StateHolder, str, Union[HostData, RackData]], bool]
Condition = Union[Optional[str], ConditionFunc]
Precompute = Optional[Union[str, Callable[[StateHolder], Union[None, Iterable[str]]]]]


@dataclass
class AssignmentFormula:
    scope: str
    field_name: str
    value: str | Callable[[Any], bool]
    condition: Condition = None
    precompute: Precompute = None
    reason: str | None = None

    def __repr__(self) -> str:
        return (
            "AssignmentFormula("
            + ",".join(str(k) + ":" + str(v) for k, v in sorted(self.__dict__.items()))
            + ")"
        )


@dataclass
class ValueFormula:
    scope: str
    value: str | Callable[[Any], Any]
    condition: Condition = None
    precompute: Precompute = None

    def __repr__(self) -> str:
        return (
            "ValueFormula("
            + ",".join(str(k) + ":" + str(v) for k, v in sorted(self.__dict__.items()))
            + ")"
        )
