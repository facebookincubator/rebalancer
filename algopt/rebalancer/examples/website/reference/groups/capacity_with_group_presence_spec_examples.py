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
CapacityWithGroupPresenceSpec Reference Examples

This file demonstrates all the usage patterns for CapacityWithGroupPresenceSpec
shown in the reference documentation. Each function is a complete, runnable example.
"""

# example_start
# pyre-strict

from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    ConstraintSpecs,
    GoalSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    BalanceSpec,
    CapacityWithGroupPresenceBound,
    CapacityWithGroupPresenceSpec,
    CapacityWithGroupPresenceUsageIntent,
    GroupCountSpec,
    GroupCountSpecBound,
    Limit,
    LimitType,
)


def quick_example():
    """Quick example showing basic CapacityWithGroupPresenceSpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # quick_example_start
    # Each service has 10 GB overhead when present, plus actual usage
    solver.addConstraint(
        ConstraintSpecs(
            capacityWithGroupPresenceSpec=CapacityWithGroupPresenceSpec(
                name="capacity-with-overhead",
                scope="host",
                partition="service",
                dimension="memory",
                groupToPresenceWeight=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=10.0,  # Each service adds 10 GB when present
                ),
                scopeItemToLimit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=100.0,  # Max 100 GB per host
                ),
                bound=CapacityWithGroupPresenceBound.MAX,
            )
        )
    )
    # quick_example_end


def service_overhead_costs():
    """Model fixed overhead per service."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # service_overhead_costs_start
    # Each service has 5 GB metadata/connection overhead
    services = {
        "web_service": ["web_0", "web_1", "web_2"],
        "api_service": ["api_0", "api_1", "api_2"],
        "cache_service": ["cache_0", "cache_1"],
    }
    solver.addPartition("service", services)

    solver.addConstraint(
        ConstraintSpecs(
            capacityWithGroupPresenceSpec=CapacityWithGroupPresenceSpec(
                name="memory-with-service-overhead",
                scope="host",
                partition="service",
                dimension="memory",
                groupToPresenceWeight=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=5.0,  # 5 GB overhead per service
                ),
                scopeItemToLimit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=128.0,  # 128 GB per host
                ),
            )
        )
    )

    # Example:
    # Host has web_service (2 GB actual) + api_service (3 GB actual)
    # Contribution = max(5, 2) + max(5, 3) = 5 + 5 = 10 GB
    # Even though actual usage is only 2+3=5 GB, overhead makes it 10 GB
    # service_overhead_costs_end


def per_service_custom_overhead():
    """Different overhead per service."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # per_service_custom_overhead_start
    # Premium services have higher overhead
    solver.addConstraint(
        ConstraintSpecs(
            capacityWithGroupPresenceSpec=CapacityWithGroupPresenceSpec(
                name="variable-service-overhead",
                scope="host",
                partition="service",
                dimension="memory",
                groupToPresenceWeight=Limit(
                    type=LimitType.ABSOLUTE,
                    groupLimits={
                        "web_service": 10.0,  # 10 GB overhead
                        "api_service": 15.0,  # 15 GB overhead
                        "cache_service": 5.0,  # 5 GB overhead
                    },
                    globalLimit=8.0,  # Default: 8 GB
                ),
                scopeItemToLimit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=128.0,
                ),
            )
        )
    )
    # per_service_custom_overhead_end


def connection_pool_overhead():
    """Model connection pool reserves."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("db_host")

    # connection_pool_overhead_start
    # Databases reserve connections per client service
    solver.addConstraint(
        ConstraintSpecs(
            capacityWithGroupPresenceSpec=CapacityWithGroupPresenceSpec(
                name="db-connection-reserve",
                scope="db_host",
                partition="client_service",
                dimension="connection_count",
                groupToPresenceWeight=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=50.0,  # Reserve 50 connections per service
                ),
                scopeItemToLimit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=1000.0,  # Max 1000 connections per DB
                ),
            )
        )
    )
    # connection_pool_overhead_end


def rounding_for_integer_resources():
    """Round up group contributions to whole units."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # rounding_for_integer_resources_start
    # Each service rounds up to whole CPU cores
    solver.addConstraint(
        ConstraintSpecs(
            capacityWithGroupPresenceSpec=CapacityWithGroupPresenceSpec(
                name="cpu-rounded",
                scope="host",
                partition="service",
                dimension="cpu_cores",
                groupToPresenceWeight=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=1.0,  # Min 1 core when present
                ),
                scopeItemToLimit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=64.0,  # 64 cores per host
                ),
                roundUpGroupUtilOnScopeItem=True,  # Round up to integer cores
            )
        )
    )

    # Example:
    # Service uses 2.3 cores → rounds up to 3 cores
    # Service uses 0.1 cores → max(1, 0.1) = 1 core (presence weight)
    # rounding_for_integer_resources_end


