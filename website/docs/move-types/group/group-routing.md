---
sidebar_position: 3
---

# GroupRouting

**Move Type**: Group
**Complexity**: O(groups × routing_rings) - routing-aware
**Primary Use**: Latency-optimized group placement with routing awareness

Move groups of objects based on **routing configuration** and **latency awareness**. Places objects to minimize routing latency from sources to destinations.

## Overview

`GroupRouting` (also known as `GROUP_ROUTING`) is a specialized move type that evaluates move sets for every group based on a routing configuration. For each group and each source in its routing rings, it attempts to place objects in minimum-latency destinations from the source.

**Use when**:
- Have routing configuration defined
- Optimizing for network latency
- Objects organized in groups with routing requirements
- Need routing-aware placement
- Have origin-to-destination latency tables

**Avoid when**:
- No routing configuration available
- Latency not a concern
- Simple placement without routing awareness
- No group structure

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/group_routing_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/group_routing_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `routingConfigName` | string | **Yes** | null | Name of routing configuration |
| `unassignedContainer` | string | No | null | Container for unassigned objects |

### Parameter Details

**routingConfigName**:
- Name of the routing configuration to use
- References a `RoutingLatencySpec` defined in the problem
- Specifies routing rings, latency tables, and routing policy
- Required for this move type to function

**unassignedContainer**:
- Optional container name for initially unassigned objects
- Enables moving objects from unassigned state
- If not specified, only moves already-assigned objects

## How It Works

For each **group** in the partition:

1. **Identify routing rings**: Determine routing rings for this group from config
2. **For each source**: For each source in the routing rings:
   - **Find min-latency destination**: Identify destination with minimum latency from this source
   - **Generate move**: Create move to place object at min-latency destination
3. **Evaluate move set**: Test the complete move set for this group
4. **Repeat for all groups**: Process all groups in partition
5. **Select best**: Choose the move set that improves objective most

### Visual Example

```
Routing Configuration: latency-aware placement

Group1 (routing rings: source1, source2):
  source1 → destination_A (latency: 5ms)  ✓ Best
  source1 → destination_B (latency: 20ms)
  source2 → destination_C (latency: 3ms)  ✓ Best
  source2 → destination_D (latency: 15ms)

Group2 (routing rings: source3, source4):
  source3 → destination_A (latency: 10ms) ✓ Best
  source3 → destination_C (latency: 25ms)
  source4 → destination_B (latency: 8ms)  ✓ Best
  source4 → destination_D (latency: 30ms)

For each group, place objects in min-latency destinations
```

## Complexity

**Per iteration**: O(G × R)

Where:
- G = number of groups in partition
- R = average routing rings per group

**Evaluation**: One move set per group, evaluated in parallel

## Usage Patterns

### Basic Routing

Routing-aware placement with latency optimization:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/group_routing_examples.py start=basic_routing_start end=basic_routing_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/group_routing_examples.cpp start=basic_routing_start end=basic_routing_end
```

</TabItem>
</Tabs>

### With Unassigned Container

Enable placement from unassigned state:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/group_routing_examples.py start=unassigned_start end=unassigned_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/group_routing_examples.cpp start=unassigned_start end=unassigned_end
```

</TabItem>
</Tabs>

### Multi-Region Routing

Optimize placement across regions:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/move_types/group_routing_examples.py start=multi_region_start end=multi_region_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/move_types/group_routing_examples.cpp start=multi_region_start end=multi_region_end
```

</TabItem>
</Tabs>

## Performance Characteristics

### When Does It Help?

GroupRouting helps when:
- **Latency-sensitive**: Network latency impacts performance
- **Routing awareness**: Have routing configuration and latency tables
- **Group-based routing**: Objects in groups with specific routing needs
- **Geographic distribution**: Optimizing across regions/datacenters
- **CDN/edge placement**: Content delivery optimization

GroupRouting does NOT help when:
- **No routing config**: Don't have routing configuration defined
- **Latency insensitive**: Routing latency not important
- **No group structure**: Objects not organized in groups
- **Simple placement**: Basic placement without routing awareness

## Comparison with Alternatives

| Move Type | Routing Aware | Latency Optimized | Use Case |
|-----------|---------------|-------------------|----------|
| [Single](../basic/single) | No | No | General placement |
| [ColocateGroups](colocate-groups) | No | No | Group colocation |
| **GroupRouting** | Yes | Yes | Latency-optimized routing |

## Troubleshooting

### Problem: No improving moves found

**Diagnosis**: Routing configuration may not allow beneficial moves

**Solutions**:
- Verify routing configuration is correct
- Check latency tables are populated
- Ensure groups have routing rings defined
- May already be optimally routed
- Review capacity constraints on destinations

### Problem: Routing config not found

**Diagnosis**: `routingConfigName` doesn't match defined configs

**Solutions**:
- Verify routing configuration name is correct
- Check `RoutingLatencySpec` is defined in problem
- Review routing config setup in problem definition

### Problem: Objects not moving to expected destinations

**Diagnosis**: Latency tables or routing rings issue

**Solutions**:
- Verify latency tables have correct values
- Check routing rings are defined for all groups
- Review partition and group assignments
- Ensure destinations have capacity

### Problem: Unassigned objects not moving

**Diagnosis**: Missing `unassignedContainer` parameter

**Solutions**:
- Set `unassignedContainer` parameter
- Verify unassigned container name is correct
- Check that objects are in unassigned container

## When to Use GroupRouting

**DO use when**:
- Have routing configuration defined
- Optimizing for network latency
- Objects in groups with routing requirements
- Need routing-aware placement
- Have latency tables available

**DO NOT use when**:
- No routing configuration
- Latency not important
- No group structure
- Simple placement sufficient

## Related Move Types

**Group-based alternatives**:
- [ColocateGroups](colocate-groups) - Collocate related groups
- [GroupMoveWithHintStrategies](group-move-with-hint-strategies) - Strategy hints for groups
- [GreedyGroupToScopeItem](greedy-group-to-scope-item) - Greedy group placement

**General alternatives**:
- [Single](../basic/single) - General single moves
- [SingleGreedy](../basic/single-greedy) - Greedy single moves

## Source Code

- Thrift definition: [`interface/thrift/SolverSpecs.thrift:563`](https://github.com/facebookincubator/rebalancer/blob/main/interface/thrift/SolverSpecs.thrift#L563)
- Implementation: [`solver/moves/GroupRoutingMoveType.h`](https://github.com/facebookincubator/rebalancer/blob/main/solver/moves/GroupRoutingMoveType.h)
- Tests: [`solver/moves/tests/`](https://github.com/facebookincubator/rebalancer/tree/main/solver/moves/tests/)

## Next Steps

- Try [ColocateGroups](colocate-groups) for group colocation
- Review [Move Types Overview](../) for choosing move types
- See routing documentation for latency table setup

## Notes

⚠️ **Advanced Move Type**: GroupRouting is a specialized move type requiring routing configuration setup. Refer to routing documentation and examples for proper configuration.
