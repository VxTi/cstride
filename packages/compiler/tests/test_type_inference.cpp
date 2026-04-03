#include "ast/type_inference.h"
#include "ast/nodes/literal_values.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/types.h"
#include "ast/nodes/function_declaration.h"
#include "ast/symbol_table.h"
#include "ast/symbols.h"
#include "errors.h"
#include "files.h"
#include "utils.h"
#include <gtest/gtest.h>

using namespace stride;
using namespace stride::ast;
using namespace stride::tests;

class TypeInferenceTest : public ::testing::Test
{
protected:
    std::shared_ptr<SymbolTable> context;
    std::shared_ptr<SourceFile> source;

    void SetUp() override
    {
        context = std::make_shared<SymbolTable>();
        source = std::make_shared<SourceFile>("test.sr", "");
    }

    SourcePosition dummy_sf()
    {
        return { source, 0, 0 };
    }

    Symbol dummy_sym(const std::string& name)
    {
        return Symbol(dummy_sf(), name);
    }

    std::unique_ptr<AstIdentifier> dummy_iden(const std::string& name)
    {
        return std::make_unique<AstIdentifier>(dummy_sym(name));
    }
};

TEST_F(TypeInferenceTest, InferLiteralTypes)
{
    // Int literals
    auto i32_lit = std::make_unique<AstIntLiteral>(dummy_sf(), PrimitiveType::INT32, 42, 0);
    auto i32_ty = infer_expression_type(context.get(), i32_lit.get());
    EXPECT_EQ(i32_ty->get_type_name(), "i32");

    auto i64_lit = std::make_unique<AstIntLiteral>(dummy_sf(), PrimitiveType::INT64, 42, 0);
    auto i64_ty = infer_expression_type(context.get(), i64_lit.get());
    EXPECT_EQ(i64_ty->get_type_name(), "i64");

    // Float literals
    auto f32_lit = std::make_unique<AstFpLiteral>(dummy_sf(), PrimitiveType::FLOAT32, 3.14f);
    auto f32_ty = infer_expression_type(context.get(), f32_lit.get());
    EXPECT_EQ(f32_ty->get_type_name(), "f32");

    auto f64_lit = std::make_unique<AstFpLiteral>(dummy_sf(), PrimitiveType::FLOAT64, 3.14);
    auto f64_ty = infer_expression_type(context.get(), f64_lit.get());
    EXPECT_EQ(f64_ty->get_type_name(), "f64");

    // Bool literal
    auto bool_lit = std::make_unique<AstBooleanLiteral>(dummy_sf(), true);
    auto bool_ty = infer_expression_type(context.get(), bool_lit.get());
    EXPECT_EQ(bool_ty->get_type_name(), "bool");

    // String literal
    auto str_lit = std::make_unique<AstStringLiteral>(dummy_sf(), "hello");
    auto str_ty = infer_expression_type(context.get(), str_lit.get());
    EXPECT_EQ(str_ty->get_type_name(), "string");

    // Char literal
    auto char_lit = std::make_unique<AstCharLiteral>(dummy_sf(), 'a');
    auto char_ty = infer_expression_type(context.get(), char_lit.get());
    EXPECT_EQ(char_ty->get_type_name(), "char");

    // Nil literal
    auto nil_lit = std::make_unique<AstNilLiteral>(dummy_sf());
    auto nil_ty = infer_expression_type(context.get(), nil_lit.get());
    EXPECT_EQ(nil_ty->get_type_name(), "nil");
}

TEST_F(TypeInferenceTest, InferIdentifierTypes)
{
    // Variable lookup
    Symbol var_sym = dummy_sym("x");
    context->define_variable(
        var_sym,
        std::make_unique<AstPrimitiveType>(dummy_sf(), PrimitiveType::INT32),
        VisibilityModifier::PUBLIC);

    auto iden = std::make_unique<AstIdentifier>(var_sym);
    auto ty = infer_expression_type(context.get(), iden.get());
    EXPECT_EQ(ty->get_type_name(), "i32");

    // Function lookup (as identifier)
    Symbol fn_sym = dummy_sym("foo");
    std::vector<std::unique_ptr<IAstType>> params;
    params.push_back(std::make_unique<AstPrimitiveType>(dummy_sf(), PrimitiveType::INT32));
    auto fn_ty = std::make_unique<AstFunctionType>(
        dummy_sf(),
        std::move(params),
        std::make_unique<AstPrimitiveType>(
            dummy_sf(),
            PrimitiveType::FLOAT32));
    context->define_function(fn_sym, std::move(fn_ty), VisibilityModifier::PUBLIC, 0);

    auto fn_iden = std::make_unique<AstIdentifier>(fn_sym);
    auto fn_res_ty = infer_expression_type(context.get(), fn_iden.get());
    EXPECT_EQ(fn_res_ty->get_type_name(), "(i32) -> f32");

    // Symbol not found
    auto unknown_iden = std::make_unique<AstIdentifier>(dummy_sym("unknown"));
    EXPECT_THROW(infer_expression_type(context.get(), unknown_iden.get()), stride_error);
}

