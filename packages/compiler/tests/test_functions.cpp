#include "utils.h"

#include <gtest/gtest.h>

using namespace stride::tests;

TEST(Functions, Lambda)
{
    const std::string code = R"(
        const lambda: (int32) -> int32 = (x: int32): int32 -> {
            return x + 1;
        };

        fn main(): void {
            const result: int32 = lambda(10);
        }
    )";
    assert_compiles(code);
}

TEST(Functions, DeclarationAndInvocation)
{
    const std::string code = R"(
        fn add(a: int32, b: int32): int32 {
            return a + b;
        }

        fn greet(name: string): void {
            printf("Hello!");
        }

        fn main(): void {
            const result: int32 = add(10, 5);
            printf("Stride");
            greet("Stride");
        }
    )";
    assert_parses(code);
}

TEST(Functions, Parameters)
{
    // Test potentially many parameters
    const std::string code = R"(
        fn many_params(a: int32, b: int32, c: int32, d: int32, e: int32): int32 {
            return a + b + c + d + e;
        }
    )";
    assert_parses(code);
}

TEST(Functions, ReturnVoidExplicit)
{
    const std::string code = R"(
        fn do_nothing(): void {
            return;
        }
    )";
    assert_parses(code);
}

TEST(Functions, ReturnVoidImplicit)
{
    const std::string code = R"(
        fn do_nothing_implicitly(): void {
            // No return statement
        }
    )";
    assert_parses(code);
}

TEST(Functions, ExternalFunctions)
{
    const std::string code = R"(
        extern fn malloc(size: u64): u64;
        extern fn free(ptr: u64): void;
    )";
    assert_parses(code);
}
