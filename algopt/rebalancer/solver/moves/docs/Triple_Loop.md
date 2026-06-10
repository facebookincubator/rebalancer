**Name:** TRIPLE_LOOP

Evaluates exchanging a triplet of objects.

## **Parameters**
* **timePerMove:** amount of time in seconds allowed to explore single moves out of the same hot container before selecting the best move found so far. If unset, no time limit applies and all combinations of objects in the hot container to every possible destination are guaranteed to be evaluated before selecting the best move.

## **Behavior**
The [common logic](https://www.internalfb.com/intern/wiki/ReBalancer/API/Move_Types/#common-logic) selects the hot container (most broken), which is the one it makes the most sense moving objects out of. Given that "hot container", the TRIPLE_LOOP move type will do the following:
1. Pick one object from the "hot container". We call this the "hot object".
2. Pick an object outside of the "hot container". We call this the "other object 1".
3. Pick an object outside of both the "hot container" and the container of "other object 1". We call this the "other object 2".
4. Evaluate, all at once:
    1. Moving "hot object" to the container of "other object 1".
    2. Moving "other object 1" to the container of "other object 2".
    3. Moving "other object 2" to the "hot container".
This move type will try all possible combinations of picking objects and containers in steps 1-3, and pick the best move out of all combinations evaluated in step 4.

## **Complexity**
The approximate number of neighboring assignments explored by this move type is: `objects ^ 3`

## **Diagram**
![](/intern/wiki/_download/?title=TripleLoopMoveType.png)
