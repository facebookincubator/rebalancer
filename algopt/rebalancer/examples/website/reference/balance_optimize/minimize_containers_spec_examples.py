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
MinimizeContainersSpec Reference Examples

This file demonstrates all the usage patterns for MinimizeContainersSpec shown in the
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
    MinimizeContainersFormula,
    MinimizeContainersSpec,
    ToFreeSpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


def quick_example():
    """Quick example showing basic MinimizeContainersSpec usage."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("vm")
    solver.setContainerName("host")

    # quick_example_start
    # Consolidate VMs onto fewest hosts
    solver.addGoal(
        GoalSpecs(
            minimizeContainersSpec=MinimizeContainersSpec(
                name="minimize-hosts",
                scope="host",
                dimension="vm_count",  # Count dimension
            )
        ),
        weight=10.0,  # High weight - primary goal
    )
    # quick_example_end


def basic_vm_consolidation():
    """Basic VM consolidation - pack VMs onto fewest hosts."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("vm")
    solver.setContainerName("host")

    # basic_vm_consolidation_start
    # Add count dimension (each VM = 1.0)
    vm_count = {f"vm{i}": 1.0 for i in range(100)}
    solver.addObjectDimension("count", vm_count)

    # Minimize hosts used
    solver.addGoal(
        GoalSpecs(
            minimizeContainersSpec=MinimizeContainersSpec(
                name="minimize-hosts",
                scope="host",
                dimension="count",
            )
        ),
        weight=10.0,
    )
    # basic_vm_consolidation_end


def consolidation_with_capacity():
    """Consolidation with capacity constraints - ensure limits respected."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("vm")
    solver.setContainerName("host")

    # consolidation_capacity_start
    # Minimize hosts
    solver.addGoal(
        GoalSpecs(
            minimizeContainersSpec=MinimizeContainersSpec(
                name="minimize-hosts",
                scope="host",
                dimension="count",
            )
        ),
        weight=10.0,
    )

    # Respect CPU capacity
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="cpu-capacity",
                scope="host",
                dimension="cpu_usage",
            )
        )
    )

    # Respect memory capacity
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="host",
                dimension="memory_usage",
            )
        )
    )
    # consolidation_capacity_end


def weighted_container_costs():
    """Weighted container costs - prefer freeing expensive containers."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("vm")
    solver.setContainerName("host")

    # weighted_costs_start
    # Assign costs to containers (higher = prefer to free)
    container_costs = {
        "old_host_1": 100.0,  # Old hardware, want to decommission
        "old_host_2": 100.0,
        "premium_host_1": 50.0,  # Expensive, free if possible
        "standard_host_1": 1.0,  # Normal cost
        "standard_host_2": 1.0,
    }

    solver.addGoal(
        GoalSpecs(
            minimizeContainersSpec=MinimizeContainersSpec(
                name="minimize-cost-weighted",
                scope="host",
                dimension="count",
                containerCosts=container_costs,
            )
        ),
        weight=10.0,
    )
    # weighted_costs_end


def safety_limit_on_freeing():
    """Safety limit - prevent freeing too many containers at once."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("vm")
    solver.setContainerName("host")

    # safety_limit_start
    # Free at most 10 hosts per round (safety)
    solver.addGoal(
        GoalSpecs(
            minimizeContainersSpec=MinimizeContainersSpec(
                name="gradual-consolidation",
                scope="host",
                dimension="count",
                maxFreeLimit=10,  # Max 10 hosts freed
            )
        ),
        weight=10.0,
    )
    # safety_limit_end


def rack_level_consolidation():
    """Rack-level consolidation - consolidate entire racks."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("vm")
    solver.setContainerName("host")

    # Add rack scope
    host_to_rack = {
        "host0": "rack0",
        "host1": "rack0",
        "host2": "rack1",
        "host3": "rack1",
    }
    solver.addScope("rack", host_to_rack)

    # rack_consolidation_start
    # Minimize racks used
    solver.addGoal(
        GoalSpecs(
            minimizeContainersSpec=MinimizeContainersSpec(
                name="minimize-racks",
                scope="rack",
                dimension="count",
            )
        ),
        weight=8.0,
    )

    # Also minimize hosts within used racks
    solver.addGoal(
        GoalSpecs(
            minimizeContainersSpec=MinimizeContainersSpec(
                name="minimize-hosts",
                scope="host",
                dimension="count",
            )
        ),
        weight=5.0,  # Lower weight than rack consolidation
    )
    # rack_consolidation_end


