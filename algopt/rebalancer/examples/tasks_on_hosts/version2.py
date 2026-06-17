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

# Version 2 of the code from "Tutorial: Intro to Rebalancer":
# https://www.internalfb.com/intern/wiki/ReBalancer/API/Tutorial:_Intro_to_Rebalancer/
# This is a snapshot of the code at the end of section "Goals".

# pyre-strict
from algopt.rebalancer.examples.tasks_on_hosts.pretty_print import prettyPrint
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    GoalSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    BalanceSpec,
    BalanceSpecFormula,
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

    # Add goal: balance hosts on task count
    spec = BalanceSpec(
        # The name is used to identify this spec during debugging and logging
        # It should be descriptive and intuitive
        name="balance-hosts",
        # name of the entity we want to spread our objects across
        scope="host",
        # make the task count in each host balanced
        dimension="task_count",
        # legacy formula is a combinaion of max and squares
        formula=BalanceSpecFormula.LEGACY,
        fixAverageToInitial=True,
    )
    solver.addGoal(GoalSpecs(balanceSpec=spec), 1)

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
