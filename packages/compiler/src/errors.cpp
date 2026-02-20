#include "errors.h"

using namespace stride;

std::string stride::error_type_to_string(const ErrorType error_type)
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

std::string stride::make_source_error(
    const ErrorType error_type,
    const std::string& error,
    const SourceLocation& source_position,
    const std::string& suggestion
)
{
    const auto error_type_str = error_type_to_string(error_type);
    const auto source_file = source_position.source;

    if (source_file->source.empty() || source_position.offset >= source_file->source.length())
    {
        return std::format(
            "\n┃ in {}\n┃ {}\n┃ \x1b[31m{}\x1b[0m\n┃\n{}",
            source_file->path.empty() ? "unknown" : source_file->path,
            error_type_str,
            error,
            suggestion.empty() ? "" : std::format("┃ {}", suggestion)
        );
    }

    size_t line_start = source_position.offset;
    while (line_start > 0 && source_file->source[line_start - 1] != '\n')
    {
        line_start--;
    }

    size_t line_end = source_position.offset;
    while (line_end < source_file->source.length() && source_file->source[line_end] != '\n')
    {
        line_end++;
    }

    size_t line_number = 1;
    for (size_t i = 0; i < line_start; i++)
    {
        if (source_file->source[i] == '\n')
        {
            line_number++;
        }
    }

    // Not static: must reflect the current call's source/offset.
    const std::string line_str = source_file->source.substr(line_start, line_end - line_start);

    const auto line_nr_str = std::to_string(line_number);
    const size_t column_in_line = source_position.offset - line_start;

    // Clamp underline length to the current line.
    const size_t max_len = line_str.size() > column_in_line ? (line_str.size() - column_in_line) : 0;
    const size_t underline_len = std::min(source_position.length, max_len);

    const size_t column_offset = column_in_line + line_nr_str.length() - 1;

    return std::format(
        "\n┃ {} in \x1b[4m{}\x1b[0m\n┃\n┃ {}\n┃\n┃ \x1b[0;97m{} \x1b[37m{}\x1b[0m\n┃  {} {}{}",
        error_type_str,
        source_file->path,
        error,
        line_nr_str,
        line_str,
        std::string(column_offset, ' '),
        std::string(underline_len, '^'),
        suggestion.empty() ? "" : std::format("\n┃ {}", suggestion)
    );
}

std::string stride::make_source_error(
    const ErrorType error_type,
    const std::string& error,
    const std::vector<ErrorSourceReference>& references
)
{
    if (references.empty())
    {
        return std::format("\n┃ {}\n┃ \x1b[31m{}\x1b[0m\n┃\n┃", error_type_to_string(error_type), error);
    }

    const auto& first_ref = references[0];
    const auto& source_file = first_ref.source;
    const auto error_type_str = error_type_to_string(error_type);

    if (first_ref.source_position.offset >= source_file.source.length())
    {
        return std::format(
            "\n┃ {} in {}\n┃\n┃ \x1b[31m{}\x1b[0m\n┃\n┃",
            error_type_str,
            source_file.path,
            error
        );
    }

    // Find the start of the line
    size_t line_start = first_ref.source_position.offset;
    while (line_start > 0 && line_start < source_file.source.length() && source_file.source[line_start - 1] != '\n')
    {
        line_start--;
    }

    // Find the end of the line
    size_t line_end = first_ref.source_position.offset;
    while (line_end < source_file.source.length() && source_file.source[line_end] != '\n')
    {
        line_end++;
    }

    // Calculate line number
    size_t line_number = 1;
    for (size_t i = 0; i < line_start; i++)
    {
        if (source_file.source[i] == '\n')
        {
            line_number++;
        }
    }

    const std::string line_str = source_file.source.substr(line_start, line_end - line_start);
    const auto line_nr_str = std::to_string(line_number);
    const size_t line_nr_width = line_nr_str.length();

    // Build the underline and message lines
    const size_t base_width = line_str.length() + line_nr_width + 1;
    size_t message_width = base_width;

    for (const auto& ref : references)
    {
        if (ref.source_position.offset < line_start || ref.source_position.offset >= line_end) continue;
        const size_t col_start = ref.source_position.offset - line_start;
        message_width = std::max(message_width, col_start + line_nr_width + 1 + ref.message.length());
    }

    std::string underline_str(base_width, ' ');
    std::string message_str(message_width, ' ');

    for (const auto& ref : references)
    {
        if (ref.source_position.offset < line_start || ref.source_position.offset >= line_end) continue;

        const size_t col_start = ref.source_position.offset - line_start;
        const size_t col_end = std::min(col_start + ref.source_position.length, line_str.length());
        const size_t actual_length = col_end - col_start;

        // Add underline carets
        for (size_t i = 0; i < actual_length; ++i)
        {
            underline_str[col_start + line_nr_width + 1 + i] = '^';
        }

        // Add message below the carets if provided
        if (!ref.message.empty())
        {
            for (size_t i = 0; i < ref.message.length(); ++i)
            {
                message_str[col_start + line_nr_width + 1 + i] = ref.message[i];
            }
        }
    }

    return std::format(
        "\n┃ {} in \x1b[4m{}\x1b[0m:\n┃\n┃ {}\n┃\n┃ \x1b[0;97m{}\x1b[37m {}\x1b[0m\n┃ {}\n┃ {}",
        error_type_str,
        source_file.path,
        error,
        line_number,
        line_str,
        underline_str,
        message_str
    );
}
