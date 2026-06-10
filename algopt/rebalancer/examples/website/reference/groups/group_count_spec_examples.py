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
GroupCountSpec Reference Examples

This file demonstrates all the usage patterns for GroupCountSpec shown in the
reference documentation. Each function is a complete, runnable example.
"""

# example_start
# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import ConstraintSpecs
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    GroupCountSpec,
    GroupCountSpecBound,
    GroupCountSpecDefinition,
    Limit,
    LimitType,
)


def quick_example():
    """Quick example showing basic GroupCountSpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("replica")
    solver.setContainerName("host")

    # quick_example_start
    # Max 1 replica of each database per rack (diversity)
    solver.addConstraint(
        ConstraintSpecs(
            groupCountSpec=GroupCountSpec(
                name="one-replica-per-rack",
                scope="rack",
                partitionName="replica",
                bound=GroupCountSpecBound.MAX,
                limit=Limit(type=LimitType.ABSOLUTE, globalLimit=1),
                definition=GroupCountSpecDefinition.AFTER,
            )
        )
    )
    # quick_example_end


def rack_diversity():
    """Ensure no two replicas of the same object on the same rack."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("replica")
    solver.setContainerName("host")

    # rack_diversity_start
    # Define replica partition
    solver.addPartition("replica", {})  # replica_to_objects

    # Max 1 replica per rack
    solver.addConstraint(
        ConstraintSpecs(
            groupCountSpec=GroupCountSpec(
                name="rack-diversity",
                scope="rack",
                partitionName="replica",
                bound=GroupCountSpecBound.MAX,
                limit=Limit(type=LimitType.ABSOLUTE, globalLimit=1),
            )
        )
    )
    # rack_diversity_end


def minimum_presence():
    """Ensure minimum number of groups on each container."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("replica")
    solver.setContainerName("host")

    # minimum_presence_start
    # Each datacenter must have at least 3 replicas
    solver.addConstraint(
        ConstraintSpecs(
            groupCountSpec=GroupCountSpec(
                name="min-replicas-per-dc",
                scope="datacenter",
                partitionName="replica",
                bound=GroupCountSpecBound.MIN,
                limit=Limit(type=LimitType.ABSOLUTE, globalLimit=3),
            )
        )
    )
    # minimum_presence_end


if __name__ == "__main__":
    print("Running all GroupCountSpec examples...\n")

    print("1. Quick Example...")
    quick_example()

    print("\n2. Rack Diversity...")
    rack_diversity()

    print("\n3. Minimum Presence...")
    minimum_presence()

    print("\n✓ All GroupCountSpec examples completed successfully!")
# example_end
