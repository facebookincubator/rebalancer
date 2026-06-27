---
sidebar_position: 3
---

# Optimal Solver (MIP-based)

The Optimal solver uses Mixed Integer Programming (MIP) to find provably optimal solutions. It's recommended for small to medium problems (&lt;10,000 objects) where solution quality is critical.

## How It Works

The Optimal solver:

1. **Formulates** the problem as a Mixed Integer Program (MIP)
   - Binary variables: Is object X assigned to container Y?
   - Linear constraints: Capacity, diversity, etc.
   - Objective function: Minimize goals

2. **Solves** using external MIP solver (HiGHS, Gurobi, or XPRESS)
   - Branch-and-bound search
   - Cutting planes
   - Presolving and heuristics

3. **Returns** optimal solution (or best found within time limit)
   - Proven optimal if solved to completion
   - Otherwise provides optimality gap

## Supported MIP Solvers

Rebalancer supports three MIP solvers:

| Solver | License | Performance | Scale | Best For |
|--------|---------|-------------|-------|----------|
| **HiGHS** | Open source (MIT) | Good | &lt;5K objects | Free, open source, small problems |
| **XPRESS** | Commercial + Community | Excellent | &lt;10K objects | Community license available |
| **Gurobi** | Commercial + Academic | Excellent | &lt;10K objects | Academic license, best performance |

### HiGHS (Open Source)

**Pros**:
- Completely free and open source (MIT license)
- No license required
- Good for experimentation and small problems
- Active development and community

**Cons**:
- Slower than commercial solvers
- May not scale to 10K+ objects

**Installation**:
```bash
conda install conda-forge::highs
# or
pip install highspy
```

Enable in CMake following the README.md instructions.

### XPRESS (Community License)

**Pros**:
- Free community license available
- Excellent performance
- Scales well to 10K objects

**Cons**:
- Requires registration
- Community license has size limits
- Commercial license for large-scale use

