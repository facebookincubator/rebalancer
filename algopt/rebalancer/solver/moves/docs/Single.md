**Name:** SINGLE

Evaluates moving a single object to every possible destination container. All moves are evaluated in parallel to benefit from multi-threading.

## **Parameters**
* **timePerMove:** amount of time in seconds allowed to explore single moves out of the same hot container before selecting the best move found so far. If unset, no time limit applies and all combinations of objects in the hot container to every possible destination are guaranteed to be evaluated before selecting the best move.
## **Behavior**
The [common logic](https://www.internalfb.com/intern/wiki/ReBalancer/API/Move_Types/#common-logic) selects the hot container (most broken), which is the one it makes the most sense moving objects out of. Given that "hot container", the SINGLE move type will do the following:
1. Pick one object from the "hot container". We call this the "hot object".
2. Pick a different container other than "hot container". We call this the "other container".
3. Evaluate moving "hot object" to "other container".
This move type will try all possible combinations of picking objects and containers in steps 1-2, and pick the best move out of all combinations evaluated in step 3. All moves are evaluated in parallel to benefit from multi-threading.
## **Complexity**
The approximate number of neighboring assignments explored by this move type is: `objects * containers`
## **Diagram**
![](/intern/wiki/_download/?title=SingleMoveType.png)
