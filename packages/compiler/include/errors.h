#pragma once
#include "files.h"

#include <string>
#include <utility>
#include <vector>

namespace stride
{
    enum class ErrorType
    {
        SYNTAX_ERROR      = 1,
        TYPE_ERROR        = 2,
        COMPILATION_ERROR = 3,
        SEMANTIC_ERROR    = 4,
        REFERENCE_ERROR   = 5
    };

    struct ErrorSourceReference
    {
        const SourcePosition source_position;
        std::string message;

        ErrorSourceReference(
            std::string message,
            const SourcePosition& source) :
            source_position(source),
            message(std::move(message)) {}
    };

    std::string error_type_to_string(ErrorType error_type);

    /**
     * Will produce an error for the given source file with multiple highlighted sections.
     * Each reference includes offset, length, and an optional message to display below.
     */
    std::string make_source_error(
        ErrorType error_type,
        const std::string& error,
        const std::vector<ErrorSourceReference>& references,
        const std::string& src);

    class stride_error : public std::runtime_error
    {
    public:
        const ErrorType error_type;
        std::string error;
        std::vector<ErrorSourceReference> references;

        explicit stride_error(const char* str) :
            std::runtime_error(str),
            error_type(ErrorType::COMPILATION_ERROR),
            error(str),
            references({}) {}

        explicit stride_error(const std::string& str) :
            stride_error(str.c_str()) {}

        explicit stride_error(
            const ErrorType error_type,
            std::string error,
            const std::vector<ErrorSourceReference>& references
        ) :
            runtime_error(error.c_str()),
            error_type(error_type),
            error(std::move(error)),
            references(references) {}

        explicit stride_error(
            const ErrorType error_type,
            const std::string& error,
            const SourcePosition& source
        ) :
            stride_error(
                error_type,
                error,
                std::vector{ ErrorSourceReference("", source) }
            ) {}


        [[nodiscard]]
        const char* what() const noexcept override
        {
            return this->error.c_str();
        }
    };
} // namespace stride
