---
sidebar_position: 13
---

# DisasterRecoveryCapacitySpec

**Type**: Both (Constraint or Goal)

Ensure sufficient capacity remains after disaster scenarios where scope items fail together.

## Overview

`DisasterRecoveryCapacitySpec` models disaster recovery (DR) by ensuring that if a set of scope items fail simultaneously, the remaining infrastructure has enough capacity to absorb the failover load. It maps primary objects to their secondary (backup) objects and validates capacity under failure scenarios.

**Key concepts**:
- **Disaster groups**: Sets of scope items that fail together (e.g., rack failure, datacenter outage)
- **Primary/secondary objects**: Primary objects fail over to secondary objects on remaining hosts
- **Capacity validation**: Ensures remaining scope items can handle both their normal load + failover load

**Use this when**: You need to guarantee capacity for fault tolerance, high availability, or disaster recovery.

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Unique identifier for this constraint |
| `scope` | string | Yes | - | Scope at which disasters occur (e.g., `"rack"`, `"datacenter"`) |
| `dimension` | string | Yes | - | Object dimension to validate capacity for |
| `sharedDisasterGroups` | list\<set\<string\>\> | No | each scope item separate | Sets of scope items that fail together |
| `primaryToSetOfSecondaryObjects` | map\<string, list\<string\>\> | Yes | - | Primary objects → their secondary/backup objects |

## How It Works

For each disaster group (set of failing scope items):

1. **Identify failed primaries**: Find primary objects on failing scope items
2. **Activate secondaries**: Secondary objects for those primaries become active
3. **Calculate failover load**: Measure resource consumption of activated secondaries
4. **Validate capacity**: Check if surviving scope items can handle:
   - Their existing load (objects already there)
   - Failover load (newly activated secondaries)

**Constraint**: For all disaster groups, `surviving_capacity >= existing_load + failover_load`

**Example**:
```
Before disaster:
  rack1: [db_primary_0 (10 CPU)]
  rack2: [db_replica_0_a (0 CPU, standby)]
  rack3: [db_replica_0_b (0 CPU, standby)]

Disaster scenario: rack1 fails
  rack1: FAILED
  rack2: [db_replica_0_a (10 CPU, now active!)]
  rack3: [db_replica_0_b (0 CPU, still standby)]

Constraint: rack2 must have capacity for 10 CPU failover load
```

## Common Usage Patterns

### 1. Single Rack Failure

Ensure capacity after any single rack fails:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.py start=single_rack_failure_start end=single_rack_failure_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.cpp start=single_rack_failure_start end=single_rack_failure_end
```

</TabItem>
</Tabs>

### 2. Datacenter Failure

Ensure capacity after entire datacenter fails:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.py start=datacenter_failure_start end=datacenter_failure_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.cpp start=datacenter_failure_start end=datacenter_failure_end
```

</TabItem>
</Tabs>

### 3. Availability Zone Failure

Cloud provider AZ failure tolerance:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.py start=availability_zone_failure_start end=availability_zone_failure_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.cpp start=availability_zone_failure_start end=availability_zone_failure_end
```

</TabItem>
</Tabs>

### 4. Multi-Replica Failover

Multiple replica promotion levels:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.py start=multi_replica_failover_start end=multi_replica_failover_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.cpp start=multi_replica_failover_start end=multi_replica_failover_end
```

</TabItem>
</Tabs>

### 5. Partial Failure Groups

Some scope items share disaster fate:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.py start=partial_failure_groups_start end=partial_failure_groups_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.cpp start=partial_failure_groups_start end=partial_failure_groups_end
```

</TabItem>
</Tabs>

### 6. No Shared Disaster Groups (Default)

Each scope item fails independently:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.py start=no_shared_disaster_groups_start end=no_shared_disaster_groups_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.cpp start=no_shared_disaster_groups_start end=no_shared_disaster_groups_end
```

</TabItem>
</Tabs>

### 7. Cross-Dimension DR

Different dimensions for different failure modes:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.py start=cross_dimension_dr_start end=cross_dimension_dr_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.cpp start=cross_dimension_dr_start end=cross_dimension_dr_end
```

</TabItem>
</Tabs>

### 8. Asymmetric Replica Distribution

Different numbers of replicas per primary:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.py start=asymmetric_replica_distribution_start end=asymmetric_replica_distribution_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.cpp start=asymmetric_replica_distribution_start end=asymmetric_replica_distribution_end
```

</TabItem>
</Tabs>

## Disaster Group Semantics

### Single Scope Item per Group

```python
# Each rack fails independently
sharedDisasterGroups = [
    {"rack1"},
    {"rack2"},
    {"rack3"},
]
# Tests: rack1 fails, rack2 fails, rack3 fails (3 scenarios)
```

### Multiple Scope Items per Group

```python
# Racks fail together (e.g., shared power)
sharedDisasterGroups = [
    {"rack1", "rack2"},   # Both fail
    {"rack3", "rack4"},   # Both fail
]
# Tests: {rack1, rack2} fail, {rack3, rack4} fail (2 scenarios)
```

### Overlapping Groups Not Allowed

```python
# INVALID: rack1 appears in multiple groups
sharedDisasterGroups = [
    {"rack1", "rack2"},
    {"rack1", "rack3"},  # Error: rack1 already in group
]
```

