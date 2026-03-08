#include "ast/type_inference.h"
#include "ast/nodes/literal_values.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/types.h"
#include "ast/nodes/function_declaration.h"
#include "ast/parsing_context.h"
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
    std::shared_ptr<ParsingContext> context;
    std::shared_ptr<SourceFile> source;

    void SetUp() override
    {
        context = std::make_shared<ParsingContext>();
        source = std::make_shared<SourceFile>("test.sr", "");
    }

    SourceFragment dummy_sf()
    {
        return { source, 0, 0 };
    }

    Symbol dummy_sym(const std::string& name)
    {
        return Symbol(dummy_sf(), name);
    }
};

TEST_F(TypeInferenceTest, InferLiteralTypes)
{
    // Int literals
    auto i32_lit = std::make_unique<AstIntLiteral>(dummy_sf(), context, PrimitiveType::INT32, 42, 0);
    auto i32_ty = infer_expression_type(i32_lit.get());
    EXPECT_EQ(i32_ty->to_string(), "int32");

    auto i64_lit = std::make_unique<AstIntLiteral>(dummy_sf(), context, PrimitiveType::INT64, 42, 0);
    auto i64_ty = infer_expression_type(i64_lit.get());
    EXPECT_EQ(i64_ty->to_string(), "int64");

    // Float literals
    auto f32_lit = std::make_unique<AstFpLiteral>(dummy_sf(), context, PrimitiveType::FLOAT32, 3.14f);
    auto f32_ty = infer_expression_type(f32_lit.get());
    EXPECT_EQ(f32_ty->to_string(), "float32");

    auto f64_lit = std::make_unique<AstFpLiteral>(dummy_sf(), context, PrimitiveType::FLOAT64, 3.14);
    auto f64_ty = infer_expression_type(f64_lit.get());
    EXPECT_EQ(f64_ty->to_string(), "float64");

    // Bool literal
    auto bool_lit = std::make_unique<AstBooleanLiteral>(dummy_sf(), context, true);
    auto bool_ty = infer_expression_type(bool_lit.get());
    EXPECT_EQ(bool_ty->to_string(), "bool");

    // String literal
    auto str_lit = std::make_unique<AstStringLiteral>(dummy_sf(), context, "hello");
    auto str_ty = infer_expression_type(str_lit.get());
    EXPECT_EQ(str_ty->to_string(), "string");

    // Char literal
    auto char_lit = std::make_unique<AstCharLiteral>(dummy_sf(), context, 'a');
    auto char_ty = infer_expression_type(char_lit.get());
    EXPECT_EQ(char_ty->to_string(), "char");

    // Nil literal
    auto nil_lit = std::make_unique<AstNilLiteral>(dummy_sf(), context);
    auto nil_ty = infer_expression_type(nil_lit.get());
    EXPECT_EQ(nil_ty->to_string(), "nil");
}

TEST_F(TypeInferenceTest, InferIdentifierTypes)
{
    // Variable lookup
    Symbol var_sym = dummy_sym("x");
    context->define_variable(
        var_sym,
        std::make_unique<AstPrimitiveType>(dummy_sf(), context, PrimitiveType::INT32),
        VisibilityModifier::PUBLIC);

    auto iden = std::make_unique<AstIdentifier>(context, var_sym);
    auto ty = infer_expression_type(iden.get());
    EXPECT_EQ(ty->to_string(), "int32");

    // Function lookup (as identifier)
    Symbol fn_sym = dummy_sym("foo");
    std::vector<std::unique_ptr<IAstType>> params;
    params.push_back(std::make_unique<AstPrimitiveType>(dummy_sf(), context, PrimitiveType::INT32));
    auto fn_ty = std::make_unique<AstFunctionType>(
        dummy_sf(),
        context,
        std::move(params),
        std::make_unique<AstPrimitiveType>(
            dummy_sf(),
            context,
            PrimitiveType::FLOAT32));
    context->define_function(fn_sym, std::move(fn_ty), VisibilityModifier::PUBLIC, 0);

    auto fn_iden = std::make_unique<AstIdentifier>(context, fn_sym);
    auto fn_res_ty = infer_expression_type(fn_iden.get());
    EXPECT_EQ(fn_res_ty->to_string(), "(int32) -> float32");

    // Symbol not found
    auto unknown_iden = std::make_unique<AstIdentifier>(context, dummy_sym("unknown"));
    EXPECT_THROW(infer_expression_type(unknown_iden.get()), parsing_error);
}

