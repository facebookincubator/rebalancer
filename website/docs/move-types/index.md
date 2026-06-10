---
sidebar_position: 1
---

# Move Types Reference

Move types define how Local Search explores the solution space. Each move type searches a different "neighborhood" of solutions by trying different types of object reassignments.

## Overview

When using the [Local Search solver](../solvers/local-search), you specify which move types to use. The solver tries each move type in order, looking for improvements to the objective function.

**Key Concepts:**
- **Hot container**: The container contributing most to the objective (highest cost/constraint violation)
- **Move evaluation**: Testing how a potential move affects the objective
- **Neighborhood**: The set of solutions reachable by a specific move type

## Quick Reference

| Move Type | Category | Complexity | Use When |
|-----------|----------|------------|----------|
| [Single](basic/single) | Basic | O(objects × containers) | Always - fundamental move type |
| [SingleFast](basic/single-fast) | Basic | O(objects × containers) | Need faster convergence, can accept local optimum |
| [SingleGreedy](basic/single-greedy) | Basic | O(objects × containers) | Speed critical, single-threaded OK |
| [Swap](swap/) | Swap | O(objects²) | Capacity constrained problems |
| [SwapFullContainers](swap/swap-full-containers) | Swap | O(containers²) | Moving full containers together |
| [SwapFullWithEmpty](swap/swap-full-with-empty) | Swap | O(objects × containers) | Consolidating to free containers |
| [SwapSampled](swap/swap-sampled) | Swap | O(sample²) | Large problems, need sampling |
| [SingleChain](chain/single-chain) | Chain | O(objects² × containers) | Capacity constrained, need 2-move chains |
| [SingleChainFast](chain/single-chain-fast) | Chain | O(objects² × containers) | Chain moves with early termination |
| [SingleEndChain](chain/single-end-chain) | Chain | O(objects² × containers) | Better than SingleChain (recommended) |
| [TripleLoop](advanced/triple-loop) | Advanced | O(objects³) | Need to escape deep local optima |
| [KLSearch](advanced/kl-search) | Advanced | Expensive | Graph partitioning problems |
| [ColocateGroups](group/colocate-groups) | Group | Varies | Need to move related objects together |
| [GroupRouting](group/group-routing) | Group | Varies | Routing with failover groups |
| [FixedDest](fixed/fixed-dest) | Fixed | O(objects) | Filling specific containers |
| [FixedSource](fixed/fixed-source) | Fixed | O(objects × containers) | Draining specific containers (ToFree) |

## Categories

### Basic Moves

Single-object moves that form the foundation of most local search strategies:

- **[Single](basic/single)**: Move one object to any container (most fundamental)
- **[SingleFast](basic/single-fast)**: Single moves with early termination
- **[SingleGreedy](basic/single-greedy)**: Greedy single moves (single-threaded)
- **[SingleRandomBatches](basic/single-random-batches)**: Parallel batched single moves
- **[SingleRandomStratified](basic/single-random-stratified)**: Stratified sampling of destinations
- **[SingleColdestStratified](basic/single-coldest-stratified)**: Target coldest containers per stratum
- **[SingleRandomObjectStratified](basic/single-random-object-stratified)**: Sample objects, fixed dest

### Swap Moves

Exchange objects between containers (useful when capacity-constrained):

- **[Swap](swap/)**: Exchange any two objects
- **[SwapFullContainers](swap/swap-full-containers)**: Exchange all objects between two containers
- **[SwapFullWithEmpty](swap/swap-full-with-empty)**: Move all objects to empty container
- **[SwapSampled](swap/swap-sampled)**: Sample subset of swap moves

### Chain Moves

Multi-move sequences where objects move in a chain:

- **[SingleChain](chain/single-chain)**: 2-move chain (hot container in middle)
- **[SingleChainFast](chain/single-chain-fast)**: Fast 2-move chain with parallelization
- **[SingleEndChain](chain/single-end-chain)**: 2-move chain (hot container at end) - **recommended**

