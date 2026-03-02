#include "ast/parsing_context.h"
#include "stride_runtime.h"

#include <chrono>
#include <cstdarg>
#include <cstdio>

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

void stride::runtime::register_runtime_symbols(const std::shared_ptr<ast::ParsingContext>& context)
{
    const auto fragment = SourceFragment(nullptr, 0, 0);
    std::vector<std::unique_ptr<ast::IAstType>> args;
    args.push_back(std::make_unique<ast::AstPrimitiveType>(
        fragment,
        context,
        ast::PrimitiveType::STRING
    ));
    context->define_function(
        ast::Symbol(fragment, "printf"),
        std::make_unique<ast::AstFunctionType>(
            fragment,
            context,
            std::move(args),
            std::make_unique<ast::AstPrimitiveType>(
                fragment,
                context,
                ast::PrimitiveType::INT32
            ),
            /* is_variadic = */
            true
        ),
        0
    );

    context->define_function(
        ast::Symbol(fragment, "system_time_ns"),
        std::make_unique<ast::AstFunctionType>(
            fragment,
            context,
            std::vector<std::unique_ptr<ast::IAstType>>{},
            std::make_unique<ast::AstPrimitiveType>(
                fragment,
                context,
                ast::PrimitiveType::UINT64
            )
        ),
        0
    );

    context->define_function(
        ast::Symbol(fragment, "system_time_us"),
        std::make_unique<ast::AstFunctionType>(
            fragment,
            context,
            std::vector<std::unique_ptr<ast::IAstType>>{},
            std::make_unique<ast::AstPrimitiveType>(
                fragment,
                context,
                ast::PrimitiveType::UINT64
            )
        ),
        0
    );

    context->define_function(
        ast::Symbol(fragment, "system_time_ms"),
        std::make_unique<ast::AstFunctionType>(
            fragment,
            context,
            std::vector<std::unique_ptr<ast::IAstType>>{},
            std::make_unique<ast::AstPrimitiveType>(
                fragment,
                context,
                ast::PrimitiveType::UINT64
            )
        ),
        0
    );
}
