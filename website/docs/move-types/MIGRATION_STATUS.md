# Move Types Documentation Migration - Status

## Overview

This document tracks the migration of move types documentation from the wiki to the website (Docusaurus format).

**Goal**: Create comprehensive, API-verified documentation for all move types with testable Python and C++ examples, similar to the specs reference documentation.

**Created**: 2026-01-05

## Progress Summary

### ✅ Completed

1. **API Verification**: Verified all 27 move types from `interface/thrift/SolverSpecs.thrift`
2. **Directory Structure**: Created organized category structure in `website/docs/move-types/`
3. **Index Page**: Comprehensive overview page with quick reference table and usage patterns
4. **All Move Type Documentation Pages** (27 total):
   - Basic (7): Single, SingleFast, SingleGreedy, SingleRandomBatches, SingleRandomStratified, SingleColdestStratified, SingleRandomObjectStratified
   - Swap (4): Swap, SwapFullContainers, SwapFullWithEmpty, SwapSampled
   - Chain (3): SingleChain, SingleChainFast, SingleEndChain
   - Group (4): ColocateGroups, GroupRouting, GroupMoveWithHintStrategies, GreedyGroupToScopeItem
   - Fixed (5): FixedDest, FixedSource, FixedDestMultiMove, FixedSourceMultiMove, FixedDestSwapMultiMove
   - Advanced (4): TripleLoop, KLSearch, ReplicaDrop, SwapN
5. **Example Code Implementation**: All 27 Python and 27 C++ example files with complete working examples

### 📋 TODO

1. **Update Cross-References**: Update `website/docs/solvers/local-search.md` to link to move types

2. **CI Integration**: Ensure example code is tested in CI (if not already integrated)

## Directory Structure

```
website/docs/move-types/
├── index.md                    # ✅ Overview and quick reference
├── _category_.json             # ✅ Category metadata
├── basic/                      # ✅ Basic single-object moves
│   ├── _category_.json
│   ├── single.md              # ✅ Completed
│   ├── single-fast.md         # ✅ Completed
│   ├── single-greedy.md       # ✅ Completed
│   ├── single-random-batches.md     # ✅ Completed
│   ├── single-random-stratified.md  # ✅ Completed
│   ├── single-coldest-stratified.md # ✅ Completed
│   └── single-random-object-stratified.md # ✅ Completed
├── swap/                       # ✅ Swap moves
│   ├── _category_.json
│   ├── swap.md                # ✅ Completed
│   ├── swap-full-containers.md # ✅ Completed
│   ├── swap-full-with-empty.md # ✅ Completed
│   └── swap-sampled.md        # ✅ Completed
├── chain/                      # ✅ Chain moves
│   ├── _category_.json
│   ├── single-chain.md        # ✅ Completed
│   ├── single-chain-fast.md   # ✅ Completed
│   └── single-end-chain.md    # ✅ Completed
├── group/                      # ✅ Group moves
│   ├── _category_.json
│   ├── colocate-groups.md     # ✅ Completed
│   ├── group-routing.md       # ✅ Completed
│   ├── group-move-with-hint-strategies.md # ✅ Completed
│   └── greedy-group-to-scope-item.md # ✅ Completed
├── fixed/                      # ✅ Fixed source/dest
│   ├── _category_.json
│   ├── fixed-dest.md          # ✅ Completed
│   ├── fixed-source.md        # ✅ Completed
│   ├── fixed-dest-multi-move.md # ✅ Completed
│   ├── fixed-source-multi-move.md # ✅ Completed
│   └── fixed-dest-swap-multi-move.md # ✅ Completed
└── advanced/                   # ✅ Advanced moves
    ├── _category_.json
    ├── triple-loop.md         # ✅ Completed
    ├── kl-search.md           # ✅ Completed
    ├── replica-drop.md        # ✅ Completed
    └── swap-n.md              # ✅ Completed
```

## Move Types Inventory

### From Thrift API (27 total)

