/// Tests for chained expression parsing and type inference.
/// Covers:
///   - Simple member access: a.b
///   - Multi-step chains: a.b.c.d
///   - Array subscript followed by member access: arr[i].field
///   - Member access followed by array subscript: a.b[i]
///   - Mixed chains: a[i].b.c[j].d
///   - Function call returning struct, then member access: fn().field
///   - Array of function pointers: arr[i]().field
///   - AstChainedExpression type inference unit tests
///   - AstArrayMemberAccessor with chained base type inference
///   - AstIndirectCall type inference
///   - Error cases: non-struct base, unknown member, non-function indirect call

#include "utils.h"
#include "ast/type_inference.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/literal_values.h"
#include "ast/nodes/types.h"
#include "ast/parsing_context.h"
#include "ast/symbols.h"
#include "errors.h"
#include "files.h"

#include <gtest/gtest.h>
#include <memory>

using namespace stride;
using namespace stride::ast;
using namespace stride::tests;

// =============================================================================
// Integration (parse + codegen) tests
// =============================================================================

TEST(ChainedAccessor, SimpleMemberAccess)
{
    assert_compiles(R"(
        type Point = { x: i32; y: i32; };
        fn get_x(p: Point): i32 {
            return p.x;
        }
        fn main(): void {
            const p: Point = Point::{ x: 10, y: 20 };
            const x = p.x;
        }
    )");
}

TEST(ChainedAccessor, MultiStepMemberAccess)
{
    assert_compiles(R"(
        type Inner = { value: i32; };
        type Middle = { inner: Inner; };
        type Outer = { middle: Middle; };
        fn main(): void {
            const o: Outer = Outer::{
                middle: Middle::{
                    inner: Inner::{ value: 42 }
                }
            };
            const v = o.middle.inner.value;
        }
    )");
}

TEST(ChainedAccessor, ArraySubscriptThenMember)
{
    assert_compiles(R"(
        type Point = { x: i32; y: i32; };
        fn main(): void {
            const pts: Point[] = [ Point::{ x: 1, y: 2 } ];
            const x = pts[0].x;
        }
    )");
}

TEST(ChainedAccessor, MemberThenArraySubscript)
{
    assert_compiles(R"(
        type Container = { data: i32[]; };
        fn main(): void {
            const c: Container = Container::{ data: [10, 20, 30] };
            const first = c.data[0];
        }
    )");
}

TEST(ChainedAccessor, ChainWithArrayAndMember)
{
    assert_compiles(R"(
        type Point = { x: i32; y: i32; };
        type Row = { points: Point[]; };
        fn main(): void {
            const row: Row = Row::{ points: [ Point::{ x: 3, y: 4 } ] };
            const y = row.points[0].y;
        }
    )");
}

TEST(ChainedAccessor, DeepMixedChain)
{
    assert_compiles(R"(
        type Leaf = { val: i32; };
        type Branch = { leaves: Leaf[]; };
        type Tree = { branches: Branch[]; };
        fn main(): void {
            const tree: Tree = Tree::{
                branches: [
                    Branch::{ leaves: [ Leaf::{ val: 99 } ] }
                ]
            };
            const v = tree.branches[0].leaves[0].val;
        }
    )");
}

TEST(ChainedAccessor, FunctionReturnsThenMemberAccess)
{
    assert_compiles(R"(
        type Point = { x: i32; y: i32; };
        fn make_point(): Point {
            return Point::{ x: 5, y: 6 };
        }
        fn main(): void {
            const x = make_point().x;
            const y = make_point().y;
        }
    )");
}

TEST(ChainedAccessor, FunctionReturnsThenMultiStepAccess)
{
    assert_compiles(R"(
        type Inner = { value: i32; };
        type Wrapper = { inner: Inner; };
        fn make(): Wrapper {
            return Wrapper::{ inner: Inner::{ value: 7 } };
        }
        fn main(): void {
            const v = make().inner.value;
        }
    )");
}

TEST(ChainedAccessor, FunctionReturnsThenArraySubscript)
{
    assert_compiles(R"(
        type Container = { data: i32[]; };
        fn make(): Container {
            return Container::{ data: [1, 2, 3] };
        }
        fn main(): void {
            const second = make().data[1];
        }
    )");
}

