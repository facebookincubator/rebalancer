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
Solver Overview Examples

This file demonstrates solver selection and usage patterns shown in the
solver overview documentation.
"""

# example_start
# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import SolverSpecs
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
    OptimalSolverPackage,
    OptimalSolverSpec,
)


def solver_selection():
    """Basic solver selection example."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    # solver_selection_start
    # Option 1: Local Search (fast, scales)
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                timeLimitMs=30000,  # 30 second limit
                movesLimit=100000,  # Max iterations
            )
        )
    )

    # Solve
    solution = solver.solve()
    # solver_selection_end


def combining_solvers():
    """Combine multiple solvers sequentially."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    # combining_solvers_start
    # Strategy: Local Search warmup, then Optimal refinement
    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=10000))
    )
    solver.addSolver(
        SolverSpecs(
            optimalSolverSpec=OptimalSolverSpec(
                solverPackage=OptimalSolverPackage.HIGHS, timeLimitMs=60000
            )
        )
    )

    # Local Search runs first, finds good solution
    # Optimal uses it as warmstart and refines
    # combining_solvers_end


def compare_solvers():
    """Compare Local Search to Optimal solver."""
    # compare_solvers_start
    # Try Local Search
    solver1 = ProblemSolver(service_name="example", service_scope="test")
    # ... set up problem ...
    solver1.addSolver(SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec()))
    solution1 = solver1.solve()
    print(
        f"Local Search: {solution1.objectiveValue} in {solution1.profile.solveTime}ms"
    )

    # Try Optimal
    solver2 = ProblemSolver(service_name="example", service_scope="test")
    # ... set up same problem ...
    solver2.addSolver(
        SolverSpecs(
            optimalSolverSpec=OptimalSolverSpec(
                solverPackage=OptimalSolverPackage.HIGHS, timeLimitMs=60000
            )
        )
    )
    solution2 = solver2.solve()
    print(f"Optimal: {solution2.objectiveValue} in {solution2.profile.solveTime}ms")

    # Compare quality vs speed
    gap = (
        solution1.objectiveValue - solution2.objectiveValue
    ) / solution2.objectiveValue
    print(f"Local Search is {gap * 100:.1f}% from optimal")
    print(
        f"But {solution1.profile.solveTime / solution2.profile.solveTime:.1f}x faster"
    )
    # compare_solvers_end


if __name__ == "__main__":
    print("Running all Solver Overview examples...\n")

    print("1. Solver Selection...")
    solver_selection()

    print("\n✓ All Solver Overview examples completed successfully!")
# example_end
