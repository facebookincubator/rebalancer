---
sidebar_position: 2
---

# Objects and Containers

Objects and containers are the fundamental building blocks of any Rebalancer problem. Understanding them is essential for modeling your assignment problem.

## The Assignment Model

At its core, Rebalancer solves a simple question:

> **"Which container should each object be assigned to?"**

- **Objects** are things that need to be assigned
- **Containers** are places where objects go
- Each object goes to exactly one container
- Each container can hold multiple objects

## Objects: Things to Assign

Objects represent the entities you want to place or distribute.

### Examples of Objects

| Domain | Objects |
|--------|---------|
| **Systems Engineering** | Tasks, jobs, processes, workers |
| | Shards, partitions, replicas |
| | Docker containers, pods, VMs |
| **Operations & Logistics** | Delivery routes, packages, shipments |
| | Warehouse tasks, picking orders |
| **Healthcare** | Patients, appointments, procedures |
| | Medical equipment, supplies |
| **Education** | Students, course enrollments |
| | Exams, assignments |

### Defining Objects

Objects are identified by unique string names. You define them when setting the initial assignment.

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
solver = ProblemSolver(service_name="example", service_scope="demo")

# Set what objects are called (for logging)
solver.set_object_name("task")

# Define initial assignment (implicitly defines objects)
solver.set_assignment({
    "server1": ["task0", "task1", "task2"],
    "server2": ["task3", "task4"],
    "server3": []
})
# Objects: task0, task1, task2, task3, task4
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
ProblemSolver solver(executor, "example", "demo");

solver.setObjectName("task");

std::map<std::string, std::vector<std::string>> assignment;
assignment["server1"] = {"task0", "task1", "task2"};
assignment["server2"] = {"task3", "task4"};
assignment["server3"] = {};

solver.setAssignment(assignment);
```

</TabItem>
</Tabs>

## Containers: Places for Objects

Containers represent the resources or locations that hold objects.

### Examples of Containers

| Domain | Containers |
|--------|------------|
| **Systems Engineering** | Servers, hosts, machines, cores |
| | Database hosts, storage nodes |
| | Physical servers, VMs, Kubernetes nodes |
| **Operations & Logistics** | Delivery drivers, vehicles, trucks |
| | Warehouse workers, zones, shelves |
| **Healthcare** | Hospital beds, rooms, wards |
| | Nurses, shifts, operating rooms |
| **Education** | Classrooms, courses, sections |
| | Teachers, time slots, exam rooms |

### Defining Containers

Containers are also identified by unique string names. They're defined implicitly by the initial assignment.

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
# Set what containers are called (for logging)
solver.set_container_name("server")

# Containers are defined by the keys in the assignment
solver.set_assignment({
    "server1": ["task0", "task1"],
    "server2": ["task2"],
    "server3": []  # Empty containers are included
})
# Containers: server1, server2, server3
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
solver.setContainerName("server");

std::map<std::string, std::vector<std::string>> assignment;
assignment["server1"] = {"task0", "task1"};
assignment["server2"] = {"task2"};
assignment["server3"] = {};

solver.setAssignment(assignment);
```

</TabItem>
</Tabs>

## Object and Container Types

By default, any object can be assigned to any container. However, you can restrict this using assignment constraints.

### Type Matching

Objects and containers can have incompatible characteristics (e.g., GPU tasks can only run on GPU servers). Rebalancer doesn't have a built-in type system, but you can enforce type constraints using `AvoidAssignmentsSpec`.

**Example**: GPU tasks can only run on GPU servers.

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
# Define your objects and containers
solver.set_assignment({
    "cpu_server_1": ["cpu_task_0", "cpu_task_1"],
    "cpu_server_2": ["cpu_task_2"],
    "gpu_server_1": ["gpu_task_0"],
    "gpu_server_2": [],
})

# Prevent GPU tasks from being assigned to CPU servers
solver.add_constraint(
    {
        "avoidAssignmentsSpec": {
            "name": "gpu-compatibility",
            "scope": "host",
            "assignments": [
                # GPU task 0 cannot go to CPU servers
                {
                    "object": "gpu_task_0",
                    "scopeItems": ["cpu_server_1", "cpu_server_2"],
                },
                # Add more as needed for other GPU tasks
            ],
        }
    }
)

# Similarly, you could prevent CPU tasks from wasting GPU resources
# by adding constraints to avoid assigning cpu_task_* to GPU servers
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
#include <rebalancer/interface/thrift/v2/ProblemSpecs/gen-cpp2/ProblemSpecs_types.h>

// Prevent GPU tasks from being assigned to CPU servers
AvoidAssignmentsSpec gpuCompatibility;
gpuCompatibility.name() = "gpu-compatibility";
gpuCompatibility.scope() = "host";

AvoidAssignment assignment;
assignment.object() = "gpu_task_0";
assignment.scopeItems() = {"cpu_server_1", "cpu_server_2"};

