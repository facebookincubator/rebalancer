**Name:** SINGLE_CHAIN

Evaluates all possible chain moves of 2 objects: an object moves to a different container, and a second object takes the place of the original object.

**Note:** There is a different move type [Single End Chain](https://www.internalfb.com/intern/wiki/ReBalancer/API/Move_Types/Single_End_Chain/) which is a better default choice. Both move types produce chains moves of 2 objects, the only difference is in the role that the hot container plays:

* Single Chain: the hot container is in the middle of the chain. It gives out one object and it receives one back.
* Single End Chain: the hot container is at the end of the chain. It gives out one object and it doesn't receive any back.

## **Parameters**
* **partitionNameToExploreFastChainsWithinObjectGroup:** specifies the partition name for the objects that take the place of the original hot object. If set, only objects belonging to that partition will be considered for moves. If not set, all other objects will be considered.
* **specialFastColdContainer:** specifies a fixed destination container to which to move the hot objects. If not set, all other containers will be explored.

## **Behavior**
The [common logic](https://www.internalfb.com/intern/wiki/ReBalancer/API/Move_Types/#common-logic) selects the hottest container (most broken), which is the one it makes the most sense moving objects out of. Given that hottest container, the SINGLE_CHAIN move type will do the following:
1. Pick one object from the hot container. We call this the "hot object".
2. Pick a pair of different containers other than the hot container. We call them "other container 1" and "other container 2".
3. Pick one object from "other container 1". We call this the "other object".
4. Evaluate, at the same time, moving "hot object" to "other container 2" and "other object" to "hot container".
This move type will try all possible combinations of picking objects and containers in steps 1-3, and pick the best move out of all combinations evaluated in step 4.

## **Complexity**
The approximate number of neighboring assignments explored by this move type is: `(objects ^ 2) * containers`

## **Diagram**
![](/intern/wiki/_download/?title=SingleChainMoveType.png)
