**Name:** SWAP_FULL_WITH_EMPTY_CONTAINERS

Evaluates moving all objects in the source container to every possible empty destination container. The destination containers are processed in parallel to benefit from multi-threading.

## **Behavior**
The [common logic](https://www.internalfb.com/intern/wiki/ReBalancer/API/Move_Types/#common-logic) selects the hot container (most broken), which is the one out of which it makes the most sense to move objects. Given that "hot container", the SWAP_FULL_WITH_EMPTY_CONTAINERS move type will do the following:
1. Pick a different container other than "hot container" that is currently empty of objects. We call this the "other container".
2. Move all objects in "hot container" into "other container" and evaluate.
This move type will try all possible combinations of empty containers in step 1, and evaluate after moving all objects from "hot container" into it. Each container is processed in parallel to take advantage of multi-threading. The best move found will be returned.

## **Complexity**
The number of neighboring assignments explored by this move type is: `objects * (empty containers)`

## **Diagram**
![](/intern/wiki/_download/?title=b286a087-d467-4736-9427-859a74e8cb2dimage.png)