TEST_F(TypeInferenceTest, InferTypeCast)
{
    auto lit = std::make_unique<AstIntLiteral>(dummy_sf(), PrimitiveType::INT32, 42, 0);
    auto target_ty = std::make_unique<AstPrimitiveType>(dummy_sf(), PrimitiveType::FLOAT64);
    auto cast = std::make_unique<AstTypeCastOp>(dummy_sf(), std::move(lit), std::move(target_ty));

    auto ty = infer_expression_type(context.get(), cast.get());
    EXPECT_EQ(ty->get_type_name(), "f64");
}

TEST_F(TypeInferenceTest, InferBinaryOp)
{
    // Same types
    auto lhs = std::make_unique<AstIntLiteral>(dummy_sf(), PrimitiveType::INT32, 1, 0);
    auto rhs = std::make_unique<AstIntLiteral>(dummy_sf(), PrimitiveType::INT32, 2, 0);
    auto op = std::make_unique<AstBinaryArithmeticOp>(
        dummy_sf(),
        std::move(lhs),
        BinaryOpType::ADD,
        std::move(rhs));
    EXPECT_EQ(infer_expression_type(context.get(), op.get())->get_type_name(), "i32");

    // Pointer priority (LHS)
    context->define_variable(
        dummy_sym("p"),
        std::make_unique<AstPrimitiveType>(
            dummy_sf(),
            PrimitiveType::INT32,
            SRFLAG_TYPE_PTR),
        VisibilityModifier::PUBLIC);
    context->define_variable(
        dummy_sym("i"),
        std::make_unique<AstPrimitiveType>(dummy_sf(), PrimitiveType::INT32),
        VisibilityModifier::PUBLIC);

    auto p_iden = std::make_unique<AstIdentifier>(dummy_sym("p"));
    auto i_iden = std::make_unique<AstIdentifier>(dummy_sym("i"));
    auto p_op = std::make_unique<AstBinaryArithmeticOp>(
        dummy_sf(),
        std::move(p_iden),
        BinaryOpType::ADD,
        std::move(i_iden));
    EXPECT_EQ(infer_expression_type(context.get(), p_op.get())->get_type_name(), "*i32");

    auto i_iden2 = std::make_unique<AstIdentifier>(dummy_sym("i"));
    auto p_iden2 = std::make_unique<AstIdentifier>(dummy_sym("p"));
    auto p_op2 = std::make_unique<AstBinaryArithmeticOp>(
        dummy_sf(),
        std::move(i_iden2),
        BinaryOpType::ADD,
        std::move(p_iden2));
    EXPECT_EQ(infer_expression_type(context.get(), p_op2.get())->get_type_name(), "i32");
}

TEST_F(TypeInferenceTest, InferUnaryOp)
{
    context->define_variable(
        dummy_sym("x"),
        std::make_unique<AstPrimitiveType>(dummy_sf(), PrimitiveType::INT32),
        VisibilityModifier::PUBLIC);

    // Address of
    auto iden = std::make_unique<AstIdentifier>(dummy_sym("x"));
    auto addr_op = std::make_unique<AstUnaryOp>(dummy_sf(), UnaryOpType::ADDRESS_OF, std::move(iden));
    EXPECT_EQ(infer_expression_type(context.get(), addr_op.get())->get_type_name(), "*i32");

    // Dereference
    context->define_variable(
        dummy_sym("px"),
        std::make_unique<AstPrimitiveType>(
            dummy_sf(),
            PrimitiveType::INT32,
            SRFLAG_TYPE_PTR),
        VisibilityModifier::PUBLIC);
    auto p_iden = std::make_unique<AstIdentifier>(dummy_sym("px"));
    auto deref_op = std::make_unique<AstUnaryOp>(dummy_sf(), UnaryOpType::DEREFERENCE, std::move(p_iden));
    EXPECT_EQ(infer_expression_type(context.get(), deref_op.get())->get_type_name(), "*i32");

    // Logical Not
    auto bool_lit = std::make_unique<AstBooleanLiteral>(dummy_sf(), true);
    auto not_op = std::make_unique<AstUnaryOp>(dummy_sf(), UnaryOpType::LOGICAL_NOT, std::move(bool_lit));
    EXPECT_EQ(infer_expression_type(context.get(), not_op.get())->get_type_name(), "bool");
}

