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
SingleEndChain Move Type Reference Examples

This file demonstrates all the usage patterns for SingleEndChain move type shown in the
reference documentation. Each function is a complete, runnable example.
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
    MinimizeMovementSpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
    SingleEndChainMoveTypeSpec,
    SingleMoveTypeSpec,
    SwapMoveTypeSpec,
)


def quick_example():
    """Quick example showing basic SingleEndChain usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # quick_example_start
    # Use SingleEndChain for high-quality capacity-constrained solutions
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleMoveTypeSpec(),
                    SwapMoveTypeSpec(),
                    SingleEndChainMoveTypeSpec(),  # Expensive but effective
                ]
            )
        )
    )
    # quick_example_end

    # Setup capacity-constrained problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1", "task2", "task3"],
            "host1": ["task4", "task5"],
            "host2": [],
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 2.0,
            "task1": 2.0,
            "task2": 2.0,
            "task3": 2.0,
            "task4": 4.0,
            "task5": 4.0,
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


def basic_configuration():
    """Use after Single and Swap for better quality."""
    # basic_configuration_start
    # Use after Single and Swap for better quality
    solver = ProblemSolver(service_name="example", service_scope="test")

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleMoveTypeSpec(),
                    SwapMoveTypeSpec(),
                    SingleEndChainMoveTypeSpec(),
                ]
            )
        )
    )
    # basic_configuration_end


def high_quality():
    """Best quality for capacity-constrained problems."""
    # high_quality_start
    # Best quality for capacity-constrained problems
    solver = ProblemSolver(service_name="example", service_scope="test")

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                solveTime=600,  # 10 minutes - allow time for expensive moves
                moveTypeList=[
                    SingleMoveTypeSpec(),
                    SwapMoveTypeSpec(),
                    SingleEndChainMoveTypeSpec(),
                ],
            )
        )
    )
    # high_quality_end


def production_rebalancing():
    """Production rebalancing with time limits."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # production_rebalancing_start
    # Balance quality and time for production
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="host",
                dimension="memory",
                limit=Limit(globalLimit=10.0),
            )
        )
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-memory", scope="host", dimension="memory"
            )
        ),
        weight=10.0,
    )

    solver.addGoal(
        GoalSpecs(minimizeMovementSpec=MinimizeMovementSpec(name="minimize-moves")),
        weight=1.0,
    )

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                solveTime=300,  # 5 minutes limit
                moveTypeList=[
                    SingleMoveTypeSpec(),
                    SwapMoveTypeSpec(),
                    SingleEndChainMoveTypeSpec(),  # Gets remaining time
                ],
            )
        )
    )
    # production_rebalancing_end

    # Setup production-like problem
    solver.setAssignment(
        {
            "host0": ["shard0", "shard1", "shard2"],
            "host1": ["shard3", "shard4", "shard5"],
            "host2": ["shard6"],
            "host3": [],
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "shard0": 3.0,
            "shard1": 2.0,
            "shard2": 3.0,
            "shard3": 2.5,
            "shard4": 2.5,
            "shard5": 2.0,
            "shard6": 7.0,
        },
    )

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def main():
    print("=== SingleEndChain Move Type Examples ===\n")

    print("Running quick_example...")
    quick_example()
    print("✓ quick_example completed\n")

    print("Running production_rebalancing...")
    production_rebalancing()
    print("✓ production_rebalancing completed\n")


if __name__ == "__main__":
    main()
