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

import unittest

from algopt.lp.detail.polyglot.py_bindings import lp_problem


class TestLpProblem(unittest.TestCase):
    """Tests for LP problem bindings."""

    def test_create_variables(self) -> None:
        """Test creating different types of variables."""
        problem = lp_problem.ProblemFactory.make_xpress_problem()

        # Create continuous variable
        x = problem.make_var("x")
        x.set_bounds(0.0, 10.0)

        # Create integer variable
        y = problem.make_int_var("y")
        y.set_lb(0.0)
        y.set_ub(5.0)

        # Create boolean variable
        z = problem.make_bool_var("z")

        # If we can create these without exceptions, the test passes
        self.assertIsNotNone(x)
        self.assertIsNotNone(y)
        self.assertIsNotNone(z)

    def test_expression_operations(self) -> None:
        """Test building and manipulating expressions."""
        problem = lp_problem.ProblemFactory.make_xpress_problem()

        x = problem.make_var("x")
        y = problem.make_var("y")

        # Create expression from variable
        expr = lp_problem.Expression.from_variable(x)
        expr.add(5.0)  # x + 5

        # Create another expression
        expr2 = lp_problem.Expression.from_variable(y)
        expr2.multiply(2.0)  # 2*y

        # Add expressions together
        expr.add_expr(expr2)  # x + 5 + 2*y

        # Test constant expression
        const_expr = lp_problem.Expression.from_constant(10.0)
        self.assertTrue(const_expr.is_constant())
        self.assertEqual(const_expr.get_constant(), 10.0)

    def test_simple_optimization(self) -> None:
        """Test creating and building a simple optimization problem."""
        problem = lp_problem.ProblemFactory.make_xpress_problem()
        problem.disable_logs()

        # Create variables
        x = problem.make_var("x")
        x.set_bounds(0.0, 10.0)

        y = problem.make_var("y")
        y.set_bounds(0.0, 10.0)

        # Create objective: x + y (use variable multiply to create expressions)
        obj_x = x.multiply(1.0)
        obj_y = y.multiply(1.0)
        obj_expr = obj_x
        obj_expr.add_expr(obj_y)
        problem.set_objective(obj_expr)

        # Add constraint: x >= 1
        constraint = x.ge(1.0)
        problem.new_constraint(constraint, "x_min")

        # Just verify we can solve without crashing
        problem.solve()
        solution = problem.get_solution()

        # Check that we got some solution status (don't assert exact value)
        status = solution.get_status()
        self.assertIsNotNone(status)

    def test_constraint_with_relations(self) -> None:
        """Test creating constraints using relation builders."""
        problem = lp_problem.ProblemFactory.make_xpress_problem()

        x = problem.make_var("x")
        x.set_bounds(0.0, 100.0)

        # Create constraint using relation: x >= 5
        relation = x.ge(5.0)
        constraint = problem.new_constraint(relation, "x_lower_bound")

        self.assertIsNotNone(constraint)

        # Create another constraint using expression relation
        expr = lp_problem.Expression.from_variable(x)
        expr.multiply(2.0)  # 2*x
        relation2 = expr.le(20.0)  # 2*x <= 20
        constraint2 = problem.new_constraint(relation2, "x_upper_scaled")

        self.assertIsNotNone(constraint2)

    def test_sum_variables(self) -> None:
        """Test using sum_vars to create expressions from multiple variables."""
        problem = lp_problem.ProblemFactory.make_xpress_problem()
        problem.disable_logs()

        # Create several variables
        x1 = problem.make_var("x1")
        x1.set_bounds(0.0, 10.0)

        x2 = problem.make_var("x2")
        x2.set_bounds(0.0, 10.0)

        x3 = problem.make_var("x3")
        x3.set_bounds(0.0, 10.0)

        # Use sum_vars to create an expression
        sum_expr = lp_problem.sum_vars([x1, x2, x3])

        # Verify the expression is not constant
        self.assertFalse(sum_expr.is_constant())

        # Use the summed expression as an objective
        problem.set_objective(sum_expr)

        # Add a constraint using the sum
        x4 = problem.make_var("x4")
        x4.set_bounds(0.0, 5.0)
        x5 = problem.make_var("x5")
        x5.set_bounds(0.0, 5.0)

        constraint_expr = lp_problem.sum_vars([x4, x5])
        constraint = constraint_expr.le(8.0)
        problem.new_constraint(constraint, "sum_constraint")

        # Solve and verify we get a solution
        problem.solve()
        solution = problem.get_solution()
        self.assertIsNotNone(solution.get_status())

    def test_build_sum_expr_with_add_var(self) -> None:
        """Test building a sum expression using Expression.add_var().

        This tests the pattern used in minimize_shards_with_memory._build_sum_expr
        which creates an empty Expression and uses add_var() to add variables
        one by one. The add_var() method takes ref<Variable> allowing variables
        to be reused after being added to expressions.
        """
        problem = lp_problem.ProblemFactory.make_xpress_problem()
        problem.disable_logs()

        # Create several variables
        vars_list = []
        for i in range(5):
            v = problem.make_var(f"x{i}")
            v.set_bounds(0.0, 10.0)
            vars_list.append(v)

        # Build sum expression using add_var() - mirrors _build_sum_expr pattern
        sum_expr = lp_problem.Expression()
        for v in vars_list:
            sum_expr.add_var(v)

        # Verify the expression is not constant (has variables)
        self.assertFalse(sum_expr.is_constant())

        # Verify variables can be reused after add_var (key property being tested)
        # Create another expression reusing the same variables
        sum_expr2 = lp_problem.Expression()
        for v in vars_list:
            sum_expr2.add_var(v)

        self.assertFalse(sum_expr2.is_constant())

        # Use the summed expression in a constraint
        constraint = sum_expr.le(30.0)
        problem.new_constraint(constraint, "sum_le_30")

        # Set objective using the second expression
        problem.set_objective(sum_expr2)

        # Solve and verify we get a solution
        problem.solve()
        solution = problem.get_solution()
        self.assertIsNotNone(solution.get_status())


if __name__ == "__main__":
    unittest.main()