TEST_F(TypeInferenceTest, InferLogicalAndComparison)
{
    auto lhs = std::make_unique<AstIntLiteral>(dummy_sf(), PrimitiveType::INT32, 1, 0);
    auto rhs = std::make_unique<AstIntLiteral>(dummy_sf(), PrimitiveType::INT32, 2, 0);

    const auto comp_op = std::make_unique<AstComparisonOp>(
        dummy_sf(),
        std::move(lhs),
        ComparisonOpType::EQUALS,
        std::move(rhs)
    );
    EXPECT_EQ(infer_expression_type(context.get(), comp_op.get())->get_type_name(), "bool");

    auto blhs = std::make_unique<AstBooleanLiteral>(dummy_sf(), true);
    auto brhs = std::make_unique<AstBooleanLiteral>(dummy_sf(), false);
    auto log_op = std::make_unique<AstLogicalOp>(
        dummy_sf(),
        std::move(blhs),
        LogicalOpType::AND,
        std::move(brhs));
    EXPECT_EQ(infer_expression_type(context.get(), log_op.get())->get_type_name(), "bool");
}

TEST_F(TypeInferenceTest, InferVariableDeclaration)
{
    // Non-annotated
    auto val = std::make_unique<AstIntLiteral>(dummy_sf(), PrimitiveType::INT32, 42, 0);
    auto decl = std::make_unique<AstVariableDeclaration>(
        dummy_sf(),
        "v",
        std::nullopt,
        std::move(val),
        VisibilityModifier::PUBLIC,
        0);
    EXPECT_EQ(infer_expression_type(context.get(), decl.get())->get_type_name(), "i32");

    // Annotated same type
    auto val2 = std::make_unique<AstIntLiteral>(dummy_sf(), PrimitiveType::INT32, 42, 0);
    auto ann_ty = std::make_unique<AstPrimitiveType>(dummy_sf(), PrimitiveType::INT32);
    auto decl2 = std::make_unique<AstVariableDeclaration>(
        dummy_sf(),
        "v2",
        std::move(ann_ty),
        std::move(val2),
        VisibilityModifier::PUBLIC,
        0);
    EXPECT_EQ(infer_expression_type(context.get(), decl2.get())->get_type_name(), "i32");

    // Annotated mismatch
    auto val3 = std::make_unique<AstIntLiteral>(dummy_sf(), PrimitiveType::INT64, 42, 0);
    auto ann_ty3 = std::make_unique<AstPrimitiveType>(dummy_sf(), PrimitiveType::INT32);
    auto decl3 = std::make_unique<AstVariableDeclaration>(
        dummy_sf(),
        "some_var",
        std::move(ann_ty3),
        std::move(val3),
        VisibilityModifier::PUBLIC,
        0);
    EXPECT_THROW(infer_expression_type(context.get(), decl3.get()), stride_error);
}

TEST_F(TypeInferenceTest, InferArrayAndAccessor)
{
    // Array
    ExpressionList elements;
    elements.push_back(std::make_unique<AstIntLiteral>(dummy_sf(), PrimitiveType::INT32, 1, 0));
    elements.push_back(std::make_unique<AstIntLiteral>(dummy_sf(), PrimitiveType::INT32, 2, 0));
    auto array = std::make_unique<AstArray>(dummy_sf(), std::move(elements));
    EXPECT_EQ(infer_expression_type(context.get(), array.get())->get_type_name(), "i32[]");

    // Empty array
    ExpressionList empty_elements;
    auto empty_array = std::make_unique<AstArray>(dummy_sf(), std::move(empty_elements));
    EXPECT_EQ(infer_expression_type(context.get(), empty_array.get())->get_type_name(), "*i32[]");

    // Array Accessor
    context->define_variable(
        dummy_sym("arr"),
        std::make_unique<AstArrayType>(
            dummy_sf(),
            std::make_unique<AstPrimitiveType>(
                dummy_sf(),
                PrimitiveType::INT32),
            5),
        VisibilityModifier::PUBLIC);
    auto arr_iden = std::make_unique<AstIdentifier>(dummy_sym("arr"));
    auto idx = std::make_unique<AstIntLiteral>(dummy_sf(), PrimitiveType::INT32, 0, 0);
    auto accessor = std::make_unique<AstArrayMemberAccessor>(dummy_sf(), std::move(arr_iden), std::move(idx));
    EXPECT_EQ(infer_expression_type(context.get(), accessor.get())->get_type_name(), "i32");
}

