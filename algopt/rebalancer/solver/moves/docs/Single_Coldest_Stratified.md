**Name: Single coldest stratified**

A container is deemed "coldest" if its container potential is the least, where container potential is defined using the contribution of the container to the goal values and the number of objects it has.

This move type will evaluate a single move to a sample of the coldest containers, differentiated by similar containers.

**Parameters:**

**includeEqualSizeRandomSampleForSingleColdestMoveType:** in addition to the coldest containers, the move type will also check out a random sample of containers to see if they might be a better destination

**stratifiedSampleSize:** Sample size of the containers, doubled if includeEqualSizeRandomSampleForSingleColdestMoveType is true

**Example:**

After freeing the unallocated container, the tasks that needed to be assigned distributed to the containers with the least amount of objects

6 tasks in a container about to be emptied

![](https://www.internalfb.com/intern/wiki/_download/?title=6d554099-fd4f-4bc0-8a97-be460dbc1a7b.png)

Before:

![](https://www.internalfb.com/intern/wiki/_download/?title=91903d3a-5ffe-4672-a237-6bfdcbbff4bc.png)
After:

![](https://www.internalfb.com/intern/wiki/_download/?title=3ec5053b-17e5-45a3-b33e-354900799614.png)

Usage:

```
LocalSearchSolverSpec spec;
spec.moveTypeList()->push_back(
    ProblemSolver::makeMoveTypeSpec("SINGLE_COLDEST_STRATIFIED"));
spec.stratifiedSampleSize() = 1;
spec.stopAfterMoves() = 6;
solver->addSolver(spec);

solver->addSimilarContainers(
    {{"host0", "host1", "host2", "host3", "host4", "host5"}});
auto solution = solver->solve();
```
