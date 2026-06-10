---
sidebar_position: 20
---

# ThrottlingSpec

**Type**: Constraint only

Limit the volume of objects moved into, out of, or through containers.

## Overview

`ThrottlingSpec` constrains the total volume (measured by a dimension) of objects that can move relative to specific containers. You can limit moves coming IN to containers, going OUT from containers, or ANY movement (both directions). This is useful for limiting network bandwidth consumption, protecting hosts during draining, or controlling migration rate.

**Use this when**: You want to throttle data movement to avoid overwhelming network, disk I/O, or specific hosts/racks.

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `name` | string | Yes | - | Unique identifier for this constraint |
| `scope` | string | Yes | - | Scope level (e.g., `"host"`, `"rack"`) |
| `dimension` | string | Yes | - | Object dimension to measure movement volume |
| `definition` | ThrottlingSpecDefinition | No | ANY | Which moves to count (IN/OUT/ANY) |
| `limit` | Limit | Yes | - | Maximum movement volume |
| `filter` | Filter | No | all | Apply only to filtered containers |

### ThrottlingSpecDefinition Enum

| Value | Description | Counts |
|-------|-------------|--------|
| `ANY` | **Total movement** | Objects moving IN or OUT |
| `IN` | **Incoming only** | Objects moving INTO filtered containers |
| `OUT` | **Outgoing only** | Objects moving OUT OF filtered containers |

## How It Works

For each filtered container:
1. Identify moves relative to that container (IN/OUT/ANY based on definition)
2. Sum the dimension values of moved objects
3. Constraint: total ≤ limit

**Example** (definition=OUT):
- Host initially has objects worth 1000 GB
- If 300 GB moves away, 700 GB stays
- Constraint checks: 300 ≤ limit

## Common Usage Patterns

### 1. Throttle Host Draining

Limit data leaving hosts being drained:



<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.py start=throttle_drain_start end=throttle_drain_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.cpp start=throttle_drain_start end=throttle_drain_end
```

</TabItem>
</Tabs>

### 2. Limit Rack Network Usage

Prevent saturating rack network:


<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.py start=limit_rack_network_start end=limit_rack_network_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.cpp start=limit_rack_network_start end=limit_rack_network_end
```

</TabItem>
</Tabs>

### 3. Per-Host Throttling

Different limits for different hosts:


<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.py start=per_host_throttling_start end=per_host_throttling_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.cpp start=per_host_throttling_start end=per_host_throttling_end
```

</TabItem>
</Tabs>

### 4. Gradual Migration

Limit incoming data to new datacenter:


<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.py start=gradual_migration_start end=gradual_migration_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.cpp start=gradual_migration_start end=gradual_migration_end
```

</TabItem>
</Tabs>

### 5. Protect Production Hosts

Limit churn on production hosts:


<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.py start=protect_prod_start end=protect_prod_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.cpp start=protect_prod_start end=protect_prod_end
```

</TabItem>
</Tabs>

### 6. Network-Aware Throttling

Throttle based on network traffic instead of data size:


<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.py start=network_aware_start end=network_aware_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.cpp start=network_aware_start end=network_aware_end
```

</TabItem>
</Tabs>

### 7. Object Count Throttling

Limit number of objects moved (not data size):


<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.py start=object_count_throttling_start end=object_count_throttling_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.cpp start=object_count_throttling_start end=object_count_throttling_end
```

</TabItem>
</Tabs>

## Definition Types Compared

| Definition | Example Use Case | What It Counts |
|------------|------------------|----------------|
| **ANY** | Network bandwidth limit | All moves touching container |
| **IN** | Limit receiving load | Only moves INTO container |
| **OUT** | Throttle draining | Only moves OUT OF container |

**Example**:
- Host initially: [obj1(100GB), obj2(50GB)]
- After rebalancing: [obj3(80GB)]
- IN: 80GB (obj3 arrived)
- OUT: 150GB (obj1+obj2 left)
- ANY: 230GB (80+150)

## Performance Considerations

- **Impact**: Minimal - simple move volume counting
- **Solver compatibility**: Works with both Local Search and Optimal solvers
- **Scaling**: O(number of moves × number of filtered containers)

## Common Pitfalls

### 1. Limit Too Restrictive

**Problem**: Limit so low that rebalancing can't make progress.


<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.py start=pitfall_restrictive_bad_start end=pitfall_restrictive_bad_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.cpp start=pitfall_restrictive_bad_start end=pitfall_restrictive_bad_end
```