TEST(ChainedAccessor, ChainedMemberAccessInReturn)
{
    assert_compiles(R"(
        type Inner = { value: i32; };
        type Outer = { inner: Inner; };
        fn get_value(o: Outer): i32 {
            return o.inner.value;
        }
        fn main(): void {
            const o: Outer = Outer::{ inner: Inner::{ value: 123 } };
            const v = get_value(o);
        }
    )");
}

TEST(ChainedAccessor, ChainedMemberInCondition)
{
    assert_compiles(R"(
        type Stats = { score: i32; };
        type Player = { stats: Stats; };
        fn main(): void {
            const p: Player = Player::{ stats: Stats::{ score: 100 } };
            if (p.stats.score > 50) {
                const x: i32 = 1;
            }
        }
    )");
}

TEST(ChainedAccessor, ArraySubscriptInFunctionArg)
{
    assert_compiles(R"(
        type Point = { x: i32; y: i32; };
        fn print_x(x: i32): void {}
        fn main(): void {
            const pts: Point[] = [ Point::{ x: 7, y: 8 } ];
            print_x(pts[0].x);
        }
    )");
}

// =============================================================================
// Type inference unit tests
// =============================================================================

class ChainedAccessorTypeTest : public ::testing::Test
{
protected:
    std::shared_ptr<ParsingContext> context;
    std::shared_ptr<SourceFile> source;

    void SetUp() override
    {
        context = std::make_shared<ParsingContext>();
        source = std::make_shared<SourceFile>("test.sr", "");

        // Define a Point struct: { x: i32, y: f32 }
        std::vector<std::pair<std::string, std::unique_ptr<IAstType>>> point_members;
        point_members.emplace_back("x", std::make_unique<AstPrimitiveType>(sf(), context, PrimitiveType::INT32));
        point_members.emplace_back("y", std::make_unique<AstPrimitiveType>(sf(), context, PrimitiveType::FLOAT32));
        auto point_ty = std::make_unique<AstObjectType>(sf(), context, "Point", std::move(point_members));
        context->define_type(sym("Point"), std::move(point_ty), {}, VisibilityModifier::PUBLIC);

        // Define a Wrapper struct: { pt: Point, values: i32[] }
        std::vector<std::pair<std::string, std::unique_ptr<IAstType>>> wrapper_members;
        wrapper_members.emplace_back("pt", std::make_unique<AstAliasType>(sf(), context, "Point"));
        wrapper_members.emplace_back(
            "values",
            std::make_unique<AstArrayType>(
                sf(),
                context,
                std::make_unique<AstPrimitiveType>(
                    sf(),
                    context,
                    PrimitiveType::INT32),
                0));
        auto wrapper_ty = std::make_unique<AstObjectType>(sf(), context, "Wrapper", std::move(wrapper_members));
        context->define_type(sym("Wrapper"), std::move(wrapper_ty), {}, VisibilityModifier::PUBLIC);

        // Register variables
        context->define_variable(
            sym("p"),
            std::make_unique<AstAliasType>(sf(), context, "Point"),
            VisibilityModifier::PUBLIC);
        context->define_variable(
            sym("w"),
            std::make_unique<AstAliasType>(sf(), context, "Wrapper"),
            VisibilityModifier::PUBLIC);

        // Register a Point[] array variable
        context->define_variable(
            sym("pts"),
            std::make_unique<AstArrayType>(
                sf(),
                context,
                std::make_unique<AstAliasType>(sf(), context, "Point"),
                0),
            VisibilityModifier::PUBLIC);

        // Register a function returning Point
        std::vector<std::unique_ptr<IAstType>> fn_params;
        auto fn_ret = std::make_unique<AstAliasType>(sf(), context, "Point");
        auto fn_ty = std::make_unique<AstFunctionType>(sf(), context, std::move(fn_params), std::move(fn_ret));
        context->define_function(sym("make_point"), std::move(fn_ty), VisibilityModifier::PUBLIC, 0);
    }

    [[nodiscard]] SourceFragment sf() const
    {
        return { source, 0, 0 };
    }

    [[nodiscard]] Symbol sym(const std::string& name) const
    {
        return Symbol(sf(), name);
    }

    /// Builds: Identifier(name)
    [[nodiscard]] std::unique_ptr<AstIdentifier> id(const std::string& name) const
    {
        return std::make_unique<AstIdentifier>(context, sym(name));
    }

    /// Builds: ChainedExpression(base, Identifier(member))
    [[nodiscard]] std::unique_ptr<AstChainedExpression> chain(
        std::unique_ptr<IAstExpression> base,
        const std::string& member) const
    {
        return std::make_unique<AstChainedExpression>(sf(), context, std::move(base), id(member));
    }