TEST_F(TypeInferenceTest, InferTypeCast)
{
    auto lit = std::make_unique<AstIntLiteral>(dummy_sf(), context, PrimitiveType::INT32, 42, 0);
    auto target_ty = std::make_unique<AstPrimitiveType>(dummy_sf(), context, PrimitiveType::FLOAT64);
    auto cast = std::make_unique<AstTypeCastOp>(dummy_sf(), context, std::move(lit), std::move(target_ty));

    auto ty = infer_expression_type(cast.get());
    EXPECT_EQ(ty->to_string(), "float64");
}

TEST_F(TypeInferenceTest, InferBinaryOp)
{
    // Same types
    auto lhs = std::make_unique<AstIntLiteral>(dummy_sf(), context, PrimitiveType::INT32, 1, 0);
    auto rhs = std::make_unique<AstIntLiteral>(dummy_sf(), context, PrimitiveType::INT32, 2, 0);
    auto op = std::make_unique<AstBinaryArithmeticOp>(
        dummy_sf(),
        context,
        std::move(lhs),
        BinaryOpType::ADD,
        std::move(rhs));
    EXPECT_EQ(infer_expression_type(op.get())->to_string(), "int32");

    // Pointer priority (LHS)
    context->define_variable(
        dummy_sym("p"),
        std::make_unique<AstPrimitiveType>(
            dummy_sf(),
            context,
            PrimitiveType::INT32,
            SRFLAG_TYPE_PTR),
        VisibilityModifier::PUBLIC);
    context->define_variable(
        dummy_sym("i"),
        std::make_unique<AstPrimitiveType>(dummy_sf(), context, PrimitiveType::INT32),
        VisibilityModifier::PUBLIC);

    auto p_iden = std::make_unique<AstIdentifier>(context, dummy_sym("p"));
    auto i_iden = std::make_unique<AstIdentifier>(context, dummy_sym("i"));
    auto p_op = std::make_unique<AstBinaryArithmeticOp>(
        dummy_sf(),
        context,
        std::move(p_iden),
        BinaryOpType::ADD,
        std::move(i_iden));
    EXPECT_EQ(infer_expression_type(p_op.get())->to_string(), "*int32");

    auto i_iden2 = std::make_unique<AstIdentifier>(context, dummy_sym("i"));
    auto p_iden2 = std::make_unique<AstIdentifier>(context, dummy_sym("p"));
    auto p_op2 = std::make_unique<AstBinaryArithmeticOp>(
        dummy_sf(),
        context,
        std::move(i_iden2),
        BinaryOpType::ADD,
        std::move(p_iden2));
    EXPECT_EQ(infer_expression_type(p_op2.get())->to_string(), "int32");
}

TEST_F(TypeInferenceTest, InferUnaryOp)
{
    context->define_variable(
        dummy_sym("x"),
        std::make_unique<AstPrimitiveType>(dummy_sf(), context, PrimitiveType::INT32),
        VisibilityModifier::PUBLIC);

    // Address of
    auto iden = std::make_unique<AstIdentifier>(context, dummy_sym("x"));
    auto addr_op = std::make_unique<AstUnaryOp>(dummy_sf(), context, UnaryOpType::ADDRESS_OF, std::move(iden));
    EXPECT_EQ(infer_expression_type(addr_op.get())->to_string(), "*int32");

    // Dereference
    context->define_variable(
        dummy_sym("px"),
        std::make_unique<AstPrimitiveType>(
            dummy_sf(),
            context,
            PrimitiveType::INT32,
            SRFLAG_TYPE_PTR),
        VisibilityModifier::PUBLIC);
    auto p_iden = std::make_unique<AstIdentifier>(context, dummy_sym("px"));
    auto deref_op = std::make_unique<AstUnaryOp>(dummy_sf(), context, UnaryOpType::DEREFERENCE, std::move(p_iden));
    EXPECT_EQ(infer_expression_type(deref_op.get())->to_string(), "*int32");

    // Logical Not
    auto bool_lit = std::make_unique<AstBooleanLiteral>(dummy_sf(), context, true);
    auto not_op = std::make_unique<AstUnaryOp>(dummy_sf(), context, UnaryOpType::LOGICAL_NOT, std::move(bool_lit));
    EXPECT_EQ(infer_expression_type(not_op.get())->to_string(), "bool");
}

