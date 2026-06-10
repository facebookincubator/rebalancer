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

"""
ReplicaDrop Move Type Reference Examples

This file demonstrates all usage patterns shown in the reference documentation.
Each function is a complete, runnable example.
"""

# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    ConstraintSpecs,
    GoalSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    BalanceSpec,
    GroupCountSpec,
    GroupCountSpecBound,
    GroupLimitSpec,
    MinimizeSumSpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
    ReplicaDropMoveTypeSpec,
)


def quick_example():
    """Quick example showing basic ReplicaDrop usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # Define jobs with tasks
    solver.addPartition("job", {"job0": ["task0", "task1", "task2", "task3"]})

    # Define assigned scope
    solver.addScope(
        "assigned", {"host1": "assigned", "host2": "assigned", "host3": "assigned"}
    )

    # quick_example_start
    # Reduce job from 4 tasks to 2, dropping worst tasks
    solver.addConstraint(
        ConstraintSpecs(
            groupCountSpec=GroupCountSpec(
                scope="assigned",
                dimension="task_count",
                partitionName="job",
                bound=GroupCountSpecBound.EXACT,
                limit=GroupLimitSpec(defaultLimit=2),
            )
        )
    )

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    ReplicaDropMoveTypeSpec(
                        replicaDropPartition="job",
                        replicaDropScope="assigned",
                    ),
                ]
            )
        )
    )
    # quick_example_end

    # Setup over-replicated job
    solver.setAssignment(
        {
            "host1": ["task0", "task1"],
            "host2": ["task2"],
            "host3": ["task3"],
        }
    )

    # Add quality dimension (lower is better)
    solver.addObjectDimension(
        "lag",
        {
            "task0": 0.0,  # Best
            "task1": 0.01,
            "task2": 0.05,  # Worst - should be dropped
            "task3": 0.03,  # Second worst - should be dropped
        },
    )

    solver.addObjectDimension(
        "cpu",
        {
            "task0": 1.0,
            "task1": 1.0,
            "task2": 1.0,
            "task3": 1.0,
        },
    )

    # Minimize total lag (drops high-lag tasks)
    solver.addGoal(
        GoalSpecs(
            minimizeSumSpec=MinimizeSumSpec(
                name="minimize-lag", scope="assigned", dimension="lag"
            )
        ),
        weight=1.0,
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(name="balance-cpu", scope="host", dimension="cpu")
        ),
        weight=0.1,
    )

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def job_downsizing():
    """Downsize jobs to specific task counts, dropping highest-lag tasks."""
    # job_downsizing_start
    # Downsize jobs to specific task counts, dropping highest-lag tasks
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # Define jobs
    solver.addPartition(
        "job",
        {
            "job0": ["job0_task0", "job0_task1", "job0_task2", "job0_task3"],
            "job1": ["job1_task0", "job1_task1"],
        },
    )

    # Define assigned scope
    solver.addScope(
        "assigned",
        {
            "host1": "assigned",
            "host2": "assigned",
            "host3": "assigned",
            "host4": "assigned",
        },
    )

    # Add lag dimension to distinguish task quality
    solver.addObjectDimension(
        "lag",
        {
            "job0_task0": 0.0,
            "job0_task1": 0.01,
            "job0_task2": 0.03,
            "job0_task3": 0.02,
            "job1_task0": 0.06,
            "job1_task1": 0.0,
        },
    )

    solver.addConstraint(
        ConstraintSpecs(
            groupCountSpec=GroupCountSpec(
                scope="assigned",
                dimension="task_count",
                partitionName="job",
                bound=GroupCountSpecBound.EXACT,
                limit=GroupLimitSpec(groupLimits={"job0": 2, "job1": 1}),
            )
        )
    )

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    ReplicaDropMoveTypeSpec(
                        replicaDropPartition="job",
                        replicaDropScope="assigned",
                    ),
                ]
            )
        )
    )
    # job_downsizing_end


def shard_reduction():
    """Reduce shard replication from 5x to 3x."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("replica")
    solver.setContainerName("node")

    # Define shards with 5 replicas each
    solver.addPartition(
        "shard",
        {
            "shard1": ["shard1_r1", "shard1_r2", "shard1_r3", "shard1_r4", "shard1_r5"],
            "shard2": ["shard2_r1", "shard2_r2", "shard2_r3", "shard2_r4", "shard2_r5"],
        },
    )

    # Define active scope
    solver.addScope(
        "active",
        {
            "node1": "active",
            "node2": "active",
            "node3": "active",
            "node4": "active",
            "node5": "active",
        },
    )

    # shard_reduction_start
    # Reduce from 5 replicas to 3 replicas per shard
    solver.addConstraint(
        ConstraintSpecs(
            groupCountSpec=GroupCountSpec(
                scope="active",
                dimension="replica_count",
                partitionName="shard",
                bound=GroupCountSpecBound.EXACT,
                limit=GroupLimitSpec(defaultLimit=3),  # 3 replicas per shard
            )
        )
    )

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    ReplicaDropMoveTypeSpec(
                        replicaDropPartition="shard",
                        replicaDropScope="active",
                    ),
                ]
            )
        )
    )
    # shard_reduction_end

    # Setup over-replicated shards
    solver.setAssignment(
        {
            "node1": ["shard1_r1", "shard2_r1"],
            "node2": ["shard1_r2", "shard2_r2"],
            "node3": ["shard1_r3", "shard2_r3"],
            "node4": ["shard1_r4", "shard2_r4"],
            "node5": ["shard1_r5", "shard2_r5"],
        }
    )

    # Add quality dimension (staleness - lower is better)
    solver.addObjectDimension(
        "staleness",
        {
            "shard1_r1": 0.0,
            "shard1_r2": 0.0,
            "shard1_r3": 0.0,
            "shard1_r4": 10.0,  # Stale - should be dropped
            "shard1_r5": 15.0,  # Very stale - should be dropped
            "shard2_r1": 0.0,
            "shard2_r2": 0.0,
            "shard2_r3": 0.0,
            "shard2_r4": 8.0,  # Stale - should be dropped
            "shard2_r5": 12.0,  # Very stale - should be dropped
        },
    )

    solver.addObjectDimension(
        "size",
        {
            "shard1_r1": 1.0,
            "shard1_r2": 1.0,
            "shard1_r3": 1.0,
            "shard1_r4": 1.0,
            "shard1_r5": 1.0,
            "shard2_r1": 1.0,
            "shard2_r2": 1.0,
            "shard2_r3": 1.0,
            "shard2_r4": 1.0,
            "shard2_r5": 1.0,
        },
    )

    # Minimize staleness (keeps fresh replicas)
    solver.addGoal(
        GoalSpecs(
            minimizeSumSpec=MinimizeSumSpec(
                name="minimize-staleness", scope="active", dimension="staleness"
            )
        ),
        weight=1.0,
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(name="balance-size", scope="node", dimension="size")
        ),
        weight=0.1,
    )

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def quality_based():
    """Keep 2 freshest replicas, drop stale ones."""
    # quality_based_start
    # Keep 2 freshest replicas, drop stale ones
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("replica")
    solver.setContainerName("server")

    # Define replicas
    solver.addPartition(
        "data", {"dataset1": ["replica1", "replica2", "replica3", "replica4"]}
    )

    # Define online scope
    solver.addScope(
        "online",
        {
            "server1": "online",
            "server2": "online",
            "server3": "online",
        },
    )

    # Add staleness dimension (lower is better)
    solver.addObjectDimension(
        "staleness",
        {
            "replica1": 0.0,  # Fresh
            "replica2": 5.0,  # Slightly stale
            "replica3": 20.0,  # Very stale
            "replica4": 2.0,  # Mostly fresh
        },
    )

    solver.addConstraint(
        ConstraintSpecs(
            groupCountSpec=GroupCountSpec(
                scope="online",
                dimension="replica_count",
                partitionName="data",
                bound=GroupCountSpecBound.EXACT,
                limit=GroupLimitSpec(defaultLimit=2),
            )
        )
    )

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    ReplicaDropMoveTypeSpec(
                        replicaDropPartition="data",
                        replicaDropScope="online",
                    ),
                ]
            )
        )
    )
    # quality_based_end


def main():
    print("=== ReplicaDrop Move Type Examples ===\n")

    print("Running quick_example...")
    quick_example()
    print("✓ quick_example completed\n")

    print("Running shard_reduction...")
    shard_reduction()
    print("✓ shard_reduction completed\n")


if __name__ == "__main__":
    main()
