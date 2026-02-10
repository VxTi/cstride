%{
#include <string>
#include "Stride.tab.h"
%}

%option noyywrap

WHITE_SPACE [ \n\r\t\f]
COMMENT     "//"[^\r\n]*
CHAR_LITERAL   '([^\\'\r\n]|\\[^\r\n])'
IDENTIFIER     [a-zA-Z_][a-zA-Z0-9_]*
NUMBER_LITERAL [0-9]+(\.[0-9]*)?([eE][+-]?[0-9]+)?[fFdDLuU]*|0x[0-9a-fA-F]+
STRING_LITERAL \"([^\\\"\r\n]|\\[^\r\n])*\"

%x BLOCK_COMMENT

%%

"/*"                { BEGIN(BLOCK_COMMENT); }
<BLOCK_COMMENT>"*/" { BEGIN(INITIAL); }
<BLOCK_COMMENT>[^*]+ { }
<BLOCK_COMMENT>"*"   { }
<BLOCK_COMMENT><<EOF>> { BEGIN(INITIAL); }

{WHITE_SPACE}       { /* ignore */ }
{COMMENT}           { /* ignore */ }

"import"            { return IMPORT; }
"module"            { return MODULE; }
"fn"                { return FN; }
"struct"            { return STRUCT; }
"const"             { return CONST; }
"let"               { return LET; }
"extern"            { return EXTERN; }
"as"                { return AS; }
"return"            { return RETURN; }
"for"               { return FOR; }
"while"             { return WHILE; }
"if"                { return IF; }
"else"              { return ELSE; }
"void"              { return VOID; }
"i8"                { return I8; }
"i16"               { return I16; }
"i32"               { return I32; }
"i64"               { return I64; }
"u8"                { return U8; }
"u16"               { return U16; }
"u32"               { return U32; }
"u64"               { return U64; }
"f32"               { return F32; }
"f64"               { return F64; }
"bool"              { return BOOL; }
"char"              { return CHAR; }
"string"            { return STRING; }
"true"              { return BOOLEAN_LITERAL; }
"false"             { return BOOLEAN_LITERAL; }

";"                 { return SEMICOLON; }
":"                 { return COLON; }
","                 { return COMMA; }
"."                 { return DOT; }
"="                 { return EQ; }
"++"                { return PLUS_PLUS; }
"--"                { return MINUS_MINUS; }
"+="                { return PLUS_EQ; }
"-="                { return MINUS_EQ; }
"*="                { return MUL_EQ; }
"/="                { return DIV_EQ; }
"%="                { return MOD_EQ; }
"+"                 { return PLUS; }
"-"                 { return MINUS; }
"*"                 { return MUL; }
"/"                 { return DIV; }
"%"                 { return MOD; }
"!"                 { return EXCL; }
"<"                 { return LT; }
">"                 { return GT; }
"{"                 { return LBRACE; }
"}"                 { return RBRACE; }
"("                 { return LPAREN; }
")"                 { return RPAREN; }
"["                 { return LBRACKET; }
"]"                 { return RBRACKET; }
"::"                { return COLON_COLON; }
"=="                { return EQ_EQ; }
"!="                { return EXCL_EQ; }
"<="                { return LT_EQ; }
">="                { return GT_EQ; }
"&&"                { return AND; }
"||"                { return OR; }

{NUMBER_LITERAL}    { return NUMBER_LITERAL; }
{STRING_LITERAL}    { return STRING_LITERAL; }
{CHAR_LITERAL}      { return CHAR_LITERAL; }
{IDENTIFIER}        { return IDENTIFIER; }

.                   { return ERROR; }

%%
