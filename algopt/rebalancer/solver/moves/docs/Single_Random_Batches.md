**Name:** SINGLE_RANDOM_BATCHES

Evaluates moving every object to every possible destination container, processing multiple containers at a time to benefit from multi-threading. SINGLE_RANDOM_BATCHES chooses the destination containers randomly, but stops evaluating as soon as a move is found that improves the objective. Thus SINGLE_RANDOM_BATCHES may not explore all objects in the source container nor fully explore the object that is moved.

## **Parameters**
* **timePerMove:** amount of time in seconds allowed to explore single moves out of the same hot container before selecting the best move found so far. If unset, no time limit applies and all combinations of objects in the hot container to every possible destination will continue to be evaluated until a move that improves the objective is found or the combinations are exhausted.
* **randomContainerBatchSize:** number of destination containers to process at a time when considering an object to be moved. (Default: 10)

## **Behavior**
The [common logic](https://www.internalfb.com/intern/wiki/ReBalancer/API/Move_Types/#common-logic) selects the hot container (most broken), which is the one out of which it makes the most sense to move objects. Given that "hot container", the SINGLE_RANDOM_BATCHES move type will do the following:
1. Pick one object from the "hot container". We call this the "hot object".
2. Shuffle the containers other than "hot container" and select `randomContainerBatchSize` containers at a time. We call these the "other containers".
3. Evaluate moving "hot object" to each of the "other containers" in parallel.
This move type will try moving each possible object in step 1  to each other container from step 2, processing `randomContainerBatchSize` containers at a time. Once a batch of containers is found that has a move that improves the objective as evaluated in step 3, the best move from amongst that batch of containers is returned.

## **Complexity**
This move type may return after evaluating `randomContainerBatchSize` moves if one of those moves improves the objective.
In the worst case, all objects and containers need to be explored which gives the approximate number of assignments explored as: `objects * containers`.

## **Diagram**
![](/intern/wiki/_download/?title=SingleMoveType.png)