</TabItem>
</Tabs>

**Solution**: Set limit based on network capacity and desired migration time:


<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.py start=pitfall_restrictive_good_start end=pitfall_restrictive_good_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.cpp start=pitfall_restrictive_good_start end=pitfall_restrictive_good_end
```

</TabItem>
</Tabs>

### 2. Wrong Definition

**Problem**: Using wrong direction for intended throttling.


<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.py start=pitfall_wrong_definition_bad_start end=pitfall_wrong_definition_bad_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.cpp start=pitfall_wrong_definition_bad_start end=pitfall_wrong_definition_bad_end
```

</TabItem>
</Tabs>

**Solution**: Use correct definition:


<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.py start=pitfall_wrong_definition_good_start end=pitfall_wrong_definition_good_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.cpp start=pitfall_wrong_definition_good_start end=pitfall_wrong_definition_good_end
```

</TabItem>
</Tabs>

### 3. Filter Not Specified

**Problem**: No filter means throttling applies to ALL containers.


<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.py start=pitfall_no_filter_bad_start end=pitfall_no_filter_bad_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.cpp start=pitfall_no_filter_bad_start end=pitfall_no_filter_bad_end
```

</TabItem>
</Tabs>

**Solution**: Specify filter for target hosts:


<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.py start=pitfall_no_filter_good_start end=pitfall_no_filter_good_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.cpp start=pitfall_no_filter_good_start end=pitfall_no_filter_good_end
```

</TabItem>
</Tabs>

### 4. Dimension Not Defined

**Problem**: Dimension doesn't exist.


<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.py start=pitfall_no_dimension_bad_start end=pitfall_no_dimension_bad_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.cpp start=pitfall_no_dimension_bad_start end=pitfall_no_dimension_bad_end
```

</TabItem>
</Tabs>

**Solution**: Define dimension first:


<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.py start=pitfall_no_dimension_good_start end=pitfall_no_dimension_good_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.cpp start=pitfall_no_dimension_good_start end=pitfall_no_dimension_good_end
```

</TabItem>
</Tabs>

## Combining with Other Specs

### With MinimizeMovementSpec

Soft + hard movement limits:


<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.py start=combining_minimize_movement_start end=combining_minimize_movement_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.cpp start=combining_minimize_movement_start end=combining_minimize_movement_end
```

</TabItem>
</Tabs>

### With ToFreeSpec

Throttled draining:


<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.py start=combining_to_free_start end=combining_to_free_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.cpp start=combining_to_free_start end=combining_to_free_end
```

</TabItem>
</Tabs>

## Verification Example

Verify throttling limits respected:


<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.py start=verification_start end=verification_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/reference/throttling/throttling_spec_examples.cpp start=verification_start end=verification_end
```

</TabItem>
</Tabs>

## Related Specs

- [MinimizeMovementSpec](movement/minimize-movement) - Soft movement minimization
- [GroupMoveLimitSpec](groups/group-move-limit) - Limit moves per group
- [AvoidMovingSpec](movement/avoid-moving) - Pin specific objects

## Source Code

- Thrift definition: `interface/thrift/ProblemSpecs.thrift:1054` (ThrottlingSpecDefinition enum)
- Thrift definition: `interface/thrift/ProblemSpecs.thrift:1060` (ThrottlingSpec)
- Implementation: `solver/constraints/Throttling.cpp`