| # | Move Type | Thrift Spec | Docs | Status |
|---|-----------|-------------|------|--------|
| 1 | Single | `SingleMoveTypeSpec` | ✅ | ✅ Done |
| 2 | SingleFast | `SingleFastMoveTypeSpec` | ✅ | ✅ Done |
| 3 | SingleGreedy | `SingleGreedyMoveTypeSpec` | ✅ | ✅ Done |
| 4 | SingleRandomBatches | `SingleRandomBatchesMoveTypeSpec` | ✅ | ✅ Done |
| 5 | SingleRandomStratified | `SingleRandomStratifiedMoveTypeSpec` | ✅ | ✅ Done |
| 6 | SingleColdestStratified | (Uses SingleRandomStratifiedMoveTypeSpec) | ✅ | ✅ Done |
| 7 | SingleRandomObjectStratified | `SingleRandomObjectStratifiedMoveTypeSpec` | ✅ | ✅ Done |
| 8 | Swap | `SwapMoveTypeSpec` | ✅ | ✅ Done |
| 9 | SwapFullContainers | `SwapFullContainersMoveTypeSpec` | ✅ | ✅ Done |
| 10 | SwapFullWithEmpty | `SwapFullWithEmptyContainersMoveTypeSpec` | ✅ | ✅ Done |
| 11 | SwapSampled | (SwapMoveTypeSpec with sampling) | ✅ | ✅ Done |
| 12 | SwapN | `SwapNMoveTypeSpec` | ✅ | ✅ Done |
| 13 | SingleChain | `SingleChainMoveTypeSpec` | ✅ | ✅ Done |
| 14 | SingleChainFast | `SingleChainFastMoveTypeSpec` | ✅ | ✅ Done |
| 15 | SingleEndChain | `SingleEndChainMoveTypeSpec` | ✅ | ✅ Done |
| 16 | TripleLoop | `TripleLoopMoveTypeSpec` | ✅ | ✅ Done |
| 17 | KLSearch | `KLSearchMoveTypeSpec` | ✅ | ✅ Done |
| 18 | ColocateGroups | `ColocateGroupsMoveTypeSpec` | ✅ | ✅ Done |
| 19 | GroupRouting | `GroupRoutingMoveTypeSpec` | ✅ | ✅ Done |
| 20 | GroupMoveWithHintStrategies | `GroupMoveWithHintStrategiesMoveTypeSpec` | ✅ | ✅ Done |
| 21 | GreedyGroupToScopeItem | `GreedyGroupToScopeItemMoveTypeSpec` | ✅ | ✅ Done |
| 22 | FixedDest | `FixedDestMoveTypeSpec` | ✅ | ✅ Done |
| 23 | FixedSource | `SingleFixedSourceMoveTypeSpec` | ✅ | ✅ Done |
| 24 | FixedDestMultiMove | `FixedDestMultiMoveTypeSpec` | ✅ | ✅ Done |
| 25 | FixedSourceMultiMove | `FixedSrcMultiMoveTypeSpec` | ✅ | ✅ Done |
| 26 | FixedDestSwapMultiMove | `FixedDestSwapMultiMoveTypeSpec` | ✅ | ✅ Done |
| 27 | ReplicaDrop | `ReplicaDropMoveTypeSpec` | ✅ | ✅ Done |

**Progress**: 27 / 27 completed (100%)

## Template Pattern

All 27 move type pages follow this consistent structure:

1. **Frontmatter**: sidebar_position
2. **Header**: Move type name, category, complexity
3. **Overview**: Description, use cases
4. **Quick Example**: Tabbed Python/C++ with external file references
5. **Parameters**: Complete parameter table
6. **How It Works**: Step-by-step explanation with visual diagram
7. **Complexity**: Big-O analysis with examples
8. **Usage Patterns**: Multiple real-world examples (4-6 patterns)
9. **Performance Characteristics**: Scalability, parallelization, memory
10. **Comparison**: with variants/alternatives
11. **Troubleshooting**: Common problems and solutions
12. **Related Move Types**: Variants, alternatives, complementary
13. **Source Code**: Links to thrift, implementation, tests
14. **Next Steps**: Related documentation

## Example Code Pattern

All 27 move types have complete example files with this pattern:

```python
# Copyright header

"""
[Move Type] Move Type Reference Examples

This file demonstrates all usage patterns shown in the reference documentation.
Each function is a complete, runnable example.
"""

# Imports

def quick_example():
    """Quick example showing basic usage."""
    # quick_example_start
    # ... example code ...
    # quick_example_end

def pattern_name():
    """Description of pattern."""
    # pattern_name_start
    # ... example code ...
    # pattern_name_end
```

Markers (`# pattern_name_start` / `# pattern_name_end`) allow documentation to reference specific code sections.

## Next Steps

### Remaining Work

1. **Update Cross-References**: Update `website/docs/solvers/local-search.md` to link to move types documentation

2. **CI Integration**: Verify example code is tested in CI (if not already integrated)

## API References

**Thrift Definitions**:
- Move type specs: `interface/thrift/SolverSpecs.thrift` (lines 478-742)
- Related types: `interface/thrift/SolverSpecs.thrift` (lines 384-469)

**Implementation**:
- Move type classes: `solver/moves/`
- Tests: `solver/moves/tests/`

**Original Documentation**:
- Wiki pages: `wiki/API_Move_Types.md`
- Individual docs: `solver/moves/docs/*.md`

## Notes

- All 27 move type names verified against thrift API
- All documentation pages completed with consistent structure and examples
- All 54 example files (27 Python + 27 C++) implemented with complete working code
- SwapSampled is a configuration of SwapMoveTypeSpec, not a separate type
- SingleColdestStratified uses SingleRandomStratifiedMoveTypeSpec with specific config
