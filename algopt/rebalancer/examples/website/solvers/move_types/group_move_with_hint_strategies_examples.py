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
GroupMoveWithHintStrategies Move Type Reference Examples

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
    GroupMoveWithHintStrategiesMoveTypeSpec,
    LocalSearchSolverSpec,
    MoveStrategies,
    MoveStrategy,
    MoveStrategyType,
    MoveToScopeItemsSpec,
    ScopeItemList,
)


def quick_example():
    """Quick example showing basic GroupMoveWithHintStrategies usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("rank")

    # Define partitions
    solver.addPartition("table", {"table1": ["shard1", "shard2"]})
    solver.addPartition(
        "shard_type", {"row_wise": ["shard1"], "column_wise": ["shard2"]}
    )

    # quick_example_start
    # Apply different strategies for different shard types
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    GroupMoveWithHintStrategiesMoveTypeSpec(
                        primaryPartition="table",
                        secondaryPartition="shard_type",
                        moveStrategies=MoveStrategies(
                            groupToMoveStrategy={
                                "row_wise": MoveStrategy(
                                    type=MoveStrategyType.RANDOM_SAMPLING_WITH_REPLACEMENT,
                                    moveToScopeItems=MoveToScopeItemsSpec(
                                        defaultScopeItems=ScopeItemList(
                                            itemList=["node1", "node2"]
                                        ),
                                    ),
                                ),
                                "column_wise": MoveStrategy(
                                    type=MoveStrategyType.RANDOM_SAMPLING_WITHOUT_REPLACEMENT,
                                    moveToScopeItems=MoveToScopeItemsSpec(
                                        defaultScopeItems=ScopeItemList(
                                            itemList=["node1", "node2"]
                                        ),
                                    ),
                                ),
                            }
                        ),
                    ),
                ]
            )
        )
    )
    # quick_example_end

    # Setup problem
    solver.setAssignment(
        {
            "node1": [],
            "node2": ["shard1", "shard2"],
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "shard1": 3.0,
            "shard2": 3.0,
        },
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-memory", scope="rank", dimension="memory"
            )
        ),
        weight=1.0,
    )

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def torchrec():
    """Different strategies for different shard types."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("rank")

    # Define TorchRec partitions
    solver.addPartition("table", {"embeddings": ["shard1", "shard2", "shard3"]})
    solver.addPartition(
        "shard_type",
        {
            "row_wise": ["shard1"],
            "column_wise": ["shard2"],
            "data_parallel": ["shard3"],
        },
    )

    # torchrec_start
    # TorchRec table sharding with different strategies per shard type
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    GroupMoveWithHintStrategiesMoveTypeSpec(
                        primaryPartition="table",
                        secondaryPartition="shard_type",
                        moveStrategies=MoveStrategies(
                            groupToMoveStrategy={
                                "row_wise": MoveStrategy(
                                    type=MoveStrategyType.RANDOM_SAMPLING_WITH_REPLACEMENT,
                                    moveToScopeItems=MoveToScopeItemsSpec(
                                        defaultScopeItems=ScopeItemList(
                                            itemList=["node1", "node2", "node3"]
                                        ),
                                    ),
                                ),
                                "column_wise": MoveStrategy(
                                    type=MoveStrategyType.RANDOM_SAMPLING_WITHOUT_REPLACEMENT,
                                    moveToScopeItems=MoveToScopeItemsSpec(
                                        defaultScopeItems=ScopeItemList(
                                            itemList=["node1", "node2", "node3"]
                                        ),
                                    ),
                                ),
                                "data_parallel": MoveStrategy(
                                    type=MoveStrategyType.RANDOM_SAMPLING_WITH_REPLACEMENT,
                                    moveToScopeItems=MoveToScopeItemsSpec(
                                        defaultScopeItems=ScopeItemList(
                                            itemList=["node1", "node2", "node3"]
                                        ),
                                    ),
                                ),
                            }
                        ),
                    ),
                ]
            )
        )
    )
    # torchrec_end

    # Setup imbalanced problem
    solver.setAssignment(
        {
            "node1": ["shard1", "shard2", "shard3"],
            "node2": [],
            "node3": [],
        }
    )

    solver.addObjectDimension(
        "compute",
        {
            "shard1": 2.0,
            "shard2": 2.0,
            "shard3": 2.0,
        },
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-compute", scope="rank", dimension="compute"
            )
        ),
        weight=1.0,
    )

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def sampling_types():
    """With replacement for flexible placement, without for exclusive."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("rack")

    # Define service and tier partitions
    solver.addPartition("service", {"web": [], "db": []})
    solver.addPartition("tier", {"primary": [], "replica": []})

    # sampling_types_start
    # With replacement for flexible placement, without for exclusive
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    GroupMoveWithHintStrategiesMoveTypeSpec(
                        primaryPartition="service",
                        secondaryPartition="tier",
                        moveStrategies=MoveStrategies(
                            groupToMoveStrategy={
                                "primary": MoveStrategy(
                                    type=MoveStrategyType.RANDOM_SAMPLING_WITHOUT_REPLACEMENT,  # Exclusive
                                    moveToScopeItems=MoveToScopeItemsSpec(
                                        defaultScopeItems=ScopeItemList(
                                            itemList=["rack1", "rack2"]
                                        ),
                                    ),
                                ),
                                "replica": MoveStrategy(
                                    type=MoveStrategyType.RANDOM_SAMPLING_WITH_REPLACEMENT,  # Flexible
                                    moveToScopeItems=MoveToScopeItemsSpec(
                                        defaultScopeItems=ScopeItemList(
                                            itemList=["rack1", "rack2"]
                                        ),
                                    ),
                                ),
                            }
                        ),
                    ),
                ]
            )
        )
    )
    # sampling_types_end


def multiple_movesets():
    """Generate multiple move sets for better solution quality."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("node")

    # Define partitions
    solver.addPartition("table", {"table1": []})
    solver.addPartition("shard_type", {"type_a": [], "type_b": []})

    # multiple_movesets_start
    # Generate multiple move sets for better solution quality
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    GroupMoveWithHintStrategiesMoveTypeSpec(
                        primaryPartition="table",
                        secondaryPartition="shard_type",
                        moveStrategies=MoveStrategies(
                            groupToMoveStrategy={
                                "type_a": MoveStrategy(
                                    type=MoveStrategyType.RANDOM_SAMPLING_WITH_REPLACEMENT,
                                    moveSetsGeneratedPerScopeItem=5,  # Generate 5 move sets
                                    moveToScopeItems=MoveToScopeItemsSpec(
                                        defaultScopeItems=ScopeItemList(
                                            itemList=["node1", "node2"]
                                        ),
                                    ),
                                ),
                                "type_b": MoveStrategy(
                                    type=MoveStrategyType.RANDOM_SAMPLING_WITHOUT_REPLACEMENT,
                                    moveSetsGeneratedPerScopeItem=3,  # Generate 3 move sets
                                    moveToScopeItems=MoveToScopeItemsSpec(
                                        defaultScopeItems=ScopeItemList(
                                            itemList=["node1", "node2"]
                                        ),
                                    ),
                                ),
                            }
                        ),
                    ),
                ]
            )
        )
    )
    # multiple_movesets_end


