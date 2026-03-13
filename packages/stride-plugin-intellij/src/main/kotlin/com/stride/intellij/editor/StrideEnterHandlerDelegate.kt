package com.stride.intellij.editor

import com.intellij.codeInsight.editorActions.enter.EnterHandlerDelegate
import com.intellij.codeInsight.editorActions.enter.EnterHandlerDelegate.Result
import com.intellij.openapi.actionSystem.DataContext
import com.intellij.openapi.editor.Editor
import com.intellij.openapi.editor.actionSystem.EditorActionHandler
import com.intellij.openapi.util.Ref
import com.intellij.openapi.util.TextRange
import com.intellij.psi.PsiDocumentManager
import com.intellij.psi.PsiFile
import com.intellij.psi.codeStyle.CodeStyleSettingsManager
import com.stride.intellij.StrideLanguage
import com.stride.intellij.psi.StrideFile

class StrideEnterHandlerDelegate : EnterHandlerDelegate {
    override fun preprocessEnter(
        file: PsiFile,
        editor: Editor,
        caretOffset: Ref<Int>,
        caretAdvance: Ref<Int>,
        dataContext: DataContext,
        originalHandler: EditorActionHandler?
    ): Result {
        return Result.Continue
    }

    override fun postProcessEnter(
        file: PsiFile,
        editor: Editor,
        dataContext: DataContext
    ): Result {
        if (file !is StrideFile) {
            return Result.Continue
        }

        val project = editor.project ?: return Result.Continue
        val document = editor.document
        val indentSize = CodeStyleSettingsManager.getSettings(project)
            .getCommonSettings(StrideLanguage).indentOptions?.INDENT_SIZE ?: 4
        val indentString = " ".repeat(indentSize)

        // Commit document to ensure PSI is up to date
        PsiDocumentManager.getInstance(project).commitDocument(document)

        val caretOffset = editor.caretModel.offset
        val lineNumber = document.getLineNumber(caretOffset)

        if (lineNumber <= 0) return Result.Continue

        // Get previous line
        val prevLineNumber = lineNumber - 1
        val prevLineStart = document.getLineStartOffset(prevLineNumber)
        val prevLineEnd = document.getLineEndOffset(prevLineNumber)
        val prevLineText = document.getText(TextRange(prevLineStart, prevLineEnd))

        // Get current line
        val currentLineStart = document.getLineStartOffset(lineNumber)
        val currentLineEnd = document.getLineEndOffset(lineNumber)
        val currentLineText = document.getText(TextRange(currentLineStart, currentLineEnd))

        // Calculate base indentation from previous line
        val baseIndent = prevLineText.takeWhile { it == ' ' || it == '\t' }
        val prevLineTrimmed = prevLineText.trimEnd()

        // Check if previous line ends with opening brace
        val shouldIndent = prevLineTrimmed.endsWith("{")

        // Calculate target indentation
        val targetIndent = if (shouldIndent) {
            baseIndent + indentString
        } else {
            baseIndent
        }

        // Remove any existing whitespace at the start of the current line
        val currentWhitespace = currentLineText.takeWhile { it == ' ' || it == '\t' }
        if (currentWhitespace.isNotEmpty()) {
            document.deleteString(currentLineStart, currentLineStart + currentWhitespace.length)
        }

        // Insert correct indentation
        if (targetIndent.isNotEmpty()) {
            document.insertString(currentLineStart, targetIndent)
            editor.caretModel.moveToOffset(currentLineStart + targetIndent.length)
        }

        return Result.Continue
    }
}
