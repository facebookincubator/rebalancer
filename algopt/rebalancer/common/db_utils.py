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

import datetime
import time
from collections.abc import Mapping, Sequence
from typing import Any, NamedTuple

import common.db.smc_db as smc_db
from algopt.rebalancer.common import utils


WRITE = "scriptrw"
READ = "scriptro"
# Allow 4 days for moves to finish before logging and alerting to unblock.
MOVE_TIMEOUT: int = 4 * 24 * 60 * 60
CONTINUOUS_BALANCING_DB = "xdb.rebalancer_continuous_balancing"
CONTINUOUS_BALANCING_META_DATA_TABLE = "continuous_balancing_meta_data"
CONTINUOUS_BALANCING_SPREAD_TABLE = "continuous_balancing_spread"
CONTINUOUS_BALANCING_POWER_TABLE = "continuous_balancing_power"


class ContinuousBalanceMetaData(NamedTuple):
    region: str
    server_type: str
    run_uuid: str
    creation_time: datetime.datetime
    num_swaps: int


def get_table_name(base_table_name: str, abtest: bool) -> str:
    return base_table_name if not abtest else f"{base_table_name}_abtest"


def readDbSamples(
    db_name: str,
    table_name: str,
    columns_to_select: Mapping[str, str],
    kvs_to_filter: Mapping[str, Any],
    skip_exceptions: bool = False,
) -> utils.Holder:
    conn = smc_db.RetryableSmcConnection(db_name, READ)
    moves = conn.select_all_dict(columns_to_select, kvs_to_filter, table_name)
    valid_moves: list[dict[str, Any]] = []
    stale_moves: list[dict[str, Any]] = []
    for move in moves:
        if not skip_exceptions and time.time() - move["timestamp"] >= MOVE_TIMEOUT:
            stale_moves.append(move)
        else:
            valid_moves.append(move)
    return utils.Holder(valid_moves=valid_moves, stale_moves=stale_moves)


def writeManyDbSamples(
    db_name: str,
    table_name: str,
    samples: Sequence[Mapping[str, Any]],
    on_dupe_update: bool = False,
    on_dupe_update_blocklist: set[str] | None = None,
    connection: smc_db.RetryableSmcConnection | None = None,
) -> None:
    if connection:
        conn = connection
    else:
        conn = smc_db.RetryableSmcConnection(db_name, WRITE)

    conn.insert_many(
        rows=samples,
        table=table_name,
        on_dupe_update=on_dupe_update,
        on_dupe_update_blocklist=on_dupe_update_blocklist,
    )

    if not connection:
        # Commit only if connection is created within this method
        conn.commit()


def deleteDbSamples(
    db_name: str,
    table_name: str,
    samples: Sequence[Mapping[str, Any]],
    connection: smc_db.RetryableSmcConnection | None = None,
) -> None:
    if connection:
        conn = connection
    else:
        conn = smc_db.RetryableSmcConnection(db_name, WRITE)

    for sample in samples:
        conn.delete_dict(kvs_to_delete=sample, table=table_name)

    if not connection:
        # Commit only if connection is created within this method
        conn.commit()


def get_latest_continuous_run_uuids(
    region: str | None, server_type: str | None
) -> list[ContinuousBalanceMetaData]:
    where_clause: list[str] = []
    if region is not None:
        where_clause.append(f"region = '{region}'")
    if server_type is not None:
        where_clause.append(f"server_type = '{server_type}'")

    sql = """
        SELECT
            c.region,
            c.server_type,
            c.uuid,
            c.creation_time,
            c.num_swaps
        FROM continuous_balancing_meta_data AS c,
            (
            SELECT
                region,
                server_type,
                MAX(creation_time) AS creation_time
            FROM continuous_balancing_meta_data
            {where}
            GROUP BY
                region,
                server_type
        ) AS b
        WHERE
            c.region = b.region
            AND c.server_type = b.server_type
            AND c.creation_time = b.creation_time
    """.format(
        where="" if not where_clause else "WHERE {}".format(" AND ".join(where_clause))
    )
    conn = smc_db.RetryableSmcConnection(CONTINUOUS_BALANCING_DB, READ)
    return [
        ContinuousBalanceMetaData(
            region=r["region"],
            server_type=r["server_type"],
            run_uuid=r["uuid"],
            creation_time=r["creation_time"],
            num_swaps=r["num_swaps"],
        )
        for r in conn.query(sql)
    ]


def get_latest_continuous_run_uuid(
    region: str | None, server_type: str | None
) -> ContinuousBalanceMetaData | None:
    uuids = get_latest_continuous_run_uuids(region, server_type)
    if len(uuids) == 0:
        return None
    else:
        return uuids[0]
