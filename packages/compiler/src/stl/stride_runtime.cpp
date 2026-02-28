#include "stl/stl_functions.h"

#include <chrono>
#include <cstdarg>
#include <cstdio>

using namespace stride::stl;

extern "C" {

int printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    const int result = std::vprintf(format, args);
    va_end(args);
    return result;
}

uint64_t system_time_ns()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
       .count();
}

uint64_t system_time_us()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
       .count();
}

uint64_t system_time_ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
       .count();
}
}

void stride::stl::register_internal_symbols(
    const std::shared_ptr<ast::ParsingContext>& context)
{
    const auto placeholder_position = SourceFragment(nullptr, 0, 0);
    /// Printf definition
    std::vector<std::unique_ptr<ast::IAstType>> printf_params = {};
    printf_params.push_back(
        std::make_unique<ast::AstPrimitiveType>(
            placeholder_position,
            context,
            ast::PrimitiveType::STRING,
            64,
            SRFLAG_TYPE_PTR));
    context->define_function(
        ast::Symbol(placeholder_position, "printf"),
        std::make_unique<ast::AstFunctionType>(
            placeholder_position,
            context,
            std::move(printf_params),
            std::make_unique<ast::AstPrimitiveType>(
                placeholder_position,
                context,
                ast::PrimitiveType::INT32,
                32),
            SRFLAG_FN_VARIADIC
        )
    );

    /// System time in nanoseconds definition
    context->define_function(
        ast::Symbol(placeholder_position, "system_time_ns"),
        std::make_unique<ast::AstFunctionType>(
            placeholder_position,
            context,
            std::vector<std::unique_ptr<ast::IAstType>>{},
            std::make_unique<ast::AstPrimitiveType>(
                placeholder_position,
                context,
                ast::PrimitiveType::UINT64,
                64)));

    /// System time in microseconds definition
    context->define_function(
        ast::Symbol(placeholder_position, "system_time_us"),
        std::make_unique<ast::AstFunctionType>(
            placeholder_position,
            context,
            std::vector<std::unique_ptr<ast::IAstType>>{},
            std::make_unique<ast::AstPrimitiveType>(
                placeholder_position,
                context,
                ast::PrimitiveType::UINT64,
                64)));

    /// System time in milliseconds definition
    context->define_function(
        ast::Symbol(placeholder_position, "system_time_ms"),
        std::make_unique<ast::AstFunctionType>(
            placeholder_position,
            context,
            std::vector<std::unique_ptr<ast::IAstType>>{},
            std::make_unique<ast::AstPrimitiveType>(
                placeholder_position,
                context,
                ast::PrimitiveType::UINT64,
                64)));
}
