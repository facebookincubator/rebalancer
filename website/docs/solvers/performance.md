---
sidebar_position: 4
---

# Performance Tuning

This guide covers performance optimization techniques for both Local Search and Optimal solvers.

## Problem Size Recommendations

| Objects | Containers | Solver | Expected Time | Tuning Priority |
|---------|------------|--------|---------------|-----------------|
| &lt;100 | &lt;50 | Optimal | Seconds | None needed |
| 100-1K | 50-500 | Either | Seconds-Minutes | Solver choice |
| 1K-10K | 500-5K | Local Search | Seconds-Minutes | Goal/constraint complexity |
| 10K-100K | 5K-50K | Local Search | Minutes | Move limits, parallelization |
| >100K | >50K | Local Search | Minutes-Hours | All optimizations |

## Quick Wins

### 1. Choose the Right Solver

**Impact**: 10-1000x speedup

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python
# Bad: Using Optimal for large problem
solver.add_solver(
        SolverSpec(
            optimalSolverSpec=OptimalSolverSpec()
        ))  # Will timeout/OOM

# Good: Use Local Search for 10K+ objects
solver.add_solver(
        SolverSpec(
            localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=30000)
        ))
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Bad: Using Optimal for large problem
solver.addSolver(OptimalSolverSpec());  // Will timeout/OOM

// Good: Use Local Search for 10K+ objects
LocalSearchSolverSpec spec;
spec.timeLimitMs = 30000;
solver.addSolver(spec);
```

</TabItem>
</Tabs>

### 2. Set Appropriate Time Limits

**Impact**: Prevent wasted time

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/performance_examples.py start=adjust_time_limit_start end=adjust_time_limit_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/performance_examples.cpp start=adjust_time_limit_start end=adjust_time_limit_end
```

</TabItem>
</Tabs>

### 3. Simplify When Possible

**Impact**: 2-10x speedup

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python
# Bad: Too many low-priority goals
solver.add_goal(balance_cpu, weight=1.0)
solver.add_goal(balance_memory, weight=1.0)
solver.add_goal(balance_disk, weight=0.1)  # Low priority
solver.add_goal(balance_network, weight=0.05)  # Very low priority

# Good: Focus on what matters
solver.add_goal(balance_cpu, weight=1.0)
solver.add_goal(balance_memory, weight=1.0)
# Skip low-priority goals for faster solving
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Bad: Too many low-priority goals
solver.addGoal(balance_cpu, 1.0);
solver.addGoal(balance_memory, 1.0);
solver.addGoal(balance_disk, 0.1);  // Low priority
solver.addGoal(balance_network, 0.05);  // Very low priority

// Good: Focus on what matters
solver.addGoal(balance_cpu, 1.0);
solver.addGoal(balance_memory, 1.0);
// Skip low-priority goals for faster solving
```

</TabItem>
</Tabs>

## Local Search Optimization

### Move Type Selection

Different move types have different costs:

| Move Type | Cost per Iteration | When to Use |
|-----------|-------------------|-------------|
| SingleMove | O(objects × containers) | Always, fast |
| SwapMove | O(objects²) | Capacity constrained |
| TripleLoop | O(objects³) | Need escape local optima |
| ChainMoves | O(objects² × containers) | Dependencies |

**Optimization**: Use only necessary move types

### Iteration Limits

Balance quality vs speed:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python
# Fast but lower quality
solver.add_solver(
        SolverSpec(
            localSearchSolverSpec=LocalSearchSolverSpec(movesLimit=10000)
        ))

# Slower but better quality
solver.add_solver(
        SolverSpec(
            localSearchSolverSpec=LocalSearchSolverSpec(movesLimit=1000000)
        ))

# Let it run until local optimum (no limit)
solver.add_solver(
        SolverSpec(
            localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=300000)
        ))  # Time limit only
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Fast but lower quality
LocalSearchSolverSpec spec1;
spec1.movesLimit = 10000;
solver.addSolver(spec1);

// Slower but better quality
LocalSearchSolverSpec spec2;
spec2.movesLimit = 1000000;
solver.addSolver(spec2);

// Let it run until local optimum (no limit)
LocalSearchSolverSpec spec3;
spec3.timeLimitMs = 300000;  // Time limit only
solver.addSolver(spec3);
```

</TabItem>
</Tabs>

### Initial Assignment Quality

**Impact**: 2-5x speedup, better final quality

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python
# Bad: Start with terrible assignment
initial = {"host0": all_objects, "host1": [], ...}  # Everything on one host

