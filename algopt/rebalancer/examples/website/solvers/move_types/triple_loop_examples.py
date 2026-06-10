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
TripleLoop Move Type Reference Examples

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
    SingleEndChainMoveTypeSpec,
    SingleMoveTypeSpec,
    SwapMoveTypeSpec,
    TripleLoopMoveTypeSpec,
)


def quick_example():
    """Quick example showing basic TripleLoop usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # quick_example_start
    # Only use TripleLoop for small problems or as last resort
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleMoveTypeSpec(),
                    SwapMoveTypeSpec(),
                    TripleLoopMoveTypeSpec(),  # Last resort for deep local optima
                ]
            )
        )
    )
    # quick_example_end

    # Setup small capacity-constrained problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1"],
            "host1": ["task2"],
            "host2": ["task3"],
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 2.5,
            "task1": 1.5,
            "task2": 3.5,
            "task3": 1.5,
        },
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="host",
                dimension="memory",
                limit=Limit(globalLimit=4.0),  # Tight capacity
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


def last_resort():
    """Try progressively more powerful moves."""
    # last_resort_start
    # Try progressively more powerful moves
    solver = ProblemSolver(service_name="example", service_scope="test")

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleMoveTypeSpec(),  # Add Single
                    SwapMoveTypeSpec(),  # Add Swap
                    SingleEndChainMoveTypeSpec(),  # Add SingleEndChain
                    TripleLoopMoveTypeSpec(),  # Add TripleLoop - last resort
                ]
            )
        )
    )
    # last_resort_end


def small_problem():
    """Only use for problems with small containers."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("server")

    # small_problem_start
    # For problems with <100 objects per container
    # WARNING: Do not use with large problems!
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SingleMoveTypeSpec(),
                    TripleLoopMoveTypeSpec(),  # Safe for small problems only
                ]
            )
        )
    )
    # small_problem_end

    # Small problem with few objects
    solver.setAssignment(
        {
            "server0": ["shard0", "shard1", "shard2"],
            "server1": ["shard3"],
            "server2": [],
        }
    )

    solver.addObjectDimension(
        "cpu",
        {
            "shard0": 2.0,
            "shard1": 1.5,
            "shard2": 1.5,
            "shard3": 4.0,
        },
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="cpu-capacity",
                scope="server",
                dimension="cpu",
                limit=Limit(globalLimit=5.0),
            )
        )
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(name="balance-cpu", scope="server", dimension="cpu")
        ),
        weight=1.0,
    )

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def time_limits():
    """Strict time limit to prevent expensive TripleLoop from taking too long."""
    # time_limits_start
    # Strict time limit to prevent expensive TripleLoop from taking too long
    solver = ProblemSolver(service_name="example", service_scope="test")

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                solveTime=60,  # Maximum 60 seconds total
                moveTypeList=[
                    SingleMoveTypeSpec(),  # Add Single
                    SwapMoveTypeSpec(),  # Add Swap
                    TripleLoopMoveTypeSpec(),  # Add TripleLoop - will stop after 60s
                ],
            )
        )
    )
    # time_limits_end


def offline():
    """Offline optimization - take time to find best solution. Use when solution quality is critical and time is available."""
    # offline_start
    # Offline optimization - take time to find best solution
    # Use when solution quality is critical and time is available
    solver = ProblemSolver(service_name="example", service_scope="test")

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                solveTime=3600,  # Allow 1 hour
                moveTypeList=[
                    SingleMoveTypeSpec(),  # Add Single
                    SwapMoveTypeSpec(),  # Add Swap
                    SingleEndChainMoveTypeSpec(),  # Add SingleEndChain
                    TripleLoopMoveTypeSpec(),  # Add TripleLoop - thorough search for best solution
                ],
            )
        )
    )
    # offline_end


def main():
    print("=== TripleLoop Move Type Examples ===\n")

    print("Running quick_example...")
    quick_example()
    print("✓ quick_example completed\n")

    print("Running small_problem...")
    small_problem()
    print("✓ small_problem completed\n")


if __name__ == "__main__":
    main()
