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
MaximizeAllocationSpec Reference Examples

This file demonstrates all the usage patterns for MaximizeAllocationSpec shown in the
reference documentation. Each function is a complete, runnable example.
"""

# example_start
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
    Filter,
    MaximizeAllocationSpec,
    MinimizeContainersSpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


def quick_example():
    """Quick example showing basic MaximizeAllocationSpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # quick_example_start
    # Prefer using budget hosts (fill them first)
    budget_hosts = ["budget_host_1", "budget_host_2", "budget_host_3"]

    solver.addGoal(
        GoalSpecs(
            maximizeAllocationSpec=MaximizeAllocationSpec(
                name="prefer-budget-hosts",
                scope="host",
                dimension="cpu_usage",
                filter=Filter(items=budget_hosts),
            )
        ),
        weight=2.0,
    )
    # quick_example_end


def prefer_cheap_hosts():
    """Fill budget-tier hosts before using premium hosts."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # prefer_cheap_hosts_start
    # Define cost tiers
    budget_hosts = ["budget_host_1", "budget_host_2", "budget_host_3"]
    premium_hosts = ["premium_host_1", "premium_host_2"]

    # Maximize allocation on budget hosts (cost savings)
    solver.addGoal(
        GoalSpecs(
            maximizeAllocationSpec=MaximizeAllocationSpec(
                name="prefer-cheap-hosts",
                scope="host",
                dimension="cpu_usage",  # Or "count" for object count
                filter=Filter(items=budget_hosts),
            )
        ),
        weight=3.0,  # Higher weight = stronger preference
    )

    # Still balance load overall (secondary)
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-all-hosts",
                scope="host",
                dimension="cpu_usage",
            )
        ),
        weight=1.0,  # Lower weight
    )
    # prefer_cheap_hosts_end


def data_locality():
    """Maximize data on hosts with local storage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # data_locality_start
    # Hosts with local SSDs
    local_storage_hosts = ["ssd_host_1", "ssd_host_2", "ssd_host_3"]

    # Prefer placing data on local storage hosts
    solver.addGoal(
        GoalSpecs(
            maximizeAllocationSpec=MaximizeAllocationSpec(
                name="prefer-local-storage",
                scope="host",
                dimension="data_size",
                filter=Filter(items=local_storage_hosts),
            )
        ),
        weight=2.0,
    )
    # data_locality_end


def rack_consolidation():
    """Consolidate workload onto specific racks."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # Add rack scope
    solver.addScope(
        "rack",
        {
            "host0": "rack1",
            "host1": "rack1",
            "host2": "rack2",
            "host3": "rack2",
            "host4": "rack3",
        },
    )

    # rack_consolidation_start
    # Target racks for consolidation (others will be freed)
    active_racks = ["rack1", "rack2", "rack3"]

    # Maximize usage on active racks
    solver.addGoal(
        GoalSpecs(
            maximizeAllocationSpec=MaximizeAllocationSpec(
                name="consolidate-racks",
                scope="rack",
                dimension="cpu_usage",
                filter=Filter(items=active_racks),
            )
        ),
        weight=5.0,
    )

    # This implicitly drains other racks
    # rack_consolidation_end


def new_infrastructure_preference():
    """Prefer new hosts over old hosts."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # new_infrastructure_preference_start
    # New datacenter hosts
    new_dc_hosts = ["new_dc_host_1", "new_dc_host_2", "new_dc_host_3"]

    # Maximize allocation on new hosts (migration incentive)
    solver.addGoal(
        GoalSpecs(
            maximizeAllocationSpec=MaximizeAllocationSpec(
                name="prefer-new-infrastructure",
                scope="host",
                dimension="count",  # Object count
                filter=Filter(items=new_dc_hosts),
            )
        ),
        weight=2.5,
    )
    # new_infrastructure_preference_end


def zone_preference():
    """Prefer specific availability zones."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # Add zone scope
    solver.addScope(
        "zone",
        {
            "host0": "zone_a",
            "host1": "zone_a",
            "host2": "zone_b",
            "host3": "zone_b",
            "host4": "zone_c",
        },
    )

    # zone_preference_start
    # Preferred zones (lower latency, cheaper network)
    preferred_zones = ["zone_a", "zone_b"]

    solver.addGoal(
        GoalSpecs(
            maximizeAllocationSpec=MaximizeAllocationSpec(
                name="prefer-zones",
                scope="zone",
                dimension="network_traffic",  # Network-intensive workloads
                filter=Filter(items=preferred_zones),
            )
        ),
        weight=1.5,
    )
    # zone_preference_end


def energy_efficiency():
    """Pack onto energy-efficient hosts."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # energy_efficiency_start
    # Modern, energy-efficient hosts
    efficient_hosts = ["efficient_host_1", "efficient_host_2"]

    # Maximize usage on efficient hosts
    solver.addGoal(
        GoalSpecs(
            maximizeAllocationSpec=MaximizeAllocationSpec(
                name="energy-efficiency",
                scope="host",
                dimension="power_usage",  # Or cpu_usage as proxy
                filter=Filter(items=efficient_hosts),
            )
        ),
        weight=2.0,
    )
    # energy_efficiency_end


