**Name:** SWAP_SAMPLED

Evaluates a sample of all possible swaps between object pairs.

## **Parameters**
Parameters for this move type are defined in LocalSearchSolverSpec. Their meaning is explained under the Behavior section.
* **swapSampledContainerSampleRatio:** double between 0 and 1 representing a portion of all containers.
* **swapSampledObjectCountPerContainer:** integer, 0 means all objects. If this number is larger than the amount of objects in the container, then all objects are selected.
* **minHotObjects:** integer not smaller than 1.

## **Behavior**
As described under [common logic](https://www.internalfb.com/intern/wiki/ReBalancer/API/Move_Types/#common-logic), the common local search logic picks the hottest container (most broken), which is the one it makes the most sense to try moving objects out of. Given that hottest container, the SWAP_SAMPLED move type will do the following:
1. Pick one object from the hot container. We call this the "hot object".
2. Pick a sample of other containers. The size of this sample is controlled by the parameter **swapSampledContainerSampleRatio**.
3. Pick **swapSampledObjectCountPerContainer** objects from each of the sampled containers. We call these the "cold objects".
4. Evaluate swapping the "hot object" with every "cold object", pick the best move among all.
5. If the best move satisfies the constraints and improves the objective, that move is applied and local search runs a new iteration.
6. Otherwise, continue from step 1. Once all objects in the hot container have been explored, give up without valid moves.

### Note
Sometimes you may want to explore more than one "hot object" and pick the best move among all of them before settling on a particular move before the next local search iteration. You can control how many "hot objects" must be explored before settling on a move by setting the parameter **minHotObjects**.
