#include "ast/parsing_context.h"
#include "utils.h"

#include <gtest/gtest.h>

using namespace stride::tests;

TEST(TypeErrors, VariableInitTypeMismatch)
{
    assert_throws_message(
        R"(
        const b: int32 = 10l;
    )",
        "Type mismatch in variable declaration: cannot assign value of type 'int64' to type 'int32'");
}

TEST(TypeErrors, StructTypeMismatch)
{
    assert_throws_message(
        R"(
        type Point = {
            x: int32;
            y: int32;
        };

        const a: Point = Point::{ x: 1.0D, y: 1 };
    )",
        "Type mismatch for member 'x' in struct initializer 'Point': expected 'int32', got 'float64'");
}

TEST(TypeErrors, StructTypeMemberCountMismatch)
{
    assert_throws_message(
        R"(
        type Point = {
            x: int32;
            y: int32;
        };

        const a: Point = Point::{ x: 1, y: 1, z: 2 };
    )",
        "Too many members found in struct 'Point': expected 2, got 3");

    assert_throws_message(
        R"(
        type Point = {
            x: int32;
            y: int32;
        };

        const a: Point = Point::{ x: 1 };
    )",
        "Too few members found in struct 'Point': expected 2, got 1");
}

TEST(TypeErrors, StructMemberTypeMismatch)
{
    assert_throws_message(
        R"(
        type Point = {
            x: int32;
            y: int32;
        };

        type Color = {
            r: int32;
            g: int32;
            b: int32;
        };

        const a: Point = Color::{ r: 1, g: 2, b: 3 };
    )",
        "Type mismatch in variable declaration: cannot assign value of type 'Color' to type 'Point'");
}

TEST(TypeErrors, StructReferenceTypeMismatch)
{
    assert_throws_message(
        R"(
        type Point = {
            x: int32;
            y: int32;
        };

        type Vec = Point;

        const a: Point = Vec::{ x: 1, y: 2 };
    )",
        "Type mismatch in variable declaration: cannot assign value of type 'Vec' to type 'Point'");
}

TEST(TypeErrors, StructMemberOrderMismatch)
{
    assert_throws_message(
        R"(
        type Point = {
            x: int32;
            y: int32;
        };

        const a: Point = Point::{ y: 1, x: 1 };
    )",
        "Struct member order mismatch at index 0: expected 'x', got 'y'");
}

TEST(TypeErrors, StructMemberUnknownField)
{
    assert_throws_message(
        R"(
        type Point = {
            x: int32;
            y: int32;
        };

        const a: Point = Point::{ x: 1, unknown: 123 };
    )",
        "Struct 'Point' has no member named 'unknown'");
}

TEST(TypeErrors, ArrayTypeMismatch)
{
    assert_throws_message(
        R"(
        let a: int32[] = [1L, 2L, 3L];
    )",
        "Type mismatch in variable declaration: cannot assign value of type 'int64[]' to type 'int32[]'");
}

TEST(TypeErrors, FunctionCallTypeMismatch)
{
    assert_compiles(R"(
        fn add(x: int32, y: int32): int32 {
            return x + y;
        }

        const result: int32 = add(1, 2);
    )");
    assert_throws_message(
        R"(
        fn add(x: int32, y: int32): int32 {
            return x + y;
        }

        const result: int32 = add(1L, 2L);
    )",
        "Function 'add(int64, int64)' was not found in this scope");
}

TEST(TypeErrors, FunctionTypeMismatch)
{
    assert_throws_message(
        R"(
        const k: (int32, int32) -> int32 = [(x: int32, y: int32): int32 -> { return 1; }];
    )",
        "Type mismatch in variable declaration: cannot assign value of type '((int32, int32) -> int32)[]' to type '(int32, int32) -> int32'");

    assert_throws_message(
        R"(
        fn test(p: int32): int32 { return 0; }

        let a: (int32, int32) -> int32 = test;
    )",
        "Type mismatch in variable declaration: cannot assign value of type '(int32) -> int32' to type '(int32, int32) -> int32'");

    assert_compiles(R"(
        fn test(p: int32): int32 { return 0; }

        const a: (int32) -> int32 = test;
    )");
}

