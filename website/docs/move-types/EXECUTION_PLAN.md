# Move Types Documentation - Execution Plan

**Goal**: Create documentation pages for Priority 1 move types (3 pages)
**Based on**: MIGRATION_PLAN.md
**Date**: 2026-01-05

## Batch 1: Priority 1 Move Types (3 tasks)

### Task 1: Create SingleFast Documentation

**File**: `website/docs/solvers/move-types/basic/single-fast.md`

**Steps**:
1. Read source materials:
   - `interface/thrift/SolverSpecs.thrift` lines 513-516 (SingleFastMoveTypeSpec)
   - `solver/moves/docs/Single_Fast.md` (existing wiki content)
   - `wiki/API_Move_Types.md` (summary)

2. Create `website/docs/solvers/move-types/basic/single-fast.md`:
   - Use `basic/single.md` as template
   - Set `sidebar_position: 2`
   - Title: "SingleFast"
   - Complexity: O(objects × containers) with early exit
   - Parameters from thrift: `destinationsToExplore`, `minHotObjects`
   - Include 4-5 usage patterns
   - Add comparison table with Single, SingleGreedy

3. Create `examples/solvers/move_types/single_fast_examples.py`:
   - Copyright header
   - Import statements
   - Functions: quick_example, basic_configuration, early_termination, combined_with_single
   - Add markers: `# quick_example_start/end`, etc.

4. Create `examples/solvers/move_types/single_fast_examples.cpp`:
   - Copyright header
   - Include statements
   - Functions matching Python version
   - Add markers: `// quick_example_start/end`, etc.

**Verification**:
```bash
npm run build
# Should succeed with no errors for single-fast.md
```

---

### Task 2: Create SwapSampled Documentation

**File**: `website/docs/solvers/move-types/swap/swap-sampled.md`

**Steps**:
1. Read source materials:
   - `interface/thrift/SolverSpecs.thrift` lines 537-557 (SwapMoveTypeSpec)
   - `solver/moves/docs/Swap_Sampled.md` (existing wiki content)
   - `wiki/API_Move_Types.md` (summary)

2. Create `website/docs/solvers/move-types/swap/swap-sampled.md`:
   - Use `swap/swap.md` as template
   - Set `sidebar_position: 4`
   - Title: "SwapSampled"
   - Note: This uses `SwapMoveTypeSpec` with `sampleSize` parameter
   - Complexity: O(sample²) instead of O(objects²)
   - Parameters: Focus on `sampleSize` configuration
   - Include 4-5 usage patterns showing different sampling strategies

3. Create `examples/solvers/move_types/swap_sampled_examples.py`:
   - Copyright header
   - Import statements including SampleSize
   - Functions: quick_example, basic_sampling, per_object_sampling, large_problem_sampling
   - Add markers

4. Create `examples/solvers/move_types/swap_sampled_examples.cpp`:
   - Copyright header
   - Include statements
   - Functions matching Python version
   - Add markers

**Verification**:
```bash
npm run build
# Should succeed with no errors for swap-sampled.md
```

---

### Task 3: Create TripleLoop Documentation

**File**: `website/docs/solvers/move-types/advanced/triple-loop.md`

**Steps**:
1. Read source materials:
   - `interface/thrift/SolverSpecs.thrift` line 559 (TripleLoopMoveTypeSpec - empty struct)
   - `solver/moves/docs/Triple_Loop.md` (existing wiki content)
   - `wiki/API_Move_Types.md` (summary)

2. Create `website/docs/solvers/move-types/advanced/triple-loop.md`:
   - Use `chain/single-end-chain.md` as template (also expensive/advanced)
   - Set `sidebar_position: 1`
   - Title: "TripleLoop"
   - Complexity: O(objects³)
   - Parameters: None (empty spec)
   - Warning: Very expensive, only for escaping deep local optima
   - Include 3-4 usage patterns
   - Add strong performance warnings

3. Create `examples/solvers/move_types/triple_loop_examples.py`:
   - Copyright header
   - Import statements
   - Functions: quick_example, escaping_local_optima, small_problem_only, with_time_limits
   - Add markers

4. Create `examples/solvers/move_types/triple_loop_examples.cpp`:
   - Copyright header
   - Include statements
   - Functions matching Python version
   - Add markers

**Verification**:
```bash
npm run build
# Should succeed with no errors for triple-loop.md
```

---

## Batch Verification

After completing all 3 tasks:

```bash
# Verify all files exist
ls -la website/docs/solvers/move-types/basic/single-fast.md
ls -la website/docs/solvers/move-types/swap/swap-sampled.md
ls -la website/docs/solvers/move-types/advanced/triple-loop.md
ls -la examples/solvers/move_types/single_fast_examples.py
ls -la examples/solvers/move_types/single_fast_examples.cpp
ls -la examples/solvers/move_types/swap_sampled_examples.py
ls -la examples/solvers/move_types/swap_sampled_examples.cpp
ls -la examples/solvers/move_types/triple_loop_examples.py
ls -la examples/solvers/move_types/triple_loop_examples.cpp

# Final build verification
npm run build
# Should succeed with no MDX errors

# Check for broken links (expected - future pages not created yet)
# Warnings about broken links are OK for now
```

## Success Criteria

- [ ] All 3 markdown files created with correct frontmatter
- [ ] All 6 example files created (3 Python + 3 C++)
- [ ] `npm run build` succeeds with no compilation errors
- [ ] Each page follows template structure from existing pages
- [ ] Parameters verified against thrift definitions
- [ ] Example markers properly formatted

## Notes

- Use `&lt;` and `&gt;` for `<` and `>` in markdown tables to avoid MDX errors
- Reference external example files with `file=algopt/rebalancer/examples/website/solvers/move_types/[name]_examples.py`
- Include start/end markers in example files: `# pattern_name_start` and `# pattern_name_end`
- All example code should have TODO comments noting they need actual implementation
