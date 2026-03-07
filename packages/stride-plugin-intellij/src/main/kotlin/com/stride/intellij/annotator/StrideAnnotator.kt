package com.stride.intellij.annotator

import com.intellij.lang.annotation.AnnotationHolder
import com.intellij.lang.annotation.Annotator
import com.intellij.lang.annotation.HighlightSeverity
import com.intellij.openapi.project.DumbAware
import com.intellij.psi.PsiElement
import com.intellij.psi.util.elementType
import com.stride.intellij.highlight.StrideSyntaxHighlighter
import com.stride.intellij.psi.*

class StrideAnnotator : Annotator, DumbAware {
    override fun annotate(element: PsiElement, holder: AnnotationHolder) {
        when (element) {
            is StrideFunctionCallExpression -> annotateFunctionCall(element, holder)
            is StrideStructInitialization -> annotateStructInitialization(element, holder)
            is StrideUserType -> annotateUserType(element, holder)
            is StrideFunctionDeclaration -> annotateFunctionDeclaration(element, holder)
            is StrideExternFunctionDeclaration -> annotateExternFunctionDeclaration(element, holder)
            is StridePostfixExpression -> annotatePostfixExpression(element, holder)
            is StrideGenericTypeArguments -> annotateGenericTypeArguments(element, holder)
            is StrideImportStatement -> annotateImportStatement(element, holder)
            is StridePackageStatement -> annotatePackageStatement(element, holder)
            is StrideModuleStatement -> annotateModuleStatement(element, holder)
        }
    }

    private fun annotateImportStatement(import: StrideImportStatement, holder: AnnotationHolder) {
        // Highlight all identifiers in import (e.g., import System::{ IO::Print })
        import.scopedIdentifierList.forEach { scopedId ->
            highlightIdentifiers(scopedId, holder, StrideSyntaxHighlighter.IMPORT_IDENTIFIER)
        }
    }

    private fun annotatePackageStatement(pkg: StridePackageStatement, holder: AnnotationHolder) {
        // Highlight all identifiers in package statement
        highlightIdentifiers(pkg.scopedIdentifier, holder, StrideSyntaxHighlighter.PACKAGE_NAME)
    }

    private fun annotateModuleStatement(module: StrideModuleStatement, holder: AnnotationHolder) {
        // Highlight all identifiers in module statement
        highlightIdentifiers(module.scopedIdentifier, holder, StrideSyntaxHighlighter.MODULE_NAME)
    }

    private fun annotateGenericTypeArguments(genericArgs: StrideGenericTypeArguments, holder: AnnotationHolder) {
        // Highlight each identifier in the generic type arguments (e.g., <T, U>)
        var child = genericArgs.node.firstChildNode
        while (child != null) {
            if (child.elementType == StrideTypes.IDENTIFIER) {
                holder.newSilentAnnotation(HighlightSeverity.INFORMATION)
                    .range(child.textRange)
                    .textAttributes(StrideSyntaxHighlighter.GENERIC_TYPE_PARAMETER)
                    .create()
            }
            child = child.treeNext
        }
    }

    private fun annotateFunctionCall(call: StrideFunctionCallExpression, holder: AnnotationHolder) {
        // Highlight each identifier in the scoped identifier (e.g., io::println)
        val scopedId = call.scopedIdentifier
        highlightIdentifiers(scopedId, holder, StrideSyntaxHighlighter.FUNCTION_CALL)
    }

    private fun annotateStructInitialization(init: StrideStructInitialization, holder: AnnotationHolder) {
        // Highlight the type name in struct initialization (e.g., Test::{ ... })
        val scopedId = init.scopedIdentifier
        highlightIdentifiers(scopedId, holder, StrideSyntaxHighlighter.STRUCT_TYPE)
    }

    private fun annotateUserType(userType: StrideUserType, holder: AnnotationHolder) {
        // Highlight type references (e.g., in variable declarations)
        val scopedId = userType.scopedIdentifier
        highlightIdentifiers(scopedId, holder, StrideSyntaxHighlighter.USER_TYPE)
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
        var child = func.node.firstChildNode
        var foundFn = false
        while (child != null) {
            if (child.elementType == StrideTypes.FN) {
                foundFn = true
            } else if (child.elementType == StrideTypes.IDENTIFIER) {
                if (foundFn) {
                    holder.newSilentAnnotation(HighlightSeverity.INFORMATION)
                        .range(child.textRange)
                        .textAttributes(StrideSyntaxHighlighter.FUNCTION_DECLARATION)
                        .create()
                    break
                }
            }
            child = child.treeNext
        }
    }

    private fun annotatePostfixExpression(postfix: StridePostfixExpression, holder: AnnotationHolder) {
        // Highlight field names in member access (e.g., obj.fieldName)
        var child = postfix.node.firstChildNode
        var foundDot = false
        while (child != null) {
            if (child.elementType == StrideTypes.DOT) {
                foundDot = true
            } else if (foundDot && child.elementType == StrideTypes.IDENTIFIER) {
                holder.newSilentAnnotation(HighlightSeverity.INFORMATION)
                    .range(child.textRange)
                    .textAttributes(StrideSyntaxHighlighter.FIELD_IDENTIFIER)
                    .create()
                foundDot = false
            }
            child = child.treeNext
        }
    }

    private fun highlightIdentifiers(
        scopedId: StrideScopedIdentifier,
        holder: AnnotationHolder,
        attributesKey: com.intellij.openapi.editor.colors.TextAttributesKey
    ) {
        // Highlight all IDENTIFIER tokens within the scoped identifier
        var child = scopedId.node.firstChildNode
        while (child != null) {
            if (child.elementType == StrideTypes.IDENTIFIER) {
                holder.newSilentAnnotation(HighlightSeverity.INFORMATION)
                    .range(child.textRange)
                    .textAttributes(attributesKey)
                    .create()
            }
            child = child.treeNext
        }
    }
}
