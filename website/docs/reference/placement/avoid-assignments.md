---
sidebar_position: 14
---

# AvoidAssignmentsSpec

**Type**: Constraint only

Block specific object-to-container assignments.

## Overview

`AvoidAssignmentsSpec` prevents specific objects from being placed on specific containers. Unlike AvoidMovingSpec which pins objects to their current location, this spec blocks particular assignments regardless of current placement.

**Use this when**: You need to prevent certain objects from going to certain containers due to incompatibility, policy restrictions, or operational constraints.

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/placement/avoid_assignments_spec_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/placement/avoid_assignments_spec_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Unique identifier for this constraint |
| `scope` | string | Yes | - | Scope level (e.g., `"host"`, `"rack"`) |
| `assignments` | list&lt;AvoidAssignment&gt; | Yes | - | List of blocked assignments |

### AvoidAssignment Structure

| Field | Type | Description |
|-------|------|-------------|
| `object` | string | Object ID that should avoid certain containers |
| `scopeItems` | list&lt;string&gt; | List of container IDs to avoid |

## Common Usage Patterns

### 1. Hardware Incompatibility

Prevent workloads from incompatible hardware:

```python
# GPU workloads can't run on CPU-only hosts
cpu_only_hosts = ["host1", "host2", "host5"]
gpu_workloads = ["gpu_task_1", "gpu_task_2", "gpu_task_3"]

assignments = [
    AvoidAssignment(object=workload, scopeItems=cpu_only_hosts)
    for workload in gpu_workloads
]

solver.add_constraint(
        ConstraintSpec(
            avoidAssignmentsSpec=AvoidAssignmentsSpec(
        name="gpu-hardware-requirement",
        scope="host",
        assignments=assignments,
    )
        )
)
```

### 2. Compliance and Isolation

Enforce data residency or compliance requirements:

```python
# EU data can't go to US datacenters
eu_data_objects = ["eu_customer_db_1", "eu_customer_db_2"]
us_datacenters = ["us_east_dc", "us_west_dc"]

assignments = [
    AvoidAssignment(object=obj, scopeItems=us_datacenters)
    for obj in eu_data_objects
]

solver.add_constraint(
        ConstraintSpec(
            avoidAssignmentsSpec=AvoidAssignmentsSpec(
        name="eu-data-residency",
        scope="datacenter",
        assignments=assignments,
    )
        )
)
```

### 3. Tenant Isolation

Prevent specific tenants from sharing hosts:

```python
# Tenant A and Tenant B can't share hosts
tenant_a_objects = ["tenant_a_vm_1", "tenant_a_vm_2"]
tenant_b_hosts = get_hosts_with_tenant_b()

assignments = [
    AvoidAssignment(object=vm, scopeItems=tenant_b_hosts)
    for vm in tenant_a_objects
]

solver.add_constraint(
        ConstraintSpec(
            avoidAssignmentsSpec=AvoidAssignmentsSpec(
        name="tenant-isolation",
        scope="host",
        assignments=assignments,
    )
        )
)
```

### 4. Maintenance and Failures

Keep objects off hosts under maintenance or with failures:

```python
# Don't place any new objects on hosts under maintenance
maintenance_hosts = ["host3", "host7", "host12"]
all_objects = list(current_assignment.keys())

# Only restrict objects not already on these hosts
new_assignments = []
for obj in all_objects:
    # Find current host
    current_host = None
    for host, objects in current_assignment.items():
        if obj in objects:
            current_host = host
            break

    # If not currently on maintenance host, prevent placement there
    if current_host not in maintenance_hosts:
        new_assignments.append(
            AvoidAssignment(object=obj, scopeItems=maintenance_hosts)
        )

solver.add_constraint(
        ConstraintSpec(
            avoidAssignmentsSpec=AvoidAssignmentsSpec(
        name="avoid-maintenance-hosts",
        scope="host",
        assignments=new_assignments,
    )
        )
)
```

### 5. License Restrictions

Respect software licensing constraints:

```python
# Licensed software can only run on licensed hosts
licensed_hosts = ["host1", "host2", "host3"]
all_hosts = [f"host{i}" for i in range(20)]
unlicensed_hosts = [h for h in all_hosts if h not in licensed_hosts]

licensed_workloads = ["oracle_db", "matlab_worker", "proprietary_app"]

assignments = [
    AvoidAssignment(object=workload, scopeItems=unlicensed_hosts)
    for workload in licensed_workloads
]

solver.add_constraint(
        ConstraintSpec(
            avoidAssignmentsSpec=AvoidAssignmentsSpec(
        name="software-license-enforcement",
        scope="host",
        assignments=assignments,
    )
        )
)
```

### 6. Affinity by Negation

Implement affinity by blocking all other options:

```python
# Object must go to specific hosts (implemented via avoidance)
required_object = "critical_service"
preferred_hosts = ["host1", "host2"]
all_hosts = [f"host{i}" for i in range(20)]
disallowed_hosts = [h for h in all_hosts if h not in preferred_hosts]

solver.add_constraint(
        ConstraintSpec(
            avoidAssignmentsSpec=AvoidAssignmentsSpec(
        name="force-placement-via-avoidance",
        scope="host",
        assignments=[
            AvoidAssignment(
                object=required_object,
                scopeItems=disallowed_hosts,
            )
        ],
    )
        )
)
```

## How It Works

For each `AvoidAssignment` in the `assignments` list:
- Solver ensures the specified `object` is never placed on any of the `scopeItems`
- If solution requires these placements, solver fails with infeasibility error

This is a **hard constraint** - there's no partial satisfaction.

## Difference from Other Specs

