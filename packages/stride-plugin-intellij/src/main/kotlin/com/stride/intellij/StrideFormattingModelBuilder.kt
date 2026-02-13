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
            .after(StrideTypes.COMMA).spaceIf(true)
            .before(StrideTypes.COMMA).spaceIf(false)
            .before(StrideTypes.SEMICOLON).spaceIf(false)

            .afterInside(StrideTypes.LBRACE, StrideTypes.STRUCT_DEFINITION).lineBreakInCode()
            .beforeInside(StrideTypes.RBRACE, StrideTypes.STRUCT_DEFINITION).lineBreakInCode()
            .afterInside(StrideTypes.LBRACE, StrideTypes.BLOCK_STATEMENT).lineBreakInCode()
            .beforeInside(StrideTypes.RBRACE, StrideTypes.BLOCK_STATEMENT).lineBreakInCode()
            .afterInside(StrideTypes.LBRACE, StrideTypes.MODULE_STATEMENT).lineBreakInCode()
            .beforeInside(StrideTypes.RBRACE, StrideTypes.MODULE_STATEMENT).lineBreakInCode()

            .around(StrideTypes.STRUCT_INIT_FIELD).spaceIf(true)
            .between(StrideTypes.LBRACE, StrideTypes.STRUCT_INIT_FIELDS).spaceIf(true)
            .between(StrideTypes.STRUCT_INIT_FIELDS, StrideTypes.RBRACE).spaceIf(true)

            .between(StrideTypes.STRUCT_DEFINITION, StrideTypes.STRUCT_DEFINITION).blankLines(1)
            .between(StrideTypes.FUNCTION_DECLARATION, StrideTypes.FUNCTION_DECLARATION).blankLines(1)
            .between(StrideTypes.STRUCT_DEFINITION, StrideTypes.FUNCTION_DECLARATION).blankLines(1)
            .between(StrideTypes.FUNCTION_DECLARATION, StrideTypes.STRUCT_DEFINITION).blankLines(1)
    }
}
