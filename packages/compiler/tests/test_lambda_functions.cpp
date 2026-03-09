#include "utils.h"

#include <gtest/gtest.h>

using namespace stride::tests;

// ============================================================================
// Basic Lambda Tests
// ============================================================================

TEST(Lambda, BasicLambdaWithSingleParameter)
{
    const std::string code = R"(
        const increment: (i32) -> i32 = (x: i32): i32 -> {
            return x + 1;
        };

        fn main(): void {
            const result: i32 = increment(10);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, BasicLambdaWithMultipleParameters)
{
    const std::string code = R"(
        const add: (i32, i32) -> i32 = (x: i32, y: i32): i32 -> {
            return x + y;
        };

        fn main(): void {
            const result: i32 = add(5, 10);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, BasicLambdaWithNoParameters)
{
    const std::string code = R"(
        const get_constant: () -> i32 = (): i32 -> {
            return 42;
        };

        fn main(): void {
            const result: i32 = get_constant();
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, LambdaReturningVoid)
{
    const std::string code = R"(
        const do_something: () -> void = (): void -> {
            // Some work
        };

        fn main(): void {
            do_something();
        }
    )";
    assert_compiles(code);
}

// ============================================================================
// Lambda Type Tests
// ============================================================================

TEST(Lambda, LambdaWithi64Parameters)
{
    const std::string code = R"(
        const multiply: (i64, i64) -> i64 = (x: i64, y: i64): i64 -> {
            return x * y;
        };

        fn main(): void {
            const result: i64 = multiply(100L, 200L);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, LambdaWithFloatParameters)
{
    const std::string code = R"(
        const divide: (f32, f32) -> f32 = (x: f32, y: f32): f32 -> {
            return x / y;
        };

        fn main(): void {
            const result: f32 = divide(10.0, 2.0);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, LambdaWithf64Parameters)
{
    const std::string code = R"(
        const power: (f64, f64) -> f64 = (x: f64, y: f64): f64 -> {
            return x * y;
        };

        fn main(): void {
            const result: f64 = power(2.0D, 3.0D);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, LambdaWithMixedParameters)
{
    const std::string code = R"(
        const convert: (i32, f64) -> f64 = (x: i32, scale: f64): f64 -> {
            return 1.0D;
        };

        fn main(): void {
            const result: f64 = convert(10, 2.5D);
        }
    )";
    assert_compiles(code);
}

// ============================================================================
// Lambda Assignment and Reassignment Tests
// ============================================================================

TEST(Lambda, LambdaAssignmentToVariable)
{
    const std::string code = R"(
        const add: (i32, i32) -> i32 = (x: i32, y: i32): i32 -> {
            return x + y;
        };

        fn main(): void {
            const another_add: (i32, i32) -> i32 = add;
            const result: i32 = another_add(3, 4);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, MutableLambdaReassignment)
{
    const std::string code = R"(
        let operation: (i32, i32) -> i32 = (x: i32, y: i32): i32 -> {
            return x + y;
        };

        fn main(): void {
            operation = (x: i32, y: i32): i32 -> {
                return x - y;
            };
            const result: i32 = operation(10, 5);
        }
    )";
    assert_compiles(code);
}

// ============================================================================
// Lambda Invocation Tests
// ============================================================================

/*
 TODO: Uncomment when supported
 TEST(Lambda, LambdaInvokedImmediately)
{
    const std::string code = R"(
        fn main(): void {
            const result: i32 = ((x: i32): i32 -> {
                return x * 2;
            })(5);
        }
    )";
    assert_compiles(code);
}*/

TEST(Lambda, LambdaInvokedMultipleTimes)
{
    const std::string code = R"(
        const square: (i32) -> i32 = (x: i32): i32 -> {
            return x * x;
        };

        fn main(): void {
            const r1: i32 = square(2);
            const r2: i32 = square(3);
            const r3: i32 = square(4);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, LambdaPassedToFunction)
{
    const std::string code = R"(
        fn apply(f: (i32) -> i32, value: i32): i32 {
            return f(value);
        }

        fn main(): void {
            const double_it: (i32) -> i32 = (x: i32): i32 -> {
                return x * 2;
            };

            const result: i32 = apply(double_it, 10);
        }
    )";
    assert_compiles(code);
}

// ============================================================================
// Lambda with Complex Bodies
// ============================================================================

TEST(Lambda, LambdaWithMultipleStatements)
{
    const std::string code = R"(
        const compute: (i32) -> i32 = (x: i32): i32 -> {
            const temp: i32 = x + 10;
            const result: i32 = temp * 2;
            return result;
        };

        fn main(): void {
            const output: i32 = compute(5);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, LambdaWithConditional)
{
    const std::string code = R"(
        const absolute: (i32) -> i32 = (x: i32): i32 -> {
            if (x < 0) {
                return -x;
            }
            return x;
        };

        fn main(): void {
            const result: i32 = absolute(-5);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, LambdaWithNestedConditionals)
{
    const std::string code = R"(
        const classify: (i32) -> i32 = (x: i32): i32 -> {
            if (x < 0) {
                return -1;
            } else {
                if (x > 0) {
                    return 1;
                }
                return 0;
            }
        };

        fn main(): void {
            const result: i32 = classify(5);
        }
    )";
    assert_compiles(code);
}

// ============================================================================
// Lambda with Many Parameters (Edge Cases)
// ============================================================================

TEST(Lambda, LambdaWithManyParameters)
{
    const std::string code = R"(
        const sum_many: (i32, i32, i32, i32, i32) -> i32 =
            (a: i32, b: i32, c: i32, d: i32, e: i32): i32 -> {
            return a + b + c + d + e;
        };

        fn main(): void {
            const result: i32 = sum_many(1, 2, 3, 4, 5);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, LambdaWithTenParameters)
{
    const std::string code = R"(
        const sum_ten: (i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) -> i32 =
            (a: i32, b: i32, c: i32, d: i32, e: i32, f: i32, g: i32, h: i32, i: i32, j: i32): i32 -> {
            return a + b + c + d + e + f + g + h + i + j;
        };

        fn main(): void {
            const result: i32 = sum_ten(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
        }
    )";
    assert_compiles(code);
}

// ============================================================================
// Lambda Nesting and Higher-Order Functions
// ============================================================================

TEST(Lambda, LambdaReturningLambda)
{
    const std::string code = R"(
        const create_adder: (i32) -> (i32) -> i32 = (x: i32): (i32) -> i32 -> {
            return (y: i32): i32 -> {
                return x + y;
            };
        };

        fn main(): void {
            const add_five: (i32) -> i32 = create_adder(5);
            const result: i32 = add_five(10);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, NestedLambdas)
{
    const std::string code = R"(
        const outer: (i32) -> i32 = (x: i32): i32 -> {
            const inner: (i32) -> i32 = (y: i32): i32 -> {
                return y + 1;
            };
            return inner(x) + 1;
        };

        fn main(): void {
            const result: i32 = outer(5);
        }
    )";
    assert_compiles(code);
}

// ============================================================================
// Lambda Type Errors
// ============================================================================

TEST(LambdaErrors, TypeMismatchInDeclaration)
{
    assert_throws_message(
        R"(
        const add: (i32, i32) -> i64 = (x: i32, y: i32): i32 -> {
            return x + y;
        };
    )",
        "Type mismatch in variable declaration: cannot assign value of type '(i32, i32) -> i32' to type '(i32, i32) -> i64'");
}

TEST(LambdaErrors, ParameterTypeMismatch)
{
    assert_throws_message(
        R"(
        const add: (i64, i64) -> i64 = (x: i32, y: i32): i64 -> {
            return 1L;
        };
    )",
        "Type mismatch in variable declaration: cannot assign value of type '(i32, i32) -> i64' to type '(i64, i64) -> i64'");
}

TEST(LambdaErrors, ParameterCountMismatch)
{
    assert_throws_message(
        R"(
        const add: (i32) -> i32 = (x: i32, y: i32): i32 -> {
            return x + y;
        };
    )",
        "Type mismatch in variable declaration: cannot assign value of type '(i32, i32) -> i32' to type '(i32) -> i32'");
}

TEST(LambdaErrors, WrongReturnType)
{
    assert_throws_message(
        R"(
        const get_number: () -> i32 = (): i32 -> {
            return 1L;
        };
    )",
        "expected a return type of 'i32', but received 'i64'");
}

TEST(LambdaErrors, MissingReturnStatement)
{
    assert_throws_message(
        R"(
        const get_number: () -> i32 = (): i32 -> {
            const x: i32 = 10;
        };
    )",
        "is missing a return statement");
}

TEST(LambdaErrors, VoidReturnWithValue)
{
    assert_throws_message(
        R"(
        const do_nothing: () -> void = (): void -> {
            return 42;
        };
    )",
        "has return type 'void' and cannot return a value");
}

