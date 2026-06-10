> Updated by Devmate ✨

**Name:** FIXED_DEST_SWAP_MULTIPLE

Evaluates swapping multiple objects between the hot container and a specified destination container. This move type supports both traditional 1:1 swaps and adaptive 1:k uneven swaps based on dimension ratios. All move sets are evaluated in parallel to benefit from multi-threading.

## **Parameters**
* **maxSampleSizeOnSrc:** the maximum number of source object bundles to consider from the hot container for swapping.
* **specialContainer:** the destination container to swap objects with.
* **greedyOnSrc:** when enabled, uses greedy selection on source objects. Required for 1:k swap functionality.
* **rasLocalSearchMetadata:** relevant metadata for RAS local search, including swap ratio configuration.
  * **swapRatioDimension:** optional dimension name used to calculate swap ratios for uneven swaps.
  * **useAdaptiveAllotments:** when enabled with swapRatioDimension, activates adaptive 1:k swap behavior.

## **Behavior**
The [common logic](https://www.internalfb.com/intern/wiki/ReBalancer/API/Move_Types/#common-logic) selects the hot container (most broken), which is the one it makes the most sense moving objects out of. Given that "hot container", the FIXED_DEST_SWAP_MULTIPLE move type supports two distinct modes:

### **Traditional Swap Mode (Default)**
When `swapRatioDimension` is not configured or `useAdaptiveAllotments` is disabled:
1. Pick a bundle of related objects from the hot container ("hot objects").
2. Pick a bundle of related objects from the special container ("cold objects").
3. Evaluate swapping all hot objects with all cold objects using cartesian product approach.
4. All possible combinations of source and destination bundles are evaluated.

### **Adaptive 1:k Swap Mode**
When `swapRatioDimension` is configured and `useAdaptiveAllotments` is enabled:
1. Pick a single object from the hot container ("hot object") - greedy selection required.
2. For each destination equivalence set, calculate swap ratio k using dimension values:
   - `k = ceil(hotObjectDimensionValue / coldObjectDimensionValue)`
   - If hot object dimension value ≤ cold object dimension value, k = 1
3. Create destination bundles containing k objects from each equivalence set.
4. Evaluate swapping the single hot object with k cold objects individually (no cartesian product).
5. Stop search once a better move is found (greedy behavior).

### **Key Behavioral Changes in Adaptive Mode**
* **Ratio-based swapping:** One source object can be exchanged for multiple destination objects based on dimension ratios
* **Dynamic bundle generation:** Destination bundle sizes are calculated per source object and equivalence set
* **Specialized evaluation path:** Uses `findBestMoveWithSwapRatio()` instead of traditional cartesian product logic
* **Greedy termination:** Stops immediately when a beneficial swap is found
* **Per-equivalence-set bundling:** Forms bundles at equivalence set granularity instead of per-server to minimize fragmentation

## **Implementation Details**
* **Swap ratio calculation:** Uses `calculateSwapRatio()` helper function with ceiling division
* **Bundle formation strategy:** Modified `FixedSrcDstMultiMoveType` to support per-equivalence-set bundle formation when adaptive allotments are enabled
* **Multi-object selection:** Uses `MultiObjectSelectionConfig` with `getBundleSizeForGroup` lambda to determine bundle sizes dynamically
* **Fragmentation optimization:** Prioritizes selecting objects from the same server when possible to minimize server fragmentation

## **Complexity**
### **Traditional Mode**
The approximate number of neighboring assignments explored:
`maxSampleSizeOnSrc * maxSampleSizeOnDst * (objects in source bundle) * (objects in destination bundle)`

### **Adaptive 1:k Mode**
The approximate number of neighboring assignments explored:
`maxSampleSizeOnSrc * (average swap ratio k across equivalence sets)`

Note: Adaptive mode typically explores fewer combinations due to greedy termination and elimination of cartesian product evaluation.

## **Example Use Case**
Consider a scenario where BGM servers (high RRU value of 2.0) need to be replaced by SKL servers (lower RRU value of 1.0):
- **Traditional mode:** Would attempt 1:1 swaps between BGM and SKL objects
- **Adaptive mode:** Calculates ratio k = ceil(2.0/1.0) = 2, swapping 1 BGM object for 2 SKL objects
- **Result:** Better resource utilization matching the actual capacity differences between server types

## **Diagram**
![Diagram placeholder for FIXED_DEST_SWAP_MULTIPLE move type visualization]

## **Requirements**
* **For adaptive 1:k swaps:** `greedyOnSrc` must be enabled, `swapRatioDimension` must be configured, and `useAdaptiveAllotments` must be true
* **Thread safety:** All move evaluations are performed in parallel using the configured thread pool
* **Memory considerations:** Adaptive mode may use less memory due to reduced cartesian product combinations
