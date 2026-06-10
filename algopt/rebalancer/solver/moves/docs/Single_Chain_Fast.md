**Name:** SINGLE_CHAIN_FAST

Evaluates all possible chain moves of two objects: an object moves to a different container, and a second object takes the place of the original object. All moves are evaluated in parallel and we stop exploring as soon as a move is found that improves the objective. Thus SINGLE_CHAIN_FAST may not explore every object.

## **Parameters**
* **partitionNameToExploreFastChainsWithinObjectGroup:** specifies the partition name for the objects that take the place of the original hot object. If set, only objects belonging to that partition will be considered for moves. If not set, all other objects will be considered.
* **specialFastColdContainer:** specifies a fixed destination container to which to move the hot objects. If not set, all other containers will be explored.

## **Behavior**
The [common logic](https://www.internalfb.com/intern/wiki/ReBalancer/API/Move_Types/#common-logic) selects the hottest container (most broken), which is the one it makes the most sense moving objects out of. Given that hottest container, the SINGLE_CHAIN_FAST move type will do the following:
1. Pick one object from the hot container. We call this the "hot object".
2. Pick a pair of different containers other than the hot container. We call them "other container 1" and "other container 2".
3. Pick one object from "other container 1". We call this the "other object".
4. Evaluate, at the same time, moving "hot object" to "other container 2" and "other object" to "hot container".
This move type will try all possible combinations of picking objects and containers in steps 1-3, and return the first move that improves the objective as evaluated in step 4. The moves are evaluated in parallel to benefit from multi-threading. Unlike SINGLE_CHAIN, SINGLE_CHAIN_FAST does not necessarily visit every object in the hot container, nor consider all possible "other objects" for each "hot object".

## **Complexity**
In the worst case, the approximate number of neighboring assignments explored by this move type is: `(objects ^ 2) * containers`

## **Diagram**
![](/intern/wiki/_download/?title=SingleChainMoveType.png)
