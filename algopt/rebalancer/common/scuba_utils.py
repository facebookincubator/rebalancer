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

import getpass
import json
import logging
import os
import sys
import time
from typing import TypeVar

from pyre_extensions import ParameterSpecification
from scubadata import ScubaData

P = ParameterSpecification("P")
T = TypeVar("T")
REBALANCER_SCUBA_TABLE = "rebalancer"


def logScubaWithRunInfo(
    table_name: str, sample: dict[str, dict[str, str | int]], silent: bool = True
) -> None:
    fbpkg = "none"
    if sys.argv[0].startswith("/var/chronos/fbpackages/"):
        parts = sys.argv[0].split("/")
        if len(parts) > 5:
            fbpkg = "{}:{}".format(parts[4], parts[5])

    if "int" not in sample:
        sample["int"] = {}

    if "normal" not in sample:
        sample["normal"] = {}

    user = getpass.getuser()
    sample["int"]["time"] = int(time.time())
    sample["normal"]["arguments"] = " ".join(sys.argv[1:])
    sample["normal"]["arguments_hash"] = str(hash(json.dumps(" ".join(sys.argv[1:]))))
    sample["normal"]["path"] = json.dumps(sys.argv[0])
    sample["normal"]["fbpkg"] = fbpkg
    sample["normal"]["user"] = user
    sample["normal"]["chronos_instance_id"] = os.environ.get(
        "CHRONOS_JOB_INSTANCE_ID", ""
    )
    cluster = os.getenv("TW_JOB_CLUSTER")
    if cluster is not None:
        sample["normal"]["tw_cluster"] = cluster
    user = os.getenv("TW_JOB_USER")
    if user is not None:
        sample["normal"]["tw_user"] = user
    name = os.getenv("TW_JOB_CLUSTER")
    if name is not None:
        sample["normal"]["tw_name"] = name
    taskid = os.getenv("TW_TASK_ID")
    if taskid is not None:
        sample["normal"]["tw_taskid"] = taskid

    if not silent:
        logging.info(
            "Logging the following sample to scuba {}".format(
                json.dumps(sample, indent=4)
            )
        )

    ScubaData(table_name).add_sample(sample)
