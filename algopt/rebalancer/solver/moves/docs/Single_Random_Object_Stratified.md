Name: Single Random Object Stratified

Evaluate moving a sample of objects to a pre defined container

Parameters:

PartitionName: Sampling will happen on a per group basis. It is important for your partition to have similar dimensions for this to work

defaultSampleSize: Number of total objects sampled

Behaviour:

Given a container and an objective, rebalancer will select random samples of similar objects and try to place that into the container to see if the objective is satisfied. By sampling objects based on similar properties, we can reduce the number of objects we need to look at to find the “best fit”

Example:

Suppose we have n = 3 million objects, all with varied object dimensions,

We can sort the objects by dimension size and divide them into Y = 5 groups (⅕ of n)

![](/intern/wiki/_download/?title=24c35543-c989-40b2-b557-89b704a69f46&id=1948780&old=0&handle=GL2xAhxCFVzHJ-EDAHkTEqc0R8QQbj8QAAAP)

Then, we can take X = 300 sample size for each group and try all possible moves to find the “best fit” within the sample space.

By doing this, we drastically reduce search space from n = 3 million to X*Y = 300 * 5 = 1500

Example Usage:

```
std::unordered_map<std::string, std::vector<std::string>> similarObjectsList =
      {{"group1", {"task0", "task1", "task2"}},
       {"group2", {"task3"}},
       {"group3", {"task4"}},
       {"group4", {"task5"}},
       {"group5", {"task6", "task7", "task8"}},
       {"group6", {"task9", "task10", "task11"}}};
  solver->addPartition("group", similarObjectsList);
  LocalSearchSolverSpec localSearchSpec;
  SingleRandomObjectStratifiedMoveTypeSpec
      singleRandomObjectStratifiedMoveTypeSpec;
  GroupList groupList;
  groupList.partitionName() = "group";
  ObjectsFromGroupsSpec objectsFromGroupsSpec;
  objectsFromGroupsSpec.groupList() = groupList;
  ObjectsToExploreOptions objectsToExploreOptions;
  objectsToExploreOptions.set_objectsFromGroupSpec(objectsFromGroupsSpec);
  singleRandomObjectStratifiedMoveTypeSpec.objectsToExploreOptions() =
      objectsToExploreOptions;
  SampleSize sampleSize;
  sampleSize.defaultSampleSize() = 12;
  singleRandomObjectStratifiedMoveTypeSpec.stratifiedSampleSize() = sampleSize;
  localSearchSpec.moveTypeList()->push_back(ProblemSolver::makeMoveTypeSpec(
      singleRandomObjectStratifiedMoveTypeSpec));
```