### Group Moves

Move multiple related objects together:

- **[ColocateGroups](group/colocate-groups)**: Move related groups to same scope item
- **[GroupRouting](group/group-routing)**: Routing-aware group moves
- **[GroupMoveWithHintStrategies](group/group-move-with-hint-strategies)**: Group moves with strategies
- **[GreedyGroupToScopeItem](group/greedy-group-to-scope-item)**: Greedy group placement

### Fixed Source/Destination

Moves with predetermined source or destination containers:

- **[FixedDest](fixed/fixed-dest)**: Move objects to specific container(s)
- **[FixedSource](fixed/fixed-source)**: Move objects from specific container(s)
- **[FixedDestMultiMove](fixed/fixed-dest-multi-move)**: Multi-object bundles to fixed dest
- **[FixedSourceMultiMove](fixed/fixed-source-multi-move)**: Multi-object bundles from fixed source
- **[FixedDestSwapMultiMove](fixed/fixed-dest-swap-multi-move)**: Swap bundles with fixed dest

### Advanced Moves

Complex move types for escaping local optima:

- **[TripleLoop](advanced/triple-loop)**: 3-object cyclic moves
- **[KLSearch](advanced/kl-search)**: Kernighan-Lin inspired search
- **[ReplicaDrop](advanced/replica-drop)**: Drop replica from specific scope

## Choosing Move Types

### Default Configuration

For most problems, start with:

```python
solver_spec = {
    "localSearchSolverSpec": {
        "moveTypeList": [
            {"singleMoveTypeSpec": {}},  # Basic single-object moves
            {"swapMoveTypeSpec": {}},    # Add swap for capacity-constrained problems
        ]
    }
}
solver.add_solver(solver_spec)
```

### By Problem Type

**Unconstrained or soft-constrained:**
- Start with `Single` only
- Add `SingleFast` if convergence too slow

**Capacity-constrained:**
- Use `Single` + `Swap`
- Add `SingleEndChain` for better quality
- Consider `SwapSampled` for large problems

**Group placement:**
- Use `ColocateGroups` when groups must be together
- Use `GroupRouting` for routing-aware placement

**Container draining (ToFree):**
- Use `FixedSource` for draining specific containers
- Much faster than exploring all source containers

**Stuck in local optimum:**
- Add `TripleLoop` for deeper search
- Try `KLSearch` for graph partitioning problems
- Use multiple runs with different seeds

### Performance Trade-offs

| Priority | Move Types | Notes |
|----------|------------|-------|
| **Speed** | `Single` or `SingleFast` only | Fast but may miss better solutions |
| **Quality** | `Single` + `Swap` + `SingleEndChain` | Balanced speed/quality |
| **Best Solution** | All applicable move types | Slow but thorough |

## Move Type Order Matters

Move types are tried in the order specified. The solver:

1. Tries moves from **hottest container** using **first move type**
2. If improvement found → applies move, restart from step 1
3. If no improvement → try **second move type** on same container
4. If still no improvement → move to **second hottest container**, restart from step 1
5. Continue until no improving move found from any container with any move type

**Recommendation**: Order move types from fast to slow:
```python
move_type_list = [
    {"singleFastMoveTypeSpec": {}},      # Try fast moves first
    {"singleMoveTypeSpec": {}},          # Then thorough single moves
    {"swapMoveTypeSpec": {}},            # Then swaps
    {"singleEndChainMoveTypeSpec": {}},  # Finally, expensive chain moves
]
```

## Common Patterns

### Fast Interactive Solving

Quick responses for UI/dashboards:

```python
# Use fast move types only
move_type_list = [{"singleFastMoveTypeSpec": {}}]
```

### Production Rebalancing

Balance speed and quality:

```python
# Good default for most production use cases
move_type_list = [
    {"singleMoveTypeSpec": {}},
    {"swapMoveTypeSpec": {}},
]
```

### Offline Optimization

Take time to find best solution:

