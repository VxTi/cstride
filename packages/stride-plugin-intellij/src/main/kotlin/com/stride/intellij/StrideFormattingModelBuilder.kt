package com.stride.intellij

import com.intellij.formatting.*
import com.intellij.psi.codeStyle.CodeStyleSettings
import com.intellij.psi.PsiElement
import com.intellij.psi.PsiFile
import com.stride.intellij.psi.StrideTypes

class StrideFormattingModelBuilder : FormattingModelBuilder {
    override fun createModel(element: PsiElement, settings: CodeStyleSettings): FormattingModel {
        val spacingBuilder = createSpaceBuilder(settings)
        return FormattingModelProvider.createFormattingModelForPsiFile(
            element.containingFile,
            StrideBlock(element.node, null, null, settings, spacingBuilder),
            settings
        )
    }

    private fun createSpaceBuilder(settings: CodeStyleSettings): SpacingBuilder {
        return SpacingBuilder(settings, StrideLanguage)
            // Generic type arguments/values: <T>, <i32, f64>
            .afterInside(StrideTypes.LT, StrideTypes.GENERIC_TYPE_ARGUMENTS).spaceIf(false)
            .beforeInside(StrideTypes.GT, StrideTypes.GENERIC_TYPE_ARGUMENTS).spaceIf(false)
            .afterInside(StrideTypes.LT, StrideTypes.GENERIC_TYPE_VALUE_ARGUMENTS).spaceIf(false)
            .beforeInside(StrideTypes.GT, StrideTypes.GENERIC_TYPE_VALUE_ARGUMENTS).spaceIf(false)
            .afterInside(StrideTypes.LT, StrideTypes.FUNCTION_CALL_EXPRESSION).spaceIf(false)
            .beforeInside(StrideTypes.GT, StrideTypes.FUNCTION_CALL_EXPRESSION).spaceIf(false)
            .afterInside(StrideTypes.LT, StrideTypes.FUNCTION_DECLARATION).spaceIf(false)
            .beforeInside(StrideTypes.GT, StrideTypes.FUNCTION_DECLARATION).spaceIf(false)
            .afterInside(StrideTypes.LT, StrideTypes.EXTERN_FUNCTION_DECLARATION).spaceIf(false)
            .beforeInside(StrideTypes.GT, StrideTypes.EXTERN_FUNCTION_DECLARATION).spaceIf(false)
            .afterInside(StrideTypes.LT, StrideTypes.TYPE_DEFINITION).spaceIf(false)
            .beforeInside(StrideTypes.GT, StrideTypes.TYPE_DEFINITION).spaceIf(false)
            .beforeInside(StrideTypes.COMMA, StrideTypes.FUNCTION_CALL_EXPRESSION).spaceIf(false)
            .beforeInside(StrideTypes.COMMA, StrideTypes.FUNCTION_DECLARATION).spaceIf(false)
            .beforeInside(StrideTypes.COMMA, StrideTypes.EXTERN_FUNCTION_DECLARATION).spaceIf(false)
            .beforeInside(StrideTypes.COMMA, StrideTypes.TYPE_DEFINITION).spaceIf(false)
            .beforeInside(StrideTypes.COMMA, StrideTypes.GENERIC_TYPE_ARGUMENTS).spaceIf(false)
            .beforeInside(StrideTypes.COMMA, StrideTypes.GENERIC_TYPE_VALUE_ARGUMENTS).spaceIf(false)
            .beforeInside(StrideTypes.LT, StrideTypes.FUNCTION_CALL_EXPRESSION).spaceIf(false)
            .afterInside(StrideTypes.GT, StrideTypes.FUNCTION_CALL_EXPRESSION).spaceIf(false)
            .beforeInside(StrideTypes.LPAREN, StrideTypes.FUNCTION_CALL_EXPRESSION).spaceIf(false)

            // Tuple types: (i32, i32)
            .afterInside(StrideTypes.LPAREN, StrideTypes.TUPLE_TYPE).spaceIf(false)
            .beforeInside(StrideTypes.RPAREN, StrideTypes.TUPLE_TYPE).spaceIf(false)
            .beforeInside(StrideTypes.COMMA, StrideTypes.TUPLE_TYPE).spaceIf(false)

            .around(StrideTypes.EQ).spaceIf(true)
            .around(StrideTypes.PLUS_EQ).spaceIf(true)
            .around(StrideTypes.MINUS_EQ).spaceIf(true)
            .around(StrideTypes.ASTERISK_EQ).spaceIf(true)
            .around(StrideTypes.SLASH_EQ).spaceIf(true)
            .around(StrideTypes.MOD_EQ).spaceIf(true)

            .around(StrideTypes.EQ_EQ).spaceIf(true)
            .around(StrideTypes.EXCL_EQ).spaceIf(true)
            .around(StrideTypes.LT).spaceIf(true)
            .around(StrideTypes.GT).spaceIf(true)
            .around(StrideTypes.LT_EQ).spaceIf(true)
            .around(StrideTypes.GT_EQ).spaceIf(true)
            .around(StrideTypes.PLUS).spaceIf(true)
            .around(StrideTypes.MINUS).spaceIf(true)
            .around(StrideTypes.ASTERISK).spaceIf(true)
            .around(StrideTypes.DIV).spaceIf(true)
            .around(StrideTypes.MOD).spaceIf(true)
            .around(StrideTypes.OPERATOR).spaceIf(true)

            .after(StrideTypes.IF).spaceIf(true)
            .after(StrideTypes.WHILE).spaceIf(true)
            .after(StrideTypes.FOR).spaceIf(true)
            .after(StrideTypes.RETURN).spaceIf(true)
            .around(StrideTypes.AS).spaceIf(true)

            .after(StrideTypes.COLON).spaceIf(true)
            .before(StrideTypes.COLON).spaceIf(false)
            // Specific comma rules must come before the general .after(COMMA) rule
            // because SpacingBuilder uses first-match semantics
            .afterInside(StrideTypes.COMMA, StrideTypes.OBJECT_INIT_FIELDS).lineBreakInCode()

            .after(StrideTypes.COMMA).spaceIf(true)
            .before(StrideTypes.COMMA).spaceIf(false)
            .before(StrideTypes.SEMICOLON).spaceIf(false)

            // Array notation: T[]
            .betweenInside(StrideTypes.LSQUARE_BRACKET, StrideTypes.RSQUARE_BRACKET, StrideTypes.TYPE).spaceIf(false)

            .afterInside(StrideTypes.LBRACE, StrideTypes.OBJECT_DEFINITION_BODY).lineBreakInCode()
            .beforeInside(StrideTypes.RBRACE, StrideTypes.OBJECT_DEFINITION_BODY).lineBreakInCode()
            .afterInside(StrideTypes.LBRACE, StrideTypes.BLOCK_STATEMENT).lineBreakInCode()
            .beforeInside(StrideTypes.RBRACE, StrideTypes.BLOCK_STATEMENT).lineBreakInCode()
            .afterInside(StrideTypes.LBRACE, StrideTypes.MODULE_STATEMENT).lineBreakInCode()
            .beforeInside(StrideTypes.RBRACE, StrideTypes.MODULE_STATEMENT).lineBreakInCode()
            .afterInside(StrideTypes.LBRACE, StrideTypes.OBJECT_INITIALIZATION).lineBreakInCode()
            .beforeInside(StrideTypes.RBRACE, StrideTypes.OBJECT_INITIALIZATION).lineBreakInCode()

            .afterInside(StrideTypes.SCOPED_IDENTIFIER, StrideTypes.MODULE_STATEMENT).spaceIf(true)
            .afterInside(StrideTypes.COLON_COLON, StrideTypes.OBJECT_INITIALIZATION).spaceIf(false)
            .afterInside(StrideTypes.LPAREN, StrideTypes.IF).spaceIf(false)
            .afterInside(StrideTypes.LPAREN, StrideTypes.WHILE).spaceIf(false)
            .afterInside(StrideTypes.LPAREN, StrideTypes.FOR).spaceIf(false)
            .beforeInside(StrideTypes.LBRACE, StrideTypes.MODULE_STATEMENT).spaceIf(true)
            .beforeInside(StrideTypes.LBRACE, StrideTypes.BLOCK_STATEMENT).spaceIf(true)
            .beforeInside(StrideTypes.LBRACE, StrideTypes.OBJECT_DEFINITION_BODY).spaceIf(true)
            .beforeInside(StrideTypes.LBRACE, StrideTypes.OBJECT_INITIALIZATION).spaceIf(false)

            // Module items should be on separate lines
            .afterInside(StrideTypes.MODULE, StrideTypes.MODULE_STATEMENT).spaceIf(true)
            .afterInside(StrideTypes.IDENTIFIER, StrideTypes.MODULE_STATEMENT).spaceIf(true)
            .afterInside(StrideTypes.MODULE_STATEMENT, StrideTypes.MODULE_STATEMENT).lineBreakInCode()
            .afterInside(StrideTypes.FUNCTION_DECLARATION, StrideTypes.MODULE_STATEMENT).lineBreakInCode()
            .afterInside(StrideTypes.TYPE_DEFINITION, StrideTypes.MODULE_STATEMENT).lineBreakInCode()
            .afterInside(StrideTypes.VARIABLE_DECLARATION_STATEMENT, StrideTypes.MODULE_STATEMENT).lineBreakInCode()
            .afterInside(StrideTypes.EXTERN_FUNCTION_DECLARATION, StrideTypes.MODULE_STATEMENT).lineBreakInCode()

            // Statements should be on separate lines - use STATEMENT wrapper
            .afterInside(StrideTypes.STATEMENT, StrideTypes.BLOCK_STATEMENT).lineBreakInCode()

            .between(StrideTypes.OBJECT_FIELD, StrideTypes.OBJECT_FIELD).lineBreakInCode()

            .between(StrideTypes.TYPE_DEFINITION, StrideTypes.TYPE_DEFINITION).blankLines(1)
            .between(StrideTypes.FUNCTION_DECLARATION, StrideTypes.FUNCTION_DECLARATION).blankLines(1)
            .between(StrideTypes.TYPE_DEFINITION, StrideTypes.FUNCTION_DECLARATION).blankLines(1)
            .between(StrideTypes.FUNCTION_DECLARATION, StrideTypes.TYPE_DEFINITION).blankLines(1)
    }
}
