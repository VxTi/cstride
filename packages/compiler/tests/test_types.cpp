#include <gtest/gtest.h>
#include "utils.h"

using namespace stride::tests;

TEST(VariableInitType, TypeMismatch)
{
    assert_throws_message(R"(
        const b: i32 = 10l;
    )", "expected type 'i32', got 'i64'");
}

TEST(StructType, TypeMismatch)
{
    assert_throws_message(R"(
        struct Point {
            x: i32;
            y: i32;
        }

        struct Color {
            x: i32;
            y: i32;
        }

        const a: Point = Color::{ x: 1, y: 2 };
    )", "expected type 'Point', got 'Color'");
}

TEST(ArrayType, TypeMismatch)
{
    assert_throws_message(R"(
        let a: i32[] = [1L, 2L, 3L];
    )", "expected type 'Array<i32>', got 'Array<i64>'");
}

/*
TEST(ArrayType, PartialTypeMismatch)
{
    assert_throws_message(R"(
        const b: i32[] = [1, 2L, 3];
    )", "expected type '[i32; 3]', got '[i32; 4]'");
}*/