```python
# Use all applicable move types
move_type_list = [
    {"singleMoveTypeSpec": {}},
    {"swapMoveTypeSpec": {}},
    {"singleEndChainMoveTypeSpec": {}},
    {"tripleLoopMoveTypeSpec": {}},
]
```

### Draining Containers

When using ToFreeSpec:

```python
# Much faster than exploring all containers
move_type_list = [
    {
        "singleFixedSourceMoveTypeSpec": {
            "scopeItemList": {
                "scopeName": "container",
                "scopeItems": ["host1", "host2"],  # Containers to drain
            }
        }
    }
]
```

## Configuring Move Types

Each move type has its own configuration options. See individual move type pages for details.

### Example: Configuring Swap with Sampling

```python
from rebalancer.specs import MoveTypeSpec, SampleSize, SwapMoveTypeSpec

swap_spec = MoveTypeSpec(
    swapMoveTypeSpec=SwapMoveTypeSpec(
        greedyOnSrc=True,  # Exit early on source objects
        sampleSize=SampleSize(
            defaultSampleSize=100,  # Sample 100 objects
        ),
    )
)
```

### Example: Configuring Fixed Destination

```python
from rebalancer.specs import FixedDestMoveTypeSpec, MoveTypeSpec, SampleSize

fixed_dest_spec = MoveTypeSpec(
    fixedDestMoveTypeSpec=FixedDestMoveTypeSpec(
        specialContainer="spare_capacity_container",
        sampleSize=SampleSize(defaultSampleSize=50),
    )
)
```

## Complexity Analysis

Understanding complexity helps choose appropriate move types for your problem size:

| Move Type | Per-Container Complexity | Problem Size Limit |
|-----------|-------------------------|-------------------|
| Single | O(N × C) | 100K+ objects |
| SingleFast | O(N × C) with early exit | 100K+ objects |
| Swap | O(N²) | 10K objects |
| SingleChain | O(N² × C) | 5K objects |
| TripleLoop | O(N³) | 1K objects |

Where N = objects per container, C = number of containers

**Tip**: For large problems (>10K objects), prefer:
- `Single`, `SingleFast`, or `SingleRandomStratified` (sampled)
- `SwapSampled` instead of `Swap`
- Avoid `TripleLoop` and `KLSearch`

## Troubleshooting

### Problem: Solver too slow

**Diagnosis**: Too many objects or expensive move types

**Solutions**:
- Use faster move types (`SingleFast`, `SingleGreedy`)
- Add sampling to expensive types (`SwapSampled`)
- Reduce time limits per move type
- Use stratified sampling moves

### Problem: Stuck in local optimum

**Diagnosis**: Terminates quickly with NO_IMPROVING_MOVE but poor objective

**Solutions**:
- Add more powerful move types (`SingleEndChain`, `TripleLoop`)
- Use better initial assignment
- Try multiple runs with different random seeds
- Check if problem is feasible

### Problem: Not improving constraints

**Diagnosis**: Solution violates constraints

**Solutions**:
- Verify constraints are achievable
- Use move types that can escape constraint violations (Swap, Chain moves)
- Check if initial assignment is extremely broken
- Increase constraint penalties (`invalidCost`)

## Next Steps

- Choose a move type from the categories above to see detailed documentation
- Review [Local Search Solver](../solvers/local-search) to understand how move types fit into the solver
- See [Solver Strategies](../solvers/strategies) for multi-stage solving with different move types
- Check [Performance Guide](../solvers/performance) for tuning move type performance

## Source Code

- Thrift definitions: [`interface/thrift/SolverSpecs.thrift`](https://github.com/facebookincubator/rebalancer/blob/main/interface/thrift/SolverSpecs.thrift)
- Move type implementations: [`solver/moves/`](https://github.com/facebookincubator/rebalancer/tree/main/solver/moves/)
- Tests: [`solver/moves/tests/`](https://github.com/facebookincubator/rebalancer/tree/main/solver/moves/tests/)
