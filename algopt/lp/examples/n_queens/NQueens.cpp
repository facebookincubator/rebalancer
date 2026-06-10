// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "algopt/lp/lp.h"

#include "fmt/core.h"
#include <folly/container/irange.h>
#include <folly/init/Init.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>

#include <sstream>
#include <vector>

using namespace facebook::algopt::lp;
using namespace std;

using Board = vector<vector<Variable>>;

DEFINE_string(
    solver,
    "xpress",
    "Name of the underlying solver. Options: xpress, gurobi");

DEFINE_int32(board_size, 8, "Size of the chess board.");

Problem makeProblem() {
  if (FLAGS_solver == "xpress") {
    return ProblemFactory::makeXpressProblem();
  }
  if (FLAGS_solver == "gurobi") {
    return ProblemFactory::makeGurobiProblem();
  }
  throw std::runtime_error(fmt::format("unknown solver {}", FLAGS_solver));
}

Board makeBoard(Problem& problem) {
  const int n = FLAGS_board_size;
  Board board;
  for (const auto i : folly::irange(n)) {
    board.emplace_back();
    for (const auto j : folly::irange(n)) {
      auto var = problem.makeBoolVar(fmt::format("cell_{}_{}", i, j));
      board.back().push_back(var);
    }
  }
  return board;
}

Expression sumLine(
    const Board& board,
    int startI,
    int startJ,
    int directionI,
    int directionJ) {
  const int n = board.size();

  Expression sum = 0;
  int i = startI;
  int j = startJ;

  while (0 <= i && i < n && 0 <= j && j < n) {
    sum += board[i][j];
    i += directionI;
    j += directionJ;
  }

  return sum;
}

void addTotalCountConstraint(Problem& problem, const Board& board) {
  const int n = board.size();
  Expression queenCount = 0;
  for (const auto i : folly::irange(n)) {
    for (const auto j : folly::irange(n)) {
      queenCount += board[i][j];
    }
  }
  problem.newConstraint(queenCount == n, "total_count");
}

void addRowConstraint(Problem& problem, const Board& board) {
  const int n = board.size();
  for (const auto i : folly::irange(n)) {
    const Expression count = sumLine(board, i, 0, 0, 1);
    problem.newConstraint(count <= 1, fmt::format("row_{}", i));
  }
}

void addColumnConstraint(Problem& problem, const Board& board) {
  const int n = board.size();
  for (const auto j : folly::irange(n)) {
    const Expression count = sumLine(board, 0, j, 1, 0);
    problem.newConstraint(count <= 1, fmt::format("column_{}", j));
  }
}

void addTopRightDiagonalConstraint(Problem& problem, const Board& board) {
  const int n = board.size();

  // Diagonals beginning at (k, 0)
  for (const auto k : folly::irange(n)) {
    const Expression count = sumLine(board, k, 0, 1, 1);
    problem.newConstraint(
        count <= 1, fmt::format("top_right_diagonal_{}_0", k));
  }

  // Diagonals beginning at (0, k)
  for (const auto k : folly::irange(1, n)) {
    const Expression count = sumLine(board, 0, k, 1, 1);
    problem.newConstraint(
        count <= 1, fmt::format("top_right_diagonal_0_{}", k));
  }
}

void addTopLeftDiagonalConstraint(Problem& problem, const Board& board) {
  const int n = board.size();

  // Diagonals beginning at (0, k)
  for (const auto k : folly::irange(n)) {
    const Expression count = sumLine(board, 0, k, 1, -1);
    problem.newConstraint(count <= 1, fmt::format("top_left_diagonal_0_{}", k));
  }

  // Diagonals beginning at (k, n - 1)
  for (const auto k : folly::irange(1, n)) {
    const Expression count = sumLine(board, k, n - 1, 1, -1);
    problem.newConstraint(
        count <= 1, fmt::format("top_left_diagonal_{}_{}", k, n - 1));
  }
}

void printSolution(const Board& board) {
  const int n = board.size();
  stringstream ss;
  for (const auto i : folly::irange(n)) {
    if (i != 0) {
      ss << endl;
    }
    for (const auto j : folly::irange(n)) {
      if (board[i][j].getValue() < 0.5) {
        ss << ".";
      } else {
        ss << "Q";
      }
    }
  }
  XLOG(INFO) << "Solution:";
  XLOG(INFO) << ss.str();
}

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  auto problem = makeProblem();

  // Make an NxN board represented by a matrix of boolean variables.
  auto board = makeBoard(problem);

  // The overall board must contain exactly N queens.
  addTotalCountConstraint(problem, board);

  // Each row may contain at most one queen.
  addRowConstraint(problem, board);

  // Each column may contain at most one queen.
  addColumnConstraint(problem, board);

  // Each top-to-right diagonal may contain at most one queen.
  addTopRightDiagonalConstraint(problem, board);

  // Each top-to-left diagonal may contain at most one queen.
  addTopLeftDiagonalConstraint(problem, board);

  problem.solve();

  if (problem.getStatus() != thrift::ProblemStatus::OPTIMAL_FOUND &&
      problem.getStatus() != thrift::ProblemStatus::SOLUTION_FOUND) {
    throw std::runtime_error("solution not found");
  }

  printSolution(board);
}