TEST(LambdaErrors, InvokeWithWrongArgumentType)
{
    assert_throws_message(
        R"(
        const add: (i32, i32) -> i32 = (x: i32, y: i32): i32 -> {
            return x + y;
        };

        fn main(): void {
            const result: i32 = add(1L, 2L);
        }
    )",
        "Type mismatch for argument 1 in anonymous function call 'add': expected type 'i32', got 'i64'");
}

TEST(LambdaErrors, InvokeWithWrongArgumentCount)
{
    assert_throws_message(
        R"(
        const add: (i32, i32) -> i32 = (x: i32, y: i32): i32 -> {
            return x + y;
        };

        fn main(): void {
            const result: i32 = add(1);
        }
    )",
        "Incorrect number of arguments for lambda call to 'add': expected 2, got 1");
}

// ============================================================================
// Lambda Array Tests
// ============================================================================

TEST(Lambda, LambdaInArray)
{
    const std::string code = R"(
        const lambdas: ((i32) -> i32)[] = [
            (x: i32): i32 -> { return x + 1; },
            (x: i32): i32 -> { return x * 2; },
            (x: i32): i32 -> { return x - 1; }
        ];

        fn main(): void {
            const first: (i32) -> i32 = lambdas[0];
            const result: i32 = first(10);
        }
    )";
    assert_compiles(code);
}