def consolidation_with_balance():
    """Consolidation with balance - minimize containers while balancing load."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("vm")
    solver.setContainerName("host")

    # consolidation_balance_start
    # Primary: Minimize hosts
    solver.addGoal(
        GoalSpecs(
            minimizeContainersSpec=MinimizeContainersSpec(
                name="minimize-hosts",
                scope="host",
                dimension="count",
            )
        ),
        weight=10.0,
    )

    # Secondary: Balance load on used hosts
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-remaining",
                scope="host",
                dimension="cpu_usage",
            )
        ),
        weight=2.0,  # Lower weight
    )
    # consolidation_balance_end


def incremental_consolidation():
    """Incremental consolidation - gradually consolidate over multiple rounds."""

    # incremental_consolidation_start
    def incremental_consolidation_rounds(total_rounds=5):
        """Consolidate gradually over multiple rounds."""
        for round_num in range(total_rounds):
            solver = ProblemSolver(service_name="example", service_scope="test")

            # Increase consolidation pressure each round
            weight = 2.0 * (round_num + 1)  # 2, 4, 6, 8, 10

            solver.addGoal(
                GoalSpecs(
                    minimizeContainersSpec=MinimizeContainersSpec(
                        name="minimize-hosts",
                        scope="host",
                        dimension="count",
                    )
                ),
                weight=weight,  # Increasing pressure
            )

            solution = solver.solve()
            apply_moves(solution)

            print(
                f"Round {round_num + 1}: Weight {weight}, "
                f"Hosts freed: {count_empty_hosts(solution)}"
            )

    # incremental_consolidation_end


def combining_with_to_free():
    """Combining with ToFreeSpec - targeted + general consolidation."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("vm")
    solver.setContainerName("host")

    # combining_to_free_start
    # Explicitly drain specific hosts (constraint)
    solver.addConstraint(
        ConstraintSpecs(
            toFreeSpec=ToFreeSpec(
                name="drain-targets",
                containers=["old_host_1", "old_host_2"],
            )
        )
    )

    # Also minimize total hosts used (goal)
    solver.addGoal(
        GoalSpecs(
            minimizeContainersSpec=MinimizeContainersSpec(
                name="minimize-all-hosts",
                scope="host",
                dimension="count",
            )
        ),
        weight=5.0,
    )
    # combining_to_free_end


def infeasibility_check():
    """Check if consolidation is feasible before attempting."""
    # infeasibility_check_start
    # Check if target is feasible
    total_cpu_needed = sum(vm_cpu.values())
    total_memory_needed = sum(vm_memory.values())

    target_hosts = 10
    target_cpu_capacity = target_hosts * host_cpu_capacity
    target_memory_capacity = target_hosts * host_memory_capacity

    if (
        total_cpu_needed > target_cpu_capacity
        or total_memory_needed > target_memory_capacity
    ):
        print(f"Warning: Cannot consolidate to {target_hosts} hosts!")
        print(f"  Need CPU: {total_cpu_needed}, Available: {target_cpu_capacity}")
        print(
            f"  Need Memory: {total_memory_needed}, Available: {target_memory_capacity}"
        )
    # infeasibility_check_end


def consolidation_with_balance_fix():
    """Fixing overloaded containers - consolidate AND balance."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("vm")
    solver.setContainerName("host")

    # consolidation_balance_fix_start
    # GOOD: Consolidate AND balance
    solver.addGoal(
        GoalSpecs(
            minimizeContainersSpec=MinimizeContainersSpec(
                name="minimize-hosts",
                scope="host",
                dimension="count",
            )
        ),
        weight=10.0,
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-cpu",
                scope="host",
                dimension="cpu_usage",
            )
        ),
        weight=3.0,  # Prevents overloading packed containers
    )
    # consolidation_balance_fix_end


def count_dimension_setup():
    """Proper count dimension setup."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("vm")
    solver.setContainerName("host")

    all_objects = [f"vm{i}" for i in range(100)]

    # count_dimension_start
    # GOOD: Add count dimension first
    count_dimension = {obj: 1.0 for obj in all_objects}
    solver.addObjectDimension("count", count_dimension)

    solver.addGoal(
        GoalSpecs(
            minimizeContainersSpec=MinimizeContainersSpec(
                name="minimize-hosts",
                scope="host",
                dimension="count",
            )
        ),
        weight=10.0,
    )
    # count_dimension_end