def tiered_storage_strategy():
    """Fill fast storage before slow storage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    # tiered_storage_strategy_start
    # Hosts with NVMe storage
    nvme_hosts = ["nvme_host_1", "nvme_host_2"]

    # Maximize allocation on NVMe hosts for hot data
    solver.addGoal(
        GoalSpecs(
            maximizeAllocationSpec=MaximizeAllocationSpec(
                name="prefer-nvme",
                scope="host",
                dimension="hot_data_size",  # Hot data dimension
                filter=Filter(items=nvme_hosts),
            )
        ),
        weight=4.0,
    )
    # tiered_storage_strategy_end


def combining_with_minimize_containers():
    """Consolidate onto preferred hosts."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    preferred_hosts = ["host0", "host1"]

    # combining_minimize_containers_start
    # Maximize allocation on preferred hosts
    solver.addGoal(
        GoalSpecs(
            maximizeAllocationSpec=MaximizeAllocationSpec(
                name="prefer-hosts",
                filter=Filter(items=preferred_hosts),
                dimension="count",
            )
        ),
        weight=3.0,
    )

    # Also minimize total hosts used
    solver.addGoal(
        GoalSpecs(
            minimizeContainersSpec=MinimizeContainersSpec(
                name="minimize-hosts",
                dimension="count",
            )
        ),
        weight=2.0,
    )
    # combining_minimize_containers_end


def combining_with_balance():
    """Fill preferred hosts, balance the rest."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    budget_hosts = ["host0", "host1"]

    # combining_balance_start
    # Primary: Fill budget hosts
    solver.addGoal(
        GoalSpecs(
            maximizeAllocationSpec=MaximizeAllocationSpec(
                name="fill-budget",
                filter=Filter(items=budget_hosts),
                dimension="cpu_usage",
            )
        ),
        weight=4.0,
    )

    # Secondary: Balance across all hosts
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-all",
                scope="host",
                dimension="cpu_usage",
            )
        ),
        weight=1.0,
    )
    # combining_balance_end


def combining_with_capacity():
    """Maximize within capacity limits."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("task")
    solver.setContainerName("host")

    preferred_hosts = ["host0", "host1"]

    # combining_capacity_start
    # Hard limit: Capacity
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="capacity",
                scope="host",
                dimension="cpu_usage",
            )
        )
    )

    # Soft goal: Maximize on preferred hosts (within capacity)
    solver.addGoal(
        GoalSpecs(
            maximizeAllocationSpec=MaximizeAllocationSpec(
                name="maximize-preferred",
                filter=Filter(items=preferred_hosts),
                dimension="cpu_usage",
            )
        ),
        weight=3.0,
    )
    # combining_capacity_end


def verification_example():
    """Verify allocation maximization."""

    # verification_example_start
    def verify_maximize_allocation(solution, dimension_values, filtered_containers):
        """Verify allocation was maximized on filtered containers.

        Args:
            solution: Solver solution
            dimension_values: Object dimension values
            filtered_containers: Containers to check

        Returns:
            Total allocation on filtered containers
        """
        total_on_filtered = 0.0
        total_on_other = 0.0

        for container, objects in solution.assignment.items():
            total_for_container = sum(dimension_values.get(obj, 0.0) for obj in objects)

            if container in filtered_containers:
                total_on_filtered += total_for_container
            else:
                total_on_other += total_for_container

        total = total_on_filtered + total_on_other
        pct_on_filtered = (total_on_filtered / total * 100) if total > 0 else 0

        print(
            f"Allocation on filtered containers: {total_on_filtered:.1f} "
            f"({pct_on_filtered:.1f}% of total)"
        )
        print(f"Allocation on other containers: {total_on_other:.1f}")

        return total_on_filtered

    # verification_example_end
    return verify_maximize_allocation


def cost_savings_example():
    """Calculate cost savings from preferring cheap hosts."""

    # cost_savings_example_start
    def calculate_cost_savings(
        solution,
        dimension_values,
        cheap_hosts,
        expensive_hosts,
        cost_per_unit_cheap,
        cost_per_unit_expensive,
    ):
        """Calculate cost savings from preferring cheap hosts.

        Args:
            solution: Solver solution
            dimension_values: Object dimension values (e.g., cpu_usage)
            cheap_hosts: List of cheap host IDs
            expensive_hosts: List of expensive host IDs
            cost_per_unit_cheap: Cost per CPU on cheap hosts ($/CPU/month)
            cost_per_unit_expensive: Cost per CPU on expensive hosts

        Returns:
            Monthly savings
        """
        usage_on_cheap = 0.0
        usage_on_expensive = 0.0

        for container, objects in solution.assignment.items():
            usage = sum(dimension_values.get(obj, 0.0) for obj in objects)

            if container in cheap_hosts:
                usage_on_cheap += usage
            elif container in expensive_hosts:
                usage_on_expensive += usage

        cost_cheap = usage_on_cheap * cost_per_unit_cheap
        cost_expensive = usage_on_expensive * cost_per_unit_expensive

        # Calculate what it would cost if all on expensive
        total_usage = usage_on_cheap + usage_on_expensive
        cost_all_expensive = total_usage * cost_per_unit_expensive

        monthly_savings = cost_all_expensive - (cost_cheap + cost_expensive)

        print(f"Cost analysis:")
        print(
            f"  Usage on cheap hosts: {usage_on_cheap:.1f} ({usage_on_cheap / total_usage * 100:.1f}%)"
        )
        print(f"  Usage on expensive hosts: {usage_on_expensive:.1f}")
        print(f"  Monthly cost: ${cost_cheap + cost_expensive:.2f}")
        print(f"  Savings vs all-expensive: ${monthly_savings:.2f}/month")

        return monthly_savings

    # cost_savings_example_end
    return calculate_cost_savings


