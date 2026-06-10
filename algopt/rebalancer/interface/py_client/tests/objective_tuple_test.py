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
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    ConstraintSpecs,
    GoalSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    CapacitySpec,
    CapacitySpecDefinition,
    Limit,
    LimitType,
    ToFreeSpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import OptimalSolverSpec


class TestObjectiveTuples(BaseFacebookTestCase):
    def test_objective_tuples(self) -> None:
        # In this example there are 3 hosts and 7 tasks with the following initial
        # assignment:
        # host0: task0, task1, task2, task3
        # host1: task4, task5 task6
        # host2:
        # There's a capacity constraint with the AFTER definition, limiting the
        # capacity of each host to no more than 5 tasks.
        # There are two toFree goals: one to "toFree" host0 at tuple index 1 and
        # another to "toFree" host1 at tupleIndex 2. This implies that the first goal is more important than the second one.
        solver = ProblemSolver(service_name="rebalancer", service_scope="tests")
        solver.setObjectName("task")
        solver.setContainerName("host")
        solver.shouldUseDynamicObjectOrdering(True)

        # host0 and host1 start with 2 tasks each, host2 starts with a single task.
        solver.setAssignment(
            {
                "host0": ["task0", "task1", "task2", "task3"],
                "host1": ["task4", "task5", "task6"],
                "host2": [],
            }
        )

        solver.addGoalBoundary()

        # goal at tuple index 1 to drain host0; note that although we don't specify the tuplePos,
        # "addGoalBoundary()" adds the new goal at tuple index 1
        solver.addGoal(
            GoalSpecs(
                toFreeSpec=ToFreeSpec(
                    name="drain host0",
                    containers=["host0"],
                ),
            )
        )

        # goal at tuple index 2 is to drain host1;
        solver.addGoal(
            GoalSpecs(
                toFreeSpec=ToFreeSpec(
                    name="drain host1",
                    containers=["host1"],
                ),
            ),
            weight=1,
            tuplePos=2,
        )

        # Note: the dimension task_count is implicit and it's defined as 1 for every
        # task (object). Since we are using absolute limits, it is irrelevant how we
        # define the same dimension on the hosts (scope items).
        solver.addConstraint(
            ConstraintSpecs(
                capacitySpec=CapacitySpec(
                    name="at_most_5_tasks_per_host",
                    scope="host",
                    dimension="task_count",
                    definition=CapacitySpecDefinition.AFTER,
                    limit=Limit(type=LimitType.ABSOLUTE, globalLimit=5.0),
                )
            )
        )

        # use optimal solver
        solver.addSolver(SolverSpecs(optimalSolverSpec=OptimalSolverSpec()))

        # Get a solution from Rebalancer.
        solution = solver.solve()

        # expect three objectives, at tuple index 0 (w.r.t. broken constraints), 1 (w.r.t. toFree for host0),
        # and 2 (w.r.t. toFree for host1)
        self.assertEqual(len(solution.finalGlobalObjective().goals), 3)

        # We expect all the tasks in host0 to move to host2 due to the first priority goal,
        # and one of the tasks from host1 to move to host2 because of the second priority
        # goal and the capacity constraint
        ntasksInHost = {host: 0 for host in ["host0", "host1", "host2"]}
        for task in solution.assignment:
            host = solution.assignment[task]
            ntasksInHost[host] += 1

        self.assertEqual(ntasksInHost["host0"], 0)
        self.assertEqual(ntasksInHost["host1"], 2)
        self.assertEqual(ntasksInHost["host2"], 5)
