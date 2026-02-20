package com.stride.intellij.annotator

import com.intellij.lang.annotation.AnnotationHolder
import com.intellij.lang.annotation.Annotator
import com.intellij.lang.annotation.HighlightSeverity
import com.intellij.psi.PsiElement
import com.intellij.psi.util.elementType
import com.stride.intellij.highlight.StrideSyntaxHighlighter
import com.stride.intellij.psi.*

class StrideAnnotator : Annotator {
    override fun annotate(element: PsiElement, holder: AnnotationHolder) {
        when (element) {
            is StrideFunctionCallExpression -> annotateFunctionCall(element, holder)
            is StrideStructInitialization -> annotateStructInitialization(element, holder)
            is StrideUserType -> annotateUserType(element, holder)
            is StrideStructInitField -> annotateStructInitField(element, holder)
            is StrideFunctionDeclaration -> annotateFunctionDeclaration(element, holder)
            is StrideExternFunctionDeclaration -> annotateExternFunctionDeclaration(element, holder)
            is StridePostfixExpression -> annotatePostfixExpression(element, holder)
            is StrideScopedIdentifier -> annotateScopedIdentifier(element, holder)
        }
    }

    private fun annotateFunctionCall(call: StrideFunctionCallExpression, holder: AnnotationHolder) {
        // Highlight each identifier in the scoped identifier (e.g., io::println)
        val scopedId = call.scopedIdentifier
        highlightIdentifiers(scopedId, holder, StrideSyntaxHighlighter.FUNCTION_CALL)
    }

    private fun annotateStructInitialization(init: StrideStructInitialization, holder: AnnotationHolder) {
        // Highlight the type name in struct initialization (e.g., Test::{ ... })
        val scopedId = init.scopedIdentifier ?: return
        highlightIdentifiers(scopedId, holder, StrideSyntaxHighlighter.SCOPED_IDENTIFIER_REFERENCE)
    }

    private fun annotateUserType(userType: StrideUserType, holder: AnnotationHolder) {
        // Highlight type references (e.g., in variable declarations)
        val scopedId = userType.scopedIdentifier ?: return
        highlightIdentifiers(scopedId, holder, StrideSyntaxHighlighter.SCOPED_IDENTIFIER_REFERENCE)
    }

    private fun annotateStructInitField(field: StrideStructInitField, holder: AnnotationHolder) {
        // Highlight field names in struct initialization
        val scopedId = field.scopedIdentifier ?: return
        highlightIdentifiers(scopedId, holder, StrideSyntaxHighlighter.IDENTIFIER)
    }

    private fun annotateFunctionDeclaration(func: StrideFunctionDeclaration, holder: AnnotationHolder) {
        // Highlight the function name in function declarations (e.g., fn myFunction(...))
        highlightFunctionName(func, holder)
    }

    private fun annotateExternFunctionDeclaration(func: StrideExternFunctionDeclaration, holder: AnnotationHolder) {
        // Highlight the function name in extern function declarations
        highlightFunctionName(func, holder)
    }

    private fun highlightFunctionName(func: PsiElement, holder: AnnotationHolder) {
        // Find the IDENTIFIER token that comes after FN keyword
        var foundFn = false
        for (child in func.node.getChildren(null)) {
            if (child.elementType == StrideTypes.FN) {
                foundFn = true
            } else if (foundFn && child.elementType == StrideTypes.IDENTIFIER) {
                holder.newSilentAnnotation(HighlightSeverity.INFORMATION)
                    .range(child.textRange)
                    .textAttributes(StrideSyntaxHighlighter.FUNCTION_DECLARATION)
                    .create()
                break
            }
        }
    }

    private fun annotatePostfixExpression(postfix: StridePostfixExpression, holder: AnnotationHolder) {
        // Highlight field names in member access (e.g., obj.fieldName)
        var foundDot = false
        for (child in postfix.node.getChildren(null)) {
            if (child.elementType == StrideTypes.DOT) {
                foundDot = true
            } else if (foundDot && child.elementType == StrideTypes.IDENTIFIER) {
                holder.newSilentAnnotation(HighlightSeverity.INFORMATION)
                    .range(child.textRange)
                    .textAttributes(StrideSyntaxHighlighter.FIELD_IDENTIFIER)
                    .create()
                foundDot = false
            }
        }
    }

    private fun annotateScopedIdentifier(scopedId: StrideScopedIdentifier, holder: AnnotationHolder) {
        // Only highlight standalone scoped identifiers (variable references, etc.)
        // Check if parent is already being handled
        val parent = scopedId.parent
        when (parent) {
            is StrideFunctionCallExpression,
            is StrideStructInitialization,
            is StrideUserType,
            is StrideStructInitField -> return // Already handled by specific annotators
            else -> {
                // Highlight as variable/identifier reference
                highlightIdentifiers(scopedId, holder, StrideSyntaxHighlighter.IDENTIFIER)
            }
        }
    }

    private fun highlightIdentifiers(
        scopedId: StrideScopedIdentifier,
        holder: AnnotationHolder,
        attributesKey: com.intellij.openapi.editor.colors.TextAttributesKey
    ) {
        // Highlight all IDENTIFIER tokens within the scoped identifier
        scopedId.node.getChildren(null).forEach { child ->
            if (child.elementType == StrideTypes.IDENTIFIER) {
                holder.newSilentAnnotation(HighlightSeverity.INFORMATION)
                    .range(child.textRange)
                    .textAttributes(attributesKey)
                    .create()
            }
        }
    }
}
