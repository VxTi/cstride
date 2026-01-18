#pragma once

#include <regex>
#include <utility>

#define TOKEN(type, pattern) { TokenDefinition(type, std::regex(pattern)) }

namespace stride::ast
{
    enum class TokenType
    {
        COMMENT,           // //
        COMMENT_MULTILINE, // /* */
        END_OF_FILE,       // EOF symbol

        LPAREN,               // (
        RPAREN,               // )
        LBRACE,               // {
        RBRACE,               // }
        LSQUARE_BRACKET,      // [
        RSQUARE_BRACKET,      // ]
        COMMA,                // ,
        LARROW_DASH,          // <-
        DASH_RARROW,          // ->
        RARROW,               // >
        LARROW,               // <
        EQUALS,               // =
        DOUBLE_EQUALS,        // ==
        NOT_EQUALS,           // !=
        LEQUALS,              // <=
        GEQUALS,              // >=
        PLUS,                 // +
        MINUS,                // -
        STAR,                 // *
        SLASH,                // /
        PERCENT,              // %
        AMPERSAND,            // &
        PIPE,                 // |
        CARET,                // ^
        TILDE,                // ~
        BANG,                 // !
        QUESTION,             // ?
        TILDE_EQUALS,         // ~=
        STAR_EQUALS,          // *=
        SLASH_EQUALS,         // /=
        PERCENT_EQUALS,       // %=
        PLUS_EQUALS,          // +=
        MINUS_EQUALS,         // -=
        AMPERSAND_EQUALS,     // &=
        PIPE_EQUALS,          // |=
        CARET_EQUALS,         // ^=
        BANG_EQUALS,          // !=
        DOUBLE_LARROW,        // <<
        DOUBLE_RARROW,        // >>
        DOUBLE_LARROW_EQUALS, // <<=
        DOUBLE_RARROW_EQUALS, // >>=
        DOUBLE_STAR_EQUALS,   // **=
        DOUBLE_PIPE,          // ||
        DOUBLE_AMPERSAND,     // &&
        DOUBLE_PLUS,          // ++
        DOUBLE_MINUS,         // --
        DOUBLE_STAR,          // **
        DOUBLE_COLON,         // ::
        SEMICOLON,            // ;
        COLON,                // :
        DOT,                  // .
        THREE_DOTS,           // ...

        /* Primitives */
        PRIMITIVE_UINT8,  // uint8
        PRIMITIVE_UINT16, // uint16
        PRIMITIVE_UINT32, // uint32
        PRIMITIVE_UINT64, // uint64
        PRIMITIVE_INT8,   // int8
        PRIMITIVE_INT16,  // int16
        PRIMITIVE_INT32,  // int32
        PRIMITIVE_INT64,  // int64

        PRIMITIVE_FLOAT32, // float32
        PRIMITIVE_FLOAT64, // float64

        PRIMITIVE_BOOL,   // bool
        PRIMITIVE_STRING, // string
        PRIMITIVE_CHAR,   // char
        PRIMITIVE_VOID,   // void
        PRIMITIVE_AUTO,   // auto

        /* Keywords */
        KEYWORD_USE,      // use
        KEYWORD_AS,       // as
        KEYWORD_ASYNC,    // async
        KEYWORD_FN,       // fn
        KEYWORD_LET,      // let
        KEYWORD_CONTINUE, // continue
        KEYWORD_CONST,    // const
        KEYWORD_DO,       // do
        KEYWORD_WHILE,    // while
        KEYWORD_FOR,      // for
        KEYWORD_SWITCH,   // switch
        KEYWORD_TRY,      // try
        KEYWORD_CATCH,    // catch
        KEYWORD_THROW,    // throw
        KEYWORD_NEW,      // new
        KEYWORD_RETURN,   // return
        KEYWORD_IF,       // if
        KEYWORD_ELSE,     // else
        KEYWORD_CLASS,    // class
        KEYWORD_THIS,     // this
        KEYWORD_STRUCT,   // struct
        KEYWORD_IMPORT,   // import
        KEYWORD_PUBLIC,   // public
        KEYWORD_MODULE,   // module
        KEYWORD_EXTERN,   // external
        KEYWORD_NIL,      // nil
        KEYWORD_OVERRIDE, // override
        KEYWORD_ENUM,     // enum
        KEYWORD_CASE,     // case
        KEYWORD_DEFAULT,  // default
        KEYWORD_BREAK,    // break

