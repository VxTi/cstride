#pragma once
#include "files.h"

#include <format>
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
        const SourceFragment source_position;
        std::string message;

        ErrorSourceReference(
            std::string message,
            const SourceFragment& source) :
            source_position(source),
            message(std::move(message)) {}
    };

    std::string error_type_to_string(ErrorType error_type);

    /**
     * Will produce an error for the given source file, at the provided
     * position and length.
     */
    std::string make_source_error(
        ErrorType error_type,
        const std::string& error,
        const SourceFragment& source_position,
        const std::string& suggestion = "");

    /**
     * Will produce an error for the given source file with multiple highlighted sections.
     * Each reference includes offset, length, and an optional message to display below.
     */
    std::string make_source_error(
        ErrorType error_type,
        const std::string& error,
        const std::vector<ErrorSourceReference>& references);

    class parsing_error : public std::runtime_error
    {
        std::string what_msg;

    public:
        explicit parsing_error(const char* str) :
            std::runtime_error(str),
            what_msg(str) {}

        explicit parsing_error(const std::string& str) :
            parsing_error(str.c_str()) {}

        explicit parsing_error(
            const ErrorType error_type,
            const std::string& error,
            const SourceFragment& source,
            const std::string& suggestion = "") :
            parsing_error(
                make_source_error(error_type, error, source, suggestion)) {}

        explicit parsing_error(
            const ErrorType error_type,
            const std::string& error,
            const std::vector<ErrorSourceReference>& references) :
            parsing_error(make_source_error(error_type, error, references)) {}

        [[nodiscard]]
        const char* what() const noexcept override
        {
            return what_msg.c_str();
        }
    };
} // namespace stride
