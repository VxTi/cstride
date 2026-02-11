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

        // Commit document to ensure PSI is up to date, although we are using Document mainly
        PsiDocumentManager.getInstance(project).commitDocument(document)

        val caretOffset = editor.caretModel.offset
        val lineNumber = document.getLineNumber(caretOffset)

        if (lineNumber <= 0) return Result.Continue

        // Get previous line
        val prevLineNumber = lineNumber - 1
        val prevLineStart = document.getLineStartOffset(prevLineNumber)
        val prevLineEnd = document.getLineEndOffset(prevLineNumber) // Excludes separator
        val prevLineText = document.getText(TextRange(prevLineStart, prevLineEnd))

        // Calculate base indentation
        val baseIndent = prevLineText.takeWhile { it == ' ' || it == '\t' }

        if (baseIndent.isNotEmpty()) {
            val currentLineStart = document.getLineStartOffset(lineNumber)
            // Insert indentation at the start of the current line
            document.insertString(currentLineStart, baseIndent)
            // Move caret to end of indentation
            editor.caretModel.moveToOffset(currentLineStart + baseIndent.length)
        }

        return Result.Continue
    }
}