TEST(TypeReferences, DeepFunctionReferential)
{
    auto [block, context] = parse_code_with_context(R"(
    fn root(x: int32): int32 {
        return x + 10;
    }

    const first_ref = root;
    const second_ref = first_ref;
    )");

    const auto symbol = context->lookup_symbol("second_ref");
    const auto field = dynamic_cast<stride::ast::definition::FieldDefinition*>(symbol);

    EXPECT_NE(symbol, nullptr) << "Expected 'second_ref' to be found in the symbol table";
    EXPECT_NE(field, nullptr)
        << "Expected 'second_ref' to be a FieldDefinition, but it was not found or was of a different type";
    EXPECT_EQ(field->get_type()->to_string(), "(int32) -> int32")
    << "Expected 'second_ref' to have type '(int32) -> int32', but got '"
    << field->get_type()->to_string() << "'";
}

TEST(TypeReferences, StructTypeReference)
{
    auto [block, context] = parse_code_with_context(R"(
    type Point = { x: int32; y: int32; };

    type Vec = Point;
    )");

    const auto symbol = context->lookup_symbol("Vec");
    const auto field = dynamic_cast<stride::ast::definition::TypeDefinition*>(symbol);

    EXPECT_NE(symbol, nullptr) << "Expected 'Vec' to be found in the symbol table";
    EXPECT_NE(field, nullptr) <<
 "Expected 'Vec' to be a TypeDefinition, but it was not found or was of a different type";
    EXPECT_EQ(field->get_type()->get_type_name(), "Point")
    << std::format("Expected 'Vec' to have type 'Point', but got '{}'",
                   field->get_type()->get_type_name()
        );
}

TEST(TypeDefinition, RecursionLimit)
{
    assert_throws_message(
        R"(
        type A = B;
        type B = A;

        const K: B?;
    )",
        "Exceeded maximum recursion depth while resolving base type of 'B'"
    );
}

TEST(OptionalTypes, OptionalMismatch)
{
    assert_throws_message(
        R"(
        let a: int32? = 10.0;
    )",
        "Type mismatch in variable declaration: cannot assign value of type 'float32' to type 'int32?'");

    assert_compiles(R"(
        let a: int32? = nil;
    )");

    assert_throws_message(
        R"(
        let a: int32 = nil;
    )",
        "Type mismatch in variable declaration: cannot assign value of type 'nil' to type 'int32'");
}

TEST(PrimitiveTypes, DominantType)
{
    assert_compiles(R"(
        const a: int64 = 10 + 20L;
        const b: float32 = 10.0 + 20;
    )");

    assert_throws_message(
        R"(
        type Point = { x: int32; y: int32; };
        const p: Point = Point::{ x: 1, y: 1 };
        const a: int32 = 10 + p; 
    )",
        "Cannot mix primitive type with named type");

    assert_throws_message(
        R"(
        type Point = { x: int32; y: int32; };
        type Color = { r: int32; g: int32; b: int32; };
        const p: Point = Point::{ x: 1, y: 1 };
        const c: Color = Color::{ r: 1, g: 1, b: 1 };
        const a = p + c;
    )",
        "Cannot compute dominant type for non-primitive types");
}

TEST(ArrayTypes, MultiDimensional)
{
    assert_compiles(R"(
        let a: int32[][] = [[1, 2], [3, 4]];
    )");
}

TEST(TypeAliases, ComplexAliases)
{
    assert_compiles(R"(
        type IntArray = int32[];
        const value = [1, 2, 3];

        fn test(a: IntArray): void {}

        fn main(): void { test(value as IntArray); }
    )");

    assert_compiles(
        R"(
        type BinaryOp = (int32, int32) -> int32;
        fn add(a: int32, b: int32): int32 { return a + b; }

        let op: BinaryOp = add;

        fn main(): int32 {
            op(1, 2);
            return 0;
        }
    )");
}

TEST(PrimitiveTypes, CharAndString)
{
    assert_compiles(
        R"(
        const c: char = 'a';
        const s: string = "hello";
    )");

    assert_throws_message(
        R"(
        const c: char = "string";
    )",
        "Type mismatch in variable declaration: cannot assign value of type 'string' to type 'char'");
}
