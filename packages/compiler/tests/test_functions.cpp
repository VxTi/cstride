#include <gtest/gtest.h>
#include "utils.h"

using namespace stride::tests;

TEST(Functions, Lambda) {
    const std::string code = R"(
        const lambda: (i32) -> i32 = (x: i32): i32 -> {
            return x + 1;
        };

        fn main(): void {
            const result: i32 = lambda(10);
        }
    )";
    assert_compiles(code);
}

TEST(Functions, DeclarationAndInvocation) {
    const std::string code = R"(
        fn add(a: i32, b: i32): i32 {
            return a + b;
        }

        fn greet(name: string): void {
            printf("Hello!");
        }

        fn main(): void {
            const result: i32 = add(10, 5);
            printf("Stride");
            greet("Stride");
        }
    )";
    assert_parses(code);
}

TEST(Functions, Parameters) {
    // Test potentially many parameters
    const std::string code = R"(
        fn many_params(a: i32, b: i32, c: i32, d: i32, e: i32): i32 {
            return a + b + c + d + e;
        }
    )";
    assert_parses(code);
}

TEST(Functions, ReturnVoidExplicit) {
    const std::string code = R"(
        fn do_nothing(): void {
            return;
        }
    )";
    assert_parses(code);
}

TEST(Functions, ReturnVoidImplicit) {
    const std::string code = R"(
        fn do_nothing_implicitly(): void {
            // No return statement
        }
    )";
    assert_parses(code);
}

TEST(Functions, ExternalFunctions) {
    const std::string code = R"(
        extern fn malloc(size: u64): u64;
        extern fn free(ptr: u64): void;
    )";
    assert_parses(code);
}
