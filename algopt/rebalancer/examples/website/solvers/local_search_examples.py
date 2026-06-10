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
Local Search Solver Examples

This file demonstrates local search solver usage patterns shown in the
solver documentation.
"""

# example_start
# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import SolverSpecs
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


def quick_example():
    """Quick example showing basic LocalSearchSolverSpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    # quick_example_start
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                timeLimitMs=30000,  # Stop after 30 seconds
                movesLimit=100000,  # Or after 100K iterations
            )
        )
    )

    solution = solver.solve()

    # Check termination reason
    print(f"Stopped because: {solution.profile.terminationReason}")
    print(f"Iterations: {solution.profile.iterations}")
    print(f"Time: {solution.profile.solveTime}ms")
    # quick_example_end


def fast_interactive():
    """Fast interactive rebalancing for quick responses."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    # fast_interactive_start
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                timeLimitMs=5000,  # 5 second limit
            )
        )
    )
    # fast_interactive_end


def production_rebalancing():
    """Production rebalancing balancing speed and quality."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    # production_rebalancing_start
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                timeLimitMs=60000,  # 1 minute
                movesLimit=500000,  # 500K moves
            )
        )
    )
    # production_rebalancing_end


def offline_optimization():
    """Offline optimization taking time for best solution."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    # offline_optimization_start
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                timeLimitMs=3600000,  # 1 hour
                # No move limit - run until no improvements
            )
        )
    )
    # offline_optimization_end


def reproducible_results():
    """Get reproducible results with fixed seed."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    # reproducible_results_start
    solver.addSolver(SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(seed=42)))
    # reproducible_results_end


def increase_limits():
    """Increase limits to improve solution quality."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    # increase_limits_start
    # Give more time
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                timeLimitMs=300000,  # 5 minutes instead of 30 seconds
                movesLimit=1000000,  # 1M moves instead of 100K
            )
        )
    )
    # increase_limits_end


if __name__ == "__main__":
    print("Running all Local Search Solver examples...\n")

    print("1. Quick Example...")
    quick_example()

    print("\n✓ All Local Search Solver examples completed successfully!")
# example_end
