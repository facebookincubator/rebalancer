---
sidebar_position: 1
---

# Core Concepts Overview

Rebalancer provides a domain-specific language (DSL) for expressing assignment problems. Understanding these core concepts will help you model and solve complex problems effectively.

## The Assignment Problem Model

At its heart, Rebalancer solves problems of the form:

> **"Assign each object to exactly one container, subject to constraints, while optimizing objectives."**

This simple abstraction is surprisingly powerful and can model many real-world problems:

**Systems Engineering:**
- **Load balancing**: Assign tasks (objects) to servers (containers)
  - *Constraints*: CPU/memory capacity limits per server
  - *Objectives*: Balance load across servers, minimize movement

- **Database sharding**: Assign shards (objects) to hosts (containers)
  - *Constraints*: Storage capacity, at least 2 replicas per shard in different racks
  - *Objectives*: Balance storage and query load, ensure fault tolerance

**Operations & Logistics:**
- **Delivery routing**: Assign drivers (objects) to routes (containers)
  - *Constraints*: Time windows, vehicle capacity, driver hours
  - *Objectives*: Balance workload across drivers, minimize total distance

- **Warehouse task assignment**: Assign picking tasks (objects) to workers (containers)
  - *Constraints*: Worker skill levels, maximum tasks per worker
  - *Objectives*: Maximize throughput, balance workload

**Healthcare:**
- **Patient-bed assignment**: Assign patients (objects) to hospital beds (containers)
  - *Constraints*: Care requirements, isolation needs, bed availability
  - *Objectives*: Minimize patient transfers, balance nurse workload

**Education:**
- **Student-class assignment**: Assign students (objects) to courses (containers)
  - *Constraints*: Prerequisites, class size limits, time conflicts
  - *Objectives*: Maximize student preferences, balance class sizes

## Core Building Blocks

### 1. Objects and Containers

- **Objects**: Things that need to be assigned (tasks, shards, workloads)
- **Containers**: Places where objects go (servers, hosts, racks)

Every object is assigned to exactly one container. Containers can hold multiple objects.

**Learn more**: [Objects and Containers](objects-and-containers)

### 2. Dimensions

**Dimensions** represent resource types or properties:

- **Object dimensions**: CPU usage, memory requirement, network bandwidth
- **Container dimensions**: CPU capacity, memory capacity, network bandwidth

Dimensions let you express multi-resource problems: "This task needs 2 CPU and 4GB RAM, this host has 16 CPU and 64GB RAM."

**Learn more**: [Dimensions](dimensions)

### 3. Scopes

**Scopes** group containers for constraints and goals:

- `scope="host"`: Each individual host
- `scope="rack"`: Groups of hosts by rack
- `scope="datacenter"`: Groups of hosts by datacenter

Example: "No more than 100GB of memory used per rack" uses `scope="rack"`.

**Learn more**: [Scopes and Partitions](scopes-and-partitions)

### 4. Partitions (Groups)

**Partitions** group objects by some property:

- Group tasks by tenant
- Group shards by database
- Group containers by service

Example: "Keep shards from the same database on different hosts" uses partitions.

**Learn more**: [Scopes and Partitions](scopes-and-partitions)

### 5. Goals and Constraints

- **Goals**: Objectives to optimize (minimize imbalance, minimize movement)
- **Constraints**: Hard rules that must be satisfied (capacity limits, avoid assignments)

