#include "ast/parsing_context.h"
#include "utils.h"

#include <gtest/gtest.h>

using namespace stride::tests;

TEST(TypeCast, IdentityCast)
{
    assert_compiles(R"(
        const a: int32 = 10;
        const b: int32 = a as int32;
    )");
}

TEST(TypeCast, Upcast)
{
    assert_compiles(R"(
        const a: int32 = 10;
        const b: int64 = a as int64;
    )");
}

TEST(TypeCast, Downcast)
{
    assert_compiles(R"(
        const a: int64 = 10L;
        const b: int32 = a as int32;
    )");
}

TEST(TypeCast, MultipleCasts)
{
    assert_compiles(R"(
        const a: int8 = 10;
        const b: int64 = (a as int32) as int64;
    )");
}

TEST(TypeCastErrors, IncompatibleTypes)
{
    assert_throws_message(
        R"(
        type Point = { x: int32; y: int32; };
        const p: Point = Point::{ x: 1, y: 1 };
        const a: int32 = p as int32;
    )",
        "Cannot cast value of type 'Point' to incompatible type 'int32'");
}

TEST(TypeCastErrors, ArrayToScalarMismatch)
{
    assert_throws_message(
        R"(
        const a: int32[] = [1, 2, 3];
        const b: int32 = a as int32;
    )",
        "Cannot cast value of type 'int32[]' to incompatible type 'int32'");
}

TEST(TypeCast, FloatToInt)
{
    // Check if bitcast is supported or if it's explicitly disallowed by is_assignable_to.
    // Based on code: it uses builder->CreateBitCast(value, target_ty) for non-integer-to-integer casts.
    // However, validation says: if (!this->_target_type->is_assignable_to(this->_value->get_type()))
    assert_compiles(R"(
        const a: float32 = 1.0;
        const b: int32 = a as int32;
    )");
}

TEST(TypeCast, OptionalCasts)
{
    assert_compiles(R"(
        const a: int32 = 10;
        const b: int32? = a as int32?;
    )");

    assert_throws_message(
        R"(
        const a: int32? = nil;
        const b: int32 = a as int32;
    )",
        "Cannot cast value of type 'int32?' to incompatible type 'int32'");
}

TEST(TypeCast, StructAliasCast)
{
    assert_compiles(R"(
        type Point = { x: int32; y: int32; };
        type Vec = { x: int32; y: int32; };

        const p: Point = Point::{ x: 1, y: 1 };
        const v: Vec = p as Vec;
    )");
}