TEST_F(TypeInferenceTest, InferArrayAccessorErrors)
{
    // Not an array
    context->define_variable(
        dummy_sym("not_arr"),
        std::make_unique<AstPrimitiveType>(dummy_sf(), PrimitiveType::INT32),
        VisibilityModifier::PUBLIC);
    auto iden = std::make_unique<AstIdentifier>(dummy_sym("not_arr"));
    auto idx = std::make_unique<AstIntLiteral>(dummy_sf(), PrimitiveType::INT32, 0, 0);
    auto access = std::make_unique<AstArrayMemberAccessor>(dummy_sf(), std::move(iden), std::move(idx));
    EXPECT_THROW(infer_expression_type(context.get(), access.get()), stride_error);
}

TEST_F(TypeInferenceTest, InferTuple)
{
    ExpressionList members;
    members.push_back(std::make_unique<AstIntLiteral>(dummy_sf(), PrimitiveType::INT32, 1, 0));
    members.push_back(std::make_unique<AstBooleanLiteral>(dummy_sf(), true));
    auto tuple = std::make_unique<AstTupleInitializer>(dummy_sf(), std::move(members));
    EXPECT_EQ(infer_expression_type(context.get(), tuple.get())->get_type_name(), "(i32, bool)");
}

TEST_F(TypeInferenceTest, InferVariadicArg)
{
    auto variadic = std::make_unique<AstVariadicArgReference>(dummy_sf());
    EXPECT_EQ(infer_expression_type(context.get(),variadic.get())->get_type_name(), "*i8");
}

TEST_F(TypeInferenceTest, InferFunctionDefinition)
{
    std::vector<std::unique_ptr<AstFunctionParameter>> params;
    params.push_back(std::make_unique<AstFunctionParameter>(
            dummy_sf(),
            "p1",
            std::make_unique<AstPrimitiveType>(
                dummy_sf(),
                PrimitiveType::INT32))
    );

    auto fn = std::make_unique<AstFunctionDeclaration>(
        dummy_sf(),
        "foo",
        std::move(params),
        nullptr,
        std::make_unique<AstPrimitiveType>(
            dummy_sf(),
            PrimitiveType::FLOAT32),
        VisibilityModifier::PUBLIC,
        0,
        std::vector<std::string>{});

    EXPECT_EQ(infer_expression_type(context.get(), fn.get())->get_type_name(), "(i32) -> f32");
}

TEST_F(TypeInferenceTest, RecursionGuard)
{
    std::unique_ptr<IAstExpression> current = std::make_unique<AstIntLiteral>(
        dummy_sf(),
        PrimitiveType::INT32,
        1,
        0);
    for (int i = 0; i < 101; ++i)
    {
        // assuming MAX_RECURSION_DEPTH is 100
        auto prev = std::move(current);
        current = std::make_unique<AstVariableReassignment>(
            dummy_sf(),
            dummy_iden("x"),
            MutativeAssignmentType::ASSIGN,
            std::move(prev));
    }

    EXPECT_THROW(infer_expression_type(context.get(), current.get()), stride_error);
}

