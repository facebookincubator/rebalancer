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

import os
from dataclasses import dataclass
from tempfile import TemporaryDirectory
from typing import Any, cast

from algopt.rebalancer.common import utils
from libfb.py.decorators import retryable
from phabricator.new_phabricator_graphql_helpers import PhabricatorPaste
from phabricator.phabricator_auth_strategy_factory import PhabricatorAuthStrategyFactory
from rebalancer.interface.thrift.Types.thrift_types import ConstraintType


CONFIG_FILE_NAME: str = "rebalancer.config.json"
OUTPUT_FILE_NAME: str = "output.json"
UNMOVABLE_HOSTS_FILE_NAME: str = "avoid_moving_hosts.json"
CANDIDATE_HOSTS_FILE_NAME: str = "candidate_hosts_map.json"
MANIFOLD_SOLVER_RUN_PREFIX: str = "solver_run"
MANIFOLD_ALLOCATION_OUTPUT_PREFIX: str = "hor_allocation_output"
PASTE_CREATION_CLIENT_ID: int = 1197447567369535

NAME: str = "name"
OBJECT_GROUP_NAME: str = "object_group_name"
SCOPE: str = "scope"


@dataclass
class ConstraintAttrs:
    groups_file_name: str
    object_group_name: str
    scope: str
    constraint_type: str
    limit_count: int | None = None
    limit_ratio: float | None = None


def parse_constraint_type(name: str) -> str:
    """
    Parses constraint type from the output name string
    """
    for constraint_type in ConstraintType.__members__.keys():
        if name.lower().startswith(constraint_type.lower()):
            return constraint_type
    return ""


def is_useful_constraint(name: str) -> bool:
    """
    Check if constraint name starts with any of the constraint
    types in ConstraintType enum
    """
    for constraint_type in ConstraintType.__members__.keys():
        if name.lower().startswith(constraint_type.lower()):
            return True
    return False


def parse_rebalancer_config_create_map(
    tmp_dir: TemporaryDirectory[str],
) -> dict[str, ConstraintAttrs]:
    """
    Parse rebalancer config file to get a mapping from constraint name
    to file name with hosts, object_group_name, scope, type and limit
    """

    config_json: dict[str, Any] = utils.readFromJsonFile(
        os.path.join(tmp_dir.name, CONFIG_FILE_NAME)
    )

    constraints: list[dict[str, str]] = config_json["constraints"]
    partition_map: dict[str, dict[str, str]] = config_json["object_partitions"]

    constraint_file_map: dict[str, ConstraintAttrs] = {}
    for constraint in constraints:
        if "partition_name" in constraint:
            # Get the file name form file path
            file_name: str = partition_map[constraint["partition_name"]][
                "partition_file"
            ].split("/")[-1]

            constraint_file_map[constraint[NAME]] = ConstraintAttrs(
                groups_file_name=file_name,
                object_group_name=constraint["partition_name"],
                scope=constraint.get(SCOPE, ""),
                constraint_type=constraint.get("type", ""),
            )
            if "limit_count" in constraint:
                constraint_file_map[constraint[NAME]].limit_count = int(
                    constraint["limit_count"]
                )
            if "limit_ratio" in constraint:
                constraint_file_map[constraint[NAME]].limit_ratio = float(
                    constraint["limit_ratio"]
                )

    return constraint_file_map


def get_all_host_names(tmp_dir: TemporaryDirectory[str], file_name: str) -> list[str]:
    """
    Get all host names from a file that contains a dict with host names
    as values
    """
    partition_host_map = utils.readFromJsonFile(os.path.join(tmp_dir.name, file_name))
    return [host for host_list in partition_host_map.values() for host in host_list]


def get_all_unmovable_hosts(tmp_dir: TemporaryDirectory[str]) -> list[str]:
    """
    Reads and returns a file with a list of hosts that are ignored by the solver
    """
    return cast(
        list[str],
        utils.readFromJsonFile(os.path.join(tmp_dir.name, UNMOVABLE_HOSTS_FILE_NAME)),
    )


def get_all_candidate_hosts(
    tmp_dir: TemporaryDirectory[str], sub_req: int
) -> list[str]:
    """
    Reads candidate map for a uuid and returns list of
    candidate host names for the given sub_req.
    Empty if sub_req doesn't exist
    """
    return cast(
        list[str],
        utils.readFromJsonFile(
            os.path.join(tmp_dir.name, CANDIDATE_HOSTS_FILE_NAME)
        ).get(str(sub_req), []),
    )


@retryable(num_tries=3)
def create_pastebin(content: str, service_user_fbid: int) -> str:
    # service_user_fbid needs INTERN_AUTOMATION ACL
    # like this: https://fburl.com/hipster/1k060dvx
    paste = PhabricatorPaste(
        PhabricatorAuthStrategyFactory.get_for_current_service(service_user_fbid),
        "algopt.rebalancer.common.parse_config_utils",
    )
    p = paste.create_phabricator_paste(
        PASTE_CREATION_CLIENT_ID,
        content=content,
        title="constraint output",
    )
    return p["url"]