## Performance Considerations

- **Impact**: Moderate to High - depends on number of disaster groups and replica mapping complexity
- **Solver compatibility**: Works with both Local Search and Optimal solvers
- **Scaling**: O(disaster groups × scope items × objects)
- **Large replica maps**: Can significantly increase problem size

## Common Pitfalls

### 1. Insufficient Capacity for Failover

**Problem**: Not enough capacity to handle failover load.

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.py start=pitfall_insufficient_capacity_bad_start end=pitfall_insufficient_capacity_bad_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.cpp start=pitfall_insufficient_capacity_bad_start end=pitfall_insufficient_capacity_bad_end
```

</TabItem>
</Tabs>

**Solution**: Ensure N+1 or N+2 capacity:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.py start=pitfall_insufficient_capacity_good_start end=pitfall_insufficient_capacity_good_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.cpp start=pitfall_insufficient_capacity_good_start end=pitfall_insufficient_capacity_good_end
```

</TabItem>
</Tabs>

### 2. Primary-Secondary Mapping Missing

**Problem**: Primary objects without secondaries.

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.py start=pitfall_missing_mapping_bad_start end=pitfall_missing_mapping_bad_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.cpp start=pitfall_missing_mapping_bad_start end=pitfall_missing_mapping_bad_end
```

</TabItem>
</Tabs>

**Solution**: Ensure all primaries have secondaries:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.py start=pitfall_missing_mapping_good_start end=pitfall_missing_mapping_good_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.cpp start=pitfall_missing_mapping_good_start end=pitfall_missing_mapping_good_end
```

</TabItem>
</Tabs>

### 3. Secondaries on Same Disaster Group as Primary

**Problem**: Primary and secondary in same failure domain.

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.py start=pitfall_same_disaster_group_bad_start end=pitfall_same_disaster_group_bad_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.cpp start=pitfall_same_disaster_group_bad_start end=pitfall_same_disaster_group_bad_end
```

</TabItem>
</Tabs>

**Solution**: Use anti-affinity to spread primaries and secondaries:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.py start=pitfall_same_disaster_group_good_start end=pitfall_same_disaster_group_good_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.cpp start=pitfall_same_disaster_group_good_start end=pitfall_same_disaster_group_good_end
```

</TabItem>
</Tabs>

### 4. Shared Disaster Groups Not Defined Correctly

**Problem**: Misunderstanding how disaster groups work.

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.py start=pitfall_disaster_groups_confusion_bad_start end=pitfall_disaster_groups_confusion_bad_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.cpp start=pitfall_disaster_groups_confusion_bad_start end=pitfall_disaster_groups_confusion_bad_end
```

</TabItem>
</Tabs>

**Solution**: Disaster groups are "OR" scenarios, not "AND":

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.py start=pitfall_disaster_groups_confusion_good_start end=pitfall_disaster_groups_confusion_good_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.cpp start=pitfall_disaster_groups_confusion_good_start end=pitfall_disaster_groups_confusion_good_end
```

</TabItem>
</Tabs>

### 5. Dimension Mismatch

**Problem**: Dimension doesn't exist or isn't relevant.

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.py start=pitfall_dimension_mismatch_bad_start end=pitfall_dimension_mismatch_bad_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.cpp start=pitfall_dimension_mismatch_bad_start end=pitfall_dimension_mismatch_bad_end
```

</TabItem>
</Tabs>

**Solution**: Use resource dimensions (CPU, memory, network):

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.py start=pitfall_dimension_mismatch_good_start end=pitfall_dimension_mismatch_good_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.cpp start=pitfall_dimension_mismatch_good_start end=pitfall_dimension_mismatch_good_end
```

</TabItem>
</Tabs>

## Combining with Other Specs

### With CapacitySpec

Normal capacity + DR capacity:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.py start=combining_with_capacity_spec_start end=combining_with_capacity_spec_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.cpp start=combining_with_capacity_spec_start end=combining_with_capacity_spec_end
```

</TabItem>
</Tabs>

### With GroupCountSpec

Spread replicas + DR capacity:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.py start=combining_with_group_count_spec_start end=combining_with_group_count_spec_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.cpp start=combining_with_group_count_spec_start end=combining_with_group_count_spec_end
```

</TabItem>
</Tabs>

### With BalanceSpec

Balance load + ensure DR capacity:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.py start=combining_with_balance_spec_start end=combining_with_balance_spec_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.cpp start=combining_with_balance_spec_start end=combining_with_balance_spec_end
```

</TabItem>
</Tabs>

## Verification Example

Verify DR capacity requirements:

```python file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.py start=verification_function_start end=verification_function_end
```

## DR Coverage Analysis

Analyze failover coverage:

```python file=algopt/rebalancer/examples/website/reference/disaster_recovery_capacity/dr_capacity_spec_examples.py start=coverage_analysis_function_start end=coverage_analysis_function_end
```

## Related Specs

- [CapacitySpec](capacity) - Basic capacity limits
- [GroupCountSpec](groups/group-count) - Spread replicas across failure domains
- [BalanceSpec](balance-optimize/balance) - Balance load

## Source Code

- Thrift definition: `interface/thrift/ProblemSpecs.thrift:449` (DisasterRecoveryCapacitySpec)
- Implementation: `solver/constraints/DisasterRecoveryCapacity.cpp`