def correct_formula_usage():
    """Using the correct formula for bin packing."""
    solver = ProblemSolver(service_name="example", service_scope="test")
    solver.setObjectName("vm")
    solver.setContainerName("host")

    # correct_formula_start
    # GOOD: Correct formula (or omit for default)
    solver.addGoal(
        GoalSpecs(
            minimizeContainersSpec=MinimizeContainersSpec(
                name="minimize-hosts",
                scope="host",
                dimension="count",
                formula=MinimizeContainersFormula.MINIMIZE_OCCUPIED_CONTAINERS,
            )
        ),
        weight=10.0,
    )
    # correct_formula_end


def verify_consolidation():
    """Verify consolidation achieved target."""

    # verify_consolidation_start
    def verify_consolidation_target(solution, target_max_containers):
        """Verify consolidation achieved target.

        Args:
            solution: Solver solution
            target_max_containers: Maximum containers target

        Returns:
            True if target met
        """
        occupied_count = sum(
            1 for objects in solution.assignment.values() if len(objects) > 0
        )

        if occupied_count <= target_max_containers:
            print(
                f"Consolidation success: {occupied_count} containers used "
                f"(target: <={target_max_containers})"
            )
            return True
        else:
            print(
                f"Consolidation missed target: {occupied_count} containers used "
                f"(target: <={target_max_containers})"
            )
            print(f"   Exceeded by: {occupied_count - target_max_containers}")
            return False

    # verify_consolidation_end


def calculate_savings():
    """Calculate cost savings from consolidation."""

    # calculate_savings_start
    def calculate_consolidation_savings(
        initial_hosts, final_hosts, cost_per_host_per_month
    ):
        """Calculate cost savings from consolidation.

        Args:
            initial_hosts: Number of hosts before consolidation
            final_hosts: Number of hosts after consolidation
            cost_per_host_per_month: Monthly cost per host

        Returns:
            Annual savings
        """
        hosts_freed = initial_hosts - final_hosts
        monthly_savings = hosts_freed * cost_per_host_per_month
        annual_savings = monthly_savings * 12

        print(f"Cost savings analysis:")
        print(f"  Initial hosts: {initial_hosts}")
        print(f"  Final hosts: {final_hosts}")
        print(f"  Hosts freed: {hosts_freed}")
        print(f"  Monthly savings: ${monthly_savings:,.2f}")
        print(f"  Annual savings: ${annual_savings:,.2f}")

        return annual_savings

    # Example
    calculate_consolidation_savings(
        initial_hosts=50,
        final_hosts=30,
        cost_per_host_per_month=500,  # $500/month per host
    )
    # Output: $120,000/year savings
    # calculate_savings_end


def analyze_consolidation():
    """Analyze consolidation results."""

    # analyze_consolidation_start
    def analyze_consolidation_results(initial_assignment, final_assignment):
        """Analyze consolidation results.

        Args:
            initial_assignment: Assignment before consolidation
            final_assignment: Assignment after consolidation
        """
        initial_occupied = sum(
            1 for objects in initial_assignment.values() if len(objects) > 0
        )
        final_occupied = sum(
            1 for objects in final_assignment.values() if len(objects) > 0
        )

        freed = initial_occupied - final_occupied
        reduction_pct = (freed / initial_occupied) * 100

        print("Consolidation Results:")
        print(f"  Initial containers: {initial_occupied}")
        print(f"  Final containers: {final_occupied}")
        print(f"  Containers freed: {freed}")
        print(f"  Reduction: {reduction_pct:.1f}%")

        # Identify which containers were freed
        initial_occupied_set = {
            c for c, objs in initial_assignment.items() if len(objs) > 0
        }
        final_occupied_set = {
            c for c, objs in final_assignment.items() if len(objs) > 0
        }

        freed_containers = initial_occupied_set - final_occupied_set

        if freed_containers:
            print(f"\nFreed containers:")
            for container in sorted(freed_containers)[:10]:
                print(f"  - {container}")
            if len(freed_containers) > 10:
                print(f"  ... and {len(freed_containers) - 10} more")

    # analyze_consolidation_end


