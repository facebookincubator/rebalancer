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
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    ConstraintSpecs,
    GoalSpecs,
    ManifoldBackupParams,
    ManifoldUploadPolicy,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    BalanceSpec,
    BalanceSpecFormula,
    CapacitySpec,
    GroupCountSpec,
    Limit,
    LimitType,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
    MoveTypeSpec,
    SingleFastMoveTypeSpec,
)


RACK_COUNT = 10
HOSTS_PER_RACK = 10
HOST_COUNT: int = RACK_COUNT * HOSTS_PER_RACK

JOB_COUNT = 500
TASKS_PER_JOB = 4
TASK_COUNT: int = JOB_COUNT * TASKS_PER_JOB


def main() -> None:
    # Create an instance of the solver.
    solver = ProblemSolver(service_name="rebalancer", service_scope="examples")

    # Set basic properties.
    solver.setObjectName("task")
    solver.setContainerName("host")

    # Initial assignment: the first replica of every job is in host0, the second
    # replica is in host10, and so on. This assignment guarantees that different
    # tasks of the same job are in different racks initially.
    initialAssignment = {f"host{i}": [] for i in range(HOST_COUNT)}
    for i in range(JOB_COUNT):
        for j in range(TASKS_PER_JOB):
            initialAssignment[f"host{10 * j}"].append(f"job{i}/{j}")
    solver.setAssignment(initialAssignment)

    # Define rack scope: host0-9 are in rack0, host10-19 are in rack1, and so on.
    hostToRack = {}
    for i in range(HOST_COUNT):
        hostToRack[f"host{i}"] = f"rack{i // HOSTS_PER_RACK}"
    solver.addScope("rack", hostToRack)

    # Define job partition.
    jobToTasks = {
        f"job{i}": [f"job{i}/{j}" for j in range(TASKS_PER_JOB)]
        for i in range(JOB_COUNT)
    }
    solver.addPartition("job", jobToTasks)

    # Resource: compute
    # Each host has a capacity of 100 of compute
    hostToCompute = {f"host{i}": 100.0 for i in range(HOST_COUNT)}
    solver.addContainerDimension("compute", hostToCompute)
    # Each task requires 1 unit of compute
    taskToCompute = {
        f"job{i}/{j}": 1.0 for i in range(JOB_COUNT) for j in range(TASKS_PER_JOB)
    }
    solver.addObjectDimension("compute", taskToCompute)

    # Resource: storage
    # Each host has a capacity of 1000 units of storage.
    hostToStorage = {f"host{i}": 1000.0 for i in range(HOST_COUNT)}
    solver.addContainerDimension("storage", hostToStorage)
    # Half the tasks require 1 unit of storage each, the other half require 10
    # units each.
    taskToStorage = {
        f"job{i}/{j}": 1.0 if i < JOB_COUNT // 2 else 10.0
        for i in range(JOB_COUNT)
        for j in range(TASKS_PER_JOB)
    }
    solver.addObjectDimension("storage", taskToStorage)

    # Constraint: compute utilization on hosts may not exceed capacity.
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                scope="host",
                dimension="compute",
                name="host_compute_capacity",
            )
        )
    )

    # Constraint: storage utilization on hosts may not exceed capacity.
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                scope="host",
                dimension="storage",
                name="host_storage_capacity",
            )
        )
    )

    # Constraint: tasks of the same job must be placed in different racks.
    solver.addConstraint(
        ConstraintSpecs(
            groupCountSpec=GroupCountSpec(
                scope="rack",
                partitionName="job",
                dimension="task_count",
                limit=Limit(type=LimitType.ABSOLUTE, globalLimit=1),
                name="job_rack_failure_domain",
            )
        )
    )

    # Goal: balance compute utilization across hosts.
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                scope="host",
                dimension="compute",
                formula=BalanceSpecFormula.IDEAL,
                name="host_compute_balance",
            )
        )
    )

    # Goal: balance storage utilization across hosts.
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                scope="host",
                dimension="storage",
                formula=BalanceSpecFormula.IDEAL,
                name="host_storage_balance",
            )
        )
    )

    # Solver: use the local search solver with the SINGLE_FAST move type.
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    MoveTypeSpec(singleFastMoveTypeSpec=SingleFastMoveTypeSpec())
                ],
            )
        )
    )

    # Save problem instance in Manifold.
    solver.setManifoldBackupParams(
        ManifoldBackupParams(
            uploadPolicy=ManifoldUploadPolicy.ALWAYS,
        )
    )

    # Enable low-level logging.
    solver.setLogLevel("dbg2")

    # Generate a solution.
    solution = solver.solve()

    # Print instructions for running the Explorer.
    print("Run Rebalancer Explorer:")
    print(
        f"buck run rebalancer/explorer/cpp_server/server:server -- --run-id {solution.runId}"
    )


if __name__ == "__main__":
    main()
