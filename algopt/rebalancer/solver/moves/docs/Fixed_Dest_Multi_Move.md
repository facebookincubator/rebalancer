**Name:** FIXED_DEST_MULTIPLE

Evaluates moving multiple objects from the hot container to the specified destination container. All move sets are evaluated in parallel to benefit from multi-threading.

## **Parameters**
* **maxSamplesPerEquivSet:** the maximum number of move sets to consider for each equivalent set. Equivalent sets are groups that are identical from local search's perspective.
* **specialContainer:** the destination container to which to move objects.
* **rasLocalSearchMetadata:** relevant metadata for RAS local search.

## **Behavior**
The [common logic](https://www.internalfb.com/intern/wiki/ReBalancer/API/Move_Types/#common-logic) selects the hot container (most broken), which is the one it makes the most sense moving objects out of. Given that "hot container", the FIXED_DEST_MULTI move type will do the following:
1. Pick an object bundle of related objects from the hot container. We call these the "hot objects".
2. Evaluate moving all "hot objects" to `specialContainer`.
This move type will consider each object bundle in the hot container, and evaluate moving all related objects to `specialContainer`. The number of move sets generated can be limited by `maxSamplesPerEquivSet`. All move sets are evaluated in parallel to benefit from multi-threading.

## **Complexity**
The approximate number of neighboring assignments explored by this move type is: maxSamplesPerEquivSet * (number of objects in bundle)

## **Diagram**
![](/intern/wiki/_download/?title=4e207350-b4b8-4d88-9057-d0cfb000bfb1image.png)