def complete_example():
    """
    Complete runnable example demonstrating MinimizeContainersSpec.

    This example shows how to use MinimizeContainersSpec to consolidate VMs
    onto fewer hosts while respecting capacity constraints and balancing load.
    """
    # Create solver
    solver = ProblemSolver(
        service_name="consolidation-example", service_scope="production"
    )

    solver.setObjectName("vm")
    solver.setContainerName("host")

    # Initial sparse assignment (VMs spread across many hosts)
    assignment = {
        "host0": ["vm0", "vm1"],
        "host1": ["vm2"],
        "host2": ["vm3", "vm4"],
        "host3": ["vm5"],
        "host4": [],
        "host5": ["vm6", "vm7"],
        "host6": ["vm8"],
        "host7": [],
        "host8": ["vm9"],
        "host9": [],
    }
    solver.setAssignment(assignment)

    # VM resources
    vm_cpu = {f"vm{i}": 2.0 + (i % 3) for i in range(10)}
    vm_memory = {f"vm{i}": 4.0 + (i % 4) for i in range(10)}
    vm_count = {f"vm{i}": 1.0 for i in range(10)}

    solver.addObjectDimension("cpu", vm_cpu)
    solver.addObjectDimension("memory", vm_memory)
    solver.addObjectDimension("count", vm_count)

    # Host capacities
    host_cpu_capacity = {f"host{i}": 20.0 for i in range(10)}
    host_memory_capacity = {f"host{i}": 40.0 for i in range(10)}

    solver.addContainerDimension("cpu_capacity", host_cpu_capacity)
    solver.addContainerDimension("memory_capacity", host_memory_capacity)

    # Add capacity constraints
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="cpu-capacity", scope="host", dimension="cpu"
            )
        )
    )
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity", scope="host", dimension="memory"
            )
        )
    )

    # Primary goal: Minimize hosts used
    solver.addGoal(
        GoalSpecs(
            minimizeContainersSpec=MinimizeContainersSpec(
                name="minimize-hosts",
                scope="host",
                dimension="count",
            )
        ),
        weight=10.0,
    )

    # Secondary goal: Balance CPU on remaining hosts
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-cpu",
                scope="host",
                dimension="cpu",
            )
        ),
        weight=2.0,
    )

    # Solve
    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=10000))
    )
    solution = solver.solve()

    # Print results
    print(f"Solution found in {solution.profile.solveTime}ms")
    print(f"Objective value: {solution.objectiveValue}")
    print(f"VMs moved: {solution.profile.moveCount}")

    # Count occupied hosts
    occupied = sum(1 for vms in solution.assignment.values() if len(vms) > 0)
    print(f"Hosts used: {occupied} (reduced from 6)")

    return solution


if __name__ == "__main__":
    print("Running all MinimizeContainers examples...")

    print("\n1. Quick Example...")
    quick_example()

    print("\n2. Basic Vm Consolidation...")
    basic_vm_consolidation()

    print("\n3. Consolidation With Capacity...")
    consolidation_with_capacity()

    print("\n4. Weighted Container Costs...")
    weighted_container_costs()

    print("\n5. Safety Limit On Freeing...")
    safety_limit_on_freeing()

    print("\n6. Rack Level Consolidation...")
    rack_level_consolidation()

    print("\n7. Consolidation With Balance...")
    consolidation_with_balance()

    print("\n8. Incremental Consolidation...")
    incremental_consolidation()

    print("\n9. Combining With To Free...")
    combining_with_to_free()

    print("\n10. Infeasibility Check...")
    infeasibility_check()

    print("\n11. Consolidation With Balance Fix...")
    consolidation_with_balance_fix()

    print("\n12. Count Dimension Setup...")
    count_dimension_setup()

    print("\n13. Correct Formula Usage...")
    correct_formula_usage()

    print("\n14. Complete Example...")
    complete_example()

    print("\n✓ All MinimizeContainers examples completed successfully!")
# example_end
