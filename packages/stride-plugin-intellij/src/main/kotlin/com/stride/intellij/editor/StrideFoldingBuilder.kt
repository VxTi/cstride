package com.stride.intellij.editor

import com.intellij.lang.ASTNode
import com.intellij.lang.folding.FoldingBuilderEx
import com.intellij.lang.folding.FoldingDescriptor
import com.intellij.openapi.editor.Document
import com.intellij.openapi.util.TextRange
import com.intellij.psi.PsiElement
import com.intellij.psi.util.PsiTreeUtil
import com.stride.intellij.psi.StrideArrayDeclarationExpression
import com.stride.intellij.psi.StrideBlockStatement
import com.stride.intellij.psi.StrideObjectDefinitionBody
import com.stride.intellij.psi.StrideEnumBody
import com.stride.intellij.psi.StrideFileHeader
import com.stride.intellij.psi.StrideTypes

class StrideFoldingBuilder : FoldingBuilderEx() {

    override fun buildFoldRegions(root: PsiElement, document: Document, quick: Boolean): Array<FoldingDescriptor> {
        val descriptors = mutableListOf<FoldingDescriptor>()

        PsiTreeUtil.processElements(root) { element ->
            when (element) {
                is StrideFileHeader -> {
                    if (element.textLength > 0) {
                        descriptors.add(FoldingDescriptor(element.node, element.textRange))
                    }
                }
                is StrideBlockStatement -> {
                    if (element.textLength > 2) {
                        descriptors.add(FoldingDescriptor(element.node, element.textRange))
                    }
                }
                is StrideEnumBody -> {
                    if (element.textLength > 2) {
                        descriptors.add(FoldingDescriptor(element.node, element.textRange))
                    }
                }
                is StrideObjectDefinitionBody -> {
                    if (element.textLength > 2) {
                        descriptors.add(FoldingDescriptor(element.node, element.textRange))
                    }
                }
                is StrideArrayDeclarationExpression -> {
                    if (element.textLength > 2) {
                        descriptors.add(FoldingDescriptor(element.node, element.textRange))
                    }
                }
            }
            true
        }

        return descriptors.toTypedArray()
    }

    override fun getPlaceholderText(node: ASTNode): String {
        return when {
            node.psi is StrideFileHeader -> "..."
            node.psi is StrideBlockStatement -> "{...}"
            node.psi is StrideObjectDefinitionBody -> "{...}"
            node.psi is StrideEnumBody -> "{...}"
            node.psi is StrideArrayDeclarationExpression -> "[...]"
            else -> "..."
        }
    }

    override fun isCollapsedByDefault(node: ASTNode): Boolean {
        return when {
          node.psi is StrideFileHeader -> true
          else -> false
         }
    }
}
