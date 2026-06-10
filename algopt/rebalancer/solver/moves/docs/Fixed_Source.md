**Name:** SINGLE_FIXED_SOURCE

Evaluates moving a single object from the specified source container(s). For each scope item, all source containers are evaluated in parallel to benefit from multi-threading.

## **Parameters**
* **scopeItemList:** list of scope items with containers from which to try moving objects.
* **stopEarlyAtScopeItemThatImprovesObjective:** if `true`, this move type will return as soon as the first scope item is found that has a move that improves the objective. If `false`, all possible moves from all specified source containers will be evaluated.
* **specialContainer:** the source container from which to move objects. If both `specialContainer` and `scopeItemList` are specified, `scopeItemList` will be used.
* **sampleSize:** the approximate number of objects to consider moving from each source container. Each object in a source container will have probability of being explored equal to `sampleSize \ (number of objects in source container)`.
* **objectBundleFormationHints:** specifies which containers require objects to be moved in bundles, and information on how those bundles should be formed.

## **Behavior**
The [common logic](https://www.internalfb.com/intern/wiki/ReBalancer/API/Move_Types/#common-logic) selects the hot container (most broken), which is the one to which objects will be moved. Given that "hot container", the SINGLE_FIXED_SOURCE move type will do the following:
1. Pick one object from one of the specified source containers. We call this the "other object".
2. With probability of `sampleSize / (number of objects in source container)`, evaluate moving "other object" to the hot container.
This move type will consider each object in each source container but only evaluate moving it to the hot container with probability based on `sampleSize`. The best move evaluated will be returned. All moves for all source containers in a scope item are evaluated in parallel to benefit from multi-threading.

## **Complexity**
The approximate number of neighboring assignments explored by this move type is: `sampleSize * (number of source containers)`

## **Diagram**
![](/intern/wiki/_download/?title=8ebd8c3b-d53f-42c1-b882-5ae3c831688bimage.png)
