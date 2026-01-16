#include <algorithm>

#include "ast/tokens/token.h"

using namespace stride::ast;


// Operator precedence is defined from highest to lowest
std::vector stride::ast::operatorPrecedence = {
    TokenType::STAR,
    TokenType::SLASH,
    TokenType::PLUS,
    TokenType::MINUS,
};

bool stride::ast::precedes(const TokenType lhs, const TokenType rhs)
{
    return std::ranges::find(operatorPrecedence, lhs) < std::ranges::find(operatorPrecedence, rhs);
}


std::vector<TokenDefinition> stride::ast::tokenTypes = {
    // Comments (should be matched first)
    TOKEN(TokenType::COMMENT, R"(//[^\n]*)"),
    TOKEN(TokenType::COMMENT_MULTILINE, R"(/\*[\s\S]*?\*/)"),

    // Keywords
    TOKEN(TokenType::KEYWORD_LET, R"(\blet\b)"),
    TOKEN(TokenType::KEYWORD_USE, R"(\buse\b)"),
    TOKEN(TokenType::KEYWORD_CONST, R"(\bconst\b)"),
    TOKEN(TokenType::KEYWORD_FN, R"(\bfn\b)"),
    TOKEN(TokenType::KEYWORD_IF, R"(\bif\b)"),
    TOKEN(TokenType::KEYWORD_ELSE, R"(\belse\b)"),
    TOKEN(TokenType::KEYWORD_WHILE, R"(\bwhile\b)"),
    TOKEN(TokenType::KEYWORD_FOR, R"(\bfor\b)"),
    TOKEN(TokenType::KEYWORD_RETURN, R"(\breturn\b)"),
    TOKEN(TokenType::KEYWORD_BREAK, R"(\bbreak\b)"),
    TOKEN(TokenType::KEYWORD_CONTINUE, R"(\bcontinue\b)"),
    TOKEN(TokenType::KEYWORD_STRUCT, R"(\bstruct\b)"),
    TOKEN(TokenType::KEYWORD_ENUM, R"(\benum\b)"),
    TOKEN(TokenType::KEYWORD_CASE, R"(\bcase\b)"),
    TOKEN(TokenType::KEYWORD_DEFAULT, R"(\bdefault\b)"),
    TOKEN(TokenType::KEYWORD_IMPORT, R"(\bimport\b)"),
    TOKEN(TokenType::KEYWORD_NIL, R"(\bnil\b)"),
    TOKEN(TokenType::KEYWORD_CLASS, R"(\bclass\b)"),
    TOKEN(TokenType::KEYWORD_THIS, R"(\bthis\b)"),
    TOKEN(TokenType::KEYWORD_PUBLIC, R"(\bpublic\b)"),
    TOKEN(TokenType::KEYWORD_MODULE, R"(\bmod\b)"),
    TOKEN(TokenType::KEYWORD_EXTERNAL, R"(\bextern\b)"),
    TOKEN(TokenType::KEYWORD_OVERRIDE, R"(\boverride\b)"),
    TOKEN(TokenType::KEYWORD_AND, R"(\band\b)"),
    TOKEN(TokenType::KEYWORD_AS, R"(\bas\b)"),
    TOKEN(TokenType::KEYWORD_ASYNC, R"(\basync\b)"),
    TOKEN(TokenType::KEYWORD_DO, R"(\bdo\b)"),
    TOKEN(TokenType::KEYWORD_SWITCH, R"(\bswitch\b)"),
    TOKEN(TokenType::KEYWORD_TRY, R"(\btry\b)"),
    TOKEN(TokenType::KEYWORD_CATCH, R"(\bcatch\b)"),
    TOKEN(TokenType::KEYWORD_THROW, R"(\bthrow\b)"),
    TOKEN(TokenType::KEYWORD_NEW, R"(\bnew\b)"),

    // Primitives
    TOKEN(TokenType::PRIMITIVE_BOOL, R"(\bbool\b)"),
    TOKEN(TokenType::PRIMITIVE_INT8, R"(\bi8\b)"),
    TOKEN(TokenType::PRIMITIVE_INT16, R"(\bi16\b)"),
    TOKEN(TokenType::PRIMITIVE_INT32, R"(\bi32\b)"),
    TOKEN(TokenType::PRIMITIVE_INT64, R"(\bi64\b)"),
    TOKEN(TokenType::PRIMITIVE_UINT8, R"(\bu8\b)"),
    TOKEN(TokenType::PRIMITIVE_UINT16, R"(\bu16\b)"),
    TOKEN(TokenType::PRIMITIVE_UINT32, R"(\bu32\b)"),
    TOKEN(TokenType::PRIMITIVE_UINT64, R"(\bu64\b)"),
    TOKEN(TokenType::PRIMITIVE_FLOAT32, R"(\bfloat32\b)"),
    TOKEN(TokenType::PRIMITIVE_FLOAT64, R"(\bfloat64\b)"),
    TOKEN(TokenType::PRIMITIVE_CHAR, R"(\bchar\b)"),
    TOKEN(TokenType::PRIMITIVE_STRING, R"(\bstring\b)"),
    TOKEN(TokenType::PRIMITIVE_VOID, R"(\bvoid\b)"),
    TOKEN(TokenType::PRIMITIVE_AUTO, R"(\bauto\b)"),

    // Multi-character operators (must come before single-char operators)
    TOKEN(TokenType::DOUBLE_STAR_EQUALS, R"(\*\*=)"),
    TOKEN(TokenType::DOUBLE_LARROW_EQUALS, R"(<<=)"),
    TOKEN(TokenType::DOUBLE_RARROW_EQUALS, R"(>>=)"),
    TOKEN(TokenType::STAR_EQUALS, R"(\*=)"),
    TOKEN(TokenType::SLASH_EQUALS, R"(/=)"),
    TOKEN(TokenType::PERCENT_EQUALS, R"(%=)"),
    TOKEN(TokenType::PLUS_EQUALS, R"(\+=)"),
    TOKEN(TokenType::MINUS_EQUALS, R"(-=)"),
    TOKEN(TokenType::AMPERSAND_EQUALS, R"(&=)"),
    TOKEN(TokenType::PIPE_EQUALS, R"(\|=)"),
    TOKEN(TokenType::CARET_EQUALS, R"(\^=)"),
    TOKEN(TokenType::TILDE_EQUALS, R"(~=)"),
    TOKEN(TokenType::BANG_EQUALS, R"(!=)"),
    TOKEN(TokenType::DOUBLE_EQUALS, R"(==)"),
    TOKEN(TokenType::LEQUALS, R"(<=)"),
    TOKEN(TokenType::GEQUALS, R"(>=)"),
    TOKEN(TokenType::DOUBLE_LARROW, R"(<<)"),
    TOKEN(TokenType::DOUBLE_RARROW, R"(>>)"),
    TOKEN(TokenType::DOUBLE_PLUS, R"(\+\+)"),
    TOKEN(TokenType::DOUBLE_MINUS, R"(--)"),
    TOKEN(TokenType::DOUBLE_STAR, R"(\*\*)"),
    TOKEN(TokenType::DOUBLE_COLON, R"(::)"),
    TOKEN(TokenType::DOUBLE_AMPERSAND, R"(&&)"),
    TOKEN(TokenType::DOUBLE_PIPE, R"(\|\|)"),
    TOKEN(TokenType::DASH_RARROW, R"(->)"),
    TOKEN(TokenType::LARROW_DASH, R"(<-)"),
    TOKEN(TokenType::THREE_DOTS, R"(\.\.\.)"),

    // Single-character operators
    TOKEN(TokenType::PLUS, R"(\+)"),
    TOKEN(TokenType::MINUS, R"(-)"),
    TOKEN(TokenType::STAR, R"(\*)"),
    TOKEN(TokenType::SLASH, R"(/)"),
    TOKEN(TokenType::PERCENT, R"(%)"),
    TOKEN(TokenType::EQUALS, R"(=)"),
    TOKEN(TokenType::LARROW, R"(<)"),
    TOKEN(TokenType::RARROW, R"(>)"),
    TOKEN(TokenType::BANG, R"(!)"),
    TOKEN(TokenType::QUESTION, R"(\?)"),
    TOKEN(TokenType::AMPERSAND, R"(&)"),
    TOKEN(TokenType::PIPE, R"(\|)"),
    TOKEN(TokenType::CARET, R"(\^)"),
    TOKEN(TokenType::TILDE, R"(~)"),
    TOKEN(TokenType::DOT, R"(\.)"),

    // Punctuation
    TOKEN(TokenType::LPAREN, R"(\()"),
    TOKEN(TokenType::RPAREN, R"(\))"),
    TOKEN(TokenType::LBRACE, R"(\{)"),
    TOKEN(TokenType::RBRACE, R"(\})"),
    TOKEN(TokenType::LSQUARE_BRACKET, R"(\[)"),
    TOKEN(TokenType::RSQUARE_BRACKET, R"(\])"),
    TOKEN(TokenType::COMMA, R"(,)"),
    TOKEN(TokenType::SEMICOLON, R"(;)"),
    TOKEN(TokenType::COLON, R"(:)"),

    // Literals (string and char literals before identifiers)
    TOKEN(TokenType::STRING_LITERAL, R"("([^"\\]|\\.)*")"),
    TOKEN(TokenType::CHAR_LITERAL, R"('([^'\\]|\\.)')"),
    TOKEN(TokenType::BOOLEAN_LITERAL, R"(\b(true|false)\b)"),
    TOKEN(TokenType::FLOAT_LITERAL, R"(\d+\.\d+)"),
    TOKEN(TokenType::INTEGER_LITERAL, R"(\d+)"),
    TOKEN(TokenType::IDENTIFIER, R"([$a-zA-Z_][$a-zA-Z0-9_]*)"),
};
