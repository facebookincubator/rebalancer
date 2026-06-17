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

# Version 4 of the code from "Tutorial: Intro to Rebalancer":
# https://www.internalfb.com/intern/wiki/ReBalancer/API/Tutorial:_Intro_to_Rebalancer/
# This is a snapshot of the code at the end of section "Dimensions".

# pyre-strict
from algopt.rebalancer.examples.tasks_on_hosts.pretty_print import prettyPrint
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    ConstraintSpecs,
    GoalSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    BalanceSpec,
    BalanceSpecFormula,
    ToFreeSpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    OptimalSolverPackage,
    OptimalSolverSpec,
)


def main() -> None:
    # Create an instance of the solver
    solver = ProblemSolver(service_name="rebalancer", service_scope="examples")

    # Basic properties
    solver.setObjectName("task")
    solver.setContainerName("host")
    solver.setAssignment(
        {
            "host0": [f"task{i}" for i in range(12)],
            "host1": [],
            "host2": [],
            "host3": [],
        }
    )

    # Add dimension: task memory
    task_memory = {f"task{i}": 1 for i in range(12)}
    # pyre-fixme[6]: For 2nd argument expected `dict[str, float]` but got `dict[str,
    #  int]`.
    solver.addObjectDimension("memory", task_memory)

    # Add dimension: host memory
    host_memory = {"host0": 10, "host1": 20, "host2": 10, "host3": 10}
    # pyre-fixme[6]: For 2nd argument expected `dict[str, float]` but got `dict[str,
    #  int]`.
    solver.addContainerDimension("memory", host_memory)

    # Add goal: balance hosts on memory
    spec = BalanceSpec(
        # The name is used to identify this spec during debugging and logging
        # It should be descriptive and intuitive
        name="balance-hosts",
        # name of the entity we want to spread our objects across
        scope="host",
        # make the memory utilization in each host balanced
        dimension="memory",
        # legacy formula is a combinaion of max and squares
        formula=BalanceSpecFormula.LEGACY,
        fixAverageToInitial=True,
    )
    solver.addGoal(GoalSpecs(balanceSpec=spec), 1)

    # Add constraint: make host0 empty
    spec = ToFreeSpec(
        name="make-host0-empty",
        # host0 must be empty in a valid solution
        containers=["host0"],
    )
    solver.addConstraint(ConstraintSpecs(toFreeSpec=spec))

    solver.addSolver(
        SolverSpecs(
            optimalSolverSpec=OptimalSolverSpec(
                solverPackage=OptimalSolverPackage.HIGHS,
            )
        )
    )

    # Generate a solution and print it
    solution = solver.solve()
    prettyPrint(solution.assignment)


if __name__ == "__main__":
    main()