def multipliers_for_replica_overhead():
    """Apply multipliers for replication."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("shard")
    solver.setContainerName("host")

    # multipliers_for_replica_overhead_start
    # Primary + replica replication overhead (2x multiplier)
    solver.addConstraint(
        ConstraintSpecs(
            capacityWithGroupPresenceSpec=CapacityWithGroupPresenceSpec(
                name="replicated-capacity",
                scope="rack",
                partition="shard",
                dimension="storage",
                groupToPresenceWeight=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=100.0,  # 100 GB min per shard
                ),
                scopeItemToLimit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=5000.0,  # 5 TB per rack
                ),
                multiplierList=[
                    Limit(
                        type=LimitType.ABSOLUTE,
                        globalLimit=2.0,  # 2x for replication
                    ),
                ],
            )
        )
    )

    # Example:
    # Shard uses 500 GB → max(100, 500) = 500 GB
    # With 2x multiplier: ceil(500 * 2) = 1000 GB counted
    # multipliers_for_replica_overhead_end


def multi_level_multipliers():
    """Cascading multipliers (e.g., replication + compression)."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # multi_level_multipliers_start
    # 2x for replication, then 0.5x for compression
    solver.addConstraint(
        ConstraintSpecs(
            capacityWithGroupPresenceSpec=CapacityWithGroupPresenceSpec(
                name="multi-multiplier",
                scope="host",
                partition="service",
                dimension="storage",
                groupToPresenceWeight=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=50.0,
                ),
                scopeItemToLimit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=1000.0,
                ),
                multiplierList=[
                    Limit(globalLimit=2.0),  # Replication
                    Limit(globalLimit=0.5),  # Compression
                ],
            )
        )
    )

    # Example:
    # Service uses 400 GB
    # After m1: ceil(400 * 2) = 800 GB
    # After m2: ceil(800 * 0.5) = 400 GB
    # multi_level_multipliers_end


def aggregation_scope():
    """Aggregate utilization at different scope level."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # aggregation_scope_start
    # Limit per rack, but aggregate at host level
    solver.addConstraint(
        ConstraintSpecs(
            capacityWithGroupPresenceSpec=CapacityWithGroupPresenceSpec(
                name="rack-limit-host-aggregation",
                scope="rack",
                partition="service",
                dimension="memory",
                aggregationScope="host",  # Aggregate at host level first
                groupToPresenceWeight=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=10.0,  # 10 GB overhead per service per host
                ),
                scopeItemToLimit=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=500.0,  # 500 GB per rack
                ),
            )
        )
    )

    # Util(rack) = sum_{host in rack} sum_{service} max(10, service_util_on_host)
    # aggregation_scope_end


def per_group_and_scope_item_limits():
    """Different limits per (group, scope item) combination."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # per_group_and_scope_item_limits_start
    # Different service limits on different hosts
    solver.addConstraint(
        ConstraintSpecs(
            capacityWithGroupPresenceSpec=CapacityWithGroupPresenceSpec(
                name="per-service-per-host-limit",
                scope="host",
                partition="service",
                dimension="cpu",
                intent=CapacityWithGroupPresenceUsageIntent.PER_GROUP_AND_SCOPE_ITEM,
                groupToPresenceWeight=Limit(
                    type=LimitType.ABSOLUTE,
                    globalLimit=1.0,
                ),
                scopeItemToLimit=Limit(
                    type=LimitType.ABSOLUTE,
                    # Limits per (service, host)
                    perGroupAndScopeItemLimits={
                        ("web_service", "host1"): 20.0,
                        ("web_service", "host2"): 30.0,
                        ("api_service", "host1"): 40.0,
                    },
                    globalLimit=50.0,  # Default
                ),
            )
        )
    )
    # per_group_and_scope_item_limits_end


def combining_with_balance():
    """Capacity with overhead + balance."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # combining_with_balance_start
    # Hard: Capacity with overhead
    solver.addConstraint(
        ConstraintSpecs(
            capacityWithGroupPresenceSpec=CapacityWithGroupPresenceSpec(
                name="capacity-overhead",
                scope="host",
                partition="service",
                dimension="memory",
                groupToPresenceWeight=Limit(globalLimit=10.0),
                scopeItemToLimit=Limit(globalLimit=128.0),
            )
        )
    )

    # Soft: Balance load
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-memory",
                scope="host",
                dimension="memory",
            )
        ),
        weight=1.0,
    )
    # combining_with_balance_end


def combining_with_group_count():
    """Limit overhead sources + capacity."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # combining_with_group_count_start
    # Limit number of services per host (to limit overhead)
    solver.addConstraint(
        ConstraintSpecs(
            groupCountSpec=GroupCountSpec(
                name="max-services-per-host",
                scope="host",
                partitionName="service",
                bound=GroupCountSpecBound.MAX,
                limit=Limit(globalLimit=5),  # Max 5 services
            )
        )
    )

    # Capacity with per-service overhead
    solver.addConstraint(
        ConstraintSpecs(
            capacityWithGroupPresenceSpec=CapacityWithGroupPresenceSpec(
                name="capacity-with-overhead",
                scope="host",
                partition="service",
                dimension="memory",
                groupToPresenceWeight=Limit(globalLimit=10.0),  # 10 GB per service
                scopeItemToLimit=Limit(globalLimit=100.0),
            )
        )
    )
    # combining_with_group_count_end


if __name__ == "__main__":
    print("Running all CapacityWithGroupPresenceSpec examples...\n")

    print("1. Quick Example...")
    quick_example()

    print("\n2. Service Overhead Costs...")
    service_overhead_costs()

    print("\n3. Per-Service Custom Overhead...")
    per_service_custom_overhead()

    print("\n4. Connection Pool Overhead...")
    connection_pool_overhead()

    print("\n5. Rounding For Integer Resources...")
    rounding_for_integer_resources()

    print("\n6. Multipliers For Replica Overhead...")
    multipliers_for_replica_overhead()

    print("\n7. Multi-Level Multipliers...")
    multi_level_multipliers()

    print("\n8. Aggregation Scope...")
    aggregation_scope()

    print("\n9. Per-Group And Scope Item Limits...")
    per_group_and_scope_item_limits()

    print("\n10. Combining With Balance...")
    combining_with_balance()

    print("\n11. Combining With Group Count...")
    combining_with_group_count()

    print("\n✓ All CapacityWithGroupPresenceSpec examples completed successfully!")
# example_end