    /// Builds: ArrayAccess(base, IntLiteral(idx))
    [[nodiscard]] std::unique_ptr<AstArrayMemberAccessor> subscript(
        std::unique_ptr<IAstExpression> base,
        int32_t idx) const
    {
        auto index = std::make_unique<AstIntLiteral>(sf(), context, PrimitiveType::INT32, idx, 0);
        return std::make_unique<AstArrayMemberAccessor>(sf(), context, std::move(base), std::move(index));
    }
};

// --- Simple chain: p.x → i32

TEST_F(ChainedAccessorTypeTest, SimpleMemberAccess_i32)
{
    const auto expr = chain(id("p"), "x");
    EXPECT_EQ(infer_expression_type(expr.get())->to_string(), "i32");
}

TEST_F(ChainedAccessorTypeTest, SimpleMemberAccess_f32)
{
    const auto expr = chain(id("p"), "y");
    EXPECT_EQ(infer_expression_type(expr.get())->to_string(), "f32");
}

// --- Two-step chain: w.pt.x → i32

TEST_F(ChainedAccessorTypeTest, TwoStepChain)
{
    // ChainedExpression(ChainedExpression(w, pt), x)
    auto inner = chain(id("w"), "pt");
    const auto outer = chain(std::move(inner), "x");
    EXPECT_EQ(infer_expression_type(outer.get())->to_string(), "i32");
}

// --- Two-step chain resolves to correct second member: w.pt.y → f32

TEST_F(ChainedAccessorTypeTest, TwoStepChain_SecondMember)
{
    auto inner = chain(id("w"), "pt");
    const auto outer = chain(std::move(inner), "y");
    EXPECT_EQ(infer_expression_type(outer.get())->to_string(), "f32");
}

// --- Array subscript then member: pts[0].x → i32

TEST_F(ChainedAccessorTypeTest, ArraySubscriptThenMember)
{
    auto access = subscript(id("pts"), 0);
    const auto expr = chain(std::move(access), "x");
    EXPECT_EQ(infer_expression_type(expr.get())->to_string(), "i32");
}

// --- Array subscript then member (y): pts[0].y → f32

TEST_F(ChainedAccessorTypeTest, ArraySubscriptThenMember_f32)
{
    auto access = subscript(id("pts"), 0);
    const auto expr = chain(std::move(access), "y");
    EXPECT_EQ(infer_expression_type(expr.get())->to_string(), "f32");
}

// --- Member then array subscript: w.values[0] → i32

TEST_F(ChainedAccessorTypeTest, MemberThenArraySubscript)
{
    auto member = chain(id("w"), "values");
    const auto expr = subscript(std::move(member), 0);
    EXPECT_EQ(infer_expression_type(expr.get())->to_string(), "i32");
}

// --- Indirect call then member: make_point().x → i32

TEST_F(ChainedAccessorTypeTest, IndirectCallThenMember)
{
    auto callee = id("make_point");
    // Set the callee's type to the function type
    // (type inference needs set_type to work on identifiers that are functions)
    // Build the FunctionCall first via AstFunctionCall; then build IndirectCall on top
    // In practice, indirect call is used on non-named callees; here we test type inference
    // by constructing AstIndirectCall directly with a correctly-typed callee.
    std::vector<std::unique_ptr<IAstType>> fn_params;
    auto fn_ret = std::make_unique<AstAliasType>(sf(), context, "Point");
    auto fn_ast_ty = std::make_unique<AstFunctionType>(sf(), context, std::move(fn_params), std::move(fn_ret));
    callee->set_type(std::move(fn_ast_ty));

    auto call = std::make_unique<AstIndirectCall>(sf(), context, std::move(callee), ExpressionList{});
    const auto expr = chain(std::move(call), "x");
    EXPECT_EQ(infer_expression_type(expr.get())->to_string(), "i32");
}

// --- Indirect call type inference: returns correct return type

TEST_F(ChainedAccessorTypeTest, IndirectCallReturnsCorrectType)
{
    auto callee = id("make_point");
    std::vector<std::unique_ptr<IAstType>> fn_params;
    auto fn_ret = std::make_unique<AstAliasType>(sf(), context, "Point");
    auto fn_ast_ty = std::make_unique<AstFunctionType>(sf(), context, std::move(fn_params), std::move(fn_ret));
    callee->set_type(std::move(fn_ast_ty));

    const auto call = std::make_unique<AstIndirectCall>(sf(), context, std::move(callee), ExpressionList{});
    EXPECT_EQ(infer_expression_type(call.get())->to_string(), "Point");
}