TEST_F(TypeInferenceTest, InferFunctionCall)
{
    // Normal function call
    std::vector<std::unique_ptr<IAstType>> params;
    params.push_back(std::make_unique<AstPrimitiveType>(dummy_sf(), PrimitiveType::INT32));
    auto fn_ty = std::make_unique<AstFunctionType>(
        dummy_sf(),
        std::move(params),
        std::make_unique<AstPrimitiveType>(
            dummy_sf(),
            PrimitiveType::FLOAT32));
    context->define_function(dummy_sym("foo"), std::move(fn_ty), VisibilityModifier::PUBLIC, 0);

    ExpressionList args;
    auto arg1 = std::make_unique<AstIntLiteral>(dummy_sf(), PrimitiveType::INT32, 1, 0);
    arg1->set_type(std::make_unique<AstPrimitiveType>(dummy_sf(), PrimitiveType::INT32));
    args.push_back(std::move(arg1));
    const auto fn_call = std::make_unique<AstFunctionCall>(
        dummy_iden("foo"),
        std::move(args),
        EMPTY_GENERIC_TYPE_LIST,
        0);

    EXPECT_EQ(infer_expression_type(context.get(), fn_call.get())->get_type_name(), "f32");

    // Lambda in variable
    std::vector<std::unique_ptr<IAstType>> l_params;
    auto l_fn_ty = std::make_unique<AstFunctionType>(
        dummy_sf(),
        std::move(l_params),
        std::make_unique<AstPrimitiveType>(
            dummy_sf(),
            PrimitiveType::INT64));
    context->define_variable(dummy_sym("bar"), std::move(l_fn_ty), VisibilityModifier::PUBLIC);

    const auto l_fn_call = std::make_unique<AstFunctionCall>(
        dummy_iden("bar"),
        ExpressionList{},
        EMPTY_GENERIC_TYPE_LIST,
        0);
    EXPECT_EQ(infer_expression_type(context.get(), l_fn_call.get())->get_type_name(), "i64");
}

TEST_F(TypeInferenceTest, InferStructAndMemberAccess)
{
    // Define struct type
    std::vector<std::pair<std::string, std::unique_ptr<IAstType>>> members;
    members.emplace_back("x", std::make_unique<AstPrimitiveType>(dummy_sf(), PrimitiveType::INT32));
    members.emplace_back("y", std::make_unique<AstPrimitiveType>(dummy_sf(), PrimitiveType::FLOAT32));
    auto struct_ty = std::make_unique<AstObjectType>(dummy_sf(), "Point", std::move(members));

    context->define_type(dummy_sym("Point"), std::move(struct_ty), {}, VisibilityModifier::PUBLIC);

    // Struct initializer
    std::vector<std::pair<std::string, std::unique_ptr<IAstExpression>>> inits;
    inits.emplace_back("x", std::make_unique<AstIntLiteral>(dummy_sf(), PrimitiveType::INT32, 1, 0));
    inits.emplace_back("y", std::make_unique<AstFpLiteral>(dummy_sf(), PrimitiveType::FLOAT32, 1.0f));
    const auto struct_init = std::make_unique<AstObjectInitializer>(dummy_sf(), "Point", std::move(inits));

    EXPECT_EQ(infer_expression_type(context.get(), struct_init.get())->get_type_name(), "Point");

    // Member access
    context->define_variable(
        dummy_sym("p"),
        std::make_unique<AstAliasType>(dummy_sf(), "Point"),
        VisibilityModifier::PUBLIC);

    // Nested member access using AstChainedExpression
    context->define_variable(
        dummy_sym("q"),
        std::make_unique<AstAliasType>(dummy_sf(), "Point"),
        VisibilityModifier::PUBLIC);
    auto base2 = std::make_unique<AstIdentifier>(dummy_sym("q"));
    auto member2 = std::make_unique<AstIdentifier>(dummy_sym("x"));
    auto chained2 = std::make_unique<AstChainedExpression>(
        dummy_sf(),
        std::move(base2),
        std::move(member2));
    EXPECT_EQ(infer_expression_type(context.get(), chained2.get())->get_type_name(), "i32");
}

TEST_F(TypeInferenceTest, InferChainedExpressionErrors)
{
    // Base not found
    auto base = std::make_unique<AstIdentifier>(dummy_sym("unknown_var"));
    auto member = std::make_unique<AstIdentifier>(dummy_sym("x"));
    auto access = std::make_unique<AstChainedExpression>(dummy_sf(), std::move(base), std::move(member));
    EXPECT_THROW(infer_expression_type(context.get(), access.get()), stride_error);

    // Base is not a struct
    context->define_variable(
        dummy_sym("i"),
        std::make_unique<AstPrimitiveType>(dummy_sf(), PrimitiveType::INT32),
        VisibilityModifier::PUBLIC);
    auto base2 = std::make_unique<AstIdentifier>(dummy_sym("i"));
    auto member2 = std::make_unique<AstIdentifier>(dummy_sym("x"));
    auto access2 = std::make_unique<AstChainedExpression>(dummy_sf(), std::move(base2), std::move(member2));
    EXPECT_THROW(infer_expression_type(context.get(), access2.get()), stride_error);
}

TEST_F(TypeInferenceTest, InferNullExpression)
{
    EXPECT_THROW(infer_expression_type(context.get(), nullptr), stride_error);
}
