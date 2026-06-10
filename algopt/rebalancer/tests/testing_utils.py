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


from algopt.rebalancer.common.utils import Holder


class MockDb:
    """
    We can create multiple DB objects for each table in tests.
    And this mimics algopt.rebalancer.common.db_utils.
    """

    def __init__(self, initial_rows: list[dict[str, str]]) -> None:
        self.stored_rows: list[dict[str, str]] = initial_rows

    def _mock_read_db(
        self,
        db_name: str,
        table_name: str,
        columns_to_select: dict[str, str],
        kvs_to_filter: dict[str, str],
        skip_exceptions: bool | None = False,
    ) -> Holder:
        return Holder(valid_moves=self.stored_rows)
