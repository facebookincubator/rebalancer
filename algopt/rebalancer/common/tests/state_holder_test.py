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

from algopt.rebalancer.common import utils
from algopt.rebalancer.common.scope import SerfScope
from algopt.rebalancer.common.state_holder import StateHolder
from libfb.py import testutil


class TestStateHolder(testutil.BaseFacebookTestCase):
    def test_all_scopes(self) -> None:
        serf_scope = SerfScope(container_to_scope={"a": "b"})
        serf_scopes = {"serf_scopes": serf_scope}

        sub_region_fd_scope = utils.Holder(container_to_scope={"a3": "b3"})
        sub_region_fd = {"sub_region_fd": sub_region_fd_scope}
        state = StateHolder(
            serf_scopes=serf_scopes,
            sub_region_fd=sub_region_fd,
        )
        self.assertEqual(state.get_scope_by_name("serf_scopes"), serf_scope)
        self.assertEqual(
            # pyre-fixme[16]: Optional type has no attribute `container_to_scope`.
            state.get_scope_by_name("sub_region_fd").container_to_scope,
            # pyre-fixme[16]: `Holder` has no attribute `container_to_scope`.
            sub_region_fd_scope.container_to_scope,
        )
        self.assertNone(state.get_scope_by_name("other_scope"))
        self.assertEqual(
            state.scopes(),
            {
                "serf_scopes": serf_scope,
                "sub_region_fd": sub_region_fd_scope,
            },
        )

    def test_no_scopes(self) -> None:
        state = StateHolder()
        self.assertNone(state.get_scope_by_name("any_scope"))
        self.assertEqual(state.scopes(), {})

    def test_one_scope(self) -> None:
        serf_scope = SerfScope(container_to_scope={"a": "b"})
        serf_scopes = {"serf_scopes": serf_scope}

        state = StateHolder(serf_scopes=serf_scopes)
        self.assertEqual(state.get_scope_by_name("serf_scopes"), serf_scope)
        self.assertNone(state.get_scope_by_name("sub_region_fd"))
        self.assertNone(state.get_scope_by_name("other_scope"))
        self.assertEqual(state.scopes(), {"serf_scopes": serf_scope})
