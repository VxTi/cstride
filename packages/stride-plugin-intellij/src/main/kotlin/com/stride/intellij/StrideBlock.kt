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
            // Block statements (function bodies, control flow blocks)
            parentType == StrideTypes.BLOCK_STATEMENT -> {
                if (elementType != StrideTypes.LBRACE && elementType != StrideTypes.RBRACE) {
                    Indent.getNormalIndent()
                } else {
                    Indent.getNoneIndent()
                }
            }
            parentType == StrideTypes.MODULE_STATEMENT -> {
                if (elementType != StrideTypes.LBRACE &&
                    elementType != StrideTypes.RBRACE &&
                    elementType != StrideTypes.MODULE &&
                    elementType != StrideTypes.IDENTIFIER &&
                    elementType != StrideTypes.SCOPED_IDENTIFIER) {
                    Indent.getNormalIndent()
                } else {
                    Indent.getNoneIndent()
                }
            }
            // Struct definition fields (within OBJECT_DEFINITION_BODY)
            parentType == StrideTypes.OBJECT_DEFINITION_BODY -> {
                if (elementType == StrideTypes.OBJECT_DEFINITION_FIELDS) {
                    Indent.getNormalIndent()
                } else {
                    Indent.getNoneIndent()
                }
            }
            // Individual struct fields
            parentType == StrideTypes.OBJECT_DEFINITION_FIELDS -> {
                Indent.getNoneIndent()
            }
            // Struct initialization fields
            parentType == StrideTypes.OBJECT_INITIALIZATION -> {
                if (elementType == StrideTypes.OBJECT_INIT_FIELDS) {
                    Indent.getNormalIndent()
                } else {
                    Indent.getNoneIndent()
                }
            }
            parentType == StrideTypes.OBJECT_INIT_FIELDS -> {
                Indent.getNoneIndent()
            }
            else -> Indent.getNoneIndent()
        }
    }

    override fun getSpacing(child1: Block?, child2: Block): Spacing? {
        return spacingBuilder.getSpacing(this, child1, child2)
    }

    override fun isLeaf(): Boolean = myNode.firstChildNode == null
}