// --- Array subscript followed by member followed by subscript (deep chain)
//     w.values[0] (just array subscript on a member array): already tested above.
//     Here: subscript → chain → subscript through wrapper's values.
//     We test: (AstArrayMemberAccessor base= chain(w, values))[0] → i32

TEST_F(ChainedAccessorTypeTest, ChainedMemberThenSubscript_ReturnsElementType)
{
    auto member_access = chain(id("w"), "values");                  // type: i32[]
    const auto arr_access = subscript(std::move(member_access), 2); // type: i32
    EXPECT_EQ(infer_expression_type(arr_access.get())->to_string(), "i32");
}

// =============================================================================
// Error cases
// =============================================================================

TEST_F(ChainedAccessorTypeTest, ErrorMemberAccessOnNonStruct)
{
    // Define an int variable, try to access .x on it → type error
    context->define_variable(
        sym("n"),
        std::make_unique<AstPrimitiveType>(sf(), context, PrimitiveType::INT32),
        VisibilityModifier::PUBLIC);

    auto expr = chain(id("n"), "x");
    EXPECT_THROW(infer_expression_type(expr.get()), parsing_error);
}

TEST_F(ChainedAccessorTypeTest, ErrorUndefinedBaseVariable)
{
    const auto expr = chain(id("does_not_exist"), "x");
    EXPECT_THROW(infer_expression_type(expr.get()), parsing_error);
}

TEST_F(ChainedAccessorTypeTest, ErrorMemberNotInStruct)
{
    const auto expr = chain(id("p"), "z"); // Point has x, y — not z
    EXPECT_THROW(infer_expression_type(expr.get()), parsing_error);
}

TEST_F(ChainedAccessorTypeTest, ErrorMemberNotInStructAtDepth2)
{
    auto inner = chain(id("w"), "pt");                   // w.pt is a Point
    auto outer = chain(std::move(inner), "nonexistent"); // Point has no such field
    EXPECT_THROW(infer_expression_type(outer.get()), parsing_error);
}

TEST_F(ChainedAccessorTypeTest, ErrorIndirectCallOnNonFunction)
{
    auto callee = id("p");
    // Set type to a non-function type
    callee->set_type(std::make_unique<AstAliasType>(sf(), context, "Point"));
    auto call = std::make_unique<AstIndirectCall>(sf(), context, std::move(callee), ExpressionList{});
    EXPECT_THROW(infer_expression_type(call.get()), parsing_error);
}

TEST_F(ChainedAccessorTypeTest, ErrorArraySubscriptOnNonArray)
{
    // Access p[0] where p is a Point (not an array) → type error
    auto index = std::make_unique<AstIntLiteral>(sf(), context, PrimitiveType::INT32, 0, 0);
    auto expr = std::make_unique<AstArrayMemberAccessor>(sf(), context, id("p"), std::move(index));
    EXPECT_THROW(infer_expression_type(expr.get()), parsing_error);
}

// =============================================================================
// Integration error tests
// =============================================================================

TEST(ChainedAccessorErrors, AccessNonExistentMember)
{
    assert_throws_message(R"(
        type Point = { x: i32; y: i32; };
        fn main(): void {
            const p: Point = Point::{ x: 1, y: 2 };
            const z = p.z;
        }
    )",
                          "not found");
}

TEST(ChainedAccessorErrors, AccessMemberOnPrimitive)
{
    assert_throws_message(R"(
        fn main(): void {
            const n: i32 = 42;
            const x = n.field;
        }
    )",
                          "struct");
}

TEST(ChainedAccessorErrors, AccessNonExistentNestedMember)
{
    assert_throws_message(R"(
        type Inner = { val: i32; };
        type Outer = { inner: Inner; };
        fn main(): void {
            const o: Outer = Outer::{ inner: Inner::{ val: 1 } };
            const bad = o.inner.bad_field;
        }
    )",
                          "not found");
}

TEST(ChainedAccessorErrors, ArraySubscriptOutOfChain_BadMember)
{
    assert_throws_message(R"(
        type Point = { x: i32; };
        fn main(): void {
            const pts: Point[] = [ Point::{ x: 1 } ];
            const v = pts[0].nonexistent;
        }
    )",
                          "not found");
}
