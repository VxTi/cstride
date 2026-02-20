#include "utils.h"

#include <gtest/gtest.h>

using namespace stride::tests;

TEST(UnaryExpressions, LogicalNot)
{
    assert_compiles("const a: bool = !true;");
    assert_compiles("const a: bool = !0;");
}

TEST(UnaryExpressions, Negate)
{
    assert_compiles("const a: int32 = -10;");
    assert_compiles("const a: float64 = -10.5D;");
}

TEST(UnaryExpressions, Complement)
{
    assert_compiles("const a: int32 = ~10;");
}

TEST(UnaryExpressions, IncrementDecrement)
{
    assert_compiles(R"(
        fn test(): void {
            let x: int32 = 10;
            ++x;
            x++;
            --x;
            x--;
        }
    )");
}

TEST(UnaryExpressions, AddressOf)
{
    assert_compiles(R"(
        let x: int32 = 10;
        const y = &x;
    )");
}

TEST(UnaryExpressions, Chained)
{
    assert_compiles("const a: bool = !!true;");
    assert_compiles("const a: int32 = - -10;");
    assert_compiles("const a: int32 = ~ ~10;");
}

TEST(UnaryErrors, ImmutableIncrement)
{
    assert_throws_message(
        R"(
        fn test(): void {
            const x: int32 = 10;
            ++x;
        }
    )",
        "Cannot modify immutable value");
}

TEST(UnaryErrors, ImmutableDecrement)
{
    assert_throws_message(
        R"(
        fn test(): void {
            const x: int32 = 10;
            --x;
        }
    )",
        "Cannot modify immutable value");
}

TEST(UnaryErrors, InvalidTypeComplement)
{
    assert_throws_message(
        R"(
        const a = ~10.5D;
    )",
        "Invalid type 'float64' for bitwise complement");
}

TEST(UnaryErrors, InvalidTypeNegate)
{
    // Assuming we can't negate a struct or something non-numeric
    assert_throws_message(
        R"(
        struct Point { x: int32; y: int32; }
        const p = Point::{ x: 1, y: 2 };
        const a = -p;
    )",
        "Invalid type 'Point' for negation operand");
}

TEST(UnaryErrors, IncrementLiteral)
{
    assert_throws_message(
        "const a = ++10;",
        "Unary operator requires identifier operand");
}

TEST(UnaryErrors, PostfixLiteral)
{
    assert_throws_message(
        "const a = 10++;",
        "Postfix operator requires identifier operand");
}

TEST(UnaryErrors, AddressOfLiteral)
{
    assert_throws_message(
        "const a = &10;",
        "Unary operator requires identifier operand");
}

TEST(UnaryErrors, DereferenceNotImplemented)
{
    assert_throws_message(
        R"(
        let x: int32 = 10;
        let y = &x;
        let z = *y;
    )",
        "Dereference not implemented yet due to opaque pointers");
}

