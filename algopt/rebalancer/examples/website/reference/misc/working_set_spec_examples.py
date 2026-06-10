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
WorkingSetSpec Reference Examples

This file demonstrates all usage patterns shown in the reference documentation.
Each function is a complete, runnable example.
"""

# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    WorkingSetMetric,
    WorkingSetSpec,
    WorkingUnit,
)


def quick_example():
    """Quick example showing basic WorkingSetSpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("service")
    solver.setContainerName("host")

    # quick_example_start
    # Optimize placement to minimize latency between coordinated services
    solver.addGoal(
        WorkingSetSpec(
            name="service-coordination",
            scope="host",
            dimension="network",
            workingUnits=[
                WorkingUnit(
                    endpoints=["frontend", "backend", "cache"],
                    weight=1.0,
                ),
            ],
            metric=WorkingSetMetric.AVG,
        ),
        weight=1.0,
    )
    # quick_example_end


def microservice_coordination():
    """Minimize latency between coordinated microservices."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("service")
    solver.setContainerName("datacenter")

    # microservice_coordination_start
    # Multiple working units for different service groups
    solver.addGoal(
        WorkingSetSpec(
            name="microservice-latency",
            scope="datacenter",
            dimension="network",
            workingUnits=[
                # Web tier services need low latency
                WorkingUnit(
                    endpoints=["web-server", "auth-service", "session-cache"],
                    weight=1.0,
                ),
                # Data processing pipeline
                WorkingUnit(
                    endpoints=["ingestion", "processing", "storage"],
                    weight=1.0,
                ),
            ],
        ),
        weight=1.0,
    )
    # microservice_coordination_end


def weighted_units():
    """Use weights to prioritize critical coordination."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("service")
    solver.setContainerName("rack")

    # weighted_units_start
    # Critical path gets higher weight
    solver.addGoal(
        WorkingSetSpec(
            name="weighted-coordination",
            scope="rack",
            dimension="network",
            workingUnits=[
                # Critical: user-facing services (high weight)
                WorkingUnit(
                    endpoints=["api-gateway", "user-db", "auth"],
                    weight=10.0,
                ),
                # Non-critical: batch processing (low weight)
                WorkingUnit(
                    endpoints=["batch-worker", "analytics-db"],
                    weight=1.0,
                ),
            ],
        ),
        weight=1.0,
    )
    # weighted_units_end
