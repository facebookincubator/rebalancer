---
sidebar_position: 25
---

# WorkingSetSpec

**Type**: Goal only

Optimize working set latency between coordinated endpoints.

## Overview

`WorkingSetSpec` minimizes the average latency of communication between groups of endpoints ("working units"). Each working unit defines a set of endpoints that need to coordinate, and the spec optimizes placement to minimize their communication latency.

**Use this when**: You have services or objects that need low-latency communication with each other, and you want to optimize their placement to minimize inter-endpoint latency.

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python" default>

```python file=algopt/rebalancer/examples/website/reference/misc/working_set_spec_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/misc/working_set_spec_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Unique identifier for this spec |
| `scope` | string | Yes | - | Scope level (e.g., `"host"`, `"datacenter"`) |
| `dimension` | string | Yes | - | Dimension to optimize |
| `workingUnits` | list&lt;WorkingUnit&gt; | Yes | - | Groups of coordinated endpoints |
| `metric` | WorkingSetMetric | No | AVG | Aggregation metric |

## WorkingUnit Structure

Each working unit defines a group of endpoints that coordinate:

| Field | Type | Description |
|-------|------|-------------|
| `endpoints` | list&lt;string&gt; | Object names that form this working unit |
| `weight` | double | Importance weight for this unit |

## Metrics

| Metric | Description |
|--------|-------------|
| **AVG** | Minimize average latency across working units (default) |

## Common Usage Patterns

### 1. Microservice Coordination

Minimize latency between coordinated microservices:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/misc/working_set_spec_examples.py start=microservice_coordination_start end=microservice_coordination_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/misc/working_set_spec_examples.cpp start=microservice_coordination_start end=microservice_coordination_end
```

</TabItem>
</Tabs>

### 2. Weighted Working Units

Use weights to prioritize critical coordination paths:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/misc/working_set_spec_examples.py start=weighted_units_start end=weighted_units_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/misc/working_set_spec_examples.cpp start=weighted_units_start end=weighted_units_end
```

</TabItem>
</Tabs>

## Performance Considerations

- **Complexity**: O(working units × endpoints²)
- **Memory**: Stores endpoint locations and latency matrix
- **Optimization**: Minimize number of working units

## Comparison with Related Specs

| Spec | Use When |
|------|----------|
| **WorkingSetSpec** | Optimize latency between coordinated endpoints |
| [PairAffinitiesSpec](../placement/pair-affinities) | Colocation preferences for object pairs |
| [ColocateGroupsSpec](../groups/colocate-groups) | Keep group members together |

## Troubleshooting

**Issue**: Latency not improving
- **Solution**: Verify endpoint names match object names and latency data is available

**Issue**: Performance slow with many endpoints
- **Solution**: Reduce number of working units or endpoints per unit

## Related Specs

- [PairAffinitiesSpec](../placement/pair-affinities) - Pair colocation
- [ColocateGroupsSpec](../groups/colocate-groups) - Group colocation
- [AssignmentAffinitiesSpec](../placement/assignment-affinities) - Object-container affinities

## Source Code

- **Thrift definition**: [`interface/thrift/ProblemSpecs.thrift` (line 1101)](https://github.com/facebookincubator/rebalancer/blob/main/interface/thrift/ProblemSpecs.thrift#L1101)
- **Implementation**: `solver/`
- **Tests**: `solver/tests/`

## Next Steps

- Review [Pair Affinities](../placement/pair-affinities) for simpler pair colocation
- See [Colocate Groups](../groups/colocate-groups) for group-based colocation
- Check [Cookbook: Affinity Placement](../../cookbook/affinity-placement) for real-world examples
