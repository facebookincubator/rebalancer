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
Basic Bin Packing and Consolidation

This example demonstrates how to pack VMs onto the minimum number of hosts
to save costs while respecting capacity constraints and maintaining balance.
"""

import random

# solution_start
# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    ConstraintSpecs,
    GoalSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    BalanceSpec,
    BalanceSpecFormula,
    CapacitySpec,
    MinimizeContainersSpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


def main():
    """Pack VMs onto minimum number of hosts."""
    solver = ProblemSolver(service_name="vm-consolidator", service_scope="production")

    solver.setObjectName("vm")
    solver.setContainerName("host")

    # Initial spread assignment (inefficient - VMs spread across all hosts)
    assignment = {f"host{i}": [] for i in range(30)}

    # Distribute VMs round-robin (spread thinly)
    for i in range(100):
        host_idx = i % 30
        assignment[f"host{host_idx}"].append(f"vm{i}")

    solver.setAssignment(assignment)

    # VM CPU usage (varies 0.5 to 8.0 cores)
    vm_cpu = {f"vm{i}": 0.5 + random.random() * 7.5 for i in range(100)}
    solver.addObjectDimension("cpu_usage", vm_cpu)

    # VM memory usage (varies 1 to 32 GB)
    vm_memory = {f"vm{i}": 1.0 + random.random() * 31.0 for i in range(100)}
    solver.addObjectDimension("memory_usage", vm_memory)

    # Add count dimension for MinimizeContainers
    vm_count = {f"vm{i}": 1.0 for i in range(100)}
    solver.addObjectDimension("count", vm_count)

    # Host capacities (uniform for simplicity)
    host_cpu_capacity = {f"host{i}": 64.0 for i in range(30)}  # 64 cores
    host_memory_capacity = {f"host{i}": 256.0 for i in range(30)}  # 256 GB

    solver.addContainerDimension("cpu_capacity", host_cpu_capacity)
    solver.addContainerDimension("memory_capacity", host_memory_capacity)

    # CONSTRAINT 1: CPU capacity
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="cpu-capacity",
                scope="host",
                dimension="cpu_usage",
            )
        )
    )

    # CONSTRAINT 2: Memory capacity
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="host",
                dimension="memory_usage",
            )
        )
    )

    # GOAL 1: Minimize number of hosts used (PRIMARY)
    solver.addGoal(
        GoalSpecs(
            minimizeContainersSpec=MinimizeContainersSpec(
                name="minimize-hosts",
                scope="host",
                dimension="count",  # Based on VM count
            )
        ),
        weight=10.0,  # High weight - this is the main goal
    )

    # GOAL 2: Balance CPU across used hosts (SECONDARY)
    # Prevents overloading some packed hosts
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-cpu",
                scope="host",
                dimension="cpu_usage",
                formula=BalanceSpecFormula.LEGACY,
                fixAverageToInitial=True,
            )
        ),
        weight=1.0,  # Lower weight than consolidation
    )

    # GOAL 3: Balance memory across used hosts (SECONDARY)
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-memory",
                scope="host",
                dimension="memory_usage",
                formula=BalanceSpecFormula.LEGACY,
                fixAverageToInitial=True,
            )
        ),
        weight=1.0,
    )

    # Use Local Search solver
    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=60000))
    )

    # Solve
    solution = solver.solve()

    # Analyze solution
    print(f"Solution found in {solution.profile.solveTime}ms")
    print(f"Objective value: {solution.objectiveValue}")
    print(f"VMs moved: {solution.profile.moveCount}")

    # Count hosts used
    hosts_with_vms = {
        host: vms for host, vms in solution.assignment.items() if len(vms) > 0
    }

    initial_hosts_used = sum(1 for vms in assignment.values() if len(vms) > 0)
    final_hosts_used = len(hosts_with_vms)

    print(f"\nConsolidation results:")
    print(f"  Initial hosts used: {initial_hosts_used}")
    print(f"  Final hosts used: {final_hosts_used}")
    print(f"  Hosts freed: {initial_hosts_used - final_hosts_used}")
    print(
        f"  Reduction: {((initial_hosts_used - final_hosts_used) / initial_hosts_used * 100):.1f}%"
    )

    # Analyze used hosts
    print(f"\nUsed host utilization:")
    print(
        f"{'Host':<10} {'VMs':<6} {'CPU':<10} {'CPU %':<8} {'Memory':<10} {'Memory %'}"
    )
    print("-" * 70)

    for host in sorted(hosts_with_vms.keys()):
        vms = hosts_with_vms[host]
        total_cpu = sum(vm_cpu.get(vm, 0) for vm in vms)
        total_mem = sum(vm_memory.get(vm, 0) for vm in vms)
        cpu_pct = (total_cpu / host_cpu_capacity[host]) * 100
        mem_pct = (total_mem / host_memory_capacity[host]) * 100

        print(
            f"{host:<10} {len(vms):<6} {total_cpu:<10.1f} {cpu_pct:<8.1f} "
            f"{total_mem:<10.1f} {mem_pct:.1f}%"
        )

    # List empty hosts (candidates for decommissioning)
    empty_hosts = [host for host, vms in solution.assignment.items() if len(vms) == 0]

    if empty_hosts:
        print(f"\nEmpty hosts (can be decommissioned):")
        for host in sorted(empty_hosts)[:10]:  # Show first 10
            print(f"  - {host}")
        if len(empty_hosts) > 10:
            print(f"  ... and {len(empty_hosts) - 10} more")

    return solution


if __name__ == "__main__":
    main()
# solution_end