TEST_F(TypeInferenceTest, InferLogicalAndComparison)
{
    auto lhs = std::make_unique<AstIntLiteral>(dummy_sf(), context, PrimitiveType::INT32, 1, 0);
    auto rhs = std::make_unique<AstIntLiteral>(dummy_sf(), context, PrimitiveType::INT32, 2, 0);

    const auto comp_op = std::make_unique<AstComparisonOp>(
        dummy_sf(),
        context,
        std::move(lhs),
        ComparisonOpType::EQUALS,
        std::move(rhs)
    );
    EXPECT_EQ(infer_expression_type(comp_op.get())->to_string(), "bool");

    auto blhs = std::make_unique<AstBooleanLiteral>(dummy_sf(), context, true);
    auto brhs = std::make_unique<AstBooleanLiteral>(dummy_sf(), context, false);
    auto log_op = std::make_unique<AstLogicalOp>(
        dummy_sf(),
        context,
        std::move(blhs),
        LogicalOpType::AND,
        std::move(brhs));
    EXPECT_EQ(infer_expression_type(log_op.get())->to_string(), "bool");
}

TEST_F(TypeInferenceTest, InferVariableDeclaration)
{
    // Non-annotated
    auto val = std::make_unique<AstIntLiteral>(dummy_sf(), context, PrimitiveType::INT32, 42, 0);
    auto decl = std::make_unique<AstVariableDeclaration>(context,
                                                         dummy_sym("v"),
                                                         std::nullopt,
                                                         std::move(val),
                                                         VisibilityModifier::PUBLIC,
                                                         0);
    EXPECT_EQ(infer_expression_type(decl.get())->to_string(), "int32");

    // Annotated same type
    auto val2 = std::make_unique<AstIntLiteral>(dummy_sf(), context, PrimitiveType::INT32, 42, 0);
    auto ann_ty = std::make_unique<AstPrimitiveType>(dummy_sf(), context, PrimitiveType::INT32);
    auto decl2 = std::make_unique<AstVariableDeclaration>(
        context,
        dummy_sym("v2"),
        std::move(ann_ty),
        std::move(val2),
        VisibilityModifier::PUBLIC,
        0);
    EXPECT_EQ(infer_expression_type(decl2.get())->to_string(), "int32");

    // Annotated mismatch
    auto val3 = std::make_unique<AstIntLiteral>(dummy_sf(), context, PrimitiveType::INT64, 42, 0);
    auto ann_ty3 = std::make_unique<AstPrimitiveType>(dummy_sf(), context, PrimitiveType::INT32);
    auto decl3 = std::make_unique<AstVariableDeclaration>(
        context,
        dummy_sym("v3"),
        std::move(ann_ty3),
        std::move(val3),
        VisibilityModifier::PUBLIC,
        0);
    EXPECT_THROW(infer_expression_type(decl3.get()), parsing_error);
}

TEST_F(TypeInferenceTest, InferArrayAndAccessor)
{
    // Array
    ExpressionList elements;
    elements.push_back(std::make_unique<AstIntLiteral>(dummy_sf(), context, PrimitiveType::INT32, 1, 0));
    elements.push_back(std::make_unique<AstIntLiteral>(dummy_sf(), context, PrimitiveType::INT32, 2, 0));
    auto array = std::make_unique<AstArray>(dummy_sf(), context, std::move(elements));
    EXPECT_EQ(infer_expression_type(array.get())->to_string(), "int32[]");

    // Empty array
    ExpressionList empty_elements;
    auto empty_array = std::make_unique<AstArray>(dummy_sf(), context, std::move(empty_elements));
    EXPECT_EQ(infer_expression_type(empty_array.get())->to_string(), "*int32[]");

    // Array Accessor
    context->define_variable(
        dummy_sym("arr"),
        std::make_unique<AstArrayType>(
            dummy_sf(),
            context,
            std::make_unique<AstPrimitiveType>(
                dummy_sf(),
                context,
                PrimitiveType::INT32),
            5),
        VisibilityModifier::PUBLIC);
    auto arr_iden = std::make_unique<AstIdentifier>(context, dummy_sym("arr"));
    auto idx = std::make_unique<AstIntLiteral>(dummy_sf(), context, PrimitiveType::INT32, 0, 0);
    auto accessor = std::make_unique<AstArrayMemberAccessor>(dummy_sf(), context, std::move(arr_iden), std::move(idx));
    EXPECT_EQ(infer_expression_type(accessor.get())->to_string(), "int32");
}

