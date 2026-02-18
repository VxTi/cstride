#include <gtest/gtest.h>
#include "utils.h"

using namespace stride::tests;

TEST(Types, VariableInitTypeMismatch)
{
    assert_throws_message(R"(
        const b: i32 = 10l;
    )", "Type mismatch in variable declaration; expected type 'i32', got 'i64'");
}

TEST(Types, StructTypeMismatch)
{
    assert_throws_message(R"(
        struct Point {
            x: i32;
            y: i32;
        }

        const a: Point = Point::{ x: 1.0D, y: 1 };
    )", "Type mismatch for member 'x' in struct initializer 'Point': expected 'i32', got 'f64'");
}

TEST(Types, StructTypeMemberCountMismatch)
{
    assert_throws_message(R"(
        struct Point {
            x: i32;
            y: i32;
        }

        const a: Point = Point::{ x: 1, y: 1, z: 2 };
    )", "Too many members found in struct 'Point': expected 2, got 3");

    assert_throws_message(R"(
        struct Point {
            x: i32;
            y: i32;
        }

        const a: Point = Point::{ x: 1 };
    )", "Too few members found in struct 'Point': expected 2, got 1");
}

TEST(Types, StructMemberTypeMismatch)
{
    assert_throws_message(R"(
        struct Point {
            x: i32;
            y: i32;
        }

        struct Color {
            r: i32;
            g: i32;
            b: i32;
        }

        const a: Point = Color::{ r: 1, g: 2, b: 3 };
    )", "Type mismatch in variable declaration; expected type 'Point', got 'Color'");
}

TEST(Types, StructReferenceTypeMismatch)
{
    assert_throws_message(R"(
        struct Point {
            x: i32;
            y: i32;
        }

        struct Vec = Point;

        const a: Point = Vec::{ x: 1, y: 2 };
    )", "Type mismatch in variable declaration; expected type 'Point', got 'Vec'");
}

TEST(Types, StructMemberOrderMismatch)
{
    assert_throws_message(R"(
        struct Point {
            x: i32;
            y: i32;
        }

        const a: Point = Point::{ y: 1, x: 1 };
    )", "Struct member order mismatch at index 0: expected 'x', got 'y'");
}

TEST(Types, StructMemberUnknownField)
{
    assert_throws_message(R"(
        struct Point {
            x: i32;
            y: i32;
        }

        const a: Point = Point::{ x: 1, unknown: 123 };
    )", "Struct 'Point' has no member named 'unknown'");
}

TEST(Types, ArrayTypeMismatch)
{
    assert_throws_message(R"(
        let a: i32[] = [1L, 2L, 3L];
    )", "Type mismatch in variable declaration; expected type 'Array<i32>', got 'Array<i64>'");
}

TEST(Types, FunctionCallTypeMismatch)
{
    assert_compiles(R"(
        fn add(x: i32, y: i32): i32 {
            return x + y;
        }

        const result: i32 = add(1, 2);
    )");
    assert_throws_message(R"(
        fn add(x: i32, y: i32): i32 {
            return x + y;
        }

        const result: i32 = add(1L, 2L);
    )", "Unable to resolve return type for function 'add'");
}

/*
TEST(ArrayType, PartialTypeMismatch)
{
    assert_throws_message(R"(
        const b: i32[] = [1, 2L, 3];
    )", "expected type '[i32; 3]', got '[i32; 4]'");
}*/