# Good: Start with reasonable distribution
initial = distribute_evenly(objects, containers)  # Pre-balance

solver.set_assignment(initial)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Bad: Start with terrible assignment
std::map<std::string, std::vector<Object>> initial;
initial["host0"] = all_objects;  // Everything on one host
initial["host1"] = {};
// ...

// Good: Start with reasonable distribution
auto initial = distributeEvenly(objects, containers);  // Pre-balance

solver.setAssignment(initial);
```

</TabItem>
</Tabs>

## Optimal Solver Optimization

### MIP Gap Tolerance

Accept "good enough" solutions:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/performance_examples.py start=set_mip_gap_start end=set_mip_gap_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/performance_examples.cpp start=set_mip_gap_start end=set_mip_gap_end
```

</TabItem>
</Tabs>

**Impact**: Can reduce solve time from hours to minutes.

### Solver Selection

Use the best MIP solver available:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python
# Fast: Gurobi (if licensed)

# Fast: XPRESS (community license)

# Slower: HiGHS (open source)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Fast: Gurobi (if licensed)

// Fast: XPRESS (community license)

// Slower: HiGHS (open source)
```

</TabItem>
</Tabs>

**Impact**: Gurobi can be 2-5x faster than HiGHS.

### Thread Control

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/performance_examples.py start=set_thread_count_start end=set_thread_count_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/performance_examples.cpp start=set_thread_count_start end=set_thread_count_end
```

</TabItem>
</Tabs>

### Warm Starting

Provide good initial solution:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/performance_examples.py start=warmstart_with_local_search_start end=warmstart_with_local_search_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/performance_examples.cpp start=warmstart_with_local_search_start end=warmstart_with_local_search_end
```

</TabItem>
</Tabs>

**Impact**: 2-10x speedup for Optimal solver.

## Goal and Constraint Optimization

### Reduce Complexity

**Impact**: 2-5x speedup

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python
# Complex: Many scopes and dimensions
solver.add_goal(
        GoalSpec(
            balanceSpec=BalanceSpec(scope="host", dimension="cpu")
        ), 1.0)
solver.add_goal(
        GoalSpec(
            balanceSpec=BalanceSpec(scope="rack", dimension="cpu")
        ), 0.5)
solver.add_goal(
        GoalSpec(
            balanceSpec=BalanceSpec(scope="dc", dimension="cpu")
        ), 0.3)
solver.add_goal(
        GoalSpec(
            balanceSpec=BalanceSpec(scope="host", dimension="memory")
        ), 1.0)
solver.add_goal(
        GoalSpec(
            balanceSpec=BalanceSpec(scope="rack", dimension="memory")
        ), 0.5)
# ... many more ...

# Simpler: Focus on key goals
solver.add_goal(
        GoalSpec(
            balanceSpec=BalanceSpec(scope="host", dimension="cpu")
        ), 1.0)
solver.add_goal(
        GoalSpec(
            balanceSpec=BalanceSpec(scope="host", dimension="memory")
        ), 1.0)
# Skip less important goals
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Complex: Many scopes and dimensions
BalanceSpec spec1("host", "cpu");
solver.addGoal(spec1, 1.0);
BalanceSpec spec2("rack", "cpu");
solver.addGoal(spec2, 0.5);
BalanceSpec spec3("dc", "cpu");
solver.addGoal(spec3, 0.3);
BalanceSpec spec4("host", "memory");
solver.addGoal(spec4, 1.0);
BalanceSpec spec5("rack", "memory");
solver.addGoal(spec5, 0.5);
// ... many more ...

// Simpler: Focus on key goals
BalanceSpec cpu_spec("host", "cpu");
solver.addGoal(cpu_spec, 1.0);
BalanceSpec mem_spec("host", "memory");
solver.addGoal(mem_spec, 1.0);
// Skip less important goals
```

</TabItem>
</Tabs>

### Use Constraints Wisely

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python
# Expensive: Many individual constraints
for obj in objects:
    solver.add_constraint(
        ConstraintSpec(
            avoidMovingSpec=AvoidMovingSpec(objects=[obj])
        ))  # 1000s of constraints

# Cheaper: Single constraint with all objects
solver.add_constraint(
        ConstraintSpec(
            avoidMovingSpec=AvoidMovingSpec(objects=fixed_objects)
        ))  # 1 constraint
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Expensive: Many individual constraints
for (const auto& obj : objects) {
    AvoidMovingSpec spec({obj});
    solver.addConstraint(spec);  // 1000s of constraints
}