TEST_F(TypeInferenceTest, InferArrayAccessorErrors)
{
    // Not an array
    context->define_variable(
        dummy_sym("not_arr"),
        std::make_unique<AstPrimitiveType>(dummy_sf(), context, PrimitiveType::INT32),
        VisibilityModifier::PUBLIC);
    auto iden = std::make_unique<AstIdentifier>(context, dummy_sym("not_arr"));
    auto idx = std::make_unique<AstIntLiteral>(dummy_sf(), context, PrimitiveType::INT32, 0, 0);
    auto access = std::make_unique<AstArrayMemberAccessor>(dummy_sf(), context, std::move(iden), std::move(idx));
    EXPECT_THROW(infer_expression_type(access.get()), parsing_error);
}

TEST_F(TypeInferenceTest, InferTuple)
{
    ExpressionList members;
    members.push_back(std::make_unique<AstIntLiteral>(dummy_sf(), context, PrimitiveType::INT32, 1, 0));
    members.push_back(std::make_unique<AstBooleanLiteral>(dummy_sf(), context, true));
    auto tuple = std::make_unique<AstTupleInitializer>(dummy_sf(), context, std::move(members));
    EXPECT_EQ(infer_expression_type(tuple.get())->to_string(), "(int32, bool)");
}

TEST_F(TypeInferenceTest, InferVariadicArg)
{
    auto variadic = std::make_unique<AstVariadicArgReference>(dummy_sf(), context);
    EXPECT_EQ(infer_expression_type(variadic.get())->to_string(), "*int8");
}

TEST_F(TypeInferenceTest, InferFunctionDefinition)
{
    std::vector<std::unique_ptr<AstFunctionParameter>> params;
    params.push_back(std::make_unique<AstFunctionParameter>(
            dummy_sf(),
            context,
            "p1",
            std::make_unique<AstPrimitiveType>(
                dummy_sf(),
                context,
                PrimitiveType::INT32))
    );

    auto fn = std::make_unique<AstFunctionDeclaration>(
        context,
        dummy_sym("foo"),
        std::move(params),
        nullptr,
        std::make_unique<AstPrimitiveType>(
            dummy_sf(),
            context,
            PrimitiveType::FLOAT32),
        VisibilityModifier::PUBLIC,
        0,
        std::vector<std::string>{});

    EXPECT_EQ(infer_expression_type(fn.get())->to_string(), "(int32) -> float32");
}

TEST_F(TypeInferenceTest, RecursionGuard)
{
    std::unique_ptr<IAstExpression> current = std::make_unique<AstIntLiteral>(
        dummy_sf(),
        context,
        PrimitiveType::INT32,
        1,
        0);
    for (int i = 0; i < 101; ++i)
    {
        // assuming MAX_RECURSION_DEPTH is 100
        auto prev = std::move(current);
        current = std::make_unique<AstVariableReassignment>(
            dummy_sf(),
            context,
            "v",
            MutativeAssignmentType::ASSIGN,
            std::move(prev));
    }

    EXPECT_THROW(infer_expression_type(current.get()), parsing_error);
}

TEST_F(TypeInferenceTest, InferFunctionCall)
{
    // Normal function call
    Symbol fn_sym = dummy_sym("foo");
    std::vector<std::unique_ptr<IAstType>> params;
    params.push_back(std::make_unique<AstPrimitiveType>(dummy_sf(), context, PrimitiveType::INT32));
    auto fn_ty = std::make_unique<AstFunctionType>(
        dummy_sf(),
        context,
        std::move(params),
        std::make_unique<AstPrimitiveType>(
            dummy_sf(),
            context,
            PrimitiveType::FLOAT32));
    context->define_function(fn_sym, std::move(fn_ty), VisibilityModifier::PUBLIC, 0);

    ExpressionList args;
    auto arg1 = std::make_unique<AstIntLiteral>(dummy_sf(), context, PrimitiveType::INT32, 1, 0);
    arg1->set_type(std::make_unique<AstPrimitiveType>(dummy_sf(), context, PrimitiveType::INT32));
    args.push_back(std::move(arg1));
    auto fn_call = std::make_unique<AstFunctionCall>(context, fn_sym, std::move(args), 0);

    EXPECT_EQ(infer_expression_type(fn_call.get())->to_string(), "float32");

    // Lambda in variable
    Symbol lambda_var = dummy_sym("bar");
    std::vector<std::unique_ptr<IAstType>> l_params;
    auto l_fn_ty = std::make_unique<AstFunctionType>(
        dummy_sf(),
        context,
        std::move(l_params),
        std::make_unique<AstPrimitiveType>(
            dummy_sf(),
            context,
            PrimitiveType::INT64));
    context->define_variable(lambda_var, std::move(l_fn_ty), VisibilityModifier::PUBLIC);

    auto l_fn_call = std::make_unique<AstFunctionCall>(
        context,
        lambda_var,
        ExpressionList{},
        0);
    EXPECT_EQ(infer_expression_type(l_fn_call.get())->to_string(), "int64");
}

