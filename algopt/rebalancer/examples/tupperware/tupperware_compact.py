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
    ManifoldBackupParams,
    ManifoldUploadPolicy,
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


def main() -> None:
    # Create an instance of the solver.
    solver = ProblemSolver(service_name="rebalancer", service_scope="examples")

    # Set basic properties.
    solver.setObjectName("task")
    solver.setContainerName("host")

    # Initial compact assignment: each job has 1 instance on each of host0,
    # host10, host20, host30 (one per rack, matching the original spread of
    # individual tasks).
    initialAssignment: dict[str, dict[str, int]] = {
        f"host{i}": {} for i in range(HOST_COUNT)
    }
    for i in range(JOB_COUNT):
        for j in range(TASKS_PER_JOB):
            initialAssignment[f"host{10 * j}"][f"job{i}"] = 1

    solver.setCompactAssignment(initialAssignment)

    # Define rack scope: host0-9 are in rack0, host10-19 are in rack1, etc.
    hostToRack = {f"host{i}": f"rack{i // HOSTS_PER_RACK}" for i in range(HOST_COUNT)}
    solver.addScope("rack", hostToRack)

    # Define job partition: each object IS its own group (since objects are
    # jobs, not individual tasks).
    solver.addPartition("job", {f"job{i}": [f"job{i}"] for i in range(JOB_COUNT)})

    # Resource: compute
    # Each host has a capacity of 100 compute units.
    hostToCompute = {f"host{i}": 100.0 for i in range(HOST_COUNT)}
    solver.addContainerDimension("compute", hostToCompute)
    # Each job uses 1 compute unit per instance.
    jobToCompute = {f"job{i}": 1.0 for i in range(JOB_COUNT)}
    solver.addObjectDimension("compute", jobToCompute)

    # Resource: storage
    # Each host has a capacity of 1000 storage units.
    hostToStorage = {f"host{i}": 1000.0 for i in range(HOST_COUNT)}
    solver.addContainerDimension("storage", hostToStorage)
    # Half the jobs require 1 unit of storage each, the other half require 10
    # units each.
    jobToStorage = {
        f"job{i}": 1.0 if i < JOB_COUNT // 2 else 10.0 for i in range(JOB_COUNT)
    }
    solver.addObjectDimension("storage", jobToStorage)

    # Constraint: compute utilization on hosts may not exceed capacity.
    solver.addConstraint(
        CapacitySpec(
            scope="host",
            dimension="compute",
            name="host_compute_capacity",
        )
    )

    # Constraint: storage utilization on hosts may not exceed capacity.
    solver.addConstraint(
        CapacitySpec(
            scope="host",
            dimension="storage",
            name="host_storage_capacity",
        )
    )

    # Constraint: each job has at most 1 instance per rack.
    solver.addConstraint(
        GroupCountSpec(
            scope="rack",
            partitionName="job",
            dimension="compute",
            limit=Limit(type=LimitType.ABSOLUTE, globalLimit=1),
            name="job_rack_failure_domain",
        )
    )

    # Goal: balance compute utilization across hosts.
    solver.addGoal(
        BalanceSpec(
            scope="host",
            dimension="compute",
            formula=BalanceSpecFormula.IDEAL,
            name="host_compute_balance",
        )
    )

    # Goal: balance storage utilization across hosts.
    solver.addGoal(
        BalanceSpec(
            scope="host",
            dimension="storage",
            formula=BalanceSpecFormula.IDEAL,
            name="host_storage_balance",
        )
    )

    # Solver: use the local search solver with the SINGLE_FAST move type.
    solver.addSolver(
        LocalSearchSolverSpec(
            moveTypeList=[
                MoveTypeSpec(singleFastMoveTypeSpec=SingleFastMoveTypeSpec())
            ],
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

    # Print results.
    if solution.compactAssignment is not None:
        compact = solution.compactAssignment.objectsPerContainer
        print(f"Containers in compact solution: {len(compact)}")
        # Print sample: first 3 containers.
        for container in sorted(compact.keys())[:3]:
            objects = compact[container]
            print(f"  {container}: {dict(sorted(objects.items())[:5])}")

    # Print instructions for running the Explorer.
    print("Run Rebalancer Explorer:")
    print(
        f"buck run rebalancer/explorer/cpp_server/server:server -- --run-id {solution.runId}"
    )


if __name__ == "__main__":
    main()
