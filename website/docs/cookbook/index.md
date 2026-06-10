---
sidebar_position: 1
---

# Cookbook

Welcome to the Rebalancer Cookbook! This section contains practical, complete examples of solving common assignment problems.

## How to Use the Cookbook

Each recipe follows the same structure:

1. **Problem Description**: Real-world scenario
2. **Problem Modeling**: How to think about it in Rebalancer terms
3. **Complete Code**: Working implementation in C++ and Python
4. **Explanation**: Why specific goals/constraints were chosen
5. **Variations**: How to adapt for related scenarios

## Quick Navigation

### Resource Allocation Problems

Load balancing and resource distribution:

- [Load Balancing Tasks Across Servers](load-balancing) - Balance CPU/memory across hosts
- [Database Shard Placement](shard-placement) - Distribute shards with capacity constraints
- [Container Bin Packing](bin-packing) - Pack containers with multiple resource dimensions

### Placement with Constraints

Problems with specific placement rules:

- [Disaster Recovery Placement](disaster-recovery) - Ensure rack/datacenter diversity
- [Colocation Requirements](colocation) - Services that must/must not be together
- [Gradual Migration](gradual-migration) - Limiting moves per iteration

### Optimization Problems

Finding optimal trade-offs:

- [Multi-Objective Optimization](multi-objective) - Balancing multiple goals
- [Affinity-Based Placement](affinity-placement) - Working with preferences

### Complex Scenarios

Advanced use cases:

- [Multi-Tenant Resource Allocation](multi-tenant) - Isolating tenants with fairness
- [Incremental Rebalancing with Throttling](throttled-rebalancing) - Rate-limited changes

## Problem Categories

### By Problem Type

| Problem Type | Examples |
|--------------|----------|
| **Load Balancing** | Distribute work evenly across resources |
| **Capacity Planning** | Pack items into fixed-size bins |
| **Placement** | Assign items to locations with rules |
| **Migration** | Move from one assignment to another |
| **Optimization** | Find best assignment for multiple objectives |

### By Industry

| Industry | Use Cases |
|----------|-----------|
| **Cloud Infrastructure** | VM placement, container orchestration |
| **Databases** | Shard distribution, replica placement |
| **CDN** | Content placement, cache distribution |
| **Networking** | Traffic routing, load balancing |
| **Storage** | Data placement, replication |

## Learning Path

**New to Rebalancer?**
1. Start with [Load Balancing Tasks](load-balancing) - simplest example
2. Move to [Database Shard Placement](shard-placement) - adds capacity constraints
3. Try [Disaster Recovery](disaster-recovery) - introduces scopes
4. Explore [Multi-Objective](multi-objective) - multiple goals

**Looking for a specific pattern?**
Use the navigation above or search for keywords like "capacity", "balance", "affinity", etc.

## Contributing Recipes

Have a useful pattern to share? We welcome cookbook contributions!

See [Contributing Guide](https://github.com/facebookincubator/rebalancer/blob/main/CONTRIBUTING.md) for how to add your recipe.

## Need Help?

- Review [Core Concepts](../core-concepts/overview) for DSL basics
- Check [Goals & Constraints Reference](../reference/) for available specs
- See [Getting Started](../getting-started/first-problem) for fundamentals

---

Choose a recipe above to get started, or continue to the [first cookbook example](load-balancing).
