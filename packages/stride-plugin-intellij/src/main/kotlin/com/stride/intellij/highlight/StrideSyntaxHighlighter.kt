package com.stride.intellij.highlight

import com.intellij.lexer.Lexer
import com.intellij.openapi.editor.DefaultLanguageHighlighterColors
import com.intellij.openapi.editor.colors.TextAttributesKey
import com.intellij.openapi.editor.colors.TextAttributesKey.createTextAttributesKey
import com.intellij.openapi.fileTypes.SyntaxHighlighterBase
import com.intellij.psi.tree.IElementType
import com.stride.intellij.lexer.StrideLexerAdapter
import com.stride.intellij.psi.StrideTypes

class StrideSyntaxHighlighter : SyntaxHighlighterBase() {
    companion object {
        val KEYWORD = createTextAttributesKey("STRIDE_KEYWORD", DefaultLanguageHighlighterColors.KEYWORD)
        val NUMBER = createTextAttributesKey("STRIDE_NUMBER", DefaultLanguageHighlighterColors.NUMBER)
        val STRING = createTextAttributesKey("STRIDE_STRING", DefaultLanguageHighlighterColors.STRING)
        val COMMENT = createTextAttributesKey("STRIDE_COMMENT", DefaultLanguageHighlighterColors.LINE_COMMENT)
        val OPERATOR = createTextAttributesKey("STRIDE_OPERATOR", DefaultLanguageHighlighterColors.OPERATION_SIGN)
        val PARENTHESES = createTextAttributesKey("STRIDE_PARENTHESES", DefaultLanguageHighlighterColors.PARENTHESES)
        val BRACES = createTextAttributesKey("STRIDE_BRACES", DefaultLanguageHighlighterColors.BRACES)
        val SEMICOLON = createTextAttributesKey("STRIDE_SEMICOLON", DefaultLanguageHighlighterColors.SEMICOLON)
        val COMMA = createTextAttributesKey("STRIDE_COMMA", DefaultLanguageHighlighterColors.COMMA)
        val DOT = createTextAttributesKey("STRIDE_DOT", DefaultLanguageHighlighterColors.DOT)
        val TYPE = createTextAttributesKey("STRIDE_TYPE", DefaultLanguageHighlighterColors.METADATA)

        private val KEYWORD_KEYS = arrayOf(KEYWORD)
        private val NUMBER_KEYS = arrayOf(NUMBER)
        private val STRING_KEYS = arrayOf(STRING)
        private val COMMENT_KEYS = arrayOf(COMMENT)
        private val OPERATOR_KEYS = arrayOf(OPERATOR)
        private val PARENTHESES_KEYS = arrayOf(PARENTHESES)
        private val BRACES_KEYS = arrayOf(BRACES)
        private val SEMICOLON_KEYS = arrayOf(SEMICOLON)
        private val COMMA_KEYS = arrayOf(COMMA)
        private val DOT_KEYS = arrayOf(DOT)
        private val TYPE_KEYS = arrayOf(TYPE)
        private val EMPTY_KEYS = emptyArray<TextAttributesKey>()
    }

    override fun getHighlightingLexer(): Lexer = StrideLexerAdapter()

    override fun getTokenHighlights(tokenType: IElementType): Array<TextAttributesKey> {
        return when (tokenType) {
            StrideTypes.MODULE,
            StrideTypes.FN, StrideTypes.STRUCT, StrideTypes.CONST,
            StrideTypes.LET, StrideTypes.EXTERN, StrideTypes.AS,
            StrideTypes.RETURN, StrideTypes.FOR, StrideTypes.WHILE, StrideTypes.IF,
            StrideTypes.ELSE -> KEYWORD_KEYS

            StrideTypes.VOID, StrideTypes.I8, StrideTypes.I16,
            StrideTypes.I32, StrideTypes.I64, StrideTypes.U8,
            StrideTypes.U16, StrideTypes.U32, StrideTypes.U64,
            StrideTypes.F32, StrideTypes.F64, StrideTypes.BOOL,
            StrideTypes.CHAR, StrideTypes.STRING -> TYPE_KEYS

            StrideTypes.NUMBER_LITERAL -> NUMBER_KEYS
            StrideTypes.STRING_LITERAL -> STRING_KEYS
            StrideTypes.BOOLEAN_LITERAL -> KEYWORD_KEYS
            StrideTypes.COMMENT -> COMMENT_KEYS
            StrideTypes.EQ, StrideTypes.PLUS, StrideTypes.MINUS, StrideTypes.MUL,
            StrideTypes.DIV, StrideTypes.MOD, StrideTypes.EXCL, StrideTypes.LT,
            StrideTypes.GT, StrideTypes.EQ_EQ, StrideTypes.EXCL_EQ, StrideTypes.LT_EQ,
            StrideTypes.GT_EQ, StrideTypes.OPERATOR -> OPERATOR_KEYS
            StrideTypes.LPAREN, StrideTypes.RPAREN -> PARENTHESES_KEYS
            StrideTypes.LBRACE, StrideTypes.RBRACE -> BRACES_KEYS
            StrideTypes.SEMICOLON -> SEMICOLON_KEYS
            StrideTypes.COMMA -> COMMA_KEYS
            StrideTypes.DOT, StrideTypes.COLON_COLON -> DOT_KEYS
            else -> EMPTY_KEYS
        }
    }
}
