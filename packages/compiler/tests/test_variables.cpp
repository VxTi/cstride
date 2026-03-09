#include "utils.h"

#include <gtest/gtest.h>

using namespace stride::tests;

TEST(Variables, BasicDeclarations)
{
    const std::string code = R"(
        fn main(): void {
            const pi: f64 = 3.14159D;
            let x: i32 = 10;
            x = 20;
        }
    )";
    assert_parses(code);
}

TEST(Variables, LoopsAndMut)
{
    const std::string code = R"(
        extern fn printf(in: string, ...): void;

        fn main(): void {
            // Using let in a loop
            for (let i: i32 = 0; i < 10; i++) {
                printf("val");
            }
        }
    )";
    assert_parses(code);
}

TEST(Variables, IntegerTypes)
{
    const std::string code = R"(
        fn main(): void {
            let a: i8 = 0 as i8;
            let b: i16 = 0 as i16;
            let c: i32 = 0;
            let d: i64 = 0L;
        }
    )";
    assert_parses(code);
}

TEST(Variables, IntegerLiterals)
{
    const std::string code = R"(
        fn main(): void {
            let hex_val: i32 = 0x12345;
            let large_int: i64 = 9223372036854775807L;
        }
    )";
    assert_parses(code);
}