        IDENTIFIER,      // variableName
        STRING_LITERAL,  // "hello"
        CHAR_LITERAL,    // 'a'
        INTEGER_LITERAL, // 42
        FLOAT_LITERAL,   // 3.14
        BOOLEAN_LITERAL, // true, false
    };

    static bool is_literal(TokenType type)
    {
        switch (type)
        {
        case TokenType::INTEGER_LITERAL:
        case TokenType::FLOAT_LITERAL:
        case TokenType::STRING_LITERAL:
        case TokenType::CHAR_LITERAL:
        case TokenType::BOOLEAN_LITERAL:
            return true;
        default:
            return false;
        }
    }

    static std::string token_type_to_str(const TokenType type)
    {
        switch (type)
        {
        case TokenType::COMMENT: return "Comment";
        case TokenType::COMMENT_MULTILINE: return "Comment Multiline";
        case TokenType::END_OF_FILE: return "End of File";
        case TokenType::LPAREN: return "(";
        case TokenType::RPAREN: return ")";
        case TokenType::LBRACE: return "{";
        case TokenType::RBRACE: return "}";
        case TokenType::LSQUARE_BRACKET: return "[";
        case TokenType::RSQUARE_BRACKET: return "]";
        case TokenType::COMMA: return ",";
        case TokenType::LARROW_DASH: return "<-";
        case TokenType::DASH_RARROW: return "->";
        case TokenType::RARROW: return ">";
        case TokenType::LARROW: return "<";
        case TokenType::EQUALS: return "=";
        case TokenType::DOUBLE_EQUALS: return "==";
        case TokenType::NOT_EQUALS: return "!=";
        case TokenType::LEQUALS: return "<=";
        case TokenType::GEQUALS: return ">=";
        case TokenType::PLUS: return "+";
        case TokenType::MINUS: return "-";
        case TokenType::STAR: return "*";
        case TokenType::SLASH: return "/";
        case TokenType::PERCENT: return "%";
        case TokenType::AMPERSAND: return "&";
        case TokenType::PIPE: return "|";
        case TokenType::CARET: return "^";
        case TokenType::TILDE: return "~";
        case TokenType::BANG: return "!";
        case TokenType::QUESTION: return "?";
        case TokenType::TILDE_EQUALS: return "~=";
        case TokenType::STAR_EQUALS: return "*=";
        case TokenType::SLASH_EQUALS: return "/=";
        case TokenType::PERCENT_EQUALS: return "%=";
        case TokenType::PLUS_EQUALS: return "+=";
        case TokenType::MINUS_EQUALS: return "-=";
        case TokenType::AMPERSAND_EQUALS: return "&=";
        case TokenType::PIPE_EQUALS: return "|=";
        case TokenType::CARET_EQUALS: return "^=";
        case TokenType::BANG_EQUALS: return "!=";
        case TokenType::DOUBLE_LARROW: return "<<";
        case TokenType::DOUBLE_RARROW: return ">>";
        case TokenType::DOUBLE_LARROW_EQUALS: return "<<=";
        case TokenType::DOUBLE_RARROW_EQUALS: return ">>=";
        case TokenType::DOUBLE_STAR_EQUALS: return "**=";
        case TokenType::DOUBLE_PIPE: return "||";
        case TokenType::DOUBLE_AMPERSAND: return "&&";
        case TokenType::DOUBLE_PLUS: return "++";
        case TokenType::DOUBLE_MINUS: return "--";
        case TokenType::DOUBLE_STAR: return "**";
        case TokenType::DOUBLE_COLON: return "::";
        case TokenType::SEMICOLON: return ";";
        case TokenType::COLON: return ":";
        case TokenType::DOT: return ".";
        case TokenType::THREE_DOTS: return "...";
        case TokenType::PRIMITIVE_UINT8: return "u8";
        case TokenType::PRIMITIVE_UINT16: return "u16";
        case TokenType::PRIMITIVE_UINT32: return "u32";
        case TokenType::PRIMITIVE_UINT64: return "u64";
        case TokenType::PRIMITIVE_INT8: return "i8";
        case TokenType::PRIMITIVE_INT16: return "i16";
        case TokenType::PRIMITIVE_INT32: return "i32";
        case TokenType::PRIMITIVE_INT64: return "i64";
        case TokenType::PRIMITIVE_FLOAT32: return "f32";
        case TokenType::PRIMITIVE_FLOAT64: return "f64";
        case TokenType::PRIMITIVE_BOOL: return "bool";
        case TokenType::PRIMITIVE_STRING: return "str";
        case TokenType::PRIMITIVE_CHAR: return "char";
        case TokenType::PRIMITIVE_VOID: return "void";
        case TokenType::PRIMITIVE_AUTO: return "auto";
        case TokenType::KEYWORD_USE: return "use";
        case TokenType::KEYWORD_AS: return "as";
        case TokenType::KEYWORD_ASYNC: return "async";
        case TokenType::KEYWORD_FN: return "fn";
        case TokenType::KEYWORD_LET: return "let";
        case TokenType::KEYWORD_CONTINUE: return "continue";
        case TokenType::KEYWORD_CONST: return "const";
        case TokenType::KEYWORD_DO: return "do";
        case TokenType::KEYWORD_WHILE: return "while";
        case TokenType::KEYWORD_FOR: return "for";
        case TokenType::KEYWORD_SWITCH: return "switch";
        case TokenType::KEYWORD_TRY: return "try";
        case TokenType::KEYWORD_CATCH: return "catch";
        case TokenType::KEYWORD_THROW: return "throw";
        case TokenType::KEYWORD_NEW: return "new";
        case TokenType::KEYWORD_RETURN: return "return";
        case TokenType::KEYWORD_IF: return "if";
        case TokenType::KEYWORD_ELSE: return "else";
        case TokenType::KEYWORD_CLASS: return "class";
        case TokenType::KEYWORD_THIS: return "this";
        case TokenType::KEYWORD_STRUCT: return "struct";
        case TokenType::KEYWORD_IMPORT: return "import";
        case TokenType::KEYWORD_PUBLIC: return "public";
        case TokenType::KEYWORD_MODULE: return "module";
        case TokenType::KEYWORD_EXTERN: return "extern";
        case TokenType::KEYWORD_NIL: return "nil";
        case TokenType::KEYWORD_OVERRIDE: return "override";
        case TokenType::KEYWORD_ENUM: return "enum";
        case TokenType::KEYWORD_CASE: return "case";
        case TokenType::KEYWORD_DEFAULT: return "default";
        case TokenType::KEYWORD_BREAK: return "break";
        case TokenType::IDENTIFIER: return "<identifier>";
        case TokenType::STRING_LITERAL: return "String Literal";
        case TokenType::CHAR_LITERAL: return "Char Literal";
        case TokenType::INTEGER_LITERAL: return "Integer";
        case TokenType::FLOAT_LITERAL: return "<float>";
        case TokenType::BOOLEAN_LITERAL: return "bool";
        default: return "Unknown";
        }
    }

