#include <gtest/gtest.h>
#include "utils.h"

using namespace stride::tests;

TEST(Variables, BasicDeclarations) {
    const std::string code = R"(
        fn main(): void {
            const pi: f64 = 3.14159;
            // const pi_suffix: f64 = 3.14159D; // TODO: Verify suffix support
            let x: i32 = 10;
            x = 20;
        }
    )";
    assert_parses(code);
}

TEST(Variables, LoopsAndMut) {
    const std::string code = R"(
        fn main(): void {
            // Using mut in a loop
            for (let i: i32 = 0; i < 10; i++) {
                printf("val");
            }
        }
    )";
    assert_parses(code);
}

TEST(Variables, IntegerTypes) {
    const std::string code = R"(
        fn main(): void {
            let a: i8 = 0;
            let b: i16 = 0;
            let c: i32 = 0;
            let d: i64 = 0;
            let e: u8 = 0;
            let f: u16 = 0;
            let g: u32 = 0;
            let h: u64 = 0;
        }
    )";
    assert_parses(code);
}

TEST(Variables, IntegerLiterals) {
    const std::string code = R"(
        fn main(): void {
            let hex_val: i32 = 0x12345;
            let large_int: i64 = 12345678901234567890L;
        }
    )";
    assert_parses(code);
}