// Cheaper: Single constraint with all objects
AvoidMovingSpec spec(fixed_objects);
solver.addConstraint(spec);  // 1 constraint
```

</TabItem>
</Tabs>

### Dimension Simplification

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python
# Expensive: Many fine-grained dimensions
solver.add_object_dimension("cpu_user", ...)
solver.add_object_dimension("cpu_system", ...)
solver.add_object_dimension("cpu_io_wait", ...)

# Cheaper: Combined dimension
cpu_total = {obj: cpu_user[obj] + cpu_system[obj] + cpu_iowait[obj] for obj in objects}
solver.add_object_dimension("cpu_total", cpu_total)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Expensive: Many fine-grained dimensions
solver.addObjectDimension("cpu_user", ...);
solver.addObjectDimension("cpu_system", ...);
solver.addObjectDimension("cpu_io_wait", ...);

// Cheaper: Combined dimension
std::map<std::string, double> cpu_total;
for (const auto& obj : objects) {
    cpu_total[obj] = cpu_user[obj] + cpu_system[obj] + cpu_iowait[obj];
}
solver.addObjectDimension("cpu_total", cpu_total);
```

</TabItem>
</Tabs>

## Memory Optimization

### Object/Container Limits

Each object and container uses memory:

- Object: ~100-200 bytes
- Container: ~50-100 bytes

**Example**: 100K objects + 10K containers ≈ 15-25 MB

For very large problems (1M+ objects):
- Consider problem decomposition
- Use streaming/chunking if possible

### Dimension Storage

Each dimension adds memory:

- Per dimension: ~8 bytes per object/container

**Optimization**: Only add necessary dimensions

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python
# Adds memory: 10 dimensions × 100K objects = 8MB
for dim in all_possible_dimensions:
    solver.add_object_dimension(dim, values)

# Better: Only dimensions actually used
solver.add_object_dimension("cpu", cpu_values)
solver.add_object_dimension("memory", memory_values)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Adds memory: 10 dimensions × 100K objects = 8MB
for (const auto& dim : all_possible_dimensions) {
    solver.addObjectDimension(dim, values);
}

// Better: Only dimensions actually used
solver.addObjectDimension("cpu", cpu_values);
solver.addObjectDimension("memory", memory_values);
```

</TabItem>
</Tabs>

## Parallelization

### Multi-Threading

Both solvers use multiple threads:

- **Local Search**: Parallelizes move evaluation
- **Optimal**: MIP solver uses parallel branch-and-bound

**Optimization**: Ensure enough cores available

```bash
# Check available cores
nproc
# Or
python -c "import os; print(os.cpu_count())"
```

### Multiple Solves in Parallel

For batch processing:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python
from concurrent.futures import ThreadPoolExecutor

def solve_problem(problem_id):
    solver = ProblemSolver(...)
    # ... set up problem ...
    solver.add_solver(
        SolverSpec(
            localSearchSolverSpec=LocalSearchSolverSpec()
        ))
    return solver.solve()

# Solve multiple problems in parallel
with ThreadPoolExecutor(max_workers=4) as executor:
    results = list(executor.map(solve_problem, problem_ids))
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
#include <future>
#include <vector>

auto solve_problem(const std::string& problem_id) {
    ProblemSolver solver(...);
    // ... set up problem ...
    LocalSearchSolverSpec spec;
    solver.addSolver(spec);
    return solver.solve();
}

// Solve multiple problems in parallel
std::vector<std::future<Solution>> futures;
for (const auto& problem_id : problem_ids) {
    futures.push_back(std::async(std::launch::async, solve_problem, problem_id));
}

std::vector<Solution> results;
for (auto& future : futures) {
    results.push_back(future.get());
}
```

</TabItem>
</Tabs>

## Profiling and Debugging

### Measure Solve Time

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python
import time

start = time.time()
solution = solver.solve()
elapsed = time.time() - start

print(f"Total time: {elapsed:.2f}s")
print(f"Solver time: {solution["profile"].solveTime / 1000:.2f}s")
print(f"Setup overhead: {elapsed - solution["profile"].solveTime/1000:.2f}s")
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
#include <chrono>
#include <iostream>
#include <iomanip>

auto start = std::chrono::high_resolution_clock::now();
auto solution = solver.solve();
auto end = std::chrono::high_resolution_clock::now();
auto elapsed = std::chrono::duration<double>(end - start).count();