Goals are soft - Rebalancer tries to satisfy them as well as possible. Constraints are hard - the solution must satisfy them (or they're treated as high-priority goals if initially broken).

**Learn more**: [Goals vs Constraints](goals-vs-constraints)

## How These Fit Together

Here's a complete example showing all concepts:

### Problem: Database Shard Placement

**Objects**: 1000 database shards
**Containers**: 10 hosts across 2 datacenters
**Object Dimensions**: Each shard has memory_requirement and query_load
**Container Dimensions**: Each host has memory_capacity
**Scopes**:
- `host` scope (individual hosts)
- `datacenter` scope (groups of hosts)

**Partitions**: Shards grouped by `database_id`
**Goals**: Balance `query_load` across hosts
**Constraints**:
- Capacity: `memory_requirement` doesn't exceed `memory_capacity` per host
- Diversity: Each database has shards in both datacenters

### In Code

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
from rebalancer import ProblemSolver
from rebalancer.specs import (
    BalanceSpec,
    CapacitySpec,
    ConstraintSpec,
    GoalSpec,
    GroupCountSpec,
    Limit,
)

solver = ProblemSolver(service_name="shard-placement", service_scope="prod")

# Objects and containers
solver.set_object_name("shard")
solver.set_container_name("host")
solver.set_assignment(current_placement)

# Object dimensions
solver.add_object_dimension("memory_requirement", shard_memory)
solver.add_object_dimension("query_load", shard_qps)

# Container dimensions
solver.add_container_dimension("memory_capacity", host_memory)

# Scopes
solver.add_scope("datacenter", host_to_dc)

# Partitions
solver.add_partition("database", shard_to_database)

# Goals: balance query load
solver.add_goal(
    GoalSpec(
        balanceSpec=BalanceSpec(
            name="balance-qps", scope="host", dimension="query_load"
        )
    ),
    weight=1.0,
)

# Constraints: respect memory capacity
solver.add_constraint(
    ConstraintSpec(
        capacitySpec=CapacitySpec(
            name="memory-capacity",
            scope="host",
            dimension="memory_requirement",
        )
    )
)

# Constraints: diversity across datacenters
solver.add_constraint(
    ConstraintSpec(
        groupCountSpec=GroupCountSpec(
            name="datacenter-diversity",
            scope="datacenter",
            partitionName="database",
            limit=Limit(type="ABSOLUTE", globalLimit=1.0),
        )
    )
)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
ProblemSolver solver(executor, "shard-placement", "prod");

// Objects and containers
solver.setObjectName("shard");
solver.setContainerName("host");
solver.setAssignment(current_placement);

// Object dimensions
solver.addObjectDimension("memory_requirement", shard_memory);
solver.addObjectDimension("query_load", shard_qps);

// Container dimensions
solver.addContainerDimension("memory_capacity", host_memory);

// Scopes
solver.addScope("datacenter", host_to_dc);

// Partitions
solver.addPartition("database", shard_to_database);

// Goals: balance query load
BalanceSpec balanceQps;
balanceQps.name() = "balance-qps";
balanceQps.scope() = "host";
balanceQps.dimension() = "query_load";
solver.addGoal(balanceQps, 1.0);

// Constraints: respect memory capacity
CapacitySpec memCapacity;
memCapacity.name() = "memory-capacity";
memCapacity.scope() = "host";
memCapacity.dimension() = "memory_requirement";
solver.addConstraint(memCapacity);

// Constraints: diversity across datacenters
GroupCountSpec dcDiversity;
dcDiversity.name() = "datacenter-diversity";
dcDiversity.scope() = "datacenter";
dcDiversity.partition() = "database";
dcDiversity.limit() = Limit();
dcDiversity.limit()->globalLimit() = 1.0;
solver.addConstraint(dcDiversity);
```

</TabItem>
</Tabs>

## The Mental Model

When solving a problem with Rebalancer:

1. **Identify your assignment**: What goes where?
   - Objects = things to assign
   - Containers = places they go

2. **Define resources**: What matters?
   - Dimensions = resource types

3. **Structure the problem**: How are things organized?
   - Scopes = how containers are grouped
   - Partitions = how objects are grouped

4. **State requirements**: What must be true?
   - Constraints = hard rules
   - Goals = soft objectives

5. **Solve and inspect**: Let Rebalancer find the solution

## Visualization

```
┌─────────────────────────────────────────────────────┐
│ Problem                                             │
│                                                     │
│  Objects                          Containers        │
│  ┌──────┐                        ┌──────────┐     │
│  │ obj1 │────assigned to─────────│ container1│     │
│  └──────┘                        └──────────┘     │
│  ┌──────┐                        ┌──────────┐     │
│  │ obj2 │────assigned to─────────│ container2│     │
│  └──────┘                        └──────────┘     │
│     ↓ dimensions                      ↓ dimensions │
│  [cpu, mem]                       [capacity]       │
│     ↓ partitions                      ↓ scopes     │
│  [groups]                         [racks, DCs]     │
│                                                     │
│  Goals: minimize(imbalance)                        │
│  Constraints: capacity not exceeded                │
└─────────────────────────────────────────────────────┘
```

## Next Steps

Dive deeper into each concept:

1. [Objects and Containers](objects-and-containers) - The basics
2. [Dimensions](dimensions) - Multi-resource problems
3. [Scopes and Partitions](scopes-and-partitions) - Grouping and hierarchy
4. [Goals vs Constraints](goals-vs-constraints) - Optimization vs requirements
5. [Groups](groups) - Advanced object grouping

Or jump to practical examples in the [Cookbook](../cookbook/).
