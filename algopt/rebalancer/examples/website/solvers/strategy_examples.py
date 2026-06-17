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
Solver Strategy Examples

This file demonstrates solver strategy patterns shown in the
solver documentation.
"""

# example_start
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
    MinimizeMovementSpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
    OptimalSolverPackage,
    OptimalSolverSpec,
)


def sequential_solvers():
    """Use multiple solvers sequentially."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    # sequential_solvers_start
    # Strategy: Local Search warmup, then Optimal refinement
    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=10000))
    )
    solver.addSolver(
        SolverSpecs(
            optimalSolverSpec=OptimalSolverSpec(
                solverPackage=OptimalSolverPackage.HIGHS,
                timeLimitMs=60000,
            )
        )
    )

    # Local Search runs first, finds good solution
    # Optimal uses it as warmstart and refines
    # sequential_solvers_end


def time_limited_approach():
    """Time-limited solving approach."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    # time_limited_approach_start
    # Give Local Search 30 seconds to find good solution
    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=30000))
    )

    # Then give Optimal up to 5 minutes to refine
    solver.addSolver(
        SolverSpecs(
            optimalSolverSpec=OptimalSolverSpec(
                solverPackage=OptimalSolverPackage.HIGHS,
                timeLimitMs=300000,
                mipGap=0.05,  # Accept 5% gap
            )
        )
    )
    # time_limited_approach_end


def goal_prioritization():
    """Prioritize goals with different weights."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    # goal_prioritization_start
    # High priority: balance load
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(name="balance", scope="host", dimension="cpu")
        ),
        weight=10.0,
    )

    # Low priority: minimize movement
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-moves", scope="host", dimension="data_size"
            ),
        ),
        weight=1.0,
    )
    # goal_prioritization_end


def constraint_vs_goal():
    """Use constraints for hard requirements, goals for preferences."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    # constraint_vs_goal_start
    # Hard constraint: don't exceed capacity
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(name="capacity", scope="host", dimension="memory")
        )
    )

    # Soft goal: prefer balanced CPU
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(name="balance-cpu", scope="host", dimension="cpu")
        ),
        weight=1.0,
    )
    # constraint_vs_goal_end


if __name__ == "__main__":
    print("Running all Solver Strategy examples...\n")

    print("1. Sequential Solvers...")
    sequential_solvers()

    print("\n✓ All Solver Strategy examples completed successfully!")
# example_end
