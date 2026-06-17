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
Optimal Solver Examples

This file demonstrates optimal solver usage patterns shown in the
solver documentation.
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


def quick_example():
    """Quick example showing basic OptimalSolverSpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    # quick_example_start
    solver.addSolver(
        SolverSpecs(
            optimalSolverSpec=OptimalSolverSpec(
                solverPackage=OptimalSolverPackage.HIGHS,
                timeLimitMs=300000,  # 5 minute time limit
                mipGap=0.01,  # Stop when within 1% of optimal
            )
        )
    )

    solution = solver.solve()

    # Check if optimal
    if solution.profile.optimal:
        print("Proven optimal solution!")
    else:
        gap = solution.profile.mipGap
        print(f"Solution within {gap * 100:.2f}% of optimal")
    # quick_example_end


def small_problem_optimal():
    """Small problem that needs optimal solution."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    # small_problem_optimal_start
    # Small problem, can wait for optimal
    solver.addSolver(
        SolverSpecs(
            optimalSolverSpec=OptimalSolverSpec(
                solverPackage=OptimalSolverPackage.HIGHS,
                timeLimitMs=600000,  # 10 minutes
                mipGap=0.0,  # Don't stop until proven optimal
            )
        )
    )
    # small_problem_optimal_end


def medium_problem_time_limited():
    """Medium problem with time limit."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    # medium_problem_time_limited_start
    # Medium problem, accept "good enough"
    solver.addSolver(
        SolverSpecs(
            optimalSolverSpec=OptimalSolverSpec(
                solverPackage=OptimalSolverPackage.HIGHS,
                timeLimitMs=300000,  # 5 minutes
                mipGap=0.05,  # Stop when within 5%
            )
        )
    )
    # medium_problem_time_limited_end


def quick_optimality_check():
    """Quick check if problem can be solved."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    # quick_optimality_check_start
    # See if we can solve quickly
    solver.addSolver(
        SolverSpecs(
            optimalSolverSpec=OptimalSolverSpec(
                solverPackage=OptimalSolverPackage.HIGHS,
                timeLimitMs=60000,  # 1 minute
                mipGap=0.10,  # Accept 10% gap
            )
        )
    )
    # quick_optimality_check_end


def validate_local_search():
    """Compare Local Search to Optimal."""
    # validate_local_search_start
    # Run Local Search first
    solver1 = ProblemSolver(service_name="example", service_scope="test")
    # ... set up problem ...
    solver1.addSolver(SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec()))
    ls_solution = solver1.solve()

    # Then run Optimal on same problem
    solver2 = ProblemSolver(service_name="example", service_scope="test")
    # ... set up same problem ...
    solver2.addSolver(
        SolverSpecs(
            optimalSolverSpec=OptimalSolverSpec(
                solverPackage=OptimalSolverPackage.HIGHS,
                timeLimitMs=300000,
            )
        )
    )
    opt_solution = solver2.solve()

    # Compare
    gap = (
        ls_solution.objectiveValue - opt_solution.objectiveValue
    ) / opt_solution.objectiveValue
    print(f"Local Search was {gap * 100:.1f}% from optimal")
    # validate_local_search_end


def warm_starting():
    """Warm start optimal solver with initial solution."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    # warm_starting_start
    # Run Local Search first, then Optimal
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
    # Optimal will use Local Search result as warmstart
    # warm_starting_end


def thread_control():
    """Control thread usage for optimal solver."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    # thread_control_start
    # Use specific number of threads
    solver.addSolver(
        SolverSpecs(
            optimalSolverSpec=OptimalSolverSpec(
                solverPackage=OptimalSolverPackage.HIGHS,
                threads=4,  # Use 4 cores
            )
        )
    )
    # thread_control_end


if __name__ == "__main__":
    print("Running all Optimal Solver examples...\n")

    print("1. Quick Example...")
    quick_example()

    print("\n✓ All Optimal Solver examples completed successfully!")
# example_end
