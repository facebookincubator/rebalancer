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
SwapFullContainers Move Type Reference Examples

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
    CapacitySpec,
    Limit,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
    SingleMoveTypeSpec,
    SwapFullContainersMoveTypeSpec,
    SwapMoveTypeSpec,
)


def quick_example():
    """Quick example showing basic SwapFullContainers usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # quick_example_start
    # Exchange entire container contents
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SwapFullContainersMoveTypeSpec(),  # Full container swaps
                ]
            )
        )
    )
    # quick_example_end

    # Setup problem with imbalanced containers
    solver.setAssignment(
        {
            "host0": ["task0", "task1", "task2"],  # 12GB - overloaded
            "host1": ["task3"],  # 3GB - underutilized
            "host2": [],  # Empty
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 4.0,
            "task1": 4.0,
            "task2": 4.0,
            "task3": 3.0,
        },
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="host",
                dimension="memory",
                limit=Limit(globalLimit=16.0),
            )
        )
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-memory", scope="host", dimension="memory"
            )
        ),
        weight=1.0,
    )

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def server_migration():
    """Move entire server workload."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("server")

    # server_migration_start
    # Migrate all shards from overloaded server to another
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SwapFullContainersMoveTypeSpec(),  # Swap all shards between servers
                ]
            )
        )
    )
    # server_migration_end

    # Setup problem
    solver.setAssignment(
        {
            "server0": ["shard0", "shard1", "shard2"],  # 15GB - overloaded
            "server1": ["shard3", "shard4"],  # 8GB
            "server2": ["shard5"],  # 4GB
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "shard0": 5.0,
            "shard1": 5.0,
            "shard2": 5.0,
            "shard3": 4.0,
            "shard4": 4.0,
            "shard5": 4.0,
        },
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="server",
                dimension="memory",
                limit=Limit(globalLimit=16.0),
            )
        )
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


def consolidation():
    """Rebalance container-level load."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # consolidation_start
    # Container-level rebalancing for similar-sized containers
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SwapFullContainersMoveTypeSpec(),  # Swap entire containers
                ]
            )
        )
    )
    # consolidation_end

    # Setup problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1"],  # 8GB
            "host1": ["task2", "task3"],  # 8GB
            "host2": ["task4"],  # 4GB
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 4.0,
            "task1": 4.0,
            "task2": 4.0,
            "task3": 4.0,
            "task4": 4.0,
        },
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="host",
                dimension="memory",
                limit=Limit(globalLimit=12.0),
            )
        )
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-memory", scope="host", dimension="memory"
            )
        ),
        weight=1.0,
    )

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def combined():
    """Use for coarse adjustment, then fine-tune."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # combined_start
    # Multi-stage: coarse container rebalancing, then fine object placement
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SwapFullContainersMoveTypeSpec(),  # Container-level rebalancing
                    SwapMoveTypeSpec(),  # Fine-tune with object swaps
                    SingleMoveTypeSpec(),  # Final object placement
                ]
            )
        )
    )
    # combined_end

    # Setup problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1", "task2"],  # 9GB
            "host1": ["task3"],  # 3GB
            "host2": ["task4", "task5"],  # 6GB
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 3.0,
            "task1": 3.0,
            "task2": 3.0,
            "task3": 3.0,
            "task4": 3.0,
            "task5": 3.0,
        },
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="host",
                dimension="memory",
                limit=Limit(globalLimit=12.0),
            )
        )
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-memory", scope="host", dimension="memory"
            )
        ),
        weight=1.0,
    )

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def capacity_constrained():
    """When individual moves blocked by capacity."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # capacity_constrained_start
    # Full swaps can work when individual moves are capacity-blocked
    # Note: Full swaps may violate capacity if container sizes differ
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SwapFullContainersMoveTypeSpec(),  # Try full container exchanges
                    SwapMoveTypeSpec(),  # Fall back to capacity-neutral object swaps
                ]
            )
        )
    )
    # capacity_constrained_end

    # Setup problem where hosts are at capacity
    solver.setAssignment(
        {
            "host0": ["task0", "task1"],  # 8GB - full
            "host1": ["task2", "task3"],  # 8GB - full
            "host2": ["task4"],  # 4GB
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 4.0,
            "task1": 4.0,
            "task2": 4.0,
            "task3": 4.0,
            "task4": 4.0,
        },
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="host",
                dimension="memory",
                limit=Limit(globalLimit=8.0),
            )
        )
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-memory", scope="host", dimension="memory"
            )
        ),
        weight=1.0,
    )

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def main():
    print("=== SwapFullContainers Move Type Examples ===\n")

    print("Running quick_example...")
    quick_example()
    print("✓ quick_example completed\n")

    print("Running server_migration...")
    server_migration()
    print("✓ server_migration completed\n")


if __name__ == "__main__":
    main()
