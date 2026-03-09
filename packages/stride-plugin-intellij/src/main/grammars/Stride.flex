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
IDENTIFIER=[a-zA-Z_$][a-zA-Z0-9_$]*
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

  "pub"              { return StrideTypes.PUBLIC; }
  "package"          { return StrideTypes.PACKAGE; }
  "import"           { return StrideTypes.IMPORT; }
  "module"           { return StrideTypes.MODULE; }
  "async"            { return StrideTypes.ASYNC; }
  "fn"               { return StrideTypes.FN; }
  "nil"              { return StrideTypes.NIL; }
  "break"            { return StrideTypes.BREAK; }
  "continue"         { return StrideTypes.CONTINUE; }
  "struct"           { return StrideTypes.STRUCT; }
  "const"            { return StrideTypes.CONST; }
  "type"             { return StrideTypes.KW_TYPE; }
  "let"              { return StrideTypes.LET; }
  "extern"           { return StrideTypes.EXTERN; }
  "as"               { return StrideTypes.AS; }
  "return"           { return StrideTypes.RETURN; }
  "for"              { return StrideTypes.FOR; }
  "while"            { return StrideTypes.WHILE; }
  "if"               { return StrideTypes.IF; }
  "else"             { return StrideTypes.ELSE; }
  "void"             { return StrideTypes.VOID; }
  "i8"             { return StrideTypes.INT8; }
  "i16"            { return StrideTypes.INT16; }
  "i32"            { return StrideTypes.INT32; }
  "i64"            { return StrideTypes.INT64; }
  "u8"            { return StrideTypes.UINT8; }
  "u16"           { return StrideTypes.UINT16; }
  "u32"           { return StrideTypes.UINT32; }
  "u64"           { return StrideTypes.UINT64; }
  "f32"          { return StrideTypes.f32; }
  "f64"          { return StrideTypes.f64; }
  "bool"             { return StrideTypes.BOOL; }
  "char"             { return StrideTypes.CHAR; }
  "string"           { return StrideTypes.STRING; }
  "true"             { return StrideTypes.BOOLEAN_LITERAL; }
  "false"            { return StrideTypes.BOOLEAN_LITERAL; }

  ";"                { return StrideTypes.SEMICOLON; }
  ":"                { return StrideTypes.COLON; }
  ","                { return StrideTypes.COMMA; }
  "..."              { return StrideTypes.ELLIPSIS; }
  "."                { return StrideTypes.DOT; }
  "=="               { return StrideTypes.EQ_EQ; }
  "="                { return StrideTypes.EQ; }
  "++"               { return StrideTypes.PLUS_PLUS; }
  "--"               { return StrideTypes.MINUS_MINUS; }
  "*="               { return StrideTypes.ASTERISK_EQ; }
  "*"                { return StrideTypes.ASTERISK; }
  "/="               { return StrideTypes.SLASH_EQ; }
  "/"                { return StrideTypes.DIV; }
  "%="               { return StrideTypes.MOD_EQ; }
  "%"                { return StrideTypes.MOD; }
  "?"                { return StrideTypes.QUESTION; }
  "+="               { return StrideTypes.PLUS_EQ; }
  "|"                { return StrideTypes.PIPE; }
  "|="               { return StrideTypes.PIPE_EQ; }
  "^="               { return StrideTypes.CARET_EQ; }
  "^"                { return StrideTypes.CARET; }
  "+"                { return StrideTypes.PLUS; }
  "->"               { return StrideTypes.ARROW; }
  "-="               { return StrideTypes.MINUS_EQ; }
  "-"                { return StrideTypes.MINUS; }
  "&="               { return StrideTypes.AMPERSAND_EQ; }
  "&"                { return StrideTypes.AMPERSAND; }
  "!="               { return StrideTypes.EXCL_EQ; }
  "!"                { return StrideTypes.EXCL; }
  "~="               { return StrideTypes.TILDE_EQ; }
  "~"                { return StrideTypes.TILDE; }
  "<<="              { return StrideTypes.LEFTSHIFT_EQ; }
  "<"                { return StrideTypes.LT; }
  ">>="              { return StrideTypes.RIGHTSHIFT_EQ; }
  ">"                { return StrideTypes.GT; }
  "["                { return StrideTypes.LSQUARE_BRACKET; }
  "]"                { return StrideTypes.RSQUARE_BRACKET; }
  "{"                { return StrideTypes.LBRACE; }
  "}"                { return StrideTypes.RBRACE; }
  "("                { return StrideTypes.LPAREN; }
  ")"                { return StrideTypes.RPAREN; }
  "::"               { return StrideTypes.COLON_COLON; }
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
