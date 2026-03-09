#include "ast/parsing_context.h"
#include "utils.h"

#include <gtest/gtest.h>

using namespace stride::tests;

TEST(TypeCast, IdentityCast)
{
    assert_compiles(R"(
        const a: i32 = 10;
        const b: i32 = a as i32;
    )");
}

TEST(TypeCast, Upcast)
{
    assert_compiles(R"(
        const a: i32 = 10;
        const b: i64 = a as i64;
    )");
}

TEST(TypeCast, Downcast)
{
    assert_compiles(R"(
        const a: i64 = 10L;
        const b: i32 = a as i32;
    )");
}

TEST(TypeCast, MultipleCasts)
{
    assert_compiles(R"(
        const a: i8 = 10 as i8;
        const b: i64 = (a as i32) as i64;
    )");
}

TEST(TypeCastErrors, IncompatibleTypes)
{
    assert_throws_message(
        R"(
        type Point = { x: i32; y: i32; };
        const p: Point = Point::{ x: 1, y: 1 };
        const a: i32 = p as i32;
    )",
        "Cannot cast value of type 'Point' to incompatible type 'i32'");
}

TEST(TypeCastErrors, ArrayToScalarMismatch)
{
    assert_throws_message(
        R"(
        const a: i32[] = [1, 2, 3];
        const b: i32 = a as i32;
    )",
        "Cannot cast value of type 'i32[]' to incompatible type 'i32'");
}

TEST(TypeCast, FloatToInt)
{
    // Check if bitcast is supported or if it's explicitly disallowed by is_assignable_to.
    // Based on code: it uses builder->CreateBitCast(value, target_ty) for non-integer-to-integer casts.
    // However, validation says: if (!this->_target_type->is_assignable_to(this->_value->get_type()))
    assert_compiles(R"(
        const a: f32 = 1.0;
        const b: i32 = a as i32;
    )");
}

TEST(TypeCast, OptionalCasts)
{
    assert_compiles(R"(
        const a: i32 = 10;
        const b: i32? = a as i32?;
    )");

    assert_throws_message(
        R"(
        const a: i32? = nil;
        const b: i32 = a as i32;
    )",
        "Cannot cast value of type 'i32?' to incompatible type 'i32'");
}

TEST(TypeCast, StructAliasCast)
{
    assert_compiles(R"(
        type Point = { x: i32; y: i32; };
        type Vec = { x: i32; y: i32; };

        const p: Point = Point::{ x: 1, y: 1 };
        const v: Vec = p as Vec;
    )");
}
