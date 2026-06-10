# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Strict Colocation Using Constraint

This variation demonstrates how to make colocation mandatory using a constraint
instead of a soft goal. This ensures ALL service pairs are colocated on exactly
one host.

Use this when: Colocation is absolutely required for correctness, not just
performance optimization.
"""

# pyre-strict
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import ConstraintSpecs
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    GroupCountSpec,
    GroupCountSpecBound,
    Limit,
    LimitType,
)


def add_strict_colocation_constraint(solver):
    """Add strict colocation as a hard constraint."""
    # variation_start
    # Use GroupCountSpec to enforce colocation
    # Each service pair must be on exactly 1 host
    solver.addConstraint(
        ConstraintSpecs(
            groupCountSpec=GroupCountSpec(
                name="enforce-colocation",
                scope="host",
                partitionName="service_pair",
                bound=GroupCountSpecBound.EXACT,
                limit=Limit(type=LimitType.ABSOLUTE, globalLimit=1),
            )
        )
    )
    # variation_end


if __name__ == "__main__":
    # This is a snippet - would be integrated into a full solution
    # See basic_colocation.py for the complete example
    print("Strict colocation constraint variation")
    print("Use GroupCountSpec with EXACT bound to enforce colocation")
