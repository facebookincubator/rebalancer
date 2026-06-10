**Name:** SINGLE_FIXED_DEST

Evaluates moving a single object to the specified destination container. All moves are evaluated in parallel to benefit from multi-threading.

## **Parameters**
* **sampleSize:** the approximate number of objects to consider moving from the source container. Each object in the source container will have a probability of being explored equal to `sampleSize / (number of objects in source container)`.
* **specialContainer:** the destination container to which to move objects.

## **Behavior**
The [common logic](https://www.internalfb.com/intern/wiki/ReBalancer/API/Move_Types/#common-logic) selects the hot container (most broken), which is the one it makes the most sense moving objects out of. Given that "hot container", the SINGLE_FIXED_DEST move type will do the following:
1. Pick one object from the "hot container". We call this the "hot object".
2. With probability of `sampleSize / (number of objects in hot container)`, evaluate moving "hot object" to `specialContainer`.
This move type will consider each object in the source container, but only evaluate moving it to `specialContainer` with probability based on `sampleSize`. The best move evaluated will be returned. All moves are evaluated in parallel to benefit from multi-threading.

## **Complexity**
The approximate number of neighboring assignments explored by this move type is: `sampleSize`

## **Diagram**
![](/intern/wiki/_download/?title=4b2c28e5-dee0-4b6d-984f-9e4e27c4003cimage.png)
