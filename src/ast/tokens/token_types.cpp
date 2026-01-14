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
    // Keywords
    TOKEN(TokenType::KEYWORD_LET, R"(\blet\b)"),
    TOKEN(TokenType::KEYWORD_USE, R"(\buse\b)"),
    TOKEN(TokenType::KEYWORD_CONST, R"(\bconst\b)"),
    TOKEN(TokenType::KEYWORD_FN, R"(\bfn\b)"),
    TOKEN(TokenType::KEYWORD_IF, R"(\bif\b)"),
    TOKEN(TokenType::KEYWORD_ELSE, R"(\belse\b)"),
    TOKEN(TokenType::KEYWORD_WHILE, R"(\bwhile\b)"),
    TOKEN(TokenType::KEYWORD_FOR, R"(\bfor\b)"),
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
    TOKEN(TokenType::PRIMITIVE_BOOL, R"(\bbool\b)"),
    TOKEN(TokenType::PRIMITIVE_CHAR, R"(\bchar\b)"),
    TOKEN(TokenType::PRIMITIVE_STRING, R"(\bstr\b)"),
    TOKEN(TokenType::KEYWORD_RETURN, R"(\breturn\b)"),
    TOKEN(TokenType::KEYWORD_BREAK, R"(\bbreak\b)"),
    TOKEN(TokenType::KEYWORD_CONTINUE, R"(\bcontinue\b)"),
    TOKEN(TokenType::KEYWORD_STRUCT, R"(\bstruct\b)"),
    TOKEN(TokenType::KEYWORD_ENUM, R"(\benum\b)"),
    TOKEN(TokenType::KEYWORD_IMPORT, R"(\bimport\b)"),
    TOKEN(TokenType::KEYWORD_FN, R"(\bfn\b)"),
    TOKEN(TokenType::BOOLEAN_LITERAL, R"(\btrue|false\b)"),
    TOKEN(TokenType::KEYWORD_NIL, R"(\bnil\b)"),

    // Operators
    TOKEN(TokenType::PLUS, R"(\+)"),
    TOKEN(TokenType::MINUS, R"(-)"),
    TOKEN(TokenType::STAR, R"(\*)"),
    TOKEN(TokenType::SLASH, R"(/)"),
    TOKEN(TokenType::PERCENT, R"(%)"),
    TOKEN(TokenType::EQUALS, R"(==)"),
    TOKEN(TokenType::NOT_EQUALS, R"(!=)"),
    TOKEN(TokenType::LARROW, R"(<)"),
    TOKEN(TokenType::RARROW, R"(>)"),
    TOKEN(TokenType::LEQUALS, R"(<=)"),
    TOKEN(TokenType::GEQUALS, R"(>=)"),
    TOKEN(TokenType::DOUBLE_AMPERSAND, R"(&&)"),
    TOKEN(TokenType::DOUBLE_PIPE, R"(\|\|)"),
    TOKEN(TokenType::BANG, R"(!)"),
    TOKEN(TokenType::EQUALS, R"(=)"),
    TOKEN(TokenType::DASH_RARROW, R"(->)"),
    TOKEN(TokenType::DOT, R"(\.)"),
    TOKEN(TokenType::DOUBLE_COLON, R"(::)"),

    // Punctuation
    TOKEN(TokenType::LPAREN, R"(\()"),
    TOKEN(TokenType::RPAREN, R"(\))"),
    TOKEN(TokenType::LBRACE, R"(\{)"),
    TOKEN(TokenType::RBRACE, R"(\})"),
    TOKEN(TokenType::LBRACE, R"(\[)"),
    TOKEN(TokenType::RBRACE, R"(\])"),
    TOKEN(TokenType::COMMA, R"(,)"),
    TOKEN(TokenType::SEMICOLON, R"(;)"),
    TOKEN(TokenType::COLON, R"(:)"),

    // Literals
    TOKEN(TokenType::FLOAT_LITERAL, R"(\d+(\.\d+)?)"),
    TOKEN(TokenType::STRING_LITERAL, R"("([^"\\]|\\.)*")"),
    TOKEN(TokenType::IDENTIFIER, R"(\b[a-zA-Z_][a-zA-Z0-9_]*\b)"),
};