**Getting Started**:
1. Download from [FICO XPRESS website](https://www.fico.com/en/products/fico-xpress-optimization)
2. Get community license (free)
3. Install and add to PATH
4. Enable in CMake according to the README.md instructions.

### Gurobi (Commercial/Academic)

**Pros**:
- Best MIP performance
- Free academic licenses
- Excellent documentation and support

**Cons**:
- Requires license
- Commercial licenses expensive
- Academic licenses need verification

**Getting Started**:
1. Download from [Gurobi website](https://www.gurobi.com/)
2. Get academic license (free) or purchase commercial license
3. Install and set license file
4. Enable in CMake according to the README.md instructions.

## Quick Example

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/optimal_solver_examples.py start=quick_example_start end=quick_example_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/optimal_solver_examples.cpp start=quick_example_start end=quick_example_end
```

</TabItem>
</Tabs>

## Configuration Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `solveTime` | int | No limit | Maximum solve time in milliseconds |
| `solverPackage` | enum | XPRESS | MIP solver to use (XPRESS, GUROBI, HIGHS) |
| `xpressArgs` | map&lt;string, double&gt; | {} | Solver-specific parameters (e.g., XPRS_MIPRELSTOP, XPRS_THREADS) |
| `printFullLp` | bool | false | Print full LP formulation for debugging |
| `skipInitialAssignmentHint` | bool | false | Skip using initial assignment as warm start |
| `enablePartitionHeuristic` | bool | false | Enable partition-based heuristic |
| `simplifyLpProblem` | bool | false | Simplify LP before solving |

## Understanding MIP Gap

The **MIP gap** measures solution quality:

```
MIP Gap = (Upper Bound - Lower Bound) / Lower Bound
```

- **Lower Bound**: Best solution found so far (feasible)
- **Upper Bound**: Proven best possible objective (may be infeasible)
- **Gap = 0**: Proven optimal
- **Gap = 0.01**: Solution is within 1% of optimal

### Example

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python
solution = solver.solve()

if solution["profile"].optimal:
    print("Exactly optimal")
elif solution["profile"].mipGap == 0.0:
    print("Proven optimal (gap closed)")
elif solution["profile"].mipGap <= 0.05:
    print(f"Very good: within {solution["profile"].mipGap*100:.1f}% of optimal")
elif solution["profile"].mipGap <= 0.20:
    print(f"Acceptable: within {solution["profile"].mipGap*100:.1f}% of optimal")
else:
    print(f"Poor: {solution["profile"].mipGap*100:.1f}% gap - may want to increase time limit")
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
#include <iostream>
#include <iomanip>

auto solution = solver.solve();

if (solution.profile.optimal) {
    std::cout << "Exactly optimal\n";
} else if (solution.profile.mipGap == 0.0) {
    std::cout << "Proven optimal (gap closed)\n";
} else if (solution.profile.mipGap <= 0.05) {
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "Very good: within " << solution.profile.mipGap * 100
              << "% of optimal\n";
} else if (solution.profile.mipGap <= 0.20) {
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "Acceptable: within " << solution.profile.mipGap * 100
              << "% of optimal\n";
} else {
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "Poor: " << solution.profile.mipGap * 100
              << "% gap - may want to increase time limit\n";
}
```

</TabItem>
</Tabs>

## Performance Characteristics

### Scalability

| Problem Size | Typical Time | Notes |
|--------------|--------------|-------|
| 10 objects, 5 containers | &lt;1 second | Trivial |
| 100 objects, 20 containers | 1-10 seconds | Easy |
| 1,000 objects, 100 containers | 10 seconds - 5 minutes | Moderate |
| 5,000 objects, 500 containers | Minutes to hours | Hard |
| 10,000 objects, 1,000 containers | Hours or timeout | Very hard |
| >10,000 objects | Usually infeasible | Use Local Search |

**Note**: Times vary significantly based on:
- Problem structure
- Number/complexity of goals and constraints
- MIP solver used (Gurobi > XPRESS > HiGHS)
- Initial assignment quality

### Memory Usage

MIP solvers are memory-intensive:

- **Small problem** (100 objects): ~100 MB
- **Medium problem** (1,000 objects): ~1-5 GB
- **Large problem** (10,000 objects): ~10-50 GB (may OOM)

**Recommendation**: Monitor memory usage for problems >1,000 objects.

## Solution Quality Guarantees

### Proven Optimal

When `mipGap == 0` and `optimal == true`:
- Solution is **provably optimal**
- No other solution can be better
- Mathematically guaranteed

### Within Gap

When `mipGap > 0`:
- Solution is **within (mipGap × 100)% of optimal**
- Example: `mipGap = 0.05` means solution is at most 5% worse than optimal

### No Solution Found

If timeout before finding any feasible solution:
- Problem may be infeasible
- Or just needs more time
- Check solver logs

## Common Patterns

### Small Problem, Need Optimal

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/optimal_solver_examples.py start=small_problem_optimal_start end=small_problem_optimal_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/optimal_solver_examples.cpp start=small_problem_optimal_start end=small_problem_optimal_end
```

</TabItem>
</Tabs>

### Medium Problem, Time Limited

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/optimal_solver_examples.py start=medium_problem_time_limited_start end=medium_problem_time_limited_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/optimal_solver_examples.cpp start=medium_problem_time_limited_start end=medium_problem_time_limited_end
```

</TabItem>
</Tabs>

### Quick Optimality Check

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/optimal_solver_examples.py start=quick_optimality_check_start end=quick_optimality_check_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/optimal_solver_examples.cpp start=quick_optimality_check_start end=quick_optimality_check_end
```

</TabItem>
</Tabs>

### Validate Local Search

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/optimal_solver_examples.py start=validate_local_search_start end=validate_local_search_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/optimal_solver_examples.cpp start=validate_local_search_start end=validate_local_search_end
```

</TabItem>
</Tabs>

## Warm Starting

Provide an initial solution to help the MIP solver:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/optimal_solver_examples.py start=warm_starting_start end=warm_starting_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/optimal_solver_examples.cpp start=warm_starting_start end=warm_starting_end
```

</TabItem>
</Tabs>

Warm starting can significantly speed up solving.

## Debugging Slow Solving

### Problem: Takes forever, no progress

**Diagnosis**:

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python
# Check if solver is making progress
# Look at logs for bound improvements
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Check if solver is making progress
// Look at logs for bound improvements
```

</TabItem>
</Tabs>

**Solutions**:
- Reduce problem size (fewer objects, containers)
- Simplify goals/constraints (remove non-essential ones)
- Increase `mipGap` (accept worse solution)
- Use Local Search instead

### Problem: Runs out of memory

**Diagnosis**:
- Solver crashes or system OOM
- Large problem (>5K objects)

**Solutions**:
- Reduce problem size
- Use Local Search instead
- Add more RAM
- Reduce `threads` (less parallel memory usage)

### Problem: Can't find feasible solution

**Diagnosis**:
- Timeout with no solution found
- "Infeasible problem" error

**Solutions**:
- Check constraints for conflicts
- Relax some constraints (use as goals instead)
- Increase time limit
- Check initial assignment violates constraints

### Problem: Gap not closing

**Diagnosis**:
- MIP gap stuck at high value
- Bounds not improving

**Solutions**:
- Increase time limit (may take hours)
- Accept current gap (stop early)
- Try different MIP solver (Gurobi often better)
- Simplify problem

## Solver Selection

Which MIP solver to use?

### Use HiGHS if:
- ✅ You want completely open source
- ✅ Small problems (&lt;1,000 objects)
- ✅ Just experimenting
- ✅ Don't want to deal with licenses

### Use XPRESS if:
- ✅ You have community license
- ✅ Medium problems (1,000-10,000 objects)
- ✅ Need good performance
- ✅ Willing to register for license

### Use Gurobi if:
- ✅ You have academic/commercial license
- ✅ Need best performance
- ✅ Medium to large problems
- ✅ Already using Gurobi for other work

**Note**: All three use the same Rebalancer API - switching solvers is just a matter of installation.

## Comparison with Local Search

| Aspect | Optimal | Local Search |
|--------|---------|--------------|
| **Solution quality** | 100% optimal* | 95-99% of optimal |
| **Speed** | Slow (minutes-hours) | Fast (seconds) |
| **Scalability** | &lt;10K objects | 1M+ objects |
| **Guarantees** | Optimality gap | None |
| **Memory** | High (GBs) | Low (MBs) |
| **Best for** | Small problems, validation | Production, large problems |

\* *Given enough time*

## Advanced Settings

### Thread Control

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python file=algopt/rebalancer/examples/website/solvers/optimal_solver_examples.py start=thread_control_start end=thread_control_end
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp file=algopt/rebalancer/examples/website/solvers/optimal_solver_examples.cpp start=thread_control_start end=thread_control_end
```

</TabItem>
</Tabs>

### Solver Tuning

<Tabs groupId="programming-language">
<TabItem value="python" label="Python">

```python
# Configure solver-specific parameters via xpressArgs
solver.add_solver(
        SolverSpec(
            optimalSolverSpec=OptimalSolverSpec(
    solveTime=300000,  # 5 minutes
    xpressArgs={
        "XPRS_MIPRELSTOP": 0.01,  # Stop at 1% MIP gap
        "XPRS_THREADS": 8,  # Use 8 threads
        "XPRS_PRESOLVE": 2,  # Aggressive presolving
        "XPRS_CUTSTRATEGY": 3,  # Aggressive cuts
        "XPRS_HEURSTRATEGY": 3,  # Aggressive heuristics
    },
    enablePartitionHeuristic=True,  # Enable rebalancer heuristic
)
        ))
```

</TabItem>
<TabItem value="cpp" label="C++">

```cpp
// Configure solver-specific parameters via xpressArgs
OptimalSolverSpec spec;
spec.solveTime_ref() = 300000;  // 5 minutes
spec.xpressArgs_ref() = {
    {"XPRS_MIPRELSTOP", 0.01},  // Stop at 1% MIP gap
    {"XPRS_THREADS", 8},  // Use 8 threads
    {"XPRS_PRESOLVE", 2},  // Aggressive presolving
    {"XPRS_CUTSTRATEGY", 3},  // Aggressive cuts
    {"XPRS_HEURSTRATEGY", 3},  // Aggressive heuristics
};
spec.enablePartitionHeuristic_ref() = true;  // Enable rebalancer heuristic
solver.addSolver(spec);
```

</TabItem>
</Tabs>

**Note**: The `xpressArgs` keys depend on your chosen solver package. See [XPRESS docs](https://www.fico.com/fico-xpress-optimization/docs/latest/solver/optimizer/HTML/) or [Gurobi docs](https://www.gurobi.com/documentation/) for available parameters.

## Licensing Notes

### HiGHS
- MIT license
- Completely free
- No restrictions

### XPRESS Community License
- Free registration required
- Size limits (check FICO website for current limits)
- For academic and small commercial use
- Full commercial license available for purchase

### Gurobi Academic License
- Free for academic use
- Requires .edu email
- Annual renewal
- Cannot be used for commercial purposes

### Gurobi Commercial License
- Paid license
- No restrictions
- Pricing based on usage/features
- Contact Gurobi for pricing

## Next Steps

- **Learn Local Search**: [Local Search Guide](local-search)
- **Performance Tuning**: [Performance Guide](performance)
- **Solver Strategies**: [Strategies Guide](strategies)
- **Compare Solvers**: [Solver Overview](overview)

## Related Documentation

- [Installation](../getting-started/installation) - Installing MIP solvers
