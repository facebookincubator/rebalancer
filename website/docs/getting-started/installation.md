---
sidebar_position: 1
---

# Installation

Installation instructions are maintained in the [GitHub README.md](https://github.com/facebookincubator/rebalancer#installation) to ensure they stay up-to-date.

## Quick Overview

Rebalancer requires:
- **C++20** compatible compiler (GCC 10+, Clang 11+, or AppleClang 12+)
- **CMake** 3.20 or higher
- **Ninja** build system (recommended)
- **FBThrift** and **Folly** libraries from Meta

The README provides detailed platform-specific instructions for:
- Ubuntu/Debian Linux
- Fedora/RHEL
- macOS

## Installation Steps

Please follow the installation instructions in the [README.md file](https://github.com/facebookincubator/rebalancer#installation).

## Optional MIP Solvers

For optimal solving, Rebalancer supports:
- **HiGHS** (open source)
- **FICO Xpress** (commercial with free community license)
- **Gurobi** (commercial with free academic license)

See the README for solver-specific installation details.

## Next Steps

Once installed, proceed to [Tutorial: Build Your First Model](first-model) to learn how to use Rebalancer.
