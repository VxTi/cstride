#pragma once
#include <format>
#include <string>
#include <vector>

#include "files.h"

namespace stride
{
    enum class ErrorType
    {
        SYNTAX_ERROR   = 1,
        TYPE_ERROR     = 2,
        RUNTIME_ERROR  = 3,
        SEMANTIC_ERROR = 4
    };

    typedef struct
    {
        const SourceFile& source;
        const SourcePosition source_position;
        std::string message;
    }
    error_source_reference_t;

    std::string error_type_to_string(ErrorType error_type);

    /**
     * Will produce an error for the given source file, at the provided
     * position and length.
     */
    std::string make_source_error(
        const SourceFile& source_file,
        ErrorType error_type,
        const std::string& error,
        SourcePosition source_position,
        const std::string& suggestion = ""
    );

    /**
     * Will produce an error for the given source file with multiple highlighted sections.
     * Each reference includes offset, length, and an optional message to display below.
     */
    std::string make_source_error(
        ErrorType error_type,
        const std::string& error,
        const std::vector<error_source_reference_t>& references
    );

    class parsing_error : public std::runtime_error
    {
        std::string what_msg;

    public:
        explicit parsing_error(const char* str) : std::runtime_error(str), what_msg(str) {}

        explicit parsing_error(const std::string& str) : parsing_error(str.c_str()) {}


        [[nodiscard]]
        const char* what() const noexcept override
        {
            return what_msg.c_str();
        }
    };
}