| Aspect | AvoidAssignmentsSpec | AvoidMovingSpec | NonAcceptingSpec |
|--------|---------------------|-----------------|------------------|
| **Blocks** | Specific object→container pairs | Any movement of objects | All objects from containers |
| **Granularity** | Per-object, per-container | Per-object only | Per-container only |
| **Current location** | Irrelevant | Must keep current | Not relevant |
| **Use case** | Incompatibility | Operational stability | Drain/maintenance |

## Performance Considerations

- **Impact**: Minimal - simple constraint check
- **Solver compatibility**: Works with both Local Search and Optimal solvers
- **Scaling**: O(number of blocked assignments)
- **Search space**: Reduces solution space (can help or hurt)

## Common Pitfalls

### 1. Over-Constraining

**Problem**: Blocking too many assignments makes problem infeasible.

```python
# BAD: Block almost all possible placements
assignments = [
    AvoidAssignment(object=obj, scopeItems=most_hosts)
    for obj in all_objects
]
# May have no valid solution!
```

**Solution**: Only block truly incompatible assignments.

### 2. Conflicting with Other Constraints

**Problem**: AvoidAssignments + Capacity constraints may conflict.

```python
# Block all big hosts for large objects
solver.add_constraint(
        ConstraintSpec(
            avoidAssignmentsSpec=AvoidAssignmentsSpec(
        assignments=[
            AvoidAssignment(object="large_obj", scopeItems=big_hosts)
        ]
    )
        )
)
# But small hosts don't have capacity!
# Result: Infeasible
```

**Solution**: Verify remaining hosts have sufficient capacity.

### 3. Wrong Scope Level

**Problem**: Blocking at wrong scope level.

```python
# BAD: Object is "vm1", but blocking rack names
solver.add_constraint(
        ConstraintSpec(
            avoidAssignmentsSpec=AvoidAssignmentsSpec(
        scope="host",  # Scope is host
        assignments=[
            AvoidAssignment(
                object="vm1",
                scopeItems=["rack1", "rack2"],  # But these are racks!
            )
        ],
    )
        )
)
# Doesn't work - scope mismatch
```

**Solution**: Ensure `scopeItems` match the specified `scope` level.

### 4. Forgetting Current Placement

**Problem**: Blocking object's current location causes immediate infeasibility.

```python
# Object currently on host1
solver.add_constraint(
        ConstraintSpec(
            avoidAssignmentsSpec=AvoidAssignmentsSpec(
        assignments=[
            AvoidAssignment(
                object="obj1",
                scopeItems=["host1"],  # But obj1 is already here!
            )
        ],
    )
        )
)
# Infeasible if AvoidMoving also applied
```

**Solution**: Check current placement before adding avoid assignments.

### 5. Using When You Want Affinity

**Problem**: Using avoidance to implement affinity is inefficient.

```python
# BAD: Block 99 hosts to force placement on 1 host
solver.add_constraint(
        ConstraintSpec(
            avoidAssignmentsSpec=AvoidAssignmentsSpec(
        assignments=[
            AvoidAssignment(object="obj", scopeItems=all_other_hosts)
        ]
    )
        )
)
```

**Solution**: Use AssignmentAffinitiesSpec instead for affinity (when available).

## Combining with Other Constraints

### With CapacitySpec

```python
# Ensure avoided containers don't create capacity issues

# Block incompatible assignments
solver.add_constraint(
        ConstraintSpec(
            avoidAssignmentsSpec=AvoidAssignmentsSpec(
        name="hardware-restrictions",
        scope="host",
        assignments=gpu_restrictions,
    )
        )
)

# Ensure remaining hosts have capacity
solver.add_constraint(
        ConstraintSpec(
            capacitySpec=CapacitySpec(
        name="capacity",
        scope="host",
        dimension="memory",
    )
        )
)
```

### With AvoidMovingSpec

```python
# Pin some objects, block others from certain locations

solver.add_constraint(
        ConstraintSpec(
            avoidMovingSpec=AvoidMovingSpec(
        name="pin-stable-objects",
        objects=["stable_obj_1", "stable_obj_2"],
    )
        )
)

solver.add_constraint(
        ConstraintSpec(
            avoidAssignmentsSpec=AvoidAssignmentsSpec(
        name="block-incompatible-placements",
        scope="host",
        assignments=[
            AvoidAssignment(
                object="gpu_workload",
                scopeItems=cpu_only_hosts,
            )
        ],
    )
        )
)
```

## Dynamic Avoidance Example

Build avoidance list based on runtime state:

```python
def build_dynamic_avoidance(current_assignment, host_states):
    """Build avoidance list based on current host states."""
    assignments = []

    for obj in all_objects:
        avoid_hosts = []

        # Check each host for compatibility
        for host, state in host_states.items():
            # Avoid hosts under maintenance
            if state.get("maintenance"):
                avoid_hosts.append(host)

            # Avoid hosts with wrong hardware
            if obj.requires_gpu and not state.get("has_gpu"):
                avoid_hosts.append(host)

            # Avoid hosts in wrong region for compliance
            if obj.region != state.get("region"):
                avoid_hosts.append(host)

        if avoid_hosts:
            assignments.append(
                AvoidAssignment(object=obj, scopeItems=avoid_hosts)
            )

    return AvoidAssignmentsSpec(
        name="dynamic-avoidance",
        scope="host",
        assignments=assignments,
    )
```

## Related Specs

- [AvoidMovingSpec](../movement/avoid-moving) - Pin objects to current location
- [NonAcceptingSpec](../placement/non-accepting) - Block all objects from containers
- [AssignmentAffinitiesSpec](assignment-affinities) - Prefer specific assignments (opposite)

## Source Code

- Thrift definition: `interface/thrift/ProblemSpecs.thrift:149`
- Implementation: `solver/constraints/AvoidAssignments.cpp`
