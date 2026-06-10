**Name:** SINGLE_END_CHAIN

Evaluates all possible chain moves of 2 objects: an object moves to a different container, and a second object takes the place of the original object.

## **Parameters**
This move type doesn't have any configuration parameters.

## **Behavior**
The [common logic](https://www.internalfb.com/intern/wiki/ReBalancer/API/Move_Types/#common-logic) selects the hottest container (most broken), which is the one it makes the most sense moving objects out of. Given that hottest container, the SINGLE_CHAIN move type will do the following:
1. Pick one object from the hot container. We call this the "hot object".
2. Pick a pair of different containers other than the hot container. We call them "other container 1" and "other container 2".
3. Pick one object from "other container 1". We call this the "other object".
4. Evaluate, at the same time, moving "hot object" to "other container 1" and "other object" to "other container 2".
This move type will try all possible combinations of picking objects and containers in steps 1-3, and pick the best move out of all combinations evaluated in step 4.

## **Complexity**
The approximate number of neighboring assignments explored by this move type is: `(objects ^ 2) * containers`

## **Diagram**
![](/intern/wiki/_download/?title=SingleEndChainMoveType.png)
