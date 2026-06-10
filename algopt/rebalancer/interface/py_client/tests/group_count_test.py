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

from collections import Counter, defaultdict

from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from libfb.py.testutil import BaseFacebookTestCase
from rebalancer.interface.thrift.Types.thrift_types import AssignmentSolution
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    ConstraintSpecs,
    GoalSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    CapacitySpec,
    CapacitySpecBound,
    GroupCountSpec,
    GroupCountSpecBound,
    GroupCountSpecLimitRelativeTo,
    Limit,
    LimitType,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
    MoveTypeSpec,
    SingleMoveTypeSpec,
)


# This test is a Python translation of the C++ test at
# #algopt/rebalancer/interface/tests/GroupCountTest.cpp
class TestGroupCountSpec(BaseFacebookTestCase):
    def setUp(self) -> None:
        super().setUp()

        self.task_to_job: dict[str, str] = {
            "task0": "job0",
            "task1": "job0",
            "task2": "job0",
            "task3": "job0",
            "task4": "job0",
            "task5": "job1",
            "task6": "job1",
            "task7": "job1",
            "task8": "job1",
        }

        self.solver = ProblemSolver(service_name="rebalancer", service_scope="tests")
        self.solver.setObjectName("task")
        self.solver.setContainerName("host")
        self.solver.shouldUseDynamicObjectOrdering(True)

        # host0 has the first 5 tasks, host1 has the remaining 4 tasks.
        self.solver.setAssignment(
            {
                "host0": ["task0", "task1", "task2", "task3", "task4"],
                "host1": ["task5", "task6", "task7", "task8"],
            }
        )

        # Let Rebalancer know of the job partition, so we can later refer to it.
        job_to_tasks: dict[str, list[str]] = defaultdict(list)
        for task, job in self.task_to_job.items():
            job_to_tasks[job].append(task)

        self.solver.addPartition("job", job_to_tasks)

        # For illustration purposes, make local search's behavior predictable and
        # simple by allowing single moves only.
        self.solver.addSolver(
            SolverSpecs(
                localSearchSolverSpec=LocalSearchSolverSpec(
                    moveTypeList=[
                        MoveTypeSpec(singleMoveTypeSpec=SingleMoveTypeSpec()),
                    ]
                )
            )
        )

    def get_host_to_job_count(
        self, solution: AssignmentSolution
    ) -> defaultdict[str, Counter[str]]:
        # Count how many tasks from each job are in each host.
        host_to_job_count: defaultdict[str, Counter[str]] = defaultdict(
            lambda: Counter()
        )

        for task, host in solution.assignment.items():
            job = self.task_to_job[task]
            host_to_job_count[host][job] += 1

        return host_to_job_count

    def test_global_absolute_limit(self) -> None:
        # At most 3 tasks of the same group can be placed in the same scope item.
        self.solver.addConstraint(
            ConstraintSpecs(
                groupCountSpec=GroupCountSpec(
                    name="at_most_3_hosts_from_same_group",
                    scope="host",
                    partitionName="job",
                    dimension="task_count",
                    limit=Limit(type=LimitType.ABSOLUTE, globalLimit=3),
                )
            )
        )

        # Get a solution from Rebalancer.
        solution = self.solver.solve()

        # Count how many tasks from each job are in each host.
        host_to_job_count = self.get_host_to_job_count(solution)

        # job0 initially had 5 tasks in host0, while the limit is 3: we expect
        # Rebalancer to have moved 2 tasks of job0 from host0 to host1.
        self.assertEqual(3, host_to_job_count["host0"]["job0"])
        self.assertEqual(2, host_to_job_count["host1"]["job0"])

        # On the other hand, job1 initially had 4 tasks in host1, while the limit
        # for it is also 3: we expect Rebalancer to have moved 1 task of job1 from
        # host1 to host0.
        self.assertEqual(1, host_to_job_count["host0"]["job1"])
        self.assertEqual(3, host_to_job_count["host1"]["job1"])

    def test_global_relative_limit(self) -> None:
        # At most 70% of tasks of the same group can be placed in the same scope
        # item.
        self.solver.addConstraint(
            ConstraintSpecs(
                groupCountSpec=GroupCountSpec(
                    name="at_most_70_pct_same_group",
                    scope="host",
                    partitionName="job",
                    dimension="task_count",
                    limit=Limit(type=LimitType.RELATIVE, globalLimit=0.7),
                )
            )
        )

        # Get a solution from Rebalancer.
        solution = self.solver.solve()

        # Count how many tasks from each job are in each host.
        host_to_job_count = self.get_host_to_job_count(solution)

        # job0 initially had 5 tasks in host0, while the limit is 70% of the job's
        # tasks. In other words no more than 3 of the 5 tasks of job0 can be placed
        # in the same host. We expect Rebalancer to have moved 2 tasks of job0 from
        # host0 to host1.
        self.assertEqual(3, host_to_job_count["host0"]["job0"])
        self.assertEqual(2, host_to_job_count["host1"]["job0"])

        # On the other hand, job1 initially had 4 tasks in host1, while the limit
        # is 70% of the job's tasks. In other words, no more than 2 of the 4 tasks
        # of job1 can be placed in the same host. We expect Rebalancer to have moved
        # 2 tasks of job1 from host1 to host0.
        self.assertEqual(2, host_to_job_count["host0"]["job1"])
        self.assertEqual(2, host_to_job_count["host1"]["job1"])

    def test_per_scope_item_absolute_limit(self) -> None:
        # At most 3 tasks of the same job can be placed in host0. The remaining hosts
        # accept up to 5 tasks of the same job. Note that scope items that are not
        # explicitly listed in the limit spec fall back to using the global limit.
        self.solver.addConstraint(
            ConstraintSpecs(
                groupCountSpec=GroupCountSpec(
                    name="at_most_3_tasks_of_same_job",
                    scope="host",
                    partitionName="job",
                    dimension="task_count",
                    limit=Limit(
                        type=LimitType.ABSOLUTE,
                        globalLimit=5,
                        scopeItemLimits={"host0": 3},
                    ),
                )
            )
        )

        # Get a solution from Rebalancer.
        solution = self.solver.solve()

        # Count how many tasks from each job are in each host.
        host_to_job_count = self.get_host_to_job_count(solution)

        # host0 contains 5 tasks of the same job, but the limit is 3. We expect
        # Rebalancer to move 2 tasks of job0 from host0 to host1.
        self.assertEqual(3, host_to_job_count["host0"]["job0"])
        self.assertEqual(2, host_to_job_count["host1"]["job0"])

        # On the other hand, host1 initially has only 4 tasks of the same job. Since
        # the limit for host1 is 5, there's no incentive to move any tasks from host1
        # to host0. We expect Rebalancer to not move any tasks of job1.
        self.assertEqual(0, host_to_job_count["host0"]["job1"])
        self.assertEqual(4, host_to_job_count["host1"]["job1"])

    def test_per_scope_item_relative_limit(self) -> None:
        # host1 doesn't accept more than 50% of tasks of the same job, while host0
        # doesn't have any restrictions. By making globalLimit=1 we are saying that
        # any hosts not explicitly listed may accept 100% of tasks of the same job.
        self.solver.addConstraint(
            ConstraintSpecs(
                groupCountSpec=GroupCountSpec(
                    name="host1_max_50_pct",
                    scope="host",
                    partitionName="job",
                    dimension="task_count",
                    limit=Limit(
                        type=LimitType.RELATIVE,
                        globalLimit=1.0,
                        scopeItemLimits={"host1": 0.5},
                    ),
                )
            )
        )

        # Get a solution from Rebalancer.
        solution = self.solver.solve()

        # Count how many tasks from each job are in each host.
        host_to_job_count = self.get_host_to_job_count(solution)

        # host0 has virtually no limits on how many tasks of the same job it can
        # accept. We expect rebalancer to not move any of job0's tasks.
        self.assertEqual(5, host_to_job_count["host0"]["job0"])
        self.assertEqual(0, host_to_job_count["host1"]["job0"])

        # On the other hand, host1 can't accept more than 50% of a job's tasks. Since
        # all 4 tasks of job1 are initially placed in host1, we expect Rebalancer to
        # move half of them from host1 to host0.
        self.assertEqual(2, host_to_job_count["host0"]["job1"])
        self.assertEqual(2, host_to_job_count["host1"]["job1"])

    def test_per_group_absolute_limit(self) -> None:
        # job1 may have up to 3 of its task placed in the same host. Any other job
        # may have up to 4 tasks placed in the same host.
        self.solver.addConstraint(
            ConstraintSpecs(
                groupCountSpec=GroupCountSpec(
                    name="job_counts",
                    scope="host",
                    partitionName="job",
                    dimension="task_count",
                    limit=Limit(
                        type=LimitType.ABSOLUTE,
                        globalLimit=4,
                        groupLimits={"job1": 3},
                    ),
                )
            )
        )

        # Get a solution from Rebalancer.
        solution = self.solver.solve()

        # Count how many tasks from each job are in each host.
        host_to_job_count = self.get_host_to_job_count(solution)

        # job0 has 5 tasks initially in host0. Since the limit for job0 is 4, we
        # expect Rebalancer to move 1 task from host0 to host1.
        self.assertEqual(4, host_to_job_count["host0"]["job0"])
        self.assertEqual(1, host_to_job_count["host1"]["job0"])

        # job1 has 4 tasks initially in host1. Since the limit for job1 is 3, we
        # expect Rebalancer to move 1 task from host1 to host0.
        self.assertEqual(1, host_to_job_count["host0"]["job1"])
        self.assertEqual(3, host_to_job_count["host1"]["job1"])

    def test_per_group_relative_limit(self) -> None:
        # job0 may have up to 60% of its tasks placed in the same host. Since job0
        # has 5 tasks, that's equivalent to saying at most 3 tasks can be in the
        # same host. On the other hand, job1 may have up to 50% of its tasks placed
        # in the same host, and since it has 4 tasks, that's equivalent to a limit of
        # 2 tasks per host. Other jobs, if there were any, would have virtually no
        # limits since we are setting the global limit to 1, which means that jobs
        # not listed explicitly may have up to 100% of its task on the same host.
        self.solver.addConstraint(
            ConstraintSpecs(
                groupCountSpec=GroupCountSpec(
                    name="job0_60_pct_on_one_host",
                    scope="host",
                    partitionName="job",
                    dimension="task_count",
                    limit=Limit(
                        type=LimitType.RELATIVE,
                        globalLimit=1.0,
                        groupLimits={"job0": 0.6, "job1": 0.5},
                    ),
                )
            )
        )

        # Get a solution from Rebalancer.
        solution = self.solver.solve()

        # Count how many tasks from each job are in each host.
        host_to_job_count = self.get_host_to_job_count(solution)

        # We expect Rebalancer to move 2 tasks of job0 from host0 to host1.
        self.assertEqual(3, host_to_job_count["host0"]["job0"])
        self.assertEqual(2, host_to_job_count["host1"]["job0"])

        # We expect Rebalancer to move 2 tasks of job1 from host1 to host0.
        self.assertEqual(2, host_to_job_count["host0"]["job1"])
        self.assertEqual(2, host_to_job_count["host1"]["job1"])

    def test_per_scope_item_and_group_absolute_limit(self) -> None:
        # job0 may have up to 2 tasks placed in host0 and up to 4 tasks placed in
        # host1. job1 may have up to 1 task placed in host0. Any other combination
        # has a limit of 5.
        self.solver.addConstraint(
            ConstraintSpecs(
                groupCountSpec=GroupCountSpec(
                    name="job_placement",
                    scope="host",
                    partitionName="job",
                    dimension="task_count",
                    limit=Limit(
                        type=LimitType.ABSOLUTE,
                        globalLimit=5,
                        scopeItemToGroupLimits={
                            "host0": {"job0": 2, "job1": 1},
                            "host1": {"job0": 4},
                        },
                    ),
                )
            )
        )

        # Get a solution from Rebalancer.
        solution = self.solver.solve()

        # Count how many tasks from each job are in each host.
        host_to_job_count = self.get_host_to_job_count(solution)

        # Because job0 can only have up to 2 tasks placed in host0, while it can
        # have up to 4 in host1, we expect Rebalancer to move 3 of its 5 tasks from
        # host0 to host1.
        self.assertEqual(2, host_to_job_count["host0"]["job0"])
        self.assertEqual(3, host_to_job_count["host1"]["job0"])

        # Because job1 can have up to 5 tasks in host1, there's no incentive to move
        # any of its tasks, and we expect Rebalancer to keep them all in host1.
        self.assertEqual(0, host_to_job_count["host0"]["job1"])
        self.assertEqual(4, host_to_job_count["host1"]["job1"])

    def test_per_scope_item_and_group_relative_limit(self) -> None:
        # Any job may have up to 80% of its tasks placed in the same host, except
        # job0 which may only place up to 50% of its tasks in host0.
        self.solver.addConstraint(
            ConstraintSpecs(
                groupCountSpec=GroupCountSpec(
                    name="80_pct_on_same_host",
                    scope="host",
                    partitionName="job",
                    dimension="task_count",
                    limit=Limit(
                        type=LimitType.RELATIVE,
                        globalLimit=0.8,
                        scopeItemToGroupLimits={"host0": {"job0": 0.5}},
                    ),
                )
            )
        )

        # Get a solution from Rebalancer.
        solution = self.solver.solve()

        # Count how many tasks from each job are in each host.
        host_to_job_count = self.get_host_to_job_count(solution)

        # Because job0 may only have up to 50% of its tasks placed in host0, we
        # expect Rebalancer to move 3 of its 5 tasks from host0 to host1.
        self.assertEqual(2, host_to_job_count["host0"]["job0"])
        self.assertEqual(3, host_to_job_count["host1"]["job0"])

        # Because job1 can only have up to 80% of its tasks in host1, we expect
        # Rebalancer to move 1 of its 4 tasks from host1 to host0.
        self.assertEqual(1, host_to_job_count["host0"]["job1"])
        self.assertEqual(3, host_to_job_count["host1"]["job1"])

    def test_min_bound(self) -> None:
        # Each job must have at least 1 task placed in every host.
        self.solver.addConstraint(
            ConstraintSpecs(
                groupCountSpec=GroupCountSpec(
                    name="at_least_1_task_per_host",
                    scope="host",
                    partitionName="job",
                    dimension="task_count",
                    bound=GroupCountSpecBound.MIN,
                    limit=Limit(type=LimitType.ABSOLUTE, globalLimit=1),
                )
            )
        )

        # Get a solution from Rebalancer.
        solution = self.solver.solve()

        # Count how many tasks from each job are in each host.
        host_to_job_count = self.get_host_to_job_count(solution)

        # Because job0 doesn't have any tasks in host1, Rebalancer must move at
        # least 1 task of job0 from host0 to host1.
        self.assertEqual(4, host_to_job_count["host0"]["job0"])
        self.assertEqual(1, host_to_job_count["host1"]["job0"])

        # Because job1 doesn't have any tasks in host0, Rebalancer must move at
        # least 1 task of job1 from host1 to host0.
        self.assertEqual(1, host_to_job_count["host0"]["job1"])
        self.assertEqual(3, host_to_job_count["host1"]["job1"])

    def test_squares_objective(self) -> None:
        # By setting the upper limit to 0 tasks per host and enabling squares, we are
        # essentially minimizing the sum of squares of: the number of tasks of job X
        # in host Y, for every combination of job X and host Y. That's a reasonable
        # definition of a balance objective: minimizing it will spread out the tasks
        # of each job as evenly as possible among hosts.
        self.solver.addGoal(
            GoalSpecs(
                groupCountSpec=GroupCountSpec(
                    name="balance_jobs_on_hosts",
                    scope="host",
                    partitionName="job",
                    dimension="task_count",
                    squares=True,
                    limit=Limit(type=LimitType.ABSOLUTE, globalLimit=0),
                )
            )
        )

        # Get a solution from Rebalancer.
        solution = self.solver.solve()

        # Count how many tasks from each job are in each host.
        host_to_job_count = self.get_host_to_job_count(solution)

        # job0 has an odd number of tasks, so they can't be perfectly balanced among
        # the 2 hosts. The best balance attainable, however, is obtained when
        # Rebalancer moves 2 tasks of job0 from host0 to host1.
        self.assertEqual(3, host_to_job_count["host0"]["job0"])
        self.assertEqual(2, host_to_job_count["host1"]["job0"])

        # Because job1 has even number of tasks, it can be balanced perfectly among
        # the 2 hosts. The best balance is obtained when Rebalance moves 2 tasks of
        # job1 from host1 to host0.
        self.assertEqual(2, host_to_job_count["host0"]["job1"])
        self.assertEqual(2, host_to_job_count["host1"]["job1"])

    def test_limit_relative_to_scope_item_util(self) -> None:
        # By setting a limit relative to the scope item util, we add the constraint
        # that job0 may not account for more than 50% of the utilization of host0.
        self.solver.addConstraint(
            ConstraintSpecs(
                groupCountSpec=GroupCountSpec(
                    name="job0_not_more_than_50%_of_host0",
                    scope="host",
                    partitionName="job",
                    dimension="task_count",
                    limit=Limit(
                        type=LimitType.RELATIVE,
                        globalLimit=1,
                        scopeItemToGroupLimits={"host0": {"job0": 0.5}},
                    ),
                    limitRelativeTo=GroupCountSpecLimitRelativeTo.SCOPE_ITEM_UTIL,
                )
            )
        )

        # Prevent the trivial solution where host0 simply becomes empty by imposing
        # that hosts must contain at least one task.
        self.solver.addConstraint(
            ConstraintSpecs(
                capacitySpec=CapacitySpec(
                    name="hosts_must_not_be_empty",
                    scope="host",
                    dimension="task_count",
                    bound=CapacitySpecBound.MIN,
                    limit=Limit(type=LimitType.ABSOLUTE, globalLimit=1),
                )
            )
        )

        # Get a solution from Rebalancer.
        solution = self.solver.solve()

        # Count how many tasks from each job are in each host.
        host_to_job_count = self.get_host_to_job_count(solution)

        # In the solution Rebalancer generates, job0 accounts for exactly 50% of the
        # utilization of host0, which is valid. host0 has 2 tasks, one of job0, and
        # the other of job1. All other tasks are in host1.
        self.assertEqual(1, host_to_job_count["host0"]["job0"])
        self.assertEqual(4, host_to_job_count["host1"]["job0"])
        self.assertEqual(1, host_to_job_count["host0"]["job1"])
        self.assertEqual(3, host_to_job_count["host1"]["job1"])