TEST_F(TypeInferenceTest, InferStructAndMemberAccess)
{
    // Define struct type
    std::vector<std::pair<std::string, std::unique_ptr<IAstType>>> members;
    members.emplace_back("x", std::make_unique<AstPrimitiveType>(dummy_sf(), context, PrimitiveType::INT32));
    members.emplace_back("y", std::make_unique<AstPrimitiveType>(dummy_sf(), context, PrimitiveType::FLOAT32));
    auto struct_ty = std::make_unique<AstStructType>(dummy_sf(), context, std::move(members));

    context->define_type(dummy_sym("Point"), std::move(struct_ty), VisibilityModifier::PUBLIC);

    // Struct initializer
    std::vector<std::pair<std::string, std::unique_ptr<IAstExpression>>> inits;
    inits.emplace_back("x", std::make_unique<AstIntLiteral>(dummy_sf(), context, PrimitiveType::INT32, 1, 0));
    inits.emplace_back("y", std::make_unique<AstFpLiteral>(dummy_sf(), context, PrimitiveType::FLOAT32, 1.0f));
    auto struct_init = std::make_unique<AstStructInitializer>(dummy_sf(), context, "Point", std::move(inits));

    EXPECT_EQ(infer_expression_type(struct_init.get())->to_string(), "Point");

    // Member access
    context->define_variable(dummy_sym("p"),
                             std::make_unique<AstNamedType>(dummy_sf(), context, "Point"),
                             VisibilityModifier::PUBLIC);

    // Nested member access
    context->define_variable(dummy_sym("q"),
                             std::make_unique<AstNamedType>(dummy_sf(), context, "Point"),
                             VisibilityModifier::PUBLIC);
    auto base2 = std::make_unique<AstIdentifier>(context, dummy_sym("q"));
    std::vector<std::unique_ptr<AstIdentifier>> access_members2;
    access_members2.push_back(std::make_unique<AstIdentifier>(context, dummy_sym("x")));
    auto member_access2 = std::make_unique<AstMemberAccessor>(
        dummy_sf(),
        context,
        std::move(base2),
        std::move(access_members2));
    EXPECT_EQ(infer_expression_type(member_access2.get())->to_string(), "int32");
}

TEST_F(TypeInferenceTest, InferMemberAccessorErrors)
{
    // Base not found
    auto base = std::make_unique<AstIdentifier>(context, dummy_sym("unknown_var"));
    std::vector<std::unique_ptr<AstIdentifier>> members;
    members.push_back(std::make_unique<AstIdentifier>(context, dummy_sym("x")));
    auto access = std::make_unique<AstMemberAccessor>(dummy_sf(), context, std::move(base), std::move(members));
    EXPECT_THROW(infer_expression_type(access.get()), parsing_error);

    // Base is not a struct
    context->define_variable(dummy_sym("i"),
                             std::make_unique<AstPrimitiveType>(dummy_sf(), context, PrimitiveType::INT32),
                             VisibilityModifier::PUBLIC);
    auto base2 = std::make_unique<AstIdentifier>(context, dummy_sym("i"));
    std::vector<std::unique_ptr<AstIdentifier>> members2;
    members2.push_back(std::make_unique<AstIdentifier>(context, dummy_sym("x")));
    auto access2 = std::make_unique<AstMemberAccessor>(dummy_sf(), context, std::move(base2), std::move(members2));
    EXPECT_THROW(infer_expression_type(access2.get()), parsing_error);
}

TEST_F(TypeInferenceTest, InferNullExpression)
{
    EXPECT_THROW(infer_expression_type(nullptr), parsing_error);
}