gpuCompatibility.assignments() = {assignment};
solver.addConstraint(gpuCompatibility);
```

</TabItem>
</Tabs>

**Alternative approach using dimensions**: You can also use binary dimensions (0/1) with capacity constraints:

```python
# Mark which servers have GPUs
solver.add_container_dimension("has_gpu", {
    "cpu_server_1": 0.0,
    "cpu_server_2": 0.0,
    "gpu_server_1": 1.0,
    "gpu_server_2": 1.0,
})

# Mark which tasks require GPUs
solver.add_object_dimension("requires_gpu", {
    "gpu_task_0": 1.0,
    "cpu_task_0": 0.0,
    "cpu_task_1": 0.0,
    "cpu_task_2": 0.0,
})

# Then use AvoidAssignmentsSpec or other constraints to enforce the rule
# Note: There's no built-in "if requires_gpu=1 then must assign to has_gpu=1"
# You still need to enumerate incompatible assignments
```

## Initially Unassigned Objects

Objects don't have to be assigned initially. Unassigned objects will be placed by the solver.

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
# Method 1: Empty string container name for unassigned
solver.set_assignment({
    "server1": ["task0", "task1"],
    "": ["task2", "task3"],  # Unassigned objects
})

# Method 2: Just don't include them
# If you add objects via dimensions but don't assign them,
# they're automatically unassigned
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
std::map<std::string, std::vector<std::string>> assignment;
assignment["server1"] = {"task0", "task1"};
assignment[""] = {"task2", "task3"};  // Unassigned

solver.setAssignment(assignment);
```

</TabItem>
</Tabs>

**Use case**: Adding new objects that need initial placement.

## Fixed vs. Movable Objects

By default, all objects can move. You can pin objects to their current containers.

### Prevent All Movement

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
# Make specific objects unmovable
solver.add_constraint(
    {
        "avoidMovingSpec": {
            "name": "pin-objects",
            "objects": ["task0", "task1"],  # These cannot move
        }
    }
)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Make specific objects unmovable
AvoidMovingSpec pinObjects;
pinObjects.name() = "pin-objects";
pinObjects.objects() = {"task0", "task1"};
solver.addConstraint(pinObjects);
```

</TabItem>
</Tabs>

### Restrict Moving Once

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
# Objects can move at most once
solver.enable_restrict_moving_object_only_once()
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Objects can move at most once
solver.enableRestrictMovingObjectOnlyOnce();
```

</TabItem>
</Tabs>

**Use case**: Minimize disruption by limiting which objects can move.

## Object Naming Conventions

### Best Practices

1. **Use meaningful IDs**: `shard_123`, not `obj_123`
2. **Include relevant info**: `db_prod_shard_5`, `task_batch_worker_0`
3. **Be consistent**: Use the same naming scheme throughout
4. **Avoid special chars**: Stick to alphanumeric and `_`, `-`

### Examples

Good:
- `payment_shard_0001`
- `user_db_replica_west_1`
- `worker_task_cpu_intensive_5`

Less good:
- `obj1` (not descriptive)
- `shard/123` (special chars may cause issues)
- Inconsistent naming across objects

## Container Naming Conventions

### Best Practices

1. **Include location**: `host_dc1_rack3_server5`
2. **Be specific**: `gpu_server_8`, not just `server_8`
3. **Hierarchical**: Embed hierarchy in names when useful
4. **Consistent**: Same format for all containers

### Examples

Good:
- `host_us_west_rack12_node03`
- `gpu_server_prod_15`
- `cache_edge_london_1`

Less good:
- `h1` (not descriptive)
- Mixing formats: `host_1`, `server_two`, `srv03`

## Object and Container Properties

Objects and containers can have properties called **dimensions**.

### Object Dimensions

Properties of objects:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
# CPU usage per task
solver.add_object_dimension("cpu", {
    "task0": 2.0,  # Uses 2 CPU cores
    "task1": 1.5,
    "task2": 0.5
})

# Memory requirement per task
solver.add_object_dimension("memory", {
    "task0": 4.0,  # Requires 4GB
    "task1": 2.0,
    "task2": 1.0
})
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// CPU usage per task
std::map<std::string, double> cpu_usage = {
    {"task0", 2.0},
    {"task1", 1.5},
    {"task2", 0.5}
};
solver.addObjectDimension("cpu", cpu_usage);

// Memory requirement per task
std::map<std::string, double> memory_req = {
    {"task0", 4.0},
    {"task1", 2.0},
    {"task2", 1.0}
};
solver.addObjectDimension("memory", memory_req);
```

</TabItem>
</Tabs>

### Container Dimensions

Properties of containers:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python
# CPU capacity per server
solver.add_container_dimension("cpu_capacity", {
    "server1": 8.0,  # Has 8 CPU cores
    "server2": 16.0,
    "server3": 8.0
})

# Memory capacity per server
solver.add_container_dimension("memory_capacity", {
    "server1": 32.0,  # Has 32GB RAM
    "server2": 64.0,
    "server3": 32.0
})
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// CPU capacity per server
std::map<std::string, double> cpu_capacity = {
    {"server1", 8.0},
    {"server2", 16.0},
    {"server3", 8.0}
};
solver.addContainerDimension("cpu_capacity", cpu_capacity);

// Memory capacity per server
std::map<std::string, double> memory_capacity = {
    {"server1", 32.0},
    {"server2", 64.0},
    {"server3", 32.0}
};
solver.addContainerDimension("memory_capacity", memory_capacity);
```

