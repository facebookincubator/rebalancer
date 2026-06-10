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
from rebalancer.interface.thrift.Types.thrift_types import ConstraintParams
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    ConstraintSpecs,
    GoalSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    Filter,
    MaximizeAllocationSpec,
    ToFreeSpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    KLSearchMoveTypeSpec,
    LocalSearchSolverSpec,
    MoveTypeSpec,
    SingleMoveTypeSpec,
    SwapMoveTypeSpec,
    TripleLoopMoveTypeSpec,
)


# these tests are the same as the ones in algopt/rebalancer/interface/tests/BrokenConstraintTest.cpp
class TestBrokenConstraintPriorities(BaseFacebookTestCase):
    # pyre-fixme[3]: Return type must be annotated.
    def setup_problem(self, solver: ProblemSolver):
        # Create a simple problem with 4 hosts and 7 shards, where host h0 has 5
        # shards, host h1 has 2 shards. There are 2 constraints. First is to drain
        # h0, and the second is to drain h1. However, the second constraint is of
        # lesser priority than the first one, and in particular, if broken, the
        # broken component of the second constraint is added to goal tuple index 2
        # (instead of 0, which is the default). There is also goal at tuple index 1
        # to maximize the number of shards assigned to h3. The fact that it is at
        # goal tuple index 1 implies that minimizing this goal is considered to be
        # more important than "fixing" the second constraint.

        # Set the object and container names
        solver.setObjectName("shard")
        solver.setContainerName("host")

        # Set the initial assignment
        solver.setAssignment(
            {
                "h0": ["s1", "s2", "s3", "s4", "s5"],
                "h1": ["s6", "s7"],
                "h2": [],
                "h3": [],
            }
        )

        # add a "shard_weight" dimension to the problem
        solver.addObjectDimensionVector(
            dimensionName="shard_weight", objectToValues={}, defaultValue=1.0
        )

        # add a constraint to drain h0 this is a "default" type constraint,
        # which means it uses the default values for policy, invalidCost,
        # invalidState, and tuplePosIfBroken (or ones specified through ConstraintParams)
        solver.addConstraint(
            ConstraintSpecs(
                toFreeSpec=ToFreeSpec(
                    name="drain h0",
                    containers=["h0"],
                    dimension="shard_weight",
                )
            )
        )

        # add a constraint to drain h1 this uses the default values for
        # policy, invalidCost, and invalidState, but if broken the broken
        # component is added to goal tuple index 2
        solver.addConstraint(
            ConstraintSpecs(toFreeSpec=ToFreeSpec(name="drain h1", containers=["h1"])),
            tuplePosIfBroken=2,
        )

        # add a goal to maximize the number of shards in h3 this will
        # incentivize every object to move to h3
        # the goal is added to tupleIndex 1 with weight 1
        solver.addGoal(
            GoalSpecs(
                maximizeAllocationSpec=MaximizeAllocationSpec(
                    name="max shards in h3",
                    scope="host",
                    dimension="shard_count",
                    filter=Filter(
                        itemsWhitelist=["h3"],
                    ),
                ),
            ),
            weight=1,
            tuplePos=1,
        )
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

        return solver

    def test_basic_broken_constraint_priorities(self) -> None:
        solver = self.setup_problem(
            ProblemSolver(service_name="rebalancer", service_scope="tests")
        )
        solver.shouldUseDynamicObjectOrdering(True)
        solution = solver.solve()

        initialObjectives = solution.initialGlobalObjective.goals
        finalObjectives = solution.finalGlobalObjective.goals

        self.assertEqual(3, len(initialObjectives))
        self.assertEqual(3, len(finalObjectives))

        # expect initial value for goal at tupleIndex 0 (corresponding to the broken
        # 'toFreeH0' constraint) to be 10000 + 5*100 (i.e., violation of capacity
        # w.r.t. h0)
        initialObj1Value = initialObjectives[0].value
        self.assertEqual(initialObj1Value, 10500)
        # expect final value for goal at tupleIndex 0 to be 0
        finalObj1Value = finalObjectives[0].value
        self.assertEqual(finalObj1Value, 0)

        # expect initial value for goal at tupleIndex 1 to be 0 (as number of tasks
        # in h3 is 0)
        initialObj2Value = initialObjectives[1].value
        self.assertEqual(initialObj2Value, 0)
        # expect final value for goal at tupleIndex 1 to be -1 (i.e., all shards
        # should have moved to h3)
        finalObj2Value = finalObjectives[1].value
        self.assertEqual(finalObj2Value, -1)

        # expect initial value for goal at tupleIndex 2 to be 10000 + 2*100 (due to
        # broken 'toFreeH1' constraint)
        initialObj3Value = initialObjectives[2].value
        self.assertEqual(initialObj3Value, 10200)
        # expect final value for goal at tupleIndex 2 to be 0
        finalObj3Value = finalObjectives[2].value
        self.assertEqual(finalObj3Value, 0)

    def test_setting_constraintParams(self) -> None:
        # if a constraint is broken, then by default its broken component will be
        # added in tuple index 3
        solver = ProblemSolver(service_name="rebalancer", service_scope="tests")
        solver.shouldUseDynamicObjectOrdering(True)

        solver.setDefaultConstraintParams(
            ConstraintParams(
                invalidCost=10,
                tuplePosIfBroken=3,
            )
        )

        solver = self.setup_problem(solver)
        solver.shouldUseDynamicObjectOrdering(True)

        solution = solver.solve()

        initialObjectives = solution.initialGlobalObjective.goals
        finalObjectives = solution.finalGlobalObjective.goals

        self.assertEqual(4, len(initialObjectives))
        self.assertEqual(4, len(finalObjectives))

        # expect initial value and final value for goal at tupleIndex 0 since there
        # is no broken constraint or goal at this position
        initialObj1Value = initialObjectives[0].value
        self.assertEqual(initialObj1Value, 0)
        finalObj1Value = finalObjectives[0].value
        self.assertEqual(finalObj1Value, 0)

        # expect initial value for goal at tupleIndex 1 to be 0 (as number of tasks
        # in h3 is 0)
        initialObj2Value = initialObjectives[1].value
        self.assertEqual(initialObj2Value, 0)
        # expect final value for goal at tupleIndex 1 to be -1 (i.e., all shards
        # should have moved to h3)
        finalObj2Value = finalObjectives[1].value
        self.assertEqual(finalObj2Value, -1)

        # expect initial value for goal at tupleIndex 2 to be 10000 + 2*10 (due to
        # broken "toFree h1" constraint and invalidCost being 10)
        initialObj3Value = initialObjectives[2].value
        self.assertEqual(initialObj3Value, 10020)
        # expect final value for goal at tupleIndex 2 to be 0
        finalObj3Value = finalObjectives[2].value
        self.assertEqual(finalObj3Value, 0)

        # Note that by default broken constraints are added to tupleIndex 3. Since
        # 'toFreeH0' uses the default values, expect initial value for goal at
        # tupleIndex 3 (corresponding to the broken 'toFreeH0' constraint) to be
        # 10000 + 5*10 (i.e., violation of capacity w.r.t. h0)
        initialObj4Value = initialObjectives[3].value
        self.assertEqual(initialObj4Value, 10050)
        # expect final value for goal at tupleIndex 0 to be 0
        finalObj4Value = finalObjectives[3].value
        self.assertEqual(finalObj4Value, 0)
