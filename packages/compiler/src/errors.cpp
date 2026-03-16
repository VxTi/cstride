#include "errors.h"

#include <algorithm>
#include <format>
#include <numeric>

using namespace stride;

std::string stride::error_type_to_string(const ErrorType error_type)
{
    switch (error_type)
    {
    case ErrorType::SYNTAX_ERROR:
        return "Syntax Error";
    case ErrorType::TYPE_ERROR:
        return "Type Error";
    case ErrorType::COMPILATION_ERROR:
        return "Compilation Error";
    case ErrorType::SEMANTIC_ERROR:
        return "Semantic Error";
    case ErrorType::REFERENCE_ERROR:
        return "Reference Error";
    }
    return "Unknown Error";
}

std::string stride::make_source_error(
    const ErrorType error_type,
    const std::string& error,
    const SourceFragment& source_position,
    const std::string& suggestion)
{
    const auto error_type_str = error_type_to_string(error_type);
    const auto source_file = source_position.source;

    if (source_file->source.empty() || source_position.offset >= source_file->
                                                                 source.length())
    {
        return std::format(
            "\n\033[0;31m┃ in {}\n\033[0;31m┃ {}\n\033[0;31m┃ \x1b[31m{}\x1b[0m\n\033[0;31m┃\n\033[0;31m{}",
            source_file->path.empty() ? "unknown" : source_file->path,
            error_type_str,
            error,
            suggestion.empty() ? "" : std::format("┃ {}", suggestion));
    }

    size_t line_start = source_position.offset;
    while (line_start > 0 && source_file->source[line_start - 1] != '\n')
    {
        line_start--;
    }

    size_t line_end = source_position.offset;
    while (line_end < source_file->source.length() && source_file->source[
        line_end] != '\n')
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
    const std::string line_str = source_file->source.substr(
        line_start,
        line_end - line_start);

    const auto line_nr_str = std::to_string(line_number);
    const size_t column_in_line = source_position.offset - line_start;

    // Clamp underline length to the current line.
    const size_t max_len =
        line_str.size() > column_in_line
        ? (line_str.size() - column_in_line)
        : 0;
    const size_t underline_len = std::min(source_position.length, max_len);

    const size_t column_offset = column_in_line + line_nr_str.length() - 1;

    return std::format(
        "\n\033[0;31m┃ {} in \x1b[4m{}\x1b[0m\n\033[0;31m┃\n\033[0;31m┃ {}\n\033[0;31m┃\n\033[0;31m┃ \x1b[0;97m{} \x1b[37m{}\x1b[0m\n\033[0;31m┃  {} {}{}",
        error_type_str,
        source_file->path,
        error,
        line_nr_str,
        line_str,
        std::string(column_offset, ' '),
        std::string(underline_len, '^'),
        suggestion.empty() ? "" : std::format("\n┃ {}", suggestion));
}

