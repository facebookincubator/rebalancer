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
from libfb.py.testutil import BaseFacebookTestCase
from rebalancer.interface.thrift.Types.thrift_types import (
    GroupRoutingRings,
    RoutingRing,
)
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    ConstraintSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    CapacitySpec,
    CapacitySpecBound,
    CapacitySpecDefinition,
    GroupCountSpec,
    Limit,
    LimitType,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    KLSearchMoveTypeSpec,
    LocalSearchSolverSpec,
    MoveTypeSpec,
    SingleMoveTypeSpec,
    SwapMoveTypeSpec,
    TripleLoopMoveTypeSpec,
)


class TestObjectPartitionRoutingDimension(BaseFacebookTestCase):
    def get_group_to_routing_rings(self) -> dict[str, GroupRoutingRings]:
        totalTrafficPerTenant = 100.0
        return {
            "tenant0": GroupRoutingRings(
                routingRings=[
                    RoutingRing(
                        originScopeItem="region0",
                        originTraffic=0.98 * totalTrafficPerTenant,
                        destinationScopeItemSets=[["region1"], ["region0", "region2"]],
                    ),
                    RoutingRing(
                        originScopeItem="region1",
                        originTraffic=0.02 * totalTrafficPerTenant,
                        destinationScopeItemSets=[["region0", "region1", "region2"]],
                    ),
                ]
            ),
            "tenant1": GroupRoutingRings(
                routingRings=[
                    RoutingRing(
                        originScopeItem="region2",
                        originTraffic=totalTrafficPerTenant,
                        destinationScopeItemSets=[["region2"], ["region0", "region1"]],
                    )
                ]
            ),
        }

    def test_object_partition_routing_dimension_test(self) -> None:
        # This is the same example as in the test 'CapacityWithAbsoluteLimit' in /interface/tests/ObjectPartitionRoutingDimensionTest.cpp. For explanations, see the cpp test
        solver = ProblemSolver(service_name="rebalancer", service_scope="tests")
        solver.setObjectName("replica")
        solver.setContainerName("region")
        solver.shouldUseDynamicObjectOrdering(True)

        solver.setAssignment(
            {
                "region0": ["replica1:0"],
                "region1": ["replica0:0", "replica1:1"],
                "region2": ["replica0:1"],
            }
        )

        # define partition
        tenant_to_replicas: dict[str, list[str]] = {
            "tenant0": ["replica0:0", "replica0:1"],
            "tenant1": ["replica1:0", "replica1:1"],
        }
        solver.addPartition("tenant", tenant_to_replicas)

        # define routing config
        solver.addRoutingConfig(
            "routingConfig1",
            "region",
            "tenant",
            self.get_group_to_routing_rings(),
            # OriginToDestinationLatency
            {
                "region0": {"region0": 5, "region1": 10, "region2": 5},
                "region1": {"region0": 5, "region1": 0, "region2": 15},
                "region2": {"region0": 50, "region1": 40, "region2": 0},
            },
        )

        # define addObjectPartitionRoutingDimension
        solver.addObjectPartitionRoutingDimension(
            "load", "tenant", "routingConfig1", {"tenant0": 20, "tenant1": 50}, 0
        )

        # Constraint: At most one replica of the same tenant can be placed in the same region
        solver.addConstraint(
            ConstraintSpecs(
                groupCountSpec=GroupCountSpec(
                    name="at most one replica of a tenant in a region",
                    scope="region",
                    partitionName="tenant",
                    dimension="replica_count",
                    limit=Limit(globalLimit=1.0),
                )
            )
        )

        #  Constraint: Limit the tenant load at every region to 35
        solver.addConstraint(
            ConstraintSpecs(
                capacitySpec=CapacitySpec(
                    name="limit tenant load",
                    scope="region",
                    dimension="load",
                    definition=CapacitySpecDefinition.AFTER,
                    bound=CapacitySpecBound.MAX,
                    limit=Limit(type=LimitType.ABSOLUTE, globalLimit=35.0),
                ),
            ),
            invalidCost=1,
            invalidState=0,
        )

        solver.publishMetrics()

        solver.addSolver(
            SolverSpecs(
                localSearchSolverSpec=LocalSearchSolverSpec(
                    moveTypeList=[
                        MoveTypeSpec(singleMoveTypeSpec=SingleMoveTypeSpec()),
                        MoveTypeSpec(swapMoveTypeSpec=SwapMoveTypeSpec()),
                        MoveTypeSpec(tripleLoopMoveTypeSpec=TripleLoopMoveTypeSpec()),
                        MoveTypeSpec(klSearchMoveTypeSpec=KLSearchMoveTypeSpec()),
                    ],
                )
            )
        )

        solution = solver.solve()

        # For explanations, see the cpp test 'CapacityWithAbsoluteLimit' in /interface/tests/ObjectPartitionRoutingDimensionTest.cpp
        self.assertAlmostEqual(solution.initialObjective.value, 9.8)
        self.assertAlmostEqual(solution.finalObjective.value, 0.0)

        # verify utilizations of scopeItem and tenant->scopeItem traffic value. For both, just checking couple of values;
        # for full test see the cpp tests in /interface/tests/ObjectPartitionRoutingDimensionTest.cpp

        # verify that the initial utilization of region1 is 44.8 and its final utilization is 25.0.
        self.assertAlmostEqual(
            solution.initialMetrics.utilMetricToScopeUtils["after"]
            .scopeToDimensionUtils["region"]
            .dimensionToScopeItemUtils["load"]
            .scopeItemToValue["region1"],
            44.8,
        )
        self.assertAlmostEqual(
            solution.finalMetrics.utilMetricToScopeUtils["after"]
            .scopeToDimensionUtils["region"]
            .dimensionToScopeItemUtils["load"]
            .scopeItemToValue["region1"],
            25.0,
        )

        # verify that the initial total amount of traffic from region0 to region1 for tenant0 is 0.98,
        # and the final fraction of traffic for the same pair is 0.0.
        self.assertAlmostEqual(
            solution.initialMetrics.routingConfigToGroupTrafficMetrics["routingConfig1"]
            .groupToSourceTraffic["tenant0"]
            .sourceToDestinationTraffic["region0"]
            .scopeItemToValue["region1"],
            0.98,
        )
        self.assertAlmostEqual(
            solution.finalMetrics.routingConfigToGroupTrafficMetrics["routingConfig1"]
            .groupToSourceTraffic["tenant0"]
            .sourceToDestinationTraffic["region0"]
            .scopeItemToValue.get("region1", 0.0),
            0.0,
        )

    def test_object_partition_routing_dimension_test_with_static_values(self) -> None:
        # This is the same example as in the test 'CapacityWithAbsoluteLimitAndStaticValues' in /interface/tests/ObjectPartitionRoutingDimensionTest.cpp. For explanations, see the cpp test
        solver = ProblemSolver(service_name="rebalancer", service_scope="tests")
        solver.setObjectName("replica")
        solver.setContainerName("region")
        solver.shouldUseDynamicObjectOrdering(True)

        solver.setAssignment(
            {
                "region0": ["replica1:0"],
                "region1": ["replica0:0", "replica1:1"],
                "region2": ["replica0:1"],
            }
        )

        # define partition
        tenant_to_replicas: dict[str, list[str]] = {
            "tenant0": ["replica0:0", "replica0:1"],
            "tenant1": ["replica1:0", "replica1:1"],
        }
        solver.addPartition("tenant", tenant_to_replicas)

        # define routing config
        solver.addRoutingConfig(
            "routingConfig1",
            "region",
            "tenant",
            self.get_group_to_routing_rings(),
            # OriginToDestinationLatency
            {
                "region0": {"region0": 5, "region1": 10, "region2": 5},
                "region1": {"region0": 5, "region1": 0, "region2": 15},
                "region2": {"region0": 50, "region1": 40, "region2": 0},
            },
        )

        # define addObjectPartitionRoutingDimension
        solver.addObjectPartitionRoutingDimensionWithStaticValues(
            "load",
            "tenant",
            "routingConfig1",
            {"tenant0": 20, "tenant1": 50},
            {"tenant0": 5},
            0,
            3,
        )

        # Constraint: At most one replica of the same tenant can be placed in the same region
        solver.addConstraint(
            ConstraintSpecs(
                groupCountSpec=GroupCountSpec(
                    name="at most one replica of a tenant in a region",
                    scope="region",
                    partitionName="tenant",
                    dimension="replica_count",
                    limit=Limit(globalLimit=1.0),
                )
            )
        )

        #  Constraint: Limit the tenant load at every region to 35
        solver.addConstraint(
            ConstraintSpecs(
                capacitySpec=CapacitySpec(
                    name="limit tenant load",
                    scope="region",
                    dimension="load",
                    definition=CapacitySpecDefinition.AFTER,
                    bound=CapacitySpecBound.MAX,
                    limit=Limit(type=LimitType.ABSOLUTE, globalLimit=45.0),
                ),
            ),
            invalidCost=1,
            invalidState=0,
        )

        solver.publishMetrics()

        solver.addSolver(
            SolverSpecs(
                localSearchSolverSpec=LocalSearchSolverSpec(
                    moveTypeList=[
                        MoveTypeSpec(singleMoveTypeSpec=SingleMoveTypeSpec()),
                        MoveTypeSpec(swapMoveTypeSpec=SwapMoveTypeSpec()),
                        MoveTypeSpec(tripleLoopMoveTypeSpec=TripleLoopMoveTypeSpec()),
                        MoveTypeSpec(klSearchMoveTypeSpec=KLSearchMoveTypeSpec()),
                    ],
                )
            )
        )

        solution = solver.solve()

        # For explanations, see the cpp test 'CapacityWithAbsoluteLimit' in /interface/tests/ObjectPartitionRoutingDimensionTest.cpp
        self.assertAlmostEqual(solution.initialObjective.value, 7.8)
        self.assertAlmostEqual(solution.finalObjective.value, 0.0)

        # verify utilizations of scopeItem and tenant->scopeItem traffic value. For both, just checking couple of values;
        # for full test see the cpp tests in /interface/tests/ObjectPartitionRoutingDimensionTest.cpp

        # verify that the initial utilization of region1 is 44.8 and its final utilization is 25.0.
        self.assertAlmostEqual(
            solution.initialMetrics.utilMetricToScopeUtils["after"]
            .scopeToDimensionUtils["region"]
            .dimensionToScopeItemUtils["load"]
            .scopeItemToValue["region1"],
            52.8,
        )
        self.assertAlmostEqual(
            solution.finalMetrics.utilMetricToScopeUtils["after"]
            .scopeToDimensionUtils["region"]
            .dimensionToScopeItemUtils["load"]
            .scopeItemToValue["region1"],
            28.0,
        )

        # verify that the initial total amount of traffic from region0 to region1 for tenant0 is 0.98,
        # and the final fraction of traffic for the same pair is 0.0.
        self.assertAlmostEqual(
            solution.initialMetrics.routingConfigToGroupTrafficMetrics["routingConfig1"]
            .groupToSourceTraffic["tenant0"]
            .sourceToDestinationTraffic["region0"]
            .scopeItemToValue["region1"],
            0.98,
        )
        self.assertAlmostEqual(
            solution.finalMetrics.routingConfigToGroupTrafficMetrics["routingConfig1"]
            .groupToSourceTraffic["tenant0"]
            .sourceToDestinationTraffic["region0"]
            .scopeItemToValue.get("region1", 0.0),
            0.0,
        )
