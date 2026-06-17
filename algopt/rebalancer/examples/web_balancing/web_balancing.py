#!/usr/bin/env python3
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


# pyre-strict

import random
from collections import defaultdict

from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    AssignmentAffinitiesSpec,
    AssignmentAffinity,
    BalanceSpec,
    CapacitySpec,
    Limit,
    MinimizeMovementSpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    OptimalSolverPackage,
    OptimalSolverSpec,
)


# See tutorial at:
# https://www.internalfb.com/intern/wiki/ReBalancer/API/Tutorial/

REGIONS = ["prn", "frc", "lla", "ftw"]
MAX_RPS = {  # Capacity of each region
    "prn": 10000.0,
    "frc": 10000.0,
    "lla": 5000.0,
    "ftw": 10000.0,
}
MAX_UTILIZATION = 0.8  # only 80% of any region can be utilised

CONTINENTS = ["europe", "africa", "asia", "america_n", "america_s"]
DEFAULT_LATENCY = 1.0
LATENCY = {  # Network latency, feel free to add better data in here
    "europe": {"prn": 4.0, "lla": 0.2, "frc": 3.0, "ftw": 3.1}
}
BUCKETS_PER_CONTINENT = 100
MAX_RPS_FOR_BUCKET = 100


def problem_initial_assignment(solver: ProblemSolver) -> None:
    initial_assignment = {region: [] for region in REGIONS}
    for continent in CONTINENTS:
        for bucket in range(BUCKETS_PER_CONTINENT):
            initial_assignment[random.choice(REGIONS)].append(
                "bucket_{}_{}".format(continent, bucket)
            )
    solver.setAssignment(initial_assignment)


def problem_add_rps_dimensions(solver: ProblemSolver) -> dict[str, float]:
    rps: dict[str, float] = {}
    for continent in CONTINENTS:
        for bucket in range(BUCKETS_PER_CONTINENT):
            computed_rps = random.randrange(MAX_RPS_FOR_BUCKET)
            rps["bucket_{}_{}".format(continent, bucket)] = computed_rps
    solver.addObjectDimension("rps", rps)

    solver.addContainerDimension("rps", MAX_RPS)
    return rps


def problem_add_constraints(solver: ProblemSolver) -> None:
    limit = Limit(globalLimit=MAX_UTILIZATION)

    capacity_spec = CapacitySpec(
        name="capacity",
        limit=limit,
        scope="region",
        dimension="rps",
    )
    solver.addConstraint(capacity_spec)


def problem_add_goals(solver: ProblemSolver, rps: dict[str, float]) -> None:
    # Balance has a lower weight by design, in order to max-out
    # the traffic to LLA coming from Europe (lowest latency).
    problem_add_balance_goal(solver, weight=50.0)
    problem_reduce_latency_goal(solver, rps, weight=100.0)
    problem_add_minimize_moves_goal(solver, weight=100.0)


def problem_add_balance_goal(solver: ProblemSolver, weight: float) -> None:
    balance_spec = BalanceSpec(
        name="balance",
        scope="region",
        dimension="rps",
    )
    solver.addGoal(balance_spec, weight)


def problem_reduce_latency_goal(
    solver: ProblemSolver, rps: dict[str, float], weight: float
) -> None:
    affinities = []
    max_latency = 0.0
    for region in REGIONS:
        for continent in CONTINENTS:
            for bucket in range(BUCKETS_PER_CONTINENT):
                bucket_name = "bucket_{}_{}".format(continent, bucket)
                latency = LATENCY.get(continent, {}).get(region, DEFAULT_LATENCY)
                max_latency = max(max_latency, latency)

                spec = AssignmentAffinity(
                    objectName=bucket_name,
                    scopeItemName=region,
                    # Negative value in afinity in order to make this bucket have less
                    # afinity for cases where latency is higher.
                    # Scale by RSP since we are targeting the average latency and
                    # buckets with higher RPS would contribute more to the average.
                    affinity=-rps[bucket_name] * latency,
                )

                affinities.append(spec)

    # Normalize affinities
    # Rebalancer will use a sum of affinities of all assignments as a measure
    # objective value for any goal. If we don't do this scaling then latency goal
    # will dominate, it will have a higher absolute value compared to other goals.
    # With scaling down, we give other goals better odds of competing with this one.
    scale_by = sum(rps.values()) * max_latency
    scaled_affinities: list[AssignmentAffinity] = []
    for spec in affinities:
        scaled_affinities.append(spec(affinity=spec.affinity / scale_by))

    afinity_spec = AssignmentAffinitiesSpec(
        name="affinities", affinities=scaled_affinities
    )
    solver.addGoal(afinity_spec, weight)


def problem_add_minimize_moves_goal(solver: ProblemSolver, weight: float) -> None:
    movement_spec = MinimizeMovementSpec(
        name="minimize_movement",
        scope="region",
        dimension="rps",
    )
    solver.addGoal(movement_spec, weight)


def main() -> None:
    solver = ProblemSolver(service_name="rebalancer", service_scope="examples")
    solver.setObjectName("bucket")
    solver.setContainerName("region")

    problem_initial_assignment(solver)
    rps = problem_add_rps_dimensions(solver)
    problem_add_constraints(solver)
    problem_add_goals(solver, rps)

    solver_spec = OptimalSolverSpec(solverPackage=OptimalSolverPackage.HIGHS)
    solver.addSolver(solver_spec)

    solution = solver.solve()

    region_rps = {region: 0.0 for region in REGIONS}
    continent_region_rps = defaultdict(lambda: defaultdict(float))
    for bucket, region in solution.assignment.items():
        continent = bucket.split("_")[1]
        region_rps[region] += rps[bucket]
        continent_region_rps[continent][region] += rps[bucket]

    print("\n\nHow are we doing on utilization balancing goal?")
    for region in REGIONS:
        print(
            "Utilization of %s is %.2f%%"
            % (region, 100.0 * region_rps[region] / MAX_RPS[region])
        )
    print("\n\nHow are we doing on latency goal?")
    print("europe", dict(continent_region_rps["europe"]))
    print("Ideally, traffic from europe should prefer to go to lla.")


if __name__ == "__main__":
    main()
