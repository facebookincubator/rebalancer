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
ColocateGroups Move Type Reference Examples

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
    ColocateGroupsMoveTypeRelatedGroupsInfo,
    ColocateGroupsMoveTypeSpec,
    LocalSearchSolverSpec,
)


def quick_example():
    """Quick example showing basic ColocateGroups usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("server")

    # Define partition with replica groups
    solver.addPartition(
        "replica_group",
        {
            "primary": ["shard1_p", "shard2_p"],
            "replica1": ["shard1_r1", "shard2_r1"],
            "replica2": ["shard1_r2", "shard2_r2"],
        },
    )

    # Define region scope
    solver.addScope(
        "region",
        {
            "server1": "us-east",
            "server2": "us-east",
            "server3": "us-west",
            "server4": "us-west",
        },
    )

    # quick_example_start
    # Colocate primary and replicas in same region
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    ColocateGroupsMoveTypeSpec(
                        partitionName="replica_group",
                        relatedGroupsList=[
                            ColocateGroupsMoveTypeRelatedGroupsInfo(
                                relatedGroups={"primary", "replica1", "replica2"},
                            ),
                        ],
                        colocationScopeName="region",
                    ),
                ]
            )
        )
    )
    # quick_example_end

    # Setup problem with replicas scattered across regions
    solver.setAssignment(
        {
            "server1": ["shard1_p", "shard2_r2"],
            "server2": ["shard1_r2"],
            "server3": ["shard1_r1", "shard2_p"],
            "server4": ["shard2_r1"],
        }
    )

    solver.addObjectDimension(
        "size",
        {
            "shard1_p": 1.0,
            "shard1_r1": 1.0,
            "shard1_r2": 1.0,
            "shard2_p": 1.0,
            "shard2_r1": 1.0,
            "shard2_r2": 1.0,
        },
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-size", scope="server", dimension="size"
            )
        ),
        weight=1.0,
    )

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def replica_colocation():
    """Colocate primary and replicas in same region."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("server")

    # Define replica groups
    solver.addPartition(
        "replica_type",
        {
            "primary": ["shard1_primary", "shard2_primary"],
            "replica_1": ["shard1_replica1", "shard2_replica1"],
            "replica_2": ["shard1_replica2", "shard2_replica2"],
        },
    )

    # Define region scope
    solver.addScope(
        "region",
        {
            "server1": "region_a",
            "server2": "region_a",
            "server3": "region_b",
            "server4": "region_b",
        },
    )

    # replica_colocation_start
    # Ensure all replicas of a shard are in the same region
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    ColocateGroupsMoveTypeSpec(
                        partitionName="replica_type",
                        relatedGroupsList=[
                            ColocateGroupsMoveTypeRelatedGroupsInfo(
                                relatedGroups={"primary", "replica_1", "replica_2"},
                            ),
                        ],
                        colocationScopeName="region",
                    ),
                ]
            )
        )
    )
    # replica_colocation_end

    # Setup with misaligned replicas
    solver.setAssignment(
        {
            "server1": ["shard1_primary"],
            "server2": ["shard2_replica1"],
            "server3": ["shard1_replica1", "shard2_primary"],
            "server4": ["shard1_replica2", "shard2_replica2"],
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "shard1_primary": 2.0,
            "shard1_replica1": 2.0,
            "shard1_replica2": 2.0,
            "shard2_primary": 2.0,
            "shard2_replica1": 2.0,
            "shard2_replica2": 2.0,
        },
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-memory", scope="server", dimension="memory"
            )
        ),
        weight=1.0,
    )

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def sampling():
    """Use sampling to limit complexity."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    solver.addPartition(
        "replica_group",
        {
            "primary": ["obj1"],
            "replica1": ["obj2"],
            "replica2": ["obj3"],
        },
    )

    # sampling_start
    # Use sampling to limit complexity
    from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
        ColocateGroupsMoveTypeRelatedGroupsInfo,
    )

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    ColocateGroupsMoveTypeSpec(
                        partitionName="replica_group",
                        relatedGroupsList=[
                            ColocateGroupsMoveTypeRelatedGroupsInfo(
                                relatedGroups=["primary", "replica1", "replica2"]
                            )
                        ],
                        colocationScopeName="region",
                        defaultSampleSize=5,  # Sample 5 containers per group
                    ),
                ]
            )
        )
    )
    # sampling_end


def restricted():
    """Restrict which containers each group can use."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    solver.addPartition(
        "replica_group",
        {
            "primary": ["obj1"],
            "replica": ["obj2"],
        },
    )

    # restricted_start
    # Restrict which containers each group can use
    from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
        ColocateGroupsMoveTypeRelatedGroupsInfo,
    )

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    ColocateGroupsMoveTypeSpec(
                        partitionName="replica_group",
                        relatedGroupsList=[
                            ColocateGroupsMoveTypeRelatedGroupsInfo(
                                relatedGroups=["primary", "replica"]
                            )
                        ],
                        colocationScopeName="region",
                        colocationScopeItemToGroupToContainers={
                            "region1": {
                                "primary": {"server1", "server2"},
                                "replica": {"server3", "server4"},
                            },
                            "region2": {
                                "primary": {"server5", "server6"},
                                "replica": {"server7", "server8"},
                            },
                        },
                    ),
                ]
            )
        )
    )
    # restricted_end


def multiple_sets():
    """Multiple independent related group sets."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    solver.addPartition(
        "service_type",
        {
            "web": ["web1"],
            "web_cache": ["cache1"],
            "db": ["db1"],
            "db_replica": ["db2"],
        },
    )

    # multiple_sets_start
    # Multiple independent related group sets
    from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
        ColocateGroupsMoveTypeRelatedGroupsInfo,
    )

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    ColocateGroupsMoveTypeSpec(
                        partitionName="service_type",
                        relatedGroupsList=[
                            ColocateGroupsMoveTypeRelatedGroupsInfo(
                                relatedGroups=["web", "web_cache"]  # Web tier together
                            ),
                            ColocateGroupsMoveTypeRelatedGroupsInfo(
                                relatedGroups=["db", "db_replica"]  # DB tier together
                            ),
                        ],
                        colocationScopeName="datacenter",
                    ),
                ]
            )
        )
    )
    # multiple_sets_end


def main():
    print("=== ColocateGroups Move Type Examples ===\n")

    print("Running quick_example...")
    quick_example()
    print("✓ quick_example completed\n")

    print("Running replica_colocation...")
    replica_colocation()
    print("✓ replica_colocation completed\n")


if __name__ == "__main__":
    main()
