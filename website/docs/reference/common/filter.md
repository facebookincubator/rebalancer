---
sidebar_position: 2
---

# Filter

A `Filter` selects which scope items (or groups) a spec applies to. It is a
shared building block used by many specs. If a spec's filter is left unset, the
spec applies to all of them.

## Fields

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `itemsWhitelist` | `list<string>` | (unset) | Only the listed entries are considered. |
| `itemsBlacklist` | `list<string>` | (unset) | The listed entries are excluded; all others are considered. |
| `type` | FilterType | `SCOPE_ITEM` | What the entries name. See [FilterType](#filtertype). |

Set **at most one** of `itemsWhitelist` / `itemsBlacklist`; setting both throws an
error.

## FilterType

`type` says what the `itemsWhitelist` / `itemsBlacklist` entries refer to:

| `type` | Entries are |
|--------|-------------|
| `SCOPE_ITEM` (default) | scope item names |
| `GROUP` | group names from a partition (used by group-aware specs) |

## Example

Consider only specific scope items (whitelist):

```cpp
Filter filter;
filter.itemsWhitelist() = {"host0", "host1"};
```

Consider all but specific scope items (blacklist):

```cpp
Filter filter;
filter.itemsBlacklist() = {"host0"};
```

## Source

- Thrift definition: [`interface/thrift/ProblemSpecs.thrift`](https://github.com/facebookincubator/rebalancer/blob/main/algopt/rebalancer/interface/thrift/ProblemSpecs.thrift) (`Filter`)
