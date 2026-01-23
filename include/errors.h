#pragma once
#include <format>
#include <string>

#include "files.h"

namespace stride
{
    enum class ErrorType
    {
        SYNTAX_ERROR = 1,
        TYPE_ERROR = 2,
        RUNTIME_ERROR = 3,
        SEMANTIC_ERROR = 4
    };

    inline std::string error_type_to_string(const ErrorType error_type)
    {
        switch (error_type)
        {
        case ErrorType::SYNTAX_ERROR: return "Syntax Error";
        case ErrorType::TYPE_ERROR: return "Type Error";
        case ErrorType::RUNTIME_ERROR: return "Runtime Error";
        case ErrorType::SEMANTIC_ERROR: return "Semantic Error";
        }
        return "Unknown Error";
    }

    static std::string make_ast_error(
        const SourceFile &source,
        const int offset,
        const std::string& error
        )
    {
        if (offset >= source.source.length())
        {
            return std::format(
                "┃ in {}\n┃ \x1b[31m{}\x1b[0m\n┃\n┃",
                source.path,
                error
            );
        }

        // Find the start of the line
        size_t line_start = offset;
        while (line_start > 0 && line_start < source.source.length() && source.source[line_start - 1] != '\n')
        {
            line_start--;
        }

        // Find the end of the line
        size_t line_end = offset;
        while (line_end < source.source.length() && source.source[line_end] != '\n')
        {
            line_end++;
        }

        // Calculate line number by counting newlines before offset
        size_t line_number = 1;
        for (size_t i = 0; i < line_start; i++)
        {
            if (source.source[i] == '\n')
            {
                line_number++;
            }
        }

        // Extract the line as a string
        std::string line_str = source.source.substr(line_start, line_end - line_start);

        return std::format(
            "┃ in \x1b[4m{}\x1b[0m:\n┃\n┃ \x1b[31m{}\x1b[0m\n┃\n┃ \x1b[0;97m{}\x1b[37m {}\x1b[0m\n┃",
            source.path,
            error,
            line_number,
            line_str
        );
    }

    /**
     * Will produce an error for the given source file, at the provided
     * position and length.
     */
    static std::string make_source_error(
        const SourceFile& source_file,
        const ErrorType error_type,
        const std::string& error,
        const size_t offset,
        const size_t length
    )
    {
        const auto error_type_str = error_type_to_string(error_type);

        if (offset >= source_file.source.length())
        {
            return std::format(
                "┃ in {}\n┃ {}\n┃ \x1b[31m{}\x1b[0m\n┃\n┃",
                source_file.path,
                error_type_str,
                error
            );
        }

        // Find the start of the line
        size_t line_start = offset;
        while (line_start > 0 && line_start < source_file.source.length() && source_file.source[line_start - 1] != '\n')
        {
            line_start--;
        }

        // Find the end of the line
        size_t line_end = offset;
        while (line_end < source_file.source.length() && source_file.source[line_end] != '\n')
        {
            line_end++;
        }

        // Calculate line number by counting newlines before offset
        size_t line_number = 1;
        for (size_t i = 0; i < line_start; i++)
        {
            if (source_file.source[i] == '\n')
            {
                line_number++;
            }
        }

        // Extract the line as a string
        static std::string line_str = source_file.source.substr(line_start, line_end - line_start);

        // Calculate column offset from line start
        const auto line_nr_str = std::to_string(line_number);
        const size_t column_offset = offset - line_start + line_nr_str.length() - 1;

        // Calculate error length (currently just 1 character)

        return std::format(
            "┃ {} in \x1b[4m{}\x1b[0m\n┃\n┃ {}\n┃\n┃ \x1b[0;97m{} \x1b[37m{}\x1b[0m\n┃  {} {}\n",
            error_type_str,
            source_file.path,
            error,
            line_number,
            line_str,
            std::string(column_offset, ' '),
            std::string(length, '^')
        );
    }

    class parsing_error : public std::runtime_error
    {
        std::string what_msg;

    public:
        explicit parsing_error(const char* str) : std::runtime_error(str), what_msg(str)
        {
        }

        explicit parsing_error(const std::string& str) : parsing_error(str.c_str())
        {
        }


        [[nodiscard]]
        const char* what() const noexcept override
        {
            return what_msg.c_str();
        }
    };
}