// ============================================================================
// Lambda with Return Path Complexity
// ============================================================================

TEST(Lambda, LambdaWithMultipleReturnPaths)
{
    const std::string code = R"(
        const max: (i32, i32) -> i32 = (x: i32, y: i32): i32 -> {
            if (x > y) {
                return x;
            } else {
                return y;
            }
        };

        fn main(): void {
            const result: i32 = max(10, 20);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, LambdaWithEarlyReturn)
{
    const std::string code = R"(
        const check_positive: (i32) -> i32 = (x: i32): i32 -> {
            if (x < 0) {
                return 0;
            }
            return x;
        };

        fn main(): void {
            const result: i32 = check_positive(-5);
        }
    )";
    assert_compiles(code);
}

TEST(LambdaErrors, LambdaInconsistentReturnTypes)
{
    assert_throws_message(
        R"(
        const inconsistent: (i32) -> i32 = (x: i32): i32 -> {
            if (x > 0) {
                return 1L;
            }
            return 0;
        };
    )",
        "expected a return type of 'i32', but received 'i64'");
}

// ============================================================================
// Lambda Variable Scope Tests
// ============================================================================

TEST(Lambda, LambdaAccessingOuterScope)
{
    const std::string code = R"(
        fn main(): void {
            const factor: i32 = 10;
            const multiply_by_factor: (i32) -> i32 = (x: i32): i32 -> {
                return x * factor;
            };
            const result: i32 = multiply_by_factor(5);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, LambdaParameterShadowingOuter)
{
    const std::string code = R"(
        fn main(): void {
            const x: i32 = 100;
            const use_param: (i32) -> i32 = (x: i32): i32 -> {
                return x + 1;
            };
            const result: i32 = use_param(5);
        }
    )";
    assert_compiles(code);
}

// ============================================================================
// Complex Lambda Scenarios
// ============================================================================

TEST(Lambda, MultipleLambdasInSameScope)
{
    const std::string code = R"(
        const add: (i32, i32) -> i32 = (x: i32, y: i32): i32 -> {
            return x + y;
        };

        const subtract: (i32, i32) -> i32 = (x: i32, y: i32): i32 -> {
            return x - y;
        };

        const multiply: (i32, i32) -> i32 = (x: i32, y: i32): i32 -> {
            return x * y;
        };

        fn main(): void {
            const r1: i32 = add(10, 5);
            const r2: i32 = subtract(10, 5);
            const r3: i32 = multiply(10, 5);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, LambdaReturningVoidWithMultipleStatements)
{
    const std::string code = R"(
        const perform_operations: (i32) -> void = (x: i32): void -> {
            const temp1: i32 = x + 1;
            const temp2: i32 = temp1 * 2;
        };

        fn main(): void {
            perform_operations(5);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, LambdaWithExplicitVoidReturn)
{
    const std::string code = R"(
        const do_nothing: () -> void = (): void -> {
            return;
        };

        fn main(): void {
            do_nothing();
        }
    )";
    assert_compiles(code);
}

// ============================================================================
// Lambda Composition Tests
// ============================================================================

TEST(Lambda, LambdaComposition)
{
    const std::string code = R"(
        const add_one: (i32) -> i32 = (x: i32): i32 -> {
            return x + 1;
        };

        const double_it: (i32) -> i32 = (x: i32): i32 -> {
            return x * 2;
        };

        fn main(): void {
            const result: i32 = double_it(add_one(5));
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, LambdaChainingMultipleCalls)
{
    const std::string code = R"(
        const add_one: (i32) -> i32 = (x: i32): i32 -> {
            return x + 1;
        };

        fn main(): void {
            const result: i32 = add_one(add_one(add_one(5)));
        }
    )";
    assert_compiles(code);
}

// ============================================================================
// Edge Case: Empty Lambda Body (Should Fail)
// ============================================================================

TEST(LambdaErrors, LambdaEmptyBodyNonVoid)
{
    assert_throws_message(
        R"(
        const empty: () -> i32 = (): i32 -> {
        };
    )",
        "is missing a return statement");
}

TEST(Lambda, LambdaEmptyBodyVoid)
{
    const std::string code = R"(
        const do_nothing: () -> void = (): void -> {
        };

        fn main(): void {
            do_nothing();
        }
    )";
    assert_compiles(code);
}