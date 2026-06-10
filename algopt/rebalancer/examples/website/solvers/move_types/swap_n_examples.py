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
SwapN Move Type Reference Examples

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
    GroupCountSpec,
    GroupCountSpecBound,
    GroupLimitSpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
    SwapNMoveTypeSpec,
)


def quick_example():
    """Quick example showing basic SwapN usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("request")
    solver.setContainerName("rack")

    # Define pod scope
    solver.addScope(
        "pod",
        {
            "rack0": "pod0",
            "rack1": "pod0",
            "rack2": "pod0",
            "rack3": "pod1",
            "rack4": "pod1",
            "rack5": "pod1",
        },
    )

    # quick_example_start
    # Swap 3 objects simultaneously to satisfy exact-limit constraints
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SwapNMoveTypeSpec(
                        swapNConcurrentObjects=3,
                        swapNSourceObjects=[
                            "req0",
                            "req1",
                            "req2",
                            "req3",
                            "req4",
                            "req5",
                        ],
                        swapNDestinationScope=[
                            ["rack0", "rack1", "rack2"],  # Pod 0 racks
                            ["rack3", "rack4", "rack5"],  # Pod 1 racks
                        ],
                        swapNIterations=1000,
                    ),
                ]
            )
        )
    )
    # quick_example_end

    # Setup problem with unassigned requests
    solver.setAssignment(
        {
            "rack0": [],
            "rack1": [],
            "rack2": [],
            "rack3": [],
            "rack4": [],
            "rack5": [],
            "unassigned": ["req0", "req1", "req2", "req3", "req4", "req5"],
        }
    )

    solver.addObjectDimension(
        "capacity",
        {
            "req0": 1.0,
            "req1": 1.0,
            "req2": 1.0,
            "req3": 1.0,
            "req4": 1.0,
            "req5": 1.0,
        },
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-capacity", scope="rack", dimension="capacity"
            )
        ),
        weight=1.0,
    )

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def exact_limit():
    """Allocate requests with exact pod limits."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("request")
    solver.setContainerName("rack")

    # Define pod scope
    solver.addScope(
        "pod",
        {
            "rack0": "pod0",
            "rack1": "pod0",
            "rack2": "pod0",
            "rack3": "pod1",
            "rack4": "pod1",
            "rack5": "pod1",
        },
    )

    # Define request partition
    solver.addPartition(
        "request_group",
        {
            "group1": [
                "request0",
                "request1",
                "request2",
                "request3",
                "request4",
                "request5",
            ]
        },
    )

    # exact_limit_start
    # Exactly 3 requests per pod using SwapN
    solver.addConstraint(
        ConstraintSpecs(
            groupCountSpec=GroupCountSpec(
                scope="pod",
                dimension="request_count",
                partitionName="request_group",
                bound=GroupCountSpecBound.EXACT,
                limit=GroupLimitSpec(defaultLimit=3),
            )
        )
    )

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SwapNMoveTypeSpec(
                        swapNConcurrentObjects=3,
                        swapNSourceObjects=[
                            "request0",
                            "request1",
                            "request2",
                            "request3",
                            "request4",
                            "request5",
                        ],
                        swapNDestinationScope=[
                            ["rack0", "rack1", "rack2"],  # Pod 0
                            ["rack3", "rack4", "rack5"],  # Pod 1
                        ],
                    ),
                ]
            )
        )
    )
    # exact_limit_end

    # Setup with unassigned requests
    solver.setAssignment(
        {
            "rack0": [],
            "rack1": [],
            "rack2": [],
            "rack3": [],
            "rack4": [],
            "rack5": [],
            "unassigned": [
                "request0",
                "request1",
                "request2",
                "request3",
                "request4",
                "request5",
            ],
        }
    )

    solver.addObjectDimension(
        "size",
        {
            "request0": 1.0,
            "request1": 1.0,
            "request2": 1.0,
            "request3": 1.0,
            "request4": 1.0,
            "request5": 1.0,
        },
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(name="balance-size", scope="rack", dimension="size")
        ),
        weight=1.0,
    )

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def crp():
    """Capacity Request Portal: allocate hosts with exact pod constraints."""
    # crp_start
    # Capacity Request Portal: allocate hosts with exact pod constraints
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("host")
    solver.setContainerName("rack")

    # Define pod scope
    solver.addScope(
        "pod",
        {
            "rack0": "pod0",
            "rack1": "pod0",
            "rack2": "pod1",
            "rack3": "pod1",
        },
    )

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SwapNMoveTypeSpec(
                        swapNConcurrentObjects=3,  # 3 hosts per allocation
                        swapNSourceObjects=[
                            "host0",
                            "host1",
                            "host2",
                            "host3",
                            "host4",
                            "host5",
                        ],
                        swapNDestinationScope=[
                            ["rack0", "rack1"],  # Pod 0 racks
                            ["rack2", "rack3"],  # Pod 1 racks
                        ],
                        swapNIterations=1000000,  # 1M iterations
                    ),
                ]
            )
        )
    )
    # crp_end


def limited_iterations():
    """Use fewer iterations for faster (but lower quality) results."""
    # limited_iterations_start
    # Use fewer iterations for faster (but lower quality) results
    solver = ProblemSolver(service_name="example", service_scope="test")

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SwapNMoveTypeSpec(
                        swapNConcurrentObjects=2,
                        swapNSourceObjects=["obj0", "obj1", "obj2", "obj3"],
                        swapNDestinationScope=[
                            ["container0", "container1"],
                            ["container2", "container3"],
                        ],
                        swapNIterations=100,  # Reduced from default 1M
                    ),
                ]
            )
        )
    )
    # limited_iterations_end


def main():
    print("=== SwapN Move Type Examples ===\n")

    print("Running quick_example...")
    quick_example()
    print("✓ quick_example completed\n")

    print("Running exact_limit...")
    exact_limit()
    print("✓ exact_limit completed\n")


if __name__ == "__main__":
    main()
