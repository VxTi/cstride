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
        size_t start, end, number;
    };
    auto get_line_info = [&](size_t offset) -> LineInfo
    {
        size_t ls = offset;
        while (ls > 0 && src[ls - 1] != '\n')
            ls--;
        size_t le = offset;
        while (le < src.size() && src[le] != '\n')
            le++;
        size_t ln = 1;
        for (size_t i = 0; i < ls; i++)
            if (src[i] == '\n')
                ln++;
        return { ls, le, ln };
    };

    // Sort references by source offset.
    std::vector<size_t> order(references.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(),
              order.end(),
              [&](size_t a, size_t b)
              {
                  return references[a].source_position.offset < references[b].source_position.offset;
              });

    // Group references by source line.
    struct LineGroup
    {
        LineInfo info;
        std::vector<size_t> ref_indices;
    };
    std::vector<LineGroup> groups;
    for (size_t idx : order)
    {
        auto li = get_line_info(references[idx].source_position.offset);
        bool found = false;
        for (auto& g : groups)
        {
            if (g.info.start == li.start)
            {
                g.ref_indices.push_back(idx);
                found = true;
                break;
            }
        }
        if (!found)
            groups.push_back({ li, { idx } });
    }

    std::string result = std::format(
        "\n\033[0;31m┃ {} in \x1b[4m{}\x1b[0m:\n\033[0;31m┃\n\033[0;31m┃ {}\n\033[0;31m┃",
        error_type_str,
        source_file->path,
        error);

    for (size_t gi = 0; gi < groups.size(); gi++)
    {
        const auto& group = groups[gi];
        const std::string line_str = src.substr(
            group.info.start,
            group.info.end - group.info.start);
        const std::string line_nr_str = std::to_string(group.info.number);

        // pw: offset in underline string so underline[pw + col] aligns with
        // character col in the source line.  Source is shown as "┃ {nr} {src}",
        // underline as "┃ {underline_str}" — pw = lnw + 1 bridges the gap.
        const size_t pw = line_nr_str.size() + 1;

        result += std::format("\n\033[0;31m┃ \x1b[0;97m{}\x1b[37m {}\x1b[0m",
                              line_nr_str,
                              line_str);

        // Build combined underline for all refs on this line.
        const auto& refs = group.ref_indices;
        size_t underline_end = pw;
        for (size_t idx : refs)
        {
            const auto& ref = references[idx];
            const size_t col = ref.source_position.offset - group.info.start;
            const size_t max_ul = line_str.size() > col ? line_str.size() - col : 0;
            const size_t ul = std::min(ref.source_position.length, max_ul);
            underline_end = std::max(underline_end, pw + col + ul);
        }
        std::string underline(underline_end, ' ');
        for (size_t idx : refs)
        {
            const auto& ref = references[idx];
            const size_t col = ref.source_position.offset - group.info.start;
            const size_t max_ul = line_str.size() > col ? line_str.size() - col : 0;
            const size_t ul = std::min(ref.source_position.length, max_ul);
            for (size_t i = 0; i < ul && pw + col + i < underline.size(); i++)
                underline[pw + col + i] = '^';
        }
        result += std::format("\n\033[0;31m┃ {}", underline);

        if (refs.size() == 2)
        {
            const auto& left_ref = references[refs[0]];  // leftmost
            const auto& right_ref = references[refs[1]]; // rightmost
            const size_t col1 = left_ref.source_position.offset - group.info.start;
            const size_t col2 = right_ref.source_position.offset - group.info.start;

            // Use descend format when both refs have messages and ref1's
            // message would overlap ref2's column if placed on the same line.
            const bool need_descend = !left_ref.message.empty()
                && !right_ref.message.empty()
                && (pw + col1 + left_ref.message.size() >= pw + col2);

            if (need_descend)
            {
                // Right ref: pipe connector (for ref1) + message (for ref2)
                std::string right_line(pw + col1, ' ');
                right_line += "┃";
                right_line += std::string(col2 - col1 - 1, ' ');
                right_line += "┗ ";
                right_line += right_ref.message;
                result += std::format("\n\033[0;31m┃ {}", right_line);

                // Left ref: ┃ descent connector
                std::string connector(pw + col1, ' ');
                connector += "┃";
                result += std::format("\n\033[0;31m┃ {}", connector);

                // Left ref: message
                std::string left_line(pw + col1, ' ');
                left_line += "┗ " + left_ref.message;
                result += std::format("\n\033[0;31m┃ {}", left_line);
            }
            else
            {
                // Flat: place both messages at their column positions.
                size_t msg_end = underline_end;
                for (size_t idx : refs)
                {
                    const auto& ref = references[idx];
                    const size_t col = ref.source_position.offset - group.info.start;
                    msg_end = std::max(msg_end, pw + col + ref.message.size());
                }
                std::string msg_line(msg_end, ' ');
                bool has_msg = false;
                for (size_t idx : refs)
                {
                    const auto& ref = references[idx];
                    if (ref.message.empty())
                        continue;
                    has_msg = true;
                    const size_t col = ref.source_position.offset - group.info.start;
                    for (size_t i = 0; i < ref.message.size() && pw + col + i < msg_line.size(); i++)
                        msg_line[pw + col + i] = ref.message[i];
                }
                if (has_msg)
                    result += std::format("\n\033[0;31m┃ {}", msg_line);
            }
        }
        else
        {
            // 1 or 3+ refs: flat message layout.
            size_t msg_end = underline_end;
            for (size_t idx : refs)
            {
                const auto& ref = references[idx];
                const size_t col = ref.source_position.offset - group.info.start;
                msg_end = std::max(msg_end, pw + col + ref.message.size());
            }
            std::string msg_line(msg_end, ' ');
            bool has_msg = false;
            for (size_t idx : refs)
            {
                const auto& ref = references[idx];
                if (ref.message.empty())
                    continue;
                has_msg = true;
                const size_t col = ref.source_position.offset - group.info.start;
                for (size_t i = 0; i < ref.message.size() && pw + col + i < msg_line.size(); i++)
                    msg_line[pw + col + i] = ref.message[i];
            }
            if (has_msg)
                result += std::format("\n\033[0;31m┃ {}", msg_line);
        }

        // Blank separator between groups (different source lines).
        if (gi + 1 < groups.size())
            result += "\n\033[0;31m┃";
    }

    return result;
}
