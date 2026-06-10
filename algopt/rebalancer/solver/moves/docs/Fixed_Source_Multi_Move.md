**Name:** FIXED_SOURCE_MULTIPLE

Evaluates moving multiple objects from the specified source container to the hot container. All move sets are evaluated in parallel to benefit from multi-threading.

## **Parameters**
* **maxSamplesPerEquivSet:** the maximum number of move sets to consider for each equivalent set. Equivalent sets are groups that are identical from local search's perspective.
* **specialContainer:** the destination container to which to move objects.
* **rasLocalSearchMetadata:** relevant metadata for RAS local search.

## **Behavior**
The [common logic](https://www.internalfb.com/intern/wiki/ReBalancer/API/Move_Types/#common-logic) selects the hot container (most broken), which is the one to which objects will be moved. Given that "hot container", the FIXED_SOURCE_MULTIPLE move type will do the following:
1. Pick an object bundle of related objects from the hot container. We call these the "other objects".
2. Evaluate moving all "other objects" to the hot container.
This move type will consider each object bundle in `specialContainer` and evaluate moving all related objects to the hot container. The number of move sets generated can be limited by `maxSamplesPerEquivSet`. All move sets are evaluated in parallel to benefit from multi-threading.

## **Complexity**
The approximate number of neighboring assignments explored by this move type is: maxSamplesPerEquivSet * `(number of objects in bundle)`

## **Diagram**
![](/intern/wiki/_download/?title=7554d70b-4b77-4b50-b1d4-8b90b2411439image.png)
