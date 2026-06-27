---
sidebar_position: 1
---

# Introduction to Rebalancer

Welcome to Rebalancer, a powerful assignment solver library that provides a generic and intuitive API for defining and solving assignment problems with various optimization algorithms.

## What is an Assignment Problem?

An **assignment problem** is any problem that can be defined as deciding how to assign **objects** to **containers** such that:
- Each object is assigned to exactly one container
- The assignment satisfies a set of constraints/rules
- The assignment optimizes one or more objectives/goals

## Common Examples

Assignment problems appear across many industries and domains:

### Systems Engineering

- **Load Balancing**: Assigning tasks to servers to balance CPU utilization
- **Database Sharding**: Distributing database shards across hosts while respecting capacity
- **Container Orchestration**: Placing containers on VMs with resource constraints
- **CDN Optimization**: Assigning content to edge servers to minimize latency
- **Resource Allocation**: Distributing workloads across clusters

### Operations & Logistics

- **Delivery Route Assignment**: Assigning delivery drivers to routes while balancing workload and respecting time windows
- **Warehouse Task Scheduling**: Distributing picking tasks to workers to maximize throughput
- **Vehicle Fleet Management**: Assigning vehicles to routes based on capacity and fuel efficiency
- **Package Sorting**: Routing packages to sortation lanes to balance processing load

### Healthcare

- **Patient-Bed Assignment**: Placing patients in hospital beds while respecting care requirements and isolation needs
- **Nurse Shift Scheduling**: Assigning nurses to shifts to meet staffing requirements and skill levels
- **Operating Room Scheduling**: Allocating surgical procedures to operating rooms to maximize utilization
- **Medical Equipment Allocation**: Distributing equipment across departments based on demand and availability

### Education & Workforce

- **Student-Class Assignment**: Enrolling students in courses while respecting prerequisites and capacity limits
- **Teacher-Course Assignment**: Assigning teachers to courses based on expertise and availability
- **Employee Shift Scheduling**: Scheduling staff shifts to meet coverage requirements and preferences
- **Exam Room Allocation**: Assigning exams to rooms and time slots to avoid conflicts

### Sports & Tournaments

- **Team-Bracket Assignment**: Placing teams in tournament brackets to ensure fair matchups
- **Referee-Game Assignment**: Assigning referees to games based on availability and experience level
- **Venue Scheduling**: Allocating games to fields or courts to maximize facility utilization

## Why Rebalancer?

### Domain-Specific Language (DSL)
Rebalancer provides an expressive DSL that lets you describe your problem in intuitive terms rather than low-level mathematical formulations. Instead of writing linear programming constraints manually, you use high-level concepts like "balance CPU across hosts" or "respect memory capacity."

### Multiple Solvers
Choose the right solver for your problem:
- **Local Search**: Fast, scalable, good-enough solutions for large problems (~1M objects)
- **Optimal Solvers**: Provably optimal solutions for smaller problems (supports XPRESS, Gurobi, HiGHS)

### Extensible
- 40+ built-in goals and constraints
- Easy to add custom goals and constraints
- Support for complex multi-dimensional problems

### Production Ready
- Written in C++ for performance
- Python bindings for ease of use
- Multi-threaded parallelism
- Battle-tested at scale

## Quick Example

Here's a simple load balancing problem in Python:

```python
from rebalancer import ProblemSolver
from rebalancer.specs import (
    BalanceSpec,
    GoalSpec,
    OptimalSolverSpec,
    SolverSpec,
)

# Create solver
solver = ProblemSolver(service_name="rebalancer", service_scope="example")

# Define the problem. The OSS binding takes Thrift specs as plain dicts shaped
# like the Thrift JSON wire format. The TypedDicts in ``rebalancer.specs`` are
# a typed view over those dict shapes — at runtime they just produce the same
# dicts, but you get IDE autocomplete and editor warnings on typos.
(
    solver.set_object_name("task")
    .set_container_name("host")
    .set_assignment({
        "host0": [f"task{i}" for i in range(12)],
        "host1": [],
        "host2": [],
        "host3": [],
    })
    # Goal: balance tasks across hosts
    .add_goal(
        GoalSpec(
            balanceSpec=BalanceSpec(
                name="balance-hosts",
                scope="host",
                dimension="task_count",
                formula="LEGACY",
                fixAverageToInitial=True,
            )
        ),
        weight=1.0,
    )
    # Solve with the optimal (MIP-based) solver
    .add_solver(SolverSpec(optimalSolverSpec=OptimalSolverSpec()))
)

solution = solver.solve()
print(solution["assignment"])
```

## Next Steps

- **New to Rebalancer?** Start with the [Installation Guide](getting-started/installation) and [Tutorial: Build Your First Model](getting-started/first-model)
- **Want to understand the concepts?** Read the [Core Concepts](core-concepts/overview) section
- **Looking for API reference?** Browse the [Goals & Constraints Reference](reference/)

## Research Paper

Rebalancer is based on research published at OSDI 2024. For academic details about the design and algorithms, see our [research paper](research).

## License

Rebalancer is licensed under the Apache 2.0 License.
