**Name:** SINGLE_GREEDY

Evaluates moving every object to every possible destination container, prioritizing containers by how hot (most broken) they are, stopping as soon as a move is found that improves the objective. Thus SINGLE_GREEDY is not guaranteed to explore all objects in the source container nor to fully explore the object that is moved.

## **Parameters**
* **timePerMove:** amount of time in seconds allowed to explore single moves out of the same hot container before selecting the best move found so far. If unset, no time limit applies and all combinations of objects in the hot container to every possible destination will continue to be evaluated until a move that improves the objective is found or the combinations are exhausted.

## **Behavior**
The [common logic](https://www.internalfb.com/intern/wiki/ReBalancer/API/Move_Types/#common-logic) selects the hot container (most broken), which is the one out of which it makes the most sense to move objects. Given that "hot container", the SINGLE_GREEDY move type will do the following:
1. Pick one object from the "hot container". We call this the "hot object".
2. Pick a different container other than "hot container". We call this the "other container".
3. Evaluate moving "hot object" to "other container".
This move type will try each possible combination of picking objects and containers in steps 1-2 until a move is found that improves the objective as evaluated in step 3. The first move found that improves the objective is returned.

## **Complexity**
This move type may return after evaluating a single move if that moves improves the objective.
In the worst case, all objects and containers need to be explored which gives the approximate number of assignments as: `objects * containers`.

## **Diagram**
![](/intern/wiki/_download/?title=SingleMoveType.png)
