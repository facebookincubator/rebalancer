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
GroupRouting Move Type Reference Examples

This file demonstrates all usage patterns shown in the reference documentation.
Each function is a complete, runnable example.
"""

# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    GoalSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import BalanceSpec
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    GroupRoutingMoveTypeSpec,
    LocalSearchSolverSpec,
)


# quick_example_start
def quick_example():
    """Quick example showing basic GroupRouting usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("server")

    # Define partitions
    solver.addPartition(
        "table", {"embeddings": ["shard1", "shard2"], "features": ["shard3", "shard4"]}
    )

    # Route groups based on latency configuration
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    GroupRoutingMoveTypeSpec(
                        routingConfigName="latency-aware",
                    ),
                ]
            )
        )
    )

    # Setup problem
    solver.setAssignment(
        {
            "server1": ["shard1", "shard3"],
            "server2": ["shard2"],
            "server3": ["shard4"],
        }
    )

    solver.addObjectDimension(
        "size",
        {
            "shard1": 1.0,
            "shard2": 1.0,
            "shard3": 1.0,
            "shard4": 1.0,
        },
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-size", scope="server", dimension="size"
            )
        ),
        weight=1.0,
    )

    # Solve and print results
    solution = solver.solve()
    print(f"  Initial objective: {solution.initialObjective:.4f}")
    print(f"  Final objective: {solution.finalObjective:.4f}")
    print(f"  Improvement: {solution.initialObjective - solution.finalObjective:.4f}")


# quick_example_end


# basic_routing_start
def basic_routing():
    """Place groups based on routing configuration to minimize latency."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("server")

    solver.addPartition("table", {"embeddings": [], "features": []})

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    GroupRoutingMoveTypeSpec(
                        routingConfigName="cdn-latency",
                    ),
                ]
            )
        )
    )


# basic_routing_end


# unassigned_start
def unassigned():
    """Use unassigned container to enable placement from unassigned state."""
    solver = ProblemSolver(service_name="example", service_scope="test")

    solver.addPartition("service", {"web": [], "api": []})

    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    GroupRoutingMoveTypeSpec(
                        routingConfigName="edge-latency",
                        unassignedContainer="unassigned",
                    ),
                ]
            )
        )
    )


# unassigned_end


def multi_region():
    """Optimize placement across regions."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("replica")
    solver.setContainerName("datacenter")

    solver.addPartition(
        "service",
        {"storage": ["replica1", "replica2"], "compute": ["replica3", "replica4"]},
    )

    # multi_region_start
    # Optimize cross-region placement based on origin-to-destination latency
    solver.addSolver(
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                moveTypeList=[
                    GroupRoutingMoveTypeSpec(
                        routingConfigName="cross-region-latency",
                    ),
                ]
            )
        )
    )
    # multi_region_end

    # Setup multi-region problem
    solver.setAssignment(
        {
            "datacenter1": ["replica1", "replica3"],
            "datacenter2": ["replica2"],
            "datacenter3": ["replica4"],
        }
    )

    solver.addObjectDimension(
        "load",
        {
            "replica1": 2.0,
            "replica2": 2.0,
            "replica3": 3.0,
            "replica4": 3.0,
        },
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-load", scope="datacenter", dimension="load"
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
    print("=== GroupRouting Move Type Examples ===\n")

    print("Running quick_example...")
    quick_example()
    print("✓ quick_example completed\n")

    print("Running multi_region...")
    multi_region()
    print("✓ multi_region completed\n")


if __name__ == "__main__":
    main()