    static bool is_primitive(const TokenType type)
    {
        switch (type)
        {
        case TokenType::PRIMITIVE_INT8:
        case TokenType::PRIMITIVE_INT16:
        case TokenType::PRIMITIVE_INT32:
        case TokenType::PRIMITIVE_INT64:
        case TokenType::PRIMITIVE_FLOAT32:
        case TokenType::PRIMITIVE_FLOAT64:
        case TokenType::PRIMITIVE_BOOL:
        case TokenType::PRIMITIVE_CHAR:
        case TokenType::PRIMITIVE_STRING:
        case TokenType::PRIMITIVE_VOID:
            return true;
        default:
            return false;
        }
    }

    extern std::vector<TokenType> operatorPrecedence;

    bool precedes(TokenType lhs, TokenType rhs);

    class TokenDefinition
    {
    public:
        std::regex pattern;
        TokenType type;

        ~TokenDefinition() = default;

        TokenDefinition(
            const TokenType type,
            std::regex pattern
        ) : pattern(std::move(pattern)), type(type) {}

        bool operator ==(const TokenType& other) const
        {
            return type == other;
        }

        bool operator ==(const TokenDefinition& other) const
        {
            return type == other.type;
        }
    };

    class Token
    {
    public:
        TokenType type;
        size_t offset;
        const std::string lexeme;


        explicit Token(
            const TokenType type,
            const size_t offset,
            std::string lexeme
        ) : type(type),
            offset(offset),
            lexeme(std::move(lexeme)) {}

        bool operator ==(const TokenType& other) const
        {
            return type == other;
        }
    };

    extern std::vector<TokenDefinition> tokenTypes;

    static auto END_OF_FILE = Token(TokenType::END_OF_FILE, -1, "\0");
}
