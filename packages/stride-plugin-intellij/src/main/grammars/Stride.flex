package com.stride.intellij.lexer;

import com.intellij.lexer.FlexLexer;
import com.intellij.psi.tree.IElementType;
import com.stride.intellij.psi.StrideTypes;
import com.intellij.psi.TokenType;

%%
%state BLOCK_COMMENT
%class StrideLexer
%implements FlexLexer
%unicode
%function advance
%type IElementType
%eof{  return;
%eof}

CRLF=\R
WHITE_SPACE=[\ \n\r\t\f]
COMMENT="//"[^\r\n]*
IDENTIFIER=[a-zA-Z_][a-zA-Z0-9_]*
NUMBER_LITERAL=[0-9]+(\.[0-9]*)?([eE][+-]?[0-9]+)?[fFdDLuU]*|0x[0-9a-fA-F]+
CHAR_LITERAL='([^\\'\r\n]|\\[^\r\n])'
STRING_LITERAL=\"([^\\\"\r\n]|\\[^\r\n])*\"

%%
<BLOCK_COMMENT> {
  "*/"               { yybegin(YYINITIAL); return StrideTypes.COMMENT; }
  [^*]+              { } // Eat anything that isn't an asterisk
  "*"                { } // Eat asterisks that aren't followed by /
  <<EOF>>            { yybegin(YYINITIAL); return StrideTypes.COMMENT; }
}

<YYINITIAL> {
  {WHITE_SPACE}      { return TokenType.WHITE_SPACE; }
  {COMMENT}          { return StrideTypes.COMMENT; }

   "/*"              { yybegin(BLOCK_COMMENT); }

  "pub"             { return StrideTypes.PUB; }
  "package"          { return StrideTypes.PACKAGE; }
  "module"           { return StrideTypes.MODULE; }
  "fn"               { return StrideTypes.FN; }
  "struct"           { return StrideTypes.STRUCT; }
  "const"            { return StrideTypes.CONST; }
  "let"              { return StrideTypes.LET; }
  "extern"           { return StrideTypes.EXTERN; }
  "as"               { return StrideTypes.AS; }
  "return"           { return StrideTypes.RETURN; }
  "for"              { return StrideTypes.FOR; }
  "while"            { return StrideTypes.WHILE; }
  "if"               { return StrideTypes.IF; }
  "else"             { return StrideTypes.ELSE; }
  "void"             { return StrideTypes.VOID; }
  "i8"               { return StrideTypes.I8; }
  "i16"              { return StrideTypes.I16; }
  "i32"              { return StrideTypes.I32; }
  "i64"              { return StrideTypes.I64; }
  "u8"               { return StrideTypes.U8; }
  "u16"              { return StrideTypes.U16; }
  "u32"              { return StrideTypes.U32; }
  "u64"              { return StrideTypes.U64; }
  "f32"              { return StrideTypes.F32; }
  "f64"              { return StrideTypes.F64; }
  "bool"             { return StrideTypes.BOOL; }
  "char"             { return StrideTypes.CHAR; }
  "string"           { return StrideTypes.STRING; }
  "true"             { return StrideTypes.BOOLEAN_LITERAL; }
  "false"            { return StrideTypes.BOOLEAN_LITERAL; }

  ";"                { return StrideTypes.SEMICOLON; }
  ":"                { return StrideTypes.COLON; }
  ","                { return StrideTypes.COMMA; }
  "."                { return StrideTypes.DOT; }
  "="                { return StrideTypes.EQ; }
  "++"               { return StrideTypes.PLUS_PLUS; }
  "--"               { return StrideTypes.MINUS_MINUS; }
  "+="               { return StrideTypes.PLUS_EQ; }
  "-="               { return StrideTypes.MINUS_EQ; }
  "*="               { return StrideTypes.ASTERISK_EQ; }
  "/="               { return StrideTypes.SLASH_EQ; }
  "%="               { return StrideTypes.MOD_EQ; }
  "+"                { return StrideTypes.PLUS; }
  "-"                { return StrideTypes.MINUS; }
  "*"                { return StrideTypes.ASTERISK; }
  "&"                { return StrideTypes.AMPERSAND; }
  "/"                { return StrideTypes.DIV; }
  "%"                { return StrideTypes.MOD; }
  "!"                { return StrideTypes.EXCL; }
  "<"                { return StrideTypes.LT; }
  ">"                { return StrideTypes.GT; }
  "{"                { return StrideTypes.LBRACE; }
  "}"                { return StrideTypes.RBRACE; }
  "("                { return StrideTypes.LPAREN; }
  ")"                { return StrideTypes.RPAREN; }
  "::"               { return StrideTypes.COLON_COLON; }
  "=="               { return StrideTypes.EQ_EQ; }
  "!="               { return StrideTypes.EXCL_EQ; }
  "<="               { return StrideTypes.LT_EQ; }
  ">="               { return StrideTypes.GT_EQ; }
  "&&"               { return StrideTypes.OPERATOR; }
  "||"               { return StrideTypes.OPERATOR; }

  {NUMBER_LITERAL}   { return StrideTypes.NUMBER_LITERAL; }
  {STRING_LITERAL}   { return StrideTypes.STRING_LITERAL; }
  {CHAR_LITERAL}     { return StrideTypes.CHAR_LITERAL; }
  {IDENTIFIER}       { return StrideTypes.IDENTIFIER; }

}

[^] { return TokenType.BAD_CHARACTER; }
