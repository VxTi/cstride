#include "utils.h"

#include <gtest/gtest.h>

using namespace stride::tests;

// ============================================================================
// Basic Lambda Tests
// ============================================================================

TEST(Lambda, BasicLambdaWithSingleParameter)
{
    const std::string code = R"(
        const increment: (int32) -> int32 = (x: int32): int32 -> {
            return x + 1;
        };

        fn main(): void {
            const result: int32 = increment(10);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, BasicLambdaWithMultipleParameters)
{
    const std::string code = R"(
        const add: (int32, int32) -> int32 = (x: int32, y: int32): int32 -> {
            return x + y;
        };

        fn main(): void {
            const result: int32 = add(5, 10);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, BasicLambdaWithNoParameters)
{
    const std::string code = R"(
        const get_constant: () -> int32 = (): int32 -> {
            return 42;
        };

        fn main(): void {
            const result: int32 = get_constant();
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

TEST(Lambda, LambdaWithInt64Parameters)
{
    const std::string code = R"(
        const multiply: (int64, int64) -> int64 = (x: int64, y: int64): int64 -> {
            return x * y;
        };

        fn main(): void {
            const result: int64 = multiply(100L, 200L);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, LambdaWithFloatParameters)
{
    const std::string code = R"(
        const divide: (float32, float32) -> float32 = (x: float32, y: float32): float32 -> {
            return x / y;
        };

        fn main(): void {
            const result: float32 = divide(10.0, 2.0);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, LambdaWithFloat64Parameters)
{
    const std::string code = R"(
        const power: (float64, float64) -> float64 = (x: float64, y: float64): float64 -> {
            return x * y;
        };

        fn main(): void {
            const result: float64 = power(2.0D, 3.0D);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, LambdaWithMixedParameters)
{
    const std::string code = R"(
        const convert: (int32, float64) -> float64 = (x: int32, scale: float64): float64 -> {
            return 1.0D;
        };

        fn main(): void {
            const result: float64 = convert(10, 2.5D);
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
        const add: (int32, int32) -> int32 = (x: int32, y: int32): int32 -> {
            return x + y;
        };

        fn main(): void {
            const another_add: (int32, int32) -> int32 = add;
            const result: int32 = another_add(3, 4);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, MutableLambdaReassignment)
{
    const std::string code = R"(
        let operation: (int32, int32) -> int32 = (x: int32, y: int32): int32 -> {
            return x + y;
        };

        fn main(): void {
            operation = (x: int32, y: int32): int32 -> {
                return x - y;
            };
            const result: int32 = operation(10, 5);
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
            const result: int32 = ((x: int32): int32 -> {
                return x * 2;
            })(5);
        }
    )";
    assert_compiles(code);
}*/

TEST(Lambda, LambdaInvokedMultipleTimes)
{
    const std::string code = R"(
        const square: (int32) -> int32 = (x: int32): int32 -> {
            return x * x;
        };

        fn main(): void {
            const r1: int32 = square(2);
            const r2: int32 = square(3);
            const r3: int32 = square(4);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, LambdaPassedToFunction)
{
    const std::string code = R"(
        fn apply(f: (int32) -> int32, value: int32): int32 {
            return f(value);
        }

        fn main(): void {
            const double_it: (int32) -> int32 = (x: int32): int32 -> {
                return x * 2;
            };

            const result: int32 = apply(double_it, 10);
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
        const compute: (int32) -> int32 = (x: int32): int32 -> {
            const temp: int32 = x + 10;
            const result: int32 = temp * 2;
            return result;
        };

        fn main(): void {
            const output: int32 = compute(5);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, LambdaWithConditional)
{
    const std::string code = R"(
        const absolute: (int32) -> int32 = (x: int32): int32 -> {
            if (x < 0) {
                return -x;
            }
            return x;
        };

        fn main(): void {
            const result: int32 = absolute(-5);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, LambdaWithNestedConditionals)
{
    const std::string code = R"(
        const classify: (int32) -> int32 = (x: int32): int32 -> {
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
            const result: int32 = classify(5);
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
        const sum_many: (int32, int32, int32, int32, int32) -> int32 =
            (a: int32, b: int32, c: int32, d: int32, e: int32): int32 -> {
            return a + b + c + d + e;
        };

        fn main(): void {
            const result: int32 = sum_many(1, 2, 3, 4, 5);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, LambdaWithTenParameters)
{
    const std::string code = R"(
        const sum_ten: (int32, int32, int32, int32, int32, int32, int32, int32, int32, int32) -> int32 =
            (a: int32, b: int32, c: int32, d: int32, e: int32, f: int32, g: int32, h: int32, i: int32, j: int32): int32 -> {
            return a + b + c + d + e + f + g + h + i + j;
        };

        fn main(): void {
            const result: int32 = sum_ten(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
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
        const create_adder: (int32) -> (int32) -> int32 = (x: int32): (int32) -> int32 -> {
            return (y: int32): int32 -> {
                return x + y;
            };
        };

        fn main(): void {
            const add_five: (int32) -> int32 = create_adder(5);
            const result: int32 = add_five(10);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, NestedLambdas)
{
    const std::string code = R"(
        const outer: (int32) -> int32 = (x: int32): int32 -> {
            const inner: (int32) -> int32 = (y: int32): int32 -> {
                return y + 1;
            };
            return inner(x) + 1;
        };

        fn main(): void {
            const result: int32 = outer(5);
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
        const add: (int32, int32) -> int64 = (x: int32, y: int32): int32 -> {
            return x + y;
        };
    )",
        "Type mismatch in variable declaration; expected type '(int32, int32) -> int64', got '(int32, int32) -> int32'");
}

TEST(LambdaErrors, ParameterTypeMismatch)
{
    assert_throws_message(
        R"(
        const add: (int64, int64) -> int64 = (x: int32, y: int32): int64 -> {
            return 1L;
        };
    )",
        "Type mismatch in variable declaration; expected type '(int64, int64) -> int64', got '(int32, int32) -> int64'");
}

TEST(LambdaErrors, ParameterCountMismatch)
{
    assert_throws_message(
        R"(
        const add: (int32) -> int32 = (x: int32, y: int32): int32 -> {
            return x + y;
        };
    )",
        "Type mismatch in variable declaration; expected type '(int32) -> int32', got '(int32, int32) -> int32'");
}

TEST(LambdaErrors, WrongReturnType)
{
    assert_throws_message(
        R"(
        const get_number: () -> int32 = (): int32 -> {
            return 1L;
        };
    )",
        "expected a return type of 'int32', but received 'int64'");
}

TEST(LambdaErrors, MissingReturnStatement)
{
    assert_throws_message(
        R"(
        const get_number: () -> int32 = (): int32 -> {
            const x: int32 = 10;
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
        const add: (int32, int32) -> int32 = (x: int32, y: int32): int32 -> {
            return x + y;
        };

        fn main(): void {
            const result: int32 = add(1L, 2L);
        }
    )",
        "Type mismatch for argument 1 in anonymous function call 'add': expected type 'int32', got 'int64'");
}

TEST(LambdaErrors, InvokeWithWrongArgumentCount)
{
    assert_throws_message(
        R"(
        const add: (int32, int32) -> int32 = (x: int32, y: int32): int32 -> {
            return x + y;
        };

        fn main(): void {
            const result: int32 = add(1);
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
        const lambdas: ((int32) -> int32)[] = [
            (x: int32): int32 -> { return x + 1; },
            (x: int32): int32 -> { return x * 2; },
            (x: int32): int32 -> { return x - 1; }
        ];

        fn main(): void {
            const first: (int32) -> int32 = lambdas[0];
            const result: int32 = first(10);
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
        const max: (int32, int32) -> int32 = (x: int32, y: int32): int32 -> {
            if (x > y) {
                return x;
            } else {
                return y;
            }
        };

        fn main(): void {
            const result: int32 = max(10, 20);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, LambdaWithEarlyReturn)
{
    const std::string code = R"(
        const check_positive: (int32) -> int32 = (x: int32): int32 -> {
            if (x < 0) {
                return 0;
            }
            return x;
        };

        fn main(): void {
            const result: int32 = check_positive(-5);
        }
    )";
    assert_compiles(code);
}

TEST(LambdaErrors, LambdaInconsistentReturnTypes)
{
    assert_throws_message(
        R"(
        const inconsistent: (int32) -> int32 = (x: int32): int32 -> {
            if (x > 0) {
                return 1L;
            }
            return 0;
        };
    )",
        "expected a return type of 'int32', but received 'int64'");
}

// ============================================================================
// Lambda Variable Scope Tests
// ============================================================================

TEST(Lambda, LambdaAccessingOuterScope)
{
    const std::string code = R"(
        fn main(): void {
            const factor: int32 = 10;
            const multiply_by_factor: (int32) -> int32 = (x: int32): int32 -> {
                return x * factor;
            };
            const result: int32 = multiply_by_factor(5);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, LambdaParameterShadowingOuter)
{
    const std::string code = R"(
        fn main(): void {
            const x: int32 = 100;
            const use_param: (int32) -> int32 = (x: int32): int32 -> {
                return x + 1;
            };
            const result: int32 = use_param(5);
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
        const add: (int32, int32) -> int32 = (x: int32, y: int32): int32 -> {
            return x + y;
        };

        const subtract: (int32, int32) -> int32 = (x: int32, y: int32): int32 -> {
            return x - y;
        };

        const multiply: (int32, int32) -> int32 = (x: int32, y: int32): int32 -> {
            return x * y;
        };

        fn main(): void {
            const r1: int32 = add(10, 5);
            const r2: int32 = subtract(10, 5);
            const r3: int32 = multiply(10, 5);
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, LambdaReturningVoidWithMultipleStatements)
{
    const std::string code = R"(
        const perform_operations: (int32) -> void = (x: int32): void -> {
            const temp1: int32 = x + 1;
            const temp2: int32 = temp1 * 2;
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
        const add_one: (int32) -> int32 = (x: int32): int32 -> {
            return x + 1;
        };

        const double_it: (int32) -> int32 = (x: int32): int32 -> {
            return x * 2;
        };

        fn main(): void {
            const result: int32 = double_it(add_one(5));
        }
    )";
    assert_compiles(code);
}

TEST(Lambda, LambdaChainingMultipleCalls)
{
    const std::string code = R"(
        const add_one: (int32) -> int32 = (x: int32): int32 -> {
            return x + 1;
        };

        fn main(): void {
            const result: int32 = add_one(add_one(add_one(5)));
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
        const empty: () -> int32 = (): int32 -> {
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