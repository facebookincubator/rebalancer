**Name:** SINGLE_FAST

Evaluates moving every object to every possible destination container, but stops evaluating after fully exploring the number of objects specified by `minHotObjects` if a move is found that improves the objective. Thus SINGLE_FAST may not explore all objects in the source container before selecting a move. Each object is explored one at a time, but multiple moves for each object are evaluated in parallel to benefit from multi-threading.

## **Parameters**
* **timePerMove:** amount of time in seconds allowed to explore single moves out of the same hot container before selecting the best move found so far. If unset, no time limit applies and all combinations of objects in the hot container to every possible destination will continue to be evaluated until a move that improves the objective is found or the combinations are exhausted.
* **minHotObjects:** the minimum number of objects to fully explore before returning the best move found so far. More objects will be evaluated if no move has yet been found that improves the objective. (Default: 1)

## **Behavior**
The [common logic](https://www.internalfb.com/intern/wiki/ReBalancer/API/Move_Types/#common-logic) selects the hot container (most broken), which is the one out of which it makes the most sense to move objects. Given that "hot container", the SINGLE_FAST move type will do the following:
1. Pick one object from the "hot container". We call this the "hot object".
2. Pick a different container other than "hot container". We call this the "other container".
3. Evaluate moving "hot object" to "other container".
This move type will choose each possible object from step 1 and fully explore moving it to every possible container from step 2. The moves for an individual object are done in parallel and take advantage of multi-threading. Once at least `minHotObjects` are fully explored, the best move found that improves the objective as evaluated in step 3 will be returned. Objects will continue to be explored if no move that improves the objective has been found.

## **Complexity**
The minimum number of assignments that are evaluated is: `minHotObjects * containers`.
In the worst case, all objects and containers need to be explored which gives the approximate number of assignments as: `objects * containers`.

## **Diagram**
![](/intern/wiki/_download/?title=SingleMoveType.png)