def complete_example():
    """
    Complete runnable example demonstrating MaximizeAllocationSpec.

    This example shows how to use MaximizeAllocationSpec to preferentially
    fill budget hosts before using premium hosts, resulting in cost savings.
    """
    # Create solver
    solver = ProblemSolver(
        service_name="maximize-allocation-example", service_scope="production"
    )

    solver.setObjectName("task")
    solver.setContainerName("host")

    # Initial assignment: mixed across budget and premium hosts
    assignment = {
        "budget_host_0": ["task0", "task1"],
        "budget_host_1": ["task2"],
        "budget_host_2": [],
        "premium_host_0": ["task3", "task4", "task5"],
        "premium_host_1": ["task6", "task7", "task8", "task9"],
    }
    solver.setAssignment(assignment)

    # Task resources
    cpu_usage = {f"task{i}": 2.0 + (i % 3) for i in range(10)}
    solver.addObjectDimension("cpu_usage", cpu_usage)

    # Host capacities
    budget_capacity = {f"budget_host_{i}": 15.0 for i in range(3)}
    premium_capacity = {f"premium_host_{i}": 15.0 for i in range(2)}
    all_capacity = {**budget_capacity, **premium_capacity}
    solver.addContainerDimension("cpu_capacity", all_capacity)

    # Add capacity constraint
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="cpu-capacity", scope="host", dimension="cpu_usage"
            )
        )
    )

    # Primary goal: Maximize allocation on budget hosts
    budget_hosts = ["budget_host_0", "budget_host_1", "budget_host_2"]
    solver.addGoal(
        GoalSpecs(
            maximizeAllocationSpec=MaximizeAllocationSpec(
                name="prefer-budget-hosts",
                scope="host",
                dimension="cpu_usage",
                filter=Filter(items=budget_hosts),
            )
        ),
        weight=3.0,
    )

    # Secondary goal: Balance overall (low weight)
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-all",
                scope="host",
                dimension="cpu_usage",
            )
        ),
        weight=1.0,
    )

    # Solve
    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=10000))
    )
    solution = solver.solve()

    # Print results
    print(f"Solution found in {solution.profile.solveTime}ms")
    print(f"Objective value: {solution.objectiveValue}")
    print(f"Tasks moved: {solution.profile.moveCount}")

    # Calculate allocation on budget vs premium
    budget_cpu = 0.0
    premium_cpu = 0.0
    for host, tasks in solution.assignment.items():
        cpu = sum(cpu_usage.get(task, 0.0) for task in tasks)
        if host in budget_hosts:
            budget_cpu += cpu
        else:
            premium_cpu += cpu

    total_cpu = budget_cpu + premium_cpu
    print(f"\nAllocation breakdown:")
    print(f"  Budget hosts: {budget_cpu:.1f} CPU ({budget_cpu / total_cpu * 100:.1f}%)")
    print(
        f"  Premium hosts: {premium_cpu:.1f} CPU ({premium_cpu / total_cpu * 100:.1f}%)"
    )

    return solution


if __name__ == "__main__":
    print("Running all MaximizeAllocation examples...")

    print("\n1. Quick Example...")
    quick_example()

    print("\n2. Prefer Cheap Hosts...")
    prefer_cheap_hosts()

    print("\n3. Data Locality...")
    data_locality()

    print("\n4. Rack Consolidation...")
    rack_consolidation()

    print("\n5. New Infrastructure Preference...")
    new_infrastructure_preference()

    print("\n6. Zone Preference...")
    zone_preference()

    print("\n7. Energy Efficiency...")
    energy_efficiency()

    print("\n8. Tiered Storage Strategy...")
    tiered_storage_strategy()

    print("\n9. Combining With Minimize Containers...")
    combining_with_minimize_containers()

    print("\n10. Combining With Balance...")
    combining_with_balance()

    print("\n11. Combining With Capacity...")
    combining_with_capacity()

    print("\n12. Verification Example...")
    verification_example()

    print("\n13. Cost Savings Example...")
    cost_savings_example()

    print("\n14. Complete Example...")
    complete_example()

    print("\n✓ All MaximizeAllocation examples completed successfully!")
# example_end
