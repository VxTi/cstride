#include "ast/parsing_context.h"
#include "utils.h"

#include <gtest/gtest.h>

using namespace stride::tests;

TEST(TypeErrors, VariableInitTypeMismatch)
{
    assert_throws_message(
        R"(
        const b: i32 = 10l;
    )",
        "Type mismatch in variable declaration: cannot assign value of type 'i64' to type 'i32'");
}

TEST(TypeErrors, ObjectTypeMismatch)
{
    assert_throws_message(
        R"(
        type Point = {
            x: i32;
            y: i32;
        };

        const a: Point = Point::{ x: 1.0D, y: 1 };
    )",
        "Type mismatch for member 'x' in struct initializer 'Point': expected 'i32', got 'f64'");
}

TEST(TypeErrors, ObjectTypeMemberCountMismatch)
{
    assert_throws_message(
        R"(
        type Point = {
            x: i32;
            y: i32;
        };

        const a: Point = Point::{ x: 1, y: 1, z: 2 };
    )",
        "Too many members found in struct 'Point': expected 2, got 3");

    assert_throws_message(
        R"(
        type Point = {
            x: i32;
            y: i32;
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
            x: i32;
            y: i32;
        };

        type Color = {
            r: i32;
            g: i32;
            b: i32;
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
            x: i32;
            y: i32;
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
            x: i32;
            y: i32;
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
            x: i32;
            y: i32;
        };

        const a: Point = Point::{ x: 1, unknown: 123 };
    )",
        "Struct 'Point' has no member named 'unknown'");
}

TEST(TypeErrors, ArrayTypeMismatch)
{
    assert_throws_message(
        R"(
        let a: i32[] = [1L, 2L, 3L];
    )",
        "Type mismatch in variable declaration: cannot assign value of type 'i64[]' to type 'i32[]'");
}

TEST(TypeErrors, FunctionCallTypeMismatch)
{
    assert_compiles(R"(
        fn add(x: i32, y: i32): i32 {
            return x + y;
        }

        const result: i32 = add(1, 2);
    )");
    assert_throws_message(
        R"(
        fn add(x: i32, y: i32): i32 {
            return x + y;
        }

        const result: i32 = add(1L, 2L);
    )",
        "Function 'add(i64, i64)' was not found in this scope");
}

TEST(TypeErrors, FunctionTypeMismatch)
{
    assert_throws_message(
        R"(
        const k: (i32, i32) -> i32 = [(x: i32, y: i32): i32 -> { return 1; }];
    )",
        "Type mismatch in variable declaration: cannot assign value of type '((i32, i32) -> i32)[]' to type '(i32, i32) -> i32'");

    assert_throws_message(
        R"(
        fn test(p: i32): i32 { return 0; }

        let a: (i32, i32) -> i32 = test;
    )",
        "Type mismatch in variable declaration: cannot assign value of type '(i32) -> i32' to type '(i32, i32) -> i32'");

    assert_compiles(R"(
        fn test(p: i32): i32 { return 0; }

        const a: (i32) -> i32 = test;
    )");
}

TEST(TypeErrors, NamedFunctionTypeAssignment)
{
    EXPECT_NO_THROW({
        assert_compiles(R"(
        type BinaryOp = (i32, i32) -> i32;
        fn add(a: i32, b: i32): i32 { return a + b; }
        const secondOp: BinaryOp = add;
    )");
    });
}

TEST(TypeErrors, NamedFunctionTypeCast)
{
    EXPECT_NO_THROW({
        assert_compiles(R"(
        type BinaryOp = (i32, i32) -> i32;
        fn add(a: i32, b: i32): i32 { return a + b; }
        let op = add as BinaryOp;
    )");
    });
}

TEST(TypeReferences, DeepFunctionReferential)
{
    auto [block, context] = parse_code_with_context(R"(
    fn root(x: i32): i32 {
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
    EXPECT_EQ(field->get_type()->to_string(), "(i32) -> i32")
    << "Expected 'second_ref' to have type '(i32) -> i32', but got '"
    << field->get_type()->to_string() << "'";
}

TEST(TypeReferences, ObjectTypeReference)
{
    auto [block, context] = parse_code_with_context(R"(
    type Point = { x: i32; y: i32; };

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
        let a: i32? = 10.0;
    )",
        "Type mismatch in variable declaration: cannot assign value of type 'f32' to type 'i32?'");

    assert_compiles(R"(
        let a: i32? = nil;
    )");

    assert_throws_message(
        R"(
        let a: i32 = nil;
    )",
        "Type mismatch in variable declaration: cannot assign value of type 'nil' to type 'i32'");
}

TEST(PrimitiveTypes, DominantType)
{
    assert_compiles(R"(
        const a: i64 = 10 + 20L;
        const b: f32 = 10.0 + 20;
    )");

    assert_throws_message(
        R"(
        type Point = { x: i32; y: i32; };
        const p: Point = Point::{ x: 1, y: 1 };
        const a: i32 = 10 + p; 
    )",
        "Cannot mix primitive type with named type");

    assert_throws_message(
        R"(
        type Point = { x: i32; y: i32; };
        type Color = { r: i32; g: i32; b: i32; };
        const p: Point = Point::{ x: 1, y: 1 };
        const c: Color = Color::{ r: 1, g: 1, b: 1 };
        const a = p + c;
    )",
        "Cannot compute dominant type for non-primitive types");
}

TEST(ArrayTypes, MultiDimensional)
{
    assert_compiles(R"(
        let a: i32[][] = [[1, 2], [3, 4]];
    )");
}

TEST(TypeAliases, ComplexAliases)
{
    assert_compiles(R"(
        type IntArray = i32[];
        const value = [1, 2, 3];

        fn test(a: IntArray): void {}

        fn main(): void { test(value as IntArray); }
    )");

    assert_compiles(
        R"(
         type BinaryOp = (i32, i32) -> i32;
         fn add(a: i32, b: i32): i32 {
             return a + b;
         }

         const operation: BinaryOp = add;

         fn main(): i32 {
             operation(1, 2);
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
