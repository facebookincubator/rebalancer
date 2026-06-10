**Name:** COLOCATE_GROUPS

Evaluates moving a related set of objects in a scope item to every possible combination of containers in every different scope item. Each move set is evaluated in parallel to benefit from multi-threading.

## **Parameters**
* **partitionName:** the name of the partition. Each object in the partition can be in at most one group in the partition.
* **relatedGroupsList:** the list of which groups are related and the scope items they can be colocated in. The sets of related groups need to be disjoint.
* **colocationScopeName:** the name of the default scope in which the related groups need to be colocated. For instance, "region".
* **colocationScopeItemToGroupToContainers:** the map of valid destination containers for a (group, scope item) pair. The allowed scope items for a group can be specified and will supersede the default scope name defined above.
* **defaultSampleSize:** the number of valid containers to consider for each object. This can be useful in limiting how big the number of evaluated moves can be. See [Complexity](https://www.internalfb.com/wiki/ReBalancer/API/Move_Types/Colocate_Groups/#complexity) below. If omitted, all containers are considered.

## **Behavior**
The [common logic](https://www.internalfb.com/intern/wiki/ReBalancer/API/Move_Types/#common-logic) selects the hot container (most broken), which is the one it makes the most sense moving objects out of. Given that "hot container", the COLOCATE_GROUPS move type will do the following:
1. Pick one object from the "hot container". We call this the "hot object". The group of objects that the "hot object" belongs to is called the "hot group". If there is no "hot group", consider the next object in the "hot container". The scope item that the "hot container" belongs to is called the "hot scope item".
2. Consider all groups related to the "hot group" and pick one object from each of them such that the object belongs to the "hot scope item". These are called the "related objects". We are guaranteed that there is at least one "related object" for each related group.
3. Pick a different scope item than the "hot scope item" and valid containers in that scope item for the "hot object" and each "related object". We'll call these the "other containers". These containers do not necessarily need to be distinct from one another.
4. Evaluate the move set of moving "hot object" and each "related object" to the "other containers".
This move type will try all possible "hot objects" in step 1, but only one combination of "related objects" from step 2. Then it will try all possible combinations of containers from step 3 and pick the best move out of all combinations evaluated in step 4. All of the generated move sets are evaluated in parallel to benefit from multi-threading.

## **Complexity**
The number of moves explored by this move type can get very large. If there are n objects in the "hot container", |S| colocation scope items, |G| related groups for each group, and k valid containers that each object can move to in each scope item, then the number of move sets is: `n * |S| * |G|^k`
Setting a defaultSampleSize above can help limit how many move sets are considered.

## **Diagram**
![](/intern/wiki/_download/?title=183faff9-bef7-4c21-a04a-b964a3d64481image.png)
