**Name:** Group Move With Hint Strategies

**Parameters**

* Primary Partition: The outer partition
* Secondary Partition: The inner partition that splits the primary partition
* Scope
* HintMap

**Behaviour**

Given a partition with multiple groups, each with unique constraints, we aim to apply different move strategies based on these constraints. A naive approach would involve trying all possible combinations of the partition, resulting in an impractically large search space of approximately 2.5 billion possibilities (2.5 million objects x 1000 containers).

This brute-force method is time-consuming and fails to leverage our knowledge of the unique constraints for each group. As the number of objects is expected to grow, it is essential to develop a more efficient solution that takes advantage of this information.

The solution here is to enable giving hints to rebalancer such that we can try certain strategies for different groups with different constraints.

Each strategy will generate a move set (maybe more if the user specifies). We compare all move sets generated and return the best resulting move set.

By doing so, we can bypass unnecessary evaluations and focus on potential moves that are known to be feasible, thereby optimizing the process.

**Complexity**

**Setup:**

Let n = number of containers,

      m = number of objects,

      s = number of scope items

      g1 = number of groups for primary partition,

      g2 = number of groups for secondary partition

Real world scenario: from torchrec specs (rounded for easier calculations):

n = 1000 containers (ranks)

s = 125 scope items (nodes)

m = 2.5 million objects (shards) (worst 10 million)

g1 = 2500 groups (tables) (worst 5000)

g2 = 10 groups (shard types)

Let Gij be group with table i and shard type j

#### Assumptions:
1. All tables are initially completely unallocated (requires changes to TorchRec setup—add all objects to fake container)
2. Assume for simplicity, objects are evenly distributed across Gij -> Gij has around 2500000 / 2500 / 10 = 10 shards per Gij
3. Assume that, for a given table and sharding type, all the corresponding shard objects are identical (i.e., all objects in Gij are identical)
4. Half of the groups are using random sampling with replacement, other half is using random sampling without replacement

Unit of measurement is # of single move evaluations. This is because rebalancer can evaluate approx 100k evaluations per second (this is a very rough estimate and depends heavily on the problem setup and internal representation size)

**Expected number of evaluations needed per group of outer partition (tables):**
Let **N** be the average number of shards per Gij
Let **S** be the number of shard types
Let **T** be the number of tables
Let **K** be the number of movesets generated for each group Gij

From the problem setup, we expect **S** = 10 shard types, **T** = 2500 tables
From Assumption 2, we expect **N** = 10 shards per group
Assume we generate **K** = 1 moveset for each group Gij

Expected evaluations for each random sample is **N** = 10 since there are 10 shards per Gij. All shards are identical which means we can just place each shard to the given list of containers

Each table will have 10 shard types and each shard types will generate **K** = 1 movesets -> # of evaluations generated for 1 table is **N * S * K = O(100)**

There are 2500 tables -> # evaluations for 2500 tables = **T**  *** N * S * K =** **O(250000)**

At 100k evaluations per second, we expect **2.5 second solve time**. (**baseline**)

**Suppose** we generated 5 move sets instead of 1,
**T** = 2500
**N** = 10
**S** = 10
**K** = 5
**T * N * S * K = 2500 * 10 * 10 * 5 = 1250000**
We would have around **12.5 second solve time**

**Suppose** we assume the worst case of 10 million shards and 5000 tables. 10000000 / 5000 / 10 = 200 shards per Group Gij
**T** = 5000
**N** = 200
**S** = 10
**K** = 1
**T * N * S * K = 5000 * 200 * 10 * 1 = 10000000**
\10000000 evaluations / 100000 evaluations per second -> **100 seconds for solve time**
**Diagram**
![](/intern/wiki/_download/?title=Ea5c9eb6-3f2d-46eb-bd55-66bde3c6fb3a&id=1948990&old=0&handle=GLssAhyqrmB22q8HALW2Lo3GtRAcbj8QAAAP)