</TabItem>
</Tabs>

**Learn more**: [Dimensions](dimensions)

## Common Patterns

### Pattern 1: Homogeneous Containers

All containers are identical (same capacity):

```python
# All servers have 16 cores
num_servers = 10
solver.add_container_dimension(
    "cpu_capacity",
    {f"server{i}": 16.0 for i in range(num_servers)}
)
```

### Pattern 2: Heterogeneous Containers

Containers have different capacities:

```python
# Mix of small and large servers
solver.add_container_dimension("cpu_capacity", {
    "small_server_0": 4.0,
    "small_server_1": 4.0,
    "large_server_0": 32.0,
    "large_server_1": 32.0,
})
```

### Pattern 3: Reserved Containers

Some containers are reserved and shouldn't accept new objects:

```python
solver.add_constraint(
    {
        "nonAcceptingSpec": {
            "name": "reserved-servers",
            "scope": "host",
            "items": ["server_reserved_1", "server_reserved_2"],
        }
    }
)
```

### Pattern 4: Draining Containers

Empty specific containers (move all objects out):

```python
# Note: ``dimension`` is optional; when omitted, ``{object}_count`` is used.
# Example assumes set_object_name("task") was called.
solver.add_constraint(
    {
        "toFreeSpec": {
            "name": "drain-servers",
            "containers": ["server_to_drain_1", "server_to_drain_2"],
            "dimension": "task_count",
        }
    }
)
```

## Automatic Dimensions

Rebalancer automatically creates some dimensions:

### `{objectName}_count`

Count of objects per container. The dimension name is based on what you set with `setObjectName()`:

```python
# If you set:
solver.set_object_name("task")

# Then you can use:
solver.add_goal(
    {
        "balanceSpec": {
            "name": "balance-count",
            "scope": "host",
            "dimension": "task_count",  # Matches set_object_name("task")
        }
    },
    weight=1.0,
)

# Other examples:
# set_object_name("shard") → use "shard_count"
# set_object_name("object") → use "object_count"
```

### Task-Specific Dimensions

When you add an object dimension, corresponding container aggregations are created:

```python
# Define object CPU usage
solver.add_object_dimension("cpu", cpu_per_task)

# Rebalancer automatically sums to create container-level "cpu" utilization
# Now you can use "cpu" in specs:
solver.add_constraint(
    {"capacitySpec": {"name": "cpu-cap", "scope": "host", "dimension": "cpu"}}
)
```

## Nesting and Recursion

Can objects be containers? **Yes!**

**Example**: In container orchestration:
- VMs are containers for Docker containers
- But VMs themselves are objects assigned to physical hosts

```python
# Problem 1: Assign VMs to physical hosts
solver1 = ProblemSolver(service_name="placement", service_scope="vms")
solver1.set_object_name("vm")
solver1.set_container_name("physical_host")

# Problem 2: Assign containers to VMs
solver2 = ProblemSolver(service_name="placement", service_scope="containers")
solver2.set_object_name("container")
solver2.set_container_name("vm")
```

You'd solve these as two separate problems, possibly using the output of one as constraints for the other.

## Troubleshooting

### Problem: Object not found

**Error**: "Object 'task5' not defined"

**Cause**: Object not in initial assignment

**Solution**: Add to initial assignment, even if unassigned:
```python
assignment[""] = ["task5"]  # Unassigned objects
```

### Problem: Container not found

**Error**: "Container 'server3' not defined"

**Cause**: Container not in initial assignment keys

**Solution**: Add as empty container:
```python
assignment["server3"] = []
```

### Problem: Type mismatch

**Error**: GPU task assigned to CPU-only server

**Cause**: No type constraints defined

**Solution**: Use AvoidAssignmentsSpec to prevent incompatible assignments:
```python
# Prevent gpu_task from being assigned to cpu_server
solver.add_constraint(
    {
        "avoidAssignmentsSpec": {
            "name": "gpu-compatibility",
            "scope": "host",
            "assignments": [
                {
                    "object": "gpu_task_0",
                    "scopeItems": ["cpu_server_1", "cpu_server_2"],
                }
            ],
        }
    }
)
```

## Next Steps

- Learn about [Dimensions](dimensions) to add properties to objects and containers
- Understand [Scopes and Partitions](scopes-and-partitions) for grouping
- See [Cookbook examples](../cookbook/) for complete problem modeling

## Related Concepts

- [Dimensions](dimensions) - Properties of objects and containers
- [Scopes](scopes-and-partitions) - Grouping containers
- [Partitions](scopes-and-partitions) - Grouping objects
- [AvoidAssignmentsSpec](../reference/placement/avoid-assignments) - Prevent specific assignments
- [AvoidMovingSpec](../reference/movement/avoid-moving) - Pin objects in place
