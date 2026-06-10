**Name:** SWAP_FULL_CONTAINERS

Evaluates exchanging all objects in the source container with all objects of every possible destination container. The destination containers are processed in parallel to benefit from multi-threading.

## **Behavior**
The [common logic](https://www.internalfb.com/intern/wiki/ReBalancer/API/Move_Types/#common-logic) selects the hot container (most broken), which is the one out of which it makes the most sense to move objects. Given that "hot container", the SWAP_FULL_CONTAINERS move type will do the following:
1. Pick a different container other than "hot container". We call this the "other container".
2. Evaluate exchanging all objects in "hot container" with "other container".
This move type will try all possible combinations of containers in step 1, and evaluate swapping all objects between it and "hot container". Each container is processed in parallel to take advantage of multi-threading. The best full swap move found will be returned.

## **Complexity**
The number of neighboring assignments explored by this move type is: `objects * (containers - 1)`

## **Diagram**
![](/intern/wiki/_download/?title=bcc549d6-a0e5-4cb6-ab3b-99a36f8a3118image.png)