def unassigned():
    """Use unassigned container to enable group replacement."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    solver.addPartition("table", {"table1": []})
    solver.addPartition(
        "shard_type",
        {
            "type_a": [],
            "type_b": [],
            "type_c": [],
        },
    )

    # unassigned_start
    # Use unassigned container to enable group replacement
    from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
        MoveStrategies,
        MoveStrategy,
        MoveStrategyType,
        MoveToScopeItemsSpec,
        SecondaryGroupReplacementConfig,
    )

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    GroupMoveWithHintStrategiesMoveTypeSpec(
                        primaryPartition="table",
                        secondaryPartition="shard_type",
                        unassignedContainer="unassigned",  # Enable replacement
                        secondaryGroupReplacementConfig=SecondaryGroupReplacementConfig(
                            secondaryGroupToAllowedReplacements={
                                "type_a": [
                                    "type_b",
                                    "type_c",
                                ],  # type_a can be replaced by type_b or type_c
                                "type_b": [
                                    "type_a"
                                ],  # type_b can be replaced by type_a
                                # type_c has no entry, can be replaced by any
                            }
                        ),
                        moveStrategies=MoveStrategies(
                            groupToMoveStrategy={
                                "type_a": MoveStrategy(
                                    type=MoveStrategyType.RANDOM_SAMPLING_WITH_REPLACEMENT,
                                    moveToScopeItems=MoveToScopeItemsSpec(
                                        defaultScopeItems=ScopeItemList(
                                            itemList=["node1", "node2"]
                                        )
                                    ),
                                ),
                                "type_b": MoveStrategy(
                                    type=MoveStrategyType.RANDOM_SAMPLING_WITH_REPLACEMENT,
                                    moveToScopeItems=MoveToScopeItemsSpec(
                                        defaultScopeItems=ScopeItemList(
                                            itemList=["node1", "node2"]
                                        )
                                    ),
                                ),
                                "type_c": MoveStrategy(
                                    type=MoveStrategyType.RANDOM_SAMPLING_WITH_REPLACEMENT,
                                    moveToScopeItems=MoveToScopeItemsSpec(
                                        defaultScopeItems=ScopeItemList(
                                            itemList=["node1", "node2"]
                                        )
                                    ),
                                ),
                            }
                        ),
                    ),
                ]
            )
        )
    )
    # unassigned_end


def main():
    print("=== GroupMoveWithHintStrategies Move Type Examples ===\n")

    print("Running quick_example...")
    quick_example()
    print("✓ quick_example completed\n")

    print("Running torchrec...")
    torchrec()
    print("✓ torchrec completed\n")

    print("Running sampling_types...")
    sampling_types()
    print("✓ sampling_types completed\n")

    print("Running multiple_movesets...")
    multiple_movesets()
    print("✓ multiple_movesets completed\n")


if __name__ == "__main__":
    main()
