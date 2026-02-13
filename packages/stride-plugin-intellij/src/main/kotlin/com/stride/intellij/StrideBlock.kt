package com.stride.intellij

import com.intellij.formatting.*
import com.intellij.lang.ASTNode
import com.intellij.psi.TokenType
import com.intellij.psi.codeStyle.CodeStyleSettings
import com.intellij.psi.formatter.common.AbstractBlock
import com.intellij.psi.tree.TokenSet
import com.stride.intellij.psi.StrideTypes

class StrideBlock(
    node: ASTNode,
    private val wrap: Wrap?,
    private val alignment: Alignment?,
    private val settings: CodeStyleSettings,
    private val spacingBuilder: SpacingBuilder
) : AbstractBlock(node, wrap, alignment) {
    override fun buildChildren(): List<Block> {
        val blocks = mutableListOf<Block>()
        var child = myNode.firstChildNode
        while (child != null) {
            if (child.elementType != TokenType.WHITE_SPACE) {
                blocks.add(StrideBlock(child, null, null, settings, spacingBuilder))
            }
            child = child.treeNext
        }
        return blocks
    }

    override fun getIndent(): Indent? {
        val parent = myNode.treeParent ?: return Indent.getNoneIndent()
        val elementType = myNode.elementType
        val parentType = parent.elementType

        return when {
            parentType == StrideTypes.BLOCK_STATEMENT ||
            parentType == StrideTypes.MODULE_STATEMENT -> {
                if (elementType != StrideTypes.LBRACE && elementType != StrideTypes.RBRACE) {
                    Indent.getNormalIndent()
                } else {
                    Indent.getNoneIndent()
                }
            }
            parentType == StrideTypes.STRUCT_DEFINITION && elementType == StrideTypes.STRUCT_DEFINITION_FIELDS -> Indent.getNormalIndent()
            parentType == StrideTypes.STRUCT_DEFINITION_FIELDS -> Indent.getNoneIndent()
            parentType == StrideTypes.STRUCT_INITIALIZATION && elementType == StrideTypes.STRUCT_INIT_FIELDS -> Indent.getNormalIndent()
            parentType == StrideTypes.STRUCT_INIT_FIELDS -> Indent.getNoneIndent()
            else -> Indent.getNoneIndent()
        }
    }

    override fun getSpacing(child1: Block?, child2: Block): Spacing? {
        return spacingBuilder.getSpacing(this, child1, child2)
    }

    override fun isLeaf(): Boolean = myNode.firstChildNode == null
}
