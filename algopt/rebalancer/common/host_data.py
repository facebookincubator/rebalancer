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

from facebook.core_systems.cluster.thrift_types import (
    ClusterRackState,
    ClusterRackType,
    ClusterState,
    ClusterType,
)


@dataclass
class HostData:
    serf: dict[str, str] = field(default_factory=dict)
    tasks: list[str] = field(default_factory=list)
    name: str = ""
    asset_id: int = 0
    scheme: str = ""
    cpu_architecture: str | None = None
    server_type: int = 0
    server_type_with_flash: str = ""
    type_dimension: str = ""
    tier: str = ""
    t8_unit: str = ""
    cluster_rack_type: ClusterRackType | None = None
    # TODO: type as DeviceStatus
    status: int | None = None
    # TODO: type as EvaluationStatus
    evaluation_status: int | None = None
    cluster_rack_state: ClusterRackState | None = None
    # TODO: type as MaintenanceStatus
    maintenance_status: int | None = None
    cluster_type: ClusterType | None = None
    cluster_state: ClusterState | None = None
    flash_endurance_daily_capacity: int = 0
    failed_fetching: bool = False
    in_other_domain: bool = False
    fault_domains: dict[str, str] = field(default_factory=dict)
    turnup_scope_id: str | None = None
    outside: bool = False
    usecase_host: bool = False
    arg_tier: str | None = None
    movable: bool = False
    subsample_movable: bool | None = None
    to_allocate: bool = False
    not_accepting: bool = False
    region: str = ""
    datacenter: str = ""
    row: str = ""
    cluster: str = ""
    pod: str = ""
    rack: str = ""
    has_flash: bool = False
    hostgroup: str = ""
    serf_to_match: dict[str, str] = field(default_factory=dict)
    # mysql fields
    slot_type: int = 0
    slot_count: int = 0
    type_scheme_index: int = 0
    spare_slots: list[str] = field(default_factory=list)
    active_slots: list[str] = field(default_factory=list)
    configured_slots: list[str] = field(default_factory=list)
    replicaset_instances: list[str] = field(default_factory=list)
    filtered_reasons: list[str] = field(default_factory=list)
    data_size: int = 0
    allocator_server_type: int | None = None
    failed_drive_counter_fetching: bool = False
    maybe_sah_l1_node_id: str | None = None
    maybe_is_sah_l1_node_id_spare_pool: bool | None = None
    maybe_is_sah_l4_node_type_optimus: bool | None = None
    rru: float = 0.0

    @property
    def lsst(self) -> int:
        return int(self.serf["logical_server_subtype"])
