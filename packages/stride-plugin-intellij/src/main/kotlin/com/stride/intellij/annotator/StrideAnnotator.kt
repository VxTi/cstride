package com.stride.intellij.annotator

import com.intellij.lang.annotation.AnnotationHolder
import com.intellij.lang.annotation.Annotator
import com.intellij.lang.annotation.HighlightSeverity
import com.intellij.psi.PsiElement
import com.stride.intellij.highlight.StrideSyntaxHighlighter
import com.stride.intellij.psi.StridePostfixExpression
import com.stride.intellij.psi.StrideTypes

class StrideAnnotator : Annotator {
    override fun annotate(element: PsiElement, holder: AnnotationHolder) {
        if (element is StridePostfixExpression) {
            annotatePostfixExpression(element, holder)
        }
    }

    private fun annotatePostfixExpression(paramExpression: StridePostfixExpression, holder: AnnotationHolder) {
        var callTarget: PsiElement? = null

        // Check primary expression first
        val primary = paramExpression.primaryExpression
        val primaryScoped = primary.scopedIdentifier
        if (primaryScoped != null) {
            // Identifier list. Last one is the candidate.
            // ScopedIdentifier ::= IDENTIFIER (COLON_COLON IDENTIFIER)*
            // We can just find the last IDENTIFIER child.
            // PsiElement children include tokens.
            callTarget = primaryScoped.children.findLast { it.node.elementType == StrideTypes.IDENTIFIER }
        }

        // Now iterate children of postfix expression to process suffixes in order
        var passedPrimary = false
        val children = paramExpression.children

        // If for some reason primary is not found in children (should not happen), we handle it
        if (children.isEmpty()) return

        for (child in children) {
            if (!passedPrimary) {
                if (child == primary) {
                    passedPrimary = true
                }
                continue
            }

            val type = child.node.elementType

            if (type == StrideTypes.IDENTIFIER) {
                // This is a member access .bar
                callTarget = child
            } else if (type == StrideTypes.LPAREN) {
                // This is a call!
                if (callTarget != null) {
                    holder.newSilentAnnotation(HighlightSeverity.INFORMATION)
                        .range(callTarget)
                        .textAttributes(StrideSyntaxHighlighter.FUNCTION_CALL)
                        .create()
                    // After call, the result is likely not a function reference we can highlight simply
                    callTarget = null
                }
            } else if (type == StrideTypes.LSQUARE_BRACKET) {
                // Indexing, resets target
                callTarget = null
            }
        }
    }
}
