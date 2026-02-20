#include "utils.h"

#include <gtest/gtest.h>

using namespace stride::tests;

TEST(Variables, BasicDeclarations)
{
    const std::string code = R"(
        fn main(): void {
            const pi: float64 = 3.14159;
            let x: int32 = 10;
            x = 20;
        }
    )";
    assert_parses(code);
}

TEST(Variables, LoopsAndMut)
{
    const std::string code = R"(
        fn main(): void {
            // Using let in a loop
            for (let i: int32 = 0; i < 10; i++) {
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
            let a: int8 = 0;
            let b: int16 = 0;
            let c: int32 = 0;
            let d: int64 = 0;
            let e: u8 = 0;
            let f: u16 = 0;
            let g: u32 = 0;
            let h: u64 = 0;
        }
    )";
    assert_parses(code);
}

TEST(Variables, IntegerLiterals)
{
    const std::string code = R"(
        fn main(): void {
            let hex_val: int32 = 0x12345;
            let large_int: int64 = 9223372036854775807L;
        }
    )";
    assert_parses(code);
}