std::string stride::make_source_error(
    const ErrorType error_type,
    const std::string& error,
    const std::vector<ErrorSourceReference>& references)
{
    if (references.empty())
    {
        return std::format(
            "\n\033[0;31m┃ {}\n\033[0;31m┃ \x1b[31m{}\x1b[0m\n\033[0;31m┃\n\033[0;31m┃",
            error_type_to_string(error_type),
            error);
    }

    const auto source_file = references[0].source_position.source;
    const auto error_type_str = error_type_to_string(error_type);

    if (references[0].source_position.offset >= source_file->source.length())
    {
        return std::format(
            "\n\033[0;31m┃ {} in {}\n\033[0;31m┃\n\033[0;31m┃ \x1b[31m{}\x1b[0m\n\033[0;31m┃\n\033[0;31m┃",
            error_type_str,
            source_file->path,
            error);
    }

    const auto& src = source_file->source;

    // Compute line start, end, and 1-based number for a source offset.
    struct LineInfo
    {
        size_t line_start, line_end, line_number;
    };
    struct LineGroup
    {
        LineInfo info;
        std::vector<size_t> reference_indices;
    };

    auto get_line_info = [&](const size_t offset) -> LineInfo
    {
        size_t start = offset;
        while (start > 0 && src[start - 1] != '\n')
            start--;

        size_t end = offset;
        while (end < src.size() && src[end] != '\n')
            end++;

        size_t number = 1;
        for (size_t i = 0; i < start; i++)
        {
            if (src[i] == '\n')
                number++;
        }

        return { start, end, number };
    };

    // Sort references by source offset.
    std::vector<size_t> reference_order(references.size());
    std::iota(reference_order.begin(), reference_order.end(), 0);
    std::ranges::sort(reference_order,
                      [&](const size_t a, const size_t b)
                      {
                          return references[a].source_position.offset < references[b].source_position.offset;
                      });

    // Group references by source line.
    std::vector<LineGroup> line_groups;
    for (size_t idx : reference_order)
    {
        auto line_info = get_line_info(references[idx].source_position.offset);
        bool found = false;
        for (auto& group : line_groups)
        {
            if (group.info.line_start == line_info.line_start)
            {
                group.reference_indices.push_back(idx);
                found = true;
                break;
            }
        }
        if (!found)
        {
            line_groups.push_back({ line_info, { idx } });
        }
    }

    std::string result = std::format(
        "\n\033[0;31m┃ {} in \x1b[4m{}\x1b[0m:\n\033[0;31m┃\n\033[0;31m┃ {}\n\033[0;31m┃",
        error_type_str,
        source_file->path,
        error);

    for (size_t group_idx = 0; group_idx < line_groups.size(); group_idx++)
    {
        const auto& group = line_groups[group_idx];
        const std::string line_content = src.substr(
            group.info.line_start,
            group.info.line_end - group.info.line_start);
        const std::string line_nr_str = std::to_string(group.info.line_number);

        // base_padding: offset in underline string so underline[base_padding + column] 
        // aligns with character column in the source line.
        // Source is shown as "┃ {nr} {src}", underline as "┃ {underline_str}"
        // base_padding = line_number_length + 1 space gap.
        const size_t base_padding = line_nr_str.size() + 1;

        result += std::format(
            "\n\033[0;31m┃ \x1b[0;97m{}\x1b[37m {}\x1b[0m",
            line_nr_str,
            line_content
        );

        // Build combined underline for all references on this line.
        const auto& indices = group.reference_indices;
        size_t underline_end = base_padding;
        for (size_t idx : indices)
        {
            const auto& ref = references[idx];
            const size_t column = ref.source_position.offset - group.info.line_start;
            const size_t max_underline = line_content.size() > column ? line_content.size() - column : 0;
            const size_t underline_len = std::min(ref.source_position.length, max_underline);
            underline_end = std::max(underline_end, base_padding + column + underline_len);
        }

        std::string underline(underline_end, ' ');
        for (size_t idx : indices)
        {
            const auto& ref = references[idx];
            const size_t column = ref.source_position.offset - group.info.line_start;
            const size_t max_underline = line_content.size() > column ? line_content.size() - column : 0;
            const size_t underline_len = std::min(ref.source_position.length, max_underline);

            for (size_t i = 0; i < underline_len && base_padding + column + i < underline.size(); i++)
                underline[base_padding + column + i] = '^';
        }
        result += std::format("\n\033[0;31m┃ {}", underline);

        if (indices.size() == 2)
        {
            const auto& left_ref = references[indices[0]];  // leftmost
            const auto& right_ref = references[indices[1]]; // rightmost
            const size_t col1 = left_ref.source_position.offset - group.info.line_start;
            const size_t col2 = right_ref.source_position.offset - group.info.line_start;

            // Use descend format when both references have messages and the first reference's
            // message would overlap the second reference's column if placed on the same line.
            const bool need_descend = !left_ref.message.empty()
                && !right_ref.message.empty()
                && (base_padding + col1 + left_ref.message.size() >= base_padding + col2);

            if (need_descend)
            {
                // Line for the right-hand reference (placed first in order to be above the left message).
                // right_line: pipe connector (for left_ref) + message (for right_ref)
                std::string right_line(base_padding + col1, ' ');
                right_line += "│";
                right_line += std::string(col2 - col1 - 1, ' ');
                right_line += "└ ";
                right_line += right_ref.message;
                result += std::format("\n\033[0;31m┃ {}", right_line);

                // Connector line: vertical pipe for left reference.
                std::string connector(base_padding + col1, ' ');
                connector += "│";
                result += std::format("\n\033[0;31m┃ {}", connector);

                // Line for the left-hand reference message.
                std::string left_line(base_padding + col1, ' ');
                left_line += "└─ " + left_ref.message;
                result += std::format("\n\033[0;31m┃ {}", left_line);
            }
            else
            {
                // Flat layout: place both messages at their column positions on the same line.
                size_t msg_end = underline_end;
                for (size_t idx : indices)
                {
                    const auto& ref = references[idx];
                    const size_t column = ref.source_position.offset - group.info.line_start;
                    msg_end = std::max(msg_end, base_padding + column + ref.message.size());
                }

                std::string msg_line(msg_end, ' ');
                bool has_message = false;
                for (size_t idx : indices)
                {
                    const auto& ref = references[idx];
                    if (ref.message.empty())
                        continue;

                    has_message = true;
                    const size_t column = ref.source_position.offset - group.info.line_start;
                    for (size_t i = 0; i < ref.message.size() && base_padding + column + i < msg_line.size(); i++)
                        msg_line[base_padding + column + i] = ref.message[i];
                }
                if (has_message)
                    result += std::format("\n\033[0;31m┃ {}", msg_line);
            }
        }
        else
        {
            // 1 or 3+ references: flat message layout.
            size_t msg_end = underline_end;
            for (size_t idx : indices)
            {
                const auto& ref = references[idx];
                const size_t column = ref.source_position.offset - group.info.line_start;
                msg_end = std::max(msg_end, base_padding + column + ref.message.size());
            }

            std::string msg_line(msg_end, ' ');
            bool has_message = false;
            for (size_t idx : indices)
            {
                const auto& ref = references[idx];
                if (ref.message.empty())
                    continue;

                has_message = true;
                const size_t column = ref.source_position.offset - group.info.line_start;
                for (size_t i = 0; i < ref.message.size() && base_padding + column + i < msg_line.size(); i++)
                    msg_line[base_padding + column + i] = ref.message[i];
            }
            if (has_message)
                result += std::format("\n\033[0;31m┃ {}", msg_line);
        }

        // Blank separator between groups (different source lines).
        if (group_idx + 1 < line_groups.size())
            result += "\n\033[0;31m┃";
    }

    return result;
}
