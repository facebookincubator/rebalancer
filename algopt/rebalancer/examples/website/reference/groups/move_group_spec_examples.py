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
MoveGroupSpec Reference Examples

This file demonstrates all the usage patterns for MoveGroupSpec shown in the
reference documentation. Each function is a complete, runnable example.
"""

# example_start
# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import MoveGroupSpec


def quick_example():
    """Quick example showing basic MoveGroupSpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("replica")
    solver.setContainerName("host")

    # quick_example_start
    # Move all replicas of each shard together
    solver.addConstraint(
        MoveGroupSpec(
            name="move-shard-replicas-together",
            partitionName="shard",  # Partition grouping replicas by shard
        )
    )
    # quick_example_end


def replica_set_cohesion():
    """Keep database replicas together."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("replica")
    solver.setContainerName("host")

    # replica_set_cohesion_start
    # Partition by replica set
    replica_sets = {
        "db_shard_0": ["shard_0_primary", "shard_0_replica_1", "shard_0_replica_2"],
        "db_shard_1": ["shard_1_primary", "shard_1_replica_1", "shard_1_replica_2"],
        "db_shard_2": ["shard_2_primary", "shard_2_replica_1", "shard_2_replica_2"],
    }
    solver.addPartition("replica_set", replica_sets)

    # Move replicas together (for migration, not anti-affinity!)
    solver.addConstraint(
        MoveGroupSpec(
            name="move-replica-sets-together",
            partitionName="replica_set",
        )
    )
    # replica_set_cohesion_end


if __name__ == "__main__":
    print("Running all MoveGroupSpec examples...\n")

    print("1. Quick Example...")
    quick_example()

    print("\n2. Replica Set Cohesion...")
    replica_set_cohesion()

    print("\n✓ All MoveGroupSpec examples completed successfully!")
# example_end