std::cout << std::fixed << std::setprecision(2);
std::cout << "Total time: " << elapsed << "s\n";
std::cout << "Solver time: " << solution.profile.solveTime / 1000.0 << "s\n";
std::cout << "Setup overhead: " << elapsed - solution.profile.solveTime/1000.0 << "s\n";
```

</TabItem>
</Tabs>

### Identify Bottlenecks

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python
# The solution includes profiling information
solution = solver.solve()

# Access profiler data from solution
profile = solution["problemProfile"]

print(f"Materialization time: {profile.materializationSec:.2f} seconds")
print(f"Solving time: {profile.solveSec:.2f} seconds")

# For Local Search, access detailed move type events
if profile.localSearchProfiles:
    ls_profile = profile.localSearchProfiles[0]
    print(f"Move types used: {ls_profile.moveTypeNames}")
    for event in ls_profile.moveTypeEvents:
        print(f"  {event.moveTypeName}: {event.count} moves")

# For hierarchical profiling (detailed breakdown)
if profile.hierarchicalProfileRoot:
    root = profile.hierarchicalProfileRoot
    print(f"Total {root.eventName}: {root.duration:.2f}ms")
    for child in root.children:
        print(f"  {child.eventName}: {child.duration:.2f}ms")
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// The solution includes profiling information
auto solution = solver.solve();

// Access profiler data from solution
auto& profile = solution.problemProfile;

std::cout << "Materialization time: " << profile.materializationSec << " seconds\n";
std::cout << "Solving time: " << profile.solveSec << " seconds\n";

// For Local Search, access detailed move type events
if (!profile.localSearchProfiles.empty()) {
    auto& lsProfile = profile.localSearchProfiles[0];
    std::cout << "Move types used: ";
    for (const auto& name : lsProfile.moveTypeNames) {
        std::cout << name << " ";
    }
    std::cout << "\n";
}

// For hierarchical profiling (detailed breakdown)
if (profile.hierarchicalProfileRoot) {
    auto& root = *profile.hierarchicalProfileRoot;
    std::cout << "Total " << root.eventName << ": " << root.duration << "ms\n";
    for (const auto& child : root.children) {
        std::cout << "  " << child.eventName << ": " << child.duration << "ms\n";
    }
}
```

</TabItem>
</Tabs>

### Monitor Progress

For long-running solves, you can monitor progress by checking the solution periodically or using solver time limits with multiple iterations:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python
# Iterative solving with progress monitoring
time_budget = 300000  # 5 minutes total
iteration_time = 30000  # 30 seconds per iteration

