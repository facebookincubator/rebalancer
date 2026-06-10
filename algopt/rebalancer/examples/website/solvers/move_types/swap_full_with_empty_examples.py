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
SwapFullWithEmpty Move Type Reference Examples

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
    MinimizeContainersSpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
    SingleMoveTypeSpec,
    SwapFullWithEmptyContainersMoveTypeSpec,
)


def quick_example():
    """Quick example showing basic SwapFullWithEmpty usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # quick_example_start
    # Move all objects from hot container to empty containers
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SwapFullWithEmptyContainersMoveTypeSpec(),  # Consolidate to empties
                ]
            )
        )
    )
    # quick_example_end

    # Setup problem with scattered workload
    solver.setAssignment(
        {
            "host0": ["task0"],  # 4GB
            "host1": ["task1"],  # 4GB
            "host2": ["task2"],  # 4GB
            "host3": [],  # Empty
            "host4": [],  # Empty
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 4.0,
            "task1": 4.0,
            "task2": 4.0,
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
            minimizeContainersSpec=MinimizeContainersSpec(
                name="minimize-hosts",
                scope="host",
            ),
        ),
        weight=1.0,
    )

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def consolidation():
    """Reduce number of active containers."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("server")

    # consolidation_start
    # Consolidate workload to reduce active servers
    solver.addGoal(
        GoalSpecs(
            minimizeContainersSpec=MinimizeContainersSpec(
                name="minimize-servers",
                scope="server",
            ),
        ),
        weight=1.0,
    )

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SwapFullWithEmptyContainersMoveTypeSpec(),  # Empty servers
                    SingleMoveTypeSpec(),  # Pack remaining efficiently
                ]
            )
        )
    )
    # consolidation_end

    # Setup problem
    solver.setAssignment(
        {
            "server0": ["task0", "task1"],  # 8GB
            "server1": ["task2"],  # 4GB
            "server2": ["task3"],  # 4GB
            "server3": [],  # Empty
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 4.0,
            "task1": 4.0,
            "task2": 4.0,
            "task3": 4.0,
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

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def decommission():
    """Empty servers for maintenance."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("server")

    # decommission_start
    # Decommission specific servers by moving all shards to empties
    # Use nonAccepting or toFree constraints to mark servers for decommission
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SwapFullWithEmptyContainersMoveTypeSpec(),  # Move to empty servers
                ]
            )
        )
    )
    # decommission_end

    # Setup problem
    solver.setAssignment(
        {
            "server0": ["shard0", "shard1"],  # 8GB - to decommission
            "server1": ["shard2"],  # 4GB
            "server2": [],  # Empty
            "server3": [],  # Empty
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "shard0": 4.0,
            "shard1": 4.0,
            "shard2": 4.0,
        },
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="server",
                dimension="memory",
                limit=Limit(globalLimit=12.0),
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


def bin_packing():
    """Minimize number of active bins."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("container")

    # bin_packing_start
    # Bin-packing: minimize active containers
    solver.addGoal(
        GoalSpecs(
            minimizeContainersSpec=MinimizeContainersSpec(
                name="minimize-bins",
                scope="container",
            ),
        ),
        weight=10.0,  # High weight for consolidation
    )

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SwapFullWithEmptyContainersMoveTypeSpec(),  # Empty containers
                    SingleMoveTypeSpec(),  # Pack efficiently
                ]
            )
        )
    )
    # bin_packing_end

    # Setup problem
    solver.setAssignment(
        {
            "container0": ["task0"],  # 3GB
            "container1": ["task1"],  # 3GB
            "container2": ["task2"],  # 3GB
            "container3": [],  # Empty
            "container4": [],  # Empty
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 3.0,
            "task1": 3.0,
            "task2": 3.0,
        },
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="container",
                dimension="memory",
                limit=Limit(globalLimit=10.0),
            )
        )
    )

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def combined():
    """Use with other moves for complete consolidation."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # combined_start
    # Multi-stage consolidation strategy
    solver.addGoal(
        GoalSpecs(
            minimizeContainersSpec=MinimizeContainersSpec(
                name="minimize-containers",
                scope="container",
            ),
        ),
        weight=5.0,
    )

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    SwapFullWithEmptyContainersMoveTypeSpec(),  # Empty containers quickly
                    SingleMoveTypeSpec(),  # Efficient packing
                ]
            )
        )
    )
    # combined_end

    # Setup problem
    solver.setAssignment(
        {
            "host0": ["task0", "task1"],  # 6GB
            "host1": ["task2"],  # 3GB
            "host2": ["task3"],  # 3GB
            "host3": [],  # Empty
            "host4": [],  # Empty
        }
    )

    solver.addObjectDimension(
        "memory",
        {
            "task0": 3.0,
            "task1": 3.0,
            "task2": 3.0,
            "task3": 3.0,
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

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


def main():
    print("=== SwapFullWithEmpty Move Type Examples ===\n")

    print("Running quick_example...")
    quick_example()
    print("✓ quick_example completed\n")

    print("Running consolidation...")
    consolidation()
    print("✓ consolidation completed\n")


if __name__ == "__main__":
    main()
