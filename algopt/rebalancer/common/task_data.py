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

from algopt.rebalancer.common import utils
from algopt.rebalancer.common.job_data import JobData


@dataclass
class TaskData(utils.Holder):
    name: str
    job_data: JobData
    arg_entitlement: str
    arg_tier: str
    arg_vpool: set[str]
    arg_quota_id: str
    host: str
    state: int
    alive_up_time: int
    job: str
    cpu: Any
    ram: Any
    flash: Any
    instances: list[str] = field(default_factory=list)
    shard_replica_key: str | None = None
    usecase_task: bool = False
    to_allocate: bool = False
    movable: bool = False
    filtered_reasons: list[str] = field(default_factory=list)
