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

from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.Types.thrift_types import AssignmentSolution
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    ManifoldBackupParams,
    ManifoldUploadPolicy,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    AvoidMovingSpec,
    CapacitySpec,
    Limit,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    OptimalSolverPackage,
    OptimalSolverSpec,
)


# See tutorial at:
# https://www.internalfb.com/intern/wiki/ReBalancer/API/Tutorial/


def sudoku_initial_assignment(solver: ProblemSolver) -> None:
    puzzle = [
        "85...24..",
        "72......9",
        "..4......",
        "...1.7..2",
        "3.5...9..",
        ".4.......",
        "....8..7.",
        ".17......",
        "....36.4.",
    ]

    # Compute initial board assignment.
    initial_board = {}
    # Also compute which numbers should not be moved.
    avoid_moving = []
    for row in range(0, 9):
        numbers = set(range(1, 10))  # keep track of unused numbers in this row
        for col in range(0, 9):
            initial_board["square_{}_{}".format(row, col)] = []  # empty
            if puzzle[row][col] != ".":
                given_number = int(puzzle[row][col])
                number_label = "number_{}_in_row_{}".format(given_number, row)
                initial_board["square_{}_{}".format(row, col)].append(number_label)
                avoid_moving.append(number_label)
                numbers.remove(given_number)
        # for the remaining numbers we can just put all of them on a single
        # square in the first column (they will be redistributed by the solver)
        initial_board["square_{}_0".format(row)].extend(
            "number_{}_in_row_{}".format(number, row) for number in numbers
        )

    solver.setAssignment(initial_board)

    spec = AvoidMovingSpec(name="avoid_moving", objects=avoid_moving)
    solver.addConstraint(spec)


def sudoku_add_dimensions(solver: ProblemSolver) -> None:
    # Declare dimensions "dimension_{n}", for a number n it has value 1.
    for number in range(1, 10):
        dimensions = {}
        for row in range(9):
            # addObjectDimension expects an iterable for the value in this map
            dimensions["number_{}_in_row_{}".format(number, row)] = 1
        # pyrefly: ignore [bad-argument-type]
        solver.addObjectDimension("dimension_{}".format(number), dimensions)


def sudoku_add_scopes(solver: ProblemSolver) -> None:
    # Define scopes
    rows = {}  # each row
    columns = {}  # each column
    boxes = {}  # each 3x3 boxes (there are 9 in total)
    for row in range(9):
        for col in range(9):
            square_label = "square_{}_{}".format(row, col)
            rows[square_label] = "row_{}".format(row)
            columns[square_label] = "col_{}".format(col)
            boxes[square_label] = "box_{}_{}".format(row // 3, col // 3)
    # Add scopes to the solver
    solver.addScope("rows", rows)
    solver.addScope("columns", columns)
    solver.addScope("boxes", boxes)


def sudoku_add_constraints(solver: ProblemSolver) -> None:
    limit = Limit(globalLimit=1)

    # Define constraints that limit each dimension to 1.
    # (only one object that has a non-zero dimension value can be
    # present in each scope)
    for scope_name in ["rows", "columns", "boxes"]:
        for number in range(1, 10):
            capacity_spec = CapacitySpec(
                name=f"capacity_{scope_name}_{number}",
                scope=scope_name,
                limit=limit,
                dimension="dimension_{}".format(number),
            )
            solver.addConstraint(capacity_spec)

    # Last but not least, each square should limit to
    # only one number in the square.
    capacity_spec = CapacitySpec(
        name="capacity",
        limit=limit,
        # Default scope defined for all containers (we didn't define it)
        scope="square",
        # Default dimension defined for all objects (we didn't define it)
        dimension="number_count",
    )
    solver.addConstraint(capacity_spec)


def main() -> None:
    solver = ProblemSolver(service_name="rebalancer", service_scope="examples")
    # This is used to define the default dimension (has value 1 for all objects)
    solver.setObjectName("number")
    # This is used to define a default scope (that has all containers)
    solver.setContainerName("square")

    sudoku_initial_assignment(solver)
    sudoku_add_dimensions(solver)
    sudoku_add_scopes(solver)
    sudoku_add_constraints(solver)

    solver_spec = OptimalSolverSpec(solverPackage=OptimalSolverPackage.HIGHS)
    solver.addSolver(solver_spec)

    backupParams = ManifoldBackupParams(uploadPolicy=ManifoldUploadPolicy.ON_FAILURE)
    solver.setManifoldBackupParams(backupParams)

    solution: AssignmentSolution = solver.solve()

    # Display a list of individual moves solver made
    # pyre-fixme[16]: Optional type has no attribute `__iter__`.
    moves = [move for summary in solution.movesSummary for move in summary.moves]
    assert len(moves) > 5
    for move in moves:
        print(
            "Moving {} from {} to {}".format(
                move.object.split("_")[1], move.srcContainer, move.dstContainer
            )
        )

    # Computation is complete, produce pretty output
    output = [[0 for col in range(9)] for row in range(9)]
    for number_label, square_label in solution.assignment.items():
        number = int(number_label.split("_")[1])
        row, col = square_label.split("_")[1:3]
        output[int(row)][int(col)] = number

    print("Final board:")
    for row in range(0, 9):
        print(output[row])

    if solution.finalObjective.value > 0.01:
        raise Exception("failed to solve completely")


if __name__ == "__main__":
    main()