current_best = None
for i in range(time_budget // iteration_time):
    solver.add_solver(
        SolverSpec(
            optimalSolverSpec=OptimalSolverSpec(solveTime=iteration_time)
        ))
    solution = solver.solve()

    print(f"Iteration {i+1}: Objective = {solution["finalObjective"].value}")

    if current_best is None or solution["finalObjective"].value < current_best:
        current_best = solution["finalObjective"].value
        print(f"  New best solution found!")

    # Check solver status
    if solution["solverSummaries"]:
        end_reason = solution["solverSummaries"][0].endReason
        if end_reason == EndReason.OPTIMAL:
            print("  Proven optimal, stopping early")
            break
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Iterative solving with progress monitoring
int timeBudget = 300000;  // 5 minutes total
int iterationTime = 30000;  // 30 seconds per iteration

double currentBest = std::numeric_limits<double>::max();
for (int i = 0; i < timeBudget / iterationTime; i++) {
    OptimalSolverSpec spec;
    spec.solveTime_ref() = iterationTime;
    solver.addSolver(spec);

    auto solution = solver.solve();

    std::cout << "Iteration " << (i+1) << ": Objective = "
              << solution.finalObjective.value << "\n";

    if (solution.finalObjective.value < currentBest) {
        currentBest = solution.finalObjective.value;
        std::cout << "  New best solution found!\n";
    }

    // Check solver status
    if (!solution.solverSummaries.empty() &&
        solution.solverSummaries[0].endReason == EndReason::OPTIMAL) {
        std::cout << "  Proven optimal, stopping early\n";
        break;
    }
}
```

</TabItem>
</Tabs>

**Note**: There is no built-in progress callback API. For production use, consider running the solver in a separate thread and checking status periodically.

## Common Performance Issues

### Issue: Solver much slower than expected

**Diagnosis**:
1. Check problem size (objects, containers, dimensions)
2. Check number of goals/constraints
3. Check goal/constraint complexity

**Solutions**:
- Simplify problem (fewer goals/constraints)
- Use Local Search instead of Optimal
- Increase time limits (may just need more time)

### Issue: Runs out of memory

**Diagnosis**:
- Solver crashes or system OOM
- Very large problem or Optimal solver

**Solutions**:
- Use Local Search (much lower memory)
- Reduce problem size
- Add more RAM
- Problem decomposition

### Issue: No progress, infinite loop

**Diagnosis**:
- Solver running but no iterations/improvements
- May indicate bug or infeasible problem

**Solutions**:
- Check logs for errors
- Verify problem is feasible (constraints not contradictory)
- Try simpler problem first
- Report bug if confirmed

## Benchmarking

### Measure Baseline

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python
import time

start = time.time()
solution = solver.solve()
baseline_time = time.time() - start

print(f"Baseline: {baseline_time:.2f}s, objective={solution["objectiveValue"]}")
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
#include <chrono>
#include <iostream>
#include <iomanip>

auto start = std::chrono::high_resolution_clock::now();
auto solution = solver.solve();
auto end = std::chrono::high_resolution_clock::now();
auto baseline_time = std::chrono::duration<double>(end - start).count();

std::cout << std::fixed << std::setprecision(2);
std::cout << "Baseline: " << baseline_time << "s, objective="
          << solution.objectiveValue << "\n";
```

</TabItem>
</Tabs>

### Test Optimizations

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python
# Try different configurations
configs = [
    {"timeLimitMs": 10000},
    {"timeLimitMs": 30000},
    {"timeLimitMs": 60000},
]

for config in configs:
    solver = ProblemSolver(...)
    # ... set up problem ...
    solver.add_solver(
        SolverSpec(
            localSearchSolverSpec=LocalSearchSolverSpec(**config)
        ))

    start = time.time()
    solution = solver.solve()
    elapsed = time.time() - start

    print(f"Config {config}: {elapsed:.2f}s, objective={solution["objectiveValue"]}")
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Try different configurations
std::vector<int> time_limits = {10000, 30000, 60000};

for (int time_limit : time_limits) {
    ProblemSolver solver(...);
    // ... set up problem ...
    LocalSearchSolverSpec spec;
    spec.timeLimitMs = time_limit;
    solver.addSolver(spec);

    auto start = std::chrono::high_resolution_clock::now();
    auto solution = solver.solve();
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration<double>(end - start).count();

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Config timeLimitMs=" << time_limit << ": "
              << elapsed << "s, objective=" << solution.objectiveValue << "\n";
}
```

</TabItem>
</Tabs>

## Advanced Optimizations

### Problem Decomposition

For very large problems, solve in pieces:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python
# Split objects into chunks
chunks = split_objects_into_chunks(all_objects, chunk_size=10000)

solutions = []
for chunk in chunks:
    solver = ProblemSolver(...)
    # Set up problem with just this chunk
    solver.set_assignment(chunk_assignment)
    # ... add goals/constraints ...
    solutions.append(solver.solve())

# Merge solutions
final_solution = merge_solutions(solutions)
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Split objects into chunks
auto chunks = splitObjectsIntoChunks(all_objects, 10000);

std::vector<Solution> solutions;
for (const auto& chunk : chunks) {
    ProblemSolver solver(...);
    // Set up problem with just this chunk
    solver.setAssignment(chunk_assignment);
    // ... add goals/constraints ...
    solutions.push_back(solver.solve());
}

// Merge solutions
auto final_solution = mergeSolutions(solutions);
```

</TabItem>
</Tabs>

### Caching and Reuse

For repeated solves:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python
# Cache problem setup
problem_setup = build_problem_setup()  # Expensive

# Solve multiple times with different parameters
for params in parameter_sweep:
    solver = ProblemSolver(...)
    apply_problem_setup(solver, problem_setup)
    # Change only parameters
    solver.add_goal(
        GoalSpec(
            balanceSpec=BalanceSpec(...)
        ), weight=params['weight'])
    solution = solver.solve()
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Cache problem setup
auto problem_setup = buildProblemSetup();  // Expensive

// Solve multiple times with different parameters
for (const auto& params : parameter_sweep) {
    ProblemSolver solver(...);
    applyProblemSetup(solver, problem_setup);
    // Change only parameters
    BalanceSpec spec(...);
    solver.addGoal(spec, params.weight);
    auto solution = solver.solve();
}
```

</TabItem>
</Tabs>

## Next Steps

- **Solver Comparison**: [Solver Overview](overview)
- **Local Search Details**: [Local Search Guide](local-search)
- **Optimal Solver Details**: [Optimal Solver Guide](optimal)
- **Advanced Strategies**: [Solver Strategies](strategies)
