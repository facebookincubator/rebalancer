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
Performance Tuning Examples

This file demonstrates performance tuning patterns shown in the
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


def adjust_time_limit():
    """Adjust time limit for performance."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    # adjust_time_limit_start
    # Increase time limit for better quality
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                timeLimitMs=120000  # 2 minutes instead of 30 seconds
            )
        )
    )
    # adjust_time_limit_end


def set_mip_gap():
    """Set MIP gap tolerance for Optimal solver."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    # set_mip_gap_start
    # Accept 5% gap instead of waiting for optimality
    solver.addSolver(
        SolverSpecs(
            optimalSolverSpec=OptimalSolverSpec(
                solverPackage=OptimalSolverPackage.HIGHS,
                timeLimitMs=300000,
                mipGap=0.05,  # Stop when within 5%
            )
        )
    )
    # set_mip_gap_end


def set_thread_count():
    """Control parallelization with thread count."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    # set_thread_count_start
    # Use 8 threads for faster solving
    solver.addSolver(
        SolverSpecs(
            optimalSolverSpec=OptimalSolverSpec(
                solverPackage=OptimalSolverPackage.HIGHS,
                threads=8,
            )
        )
    )
    # set_thread_count_end


def warmstart_with_local_search():
    """Warmstart Optimal solver with Local Search."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    # warmstart_with_local_search_start
    # Run Local Search first for good initial solution
    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=30000))
    )

    # Then refine with Optimal (uses LS result as warmstart)
    solver.addSolver(
        SolverSpecs(
            optimalSolverSpec=OptimalSolverSpec(
                solverPackage=OptimalSolverPackage.HIGHS,
                timeLimitMs=180000,
                mipGap=0.02,
            )
        )
    )
    # warmstart_with_local_search_end


if __name__ == "__main__":
    print("Running all Performance Tuning examples...\n")

    print("1. Adjust Time Limit...")
    adjust_time_limit()

    print("\n✓ All Performance Tuning examples completed successfully!")
# example_end
