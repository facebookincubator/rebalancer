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
GreedyGroupToScopeItem Move Type Reference Examples

This file demonstrates all usage patterns shown in the reference documentation.
Each function is a complete, runnable example.
"""

# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    GoalSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import BalanceSpec
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    GreedyGroupToScopeItemMoveTypeSpec,
    LocalSearchSolverSpec,
)


def quick_example():
    """Quick example showing basic GreedyGroupToScopeItem usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # Define partition with jobs
    solver.addPartition("job", {"job0": ["task0", "task1", "task2"]})

    # Define scope with datacenters
    solver.addScope("datacenter", {"host1": "dc1", "host2": "dc1", "host3": "dc1"})

    # quick_example_start
    # Move entire job to one datacenter, each task on different host
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    GreedyGroupToScopeItemMoveTypeSpec(
                        groupMovesPartition="job",
                        scopeItemMovesScope="datacenter",
                    ),
                ]
            )
        )
    )
    # quick_example_end

    # Setup problem with job scattered across datacenters
    solver.setAssignment(
        {
            "host1": ["task0"],
            "host2": ["task1"],
            "host3": ["task2"],
        }
    )

    solver.addObjectDimension(
        "cpu",
        {
            "task0": 1.0,
            "task1": 1.0,
            "task2": 1.0,
        },
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(name="balance-cpu", scope="host", dimension="cpu")
        ),
        weight=1.0,
    )

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def replica_antiaffinity():
    """Place replicas on different machines within same rack."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("replica")
    solver.setContainerName("machine")

    # Define shard replicas
    solver.addPartition(
        "shard",
        {
            "shard1": ["shard1_primary", "shard1_replica1", "shard1_replica2"],
            "shard2": ["shard2_primary", "shard2_replica1", "shard2_replica2"],
        },
    )

    # Define rack scope
    solver.addScope(
        "rack",
        {
            "machine1": "rack1",
            "machine2": "rack1",
            "machine3": "rack1",
            "machine4": "rack2",
            "machine5": "rack2",
            "machine6": "rack2",
        },
    )

    # replica_antiaffinity_start
    # Place all replicas of a shard in same rack, but on different machines
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    GreedyGroupToScopeItemMoveTypeSpec(
                        groupMovesPartition="shard",
                        scopeItemMovesScope="rack",
                        nSampleSetsToExplore=2,
                    ),
                ]
            )
        )
    )
    # replica_antiaffinity_end

    # Setup with replicas scattered across racks
    solver.setAssignment(
        {
            "machine1": ["shard1_primary"],
            "machine2": ["shard2_primary"],
            "machine3": [],
            "machine4": ["shard1_replica1", "shard2_replica1"],
            "machine5": ["shard1_replica2"],
            "machine6": ["shard2_replica2"],
        }
    )

    solver.addObjectDimension(
        "size",
        {
            "shard1_primary": 1.0,
            "shard1_replica1": 1.0,
            "shard1_replica2": 1.0,
            "shard2_primary": 1.0,
            "shard2_replica1": 1.0,
            "shard2_replica2": 1.0,
        },
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-size", scope="machine", dimension="size"
            )
        ),
        weight=1.0,
    )

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def job_placement():
    """Keep job tasks together in one datacenter, spread across hosts."""
    # job_placement_start
    # Keep job tasks together in one datacenter, spread across hosts
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # Define jobs
    solver.addPartition(
        "job",
        {
            "webserver": ["web_task1", "web_task2", "web_task3"],
            "database": ["db_task1", "db_task2"],
        },
    )

    # Define datacenter scope
    solver.addScope(
        "datacenter",
        {
            "host1": "us-east",
            "host2": "us-east",
            "host3": "us-east",
            "host4": "us-west",
            "host5": "us-west",
            "host6": "us-west",
        },
    )

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    GreedyGroupToScopeItemMoveTypeSpec(
                        groupMovesPartition="job",
                        scopeItemMovesScope="datacenter",
                    ),
                ]
            )
        )
    )
    # job_placement_end


def higher_sampling():
    """Explore more container sets for better placement quality."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    solver.addPartition(
        "service", {"service1": ["instance1", "instance2", "instance3"]}
    )
    solver.addScope(
        "zone",
        {
            "node1": "zone1",
            "node2": "zone1",
            "node3": "zone1",
        },
    )

    # higher_sampling_start
    # Explore more container sets for better placement quality
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    GreedyGroupToScopeItemMoveTypeSpec(
                        groupMovesPartition="service",
                        scopeItemMovesScope="zone",
                        nSampleSetsToExplore=10,  # Higher sampling for better quality
                    ),
                ]
            )
        )
    )
    # higher_sampling_end


def main():
    print("=== GreedyGroupToScopeItem Move Type Examples ===\n")

    print("Running quick_example...")
    quick_example()
    print("✓ quick_example completed\n")

    print("Running replica_antiaffinity...")
    replica_antiaffinity()
    print("✓ replica_antiaffinity completed\n")


if __name__ == "__main__":
    main()
