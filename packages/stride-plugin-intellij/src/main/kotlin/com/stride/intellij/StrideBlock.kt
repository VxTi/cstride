package com.stride.intellij

import com.intellij.formatting.*
import com.intellij.lang.ASTNode
import com.intellij.psi.TokenType
import com.intellij.psi.codeStyle.CodeStyleSettings
import com.intellij.psi.formatter.common.AbstractBlock
import com.stride.intellij.psi.StrideTypes

class StrideBlock(
    node: ASTNode,
    private val wrap: Wrap?,
    private val alignment: Alignment?,
    private val settings: CodeStyleSettings,
    private val spacingBuilder: SpacingBuilder,
    private val strideSettings: StrideCodeStyleSettings
) : AbstractBlock(node, wrap, alignment) {
    override fun buildChildren(): List<Block> {
        val blocks = mutableListOf<Block>()
        var child = myNode.firstChildNode
        while (child != null) {
            if (child.elementType != TokenType.WHITE_SPACE) {
                blocks.add(StrideBlock(child, null, null, settings, spacingBuilder, strideSettings))
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
            parentType == StrideTypes.OBJECT_DEFINITION_BODY -> {
                if (elementType == StrideTypes.OBJECT_DEFINITION_FIELDS) {
                    Indent.getNormalIndent()
                } else {
                    Indent.getNoneIndent()
                }
            }
            parentType == StrideTypes.OBJECT_DEFINITION_FIELDS -> {
                Indent.getNoneIndent()
            }
            // Object initialization: indent fields when wrapping
            parentType == StrideTypes.OBJECT_INITIALIZATION -> {
                if (elementType == StrideTypes.OBJECT_INIT_FIELDS && shouldWrap(parent)) {
                    Indent.getNormalIndent()
                } else {
                    Indent.getNoneIndent()
                }
            }
            parentType == StrideTypes.OBJECT_INIT_FIELDS -> {
                Indent.getNoneIndent()
            }
            // Function call: indent argument list when wrapping
            parentType == StrideTypes.FUNCTION_CALL_EXPRESSION -> {
                if (elementType == StrideTypes.ARGUMENT_LIST && shouldWrap(parent)) {
                    Indent.getNormalIndent()
                } else {
                    Indent.getNoneIndent()
                }
            }
            parentType == StrideTypes.ARGUMENT_LIST -> {
                Indent.getNoneIndent()
            }
            else -> Indent.getNoneIndent()
        }
    }

    override fun getSpacing(child1: Block?, child2: Block): Spacing? {
        if (child1 == null) return spacingBuilder.getSpacing(this, child1, child2)

        val child1Node = (child1 as? StrideBlock)?.node ?: return spacingBuilder.getSpacing(this, child1, child2)
        val child2Node = (child2 as? StrideBlock)?.node ?: return spacingBuilder.getSpacing(this, child1, child2)
        val parentType = myNode.elementType

        // Dynamic wrapping for object initialization
        if (parentType == StrideTypes.OBJECT_INITIALIZATION) {
            val wrap = shouldWrap(myNode)
            val child1Type = child1Node.elementType
            val child2Type = child2Node.elementType

            if (wrap) {
                // Multi-line: line breaks after { and before }
                if (child1Type == StrideTypes.LBRACE && child2Type == StrideTypes.OBJECT_INIT_FIELDS) {
                    return lineBreakSpacing()
                }
                if (child1Type == StrideTypes.OBJECT_INIT_FIELDS && child2Type == StrideTypes.RBRACE) {
                    return lineBreakSpacing()
                }
                // Empty init: no break between { and }
                if (child1Type == StrideTypes.LBRACE && child2Type == StrideTypes.RBRACE) {
                    return noSpacing()
                }
            } else {
                // Single-line: space after { and before }
                if (child1Type == StrideTypes.LBRACE && child2Type == StrideTypes.OBJECT_INIT_FIELDS) {
                    return singleSpacing()
                }
                if (child1Type == StrideTypes.OBJECT_INIT_FIELDS && child2Type == StrideTypes.RBRACE) {
                    return singleSpacing()
                }
            }
        }

        // Dynamic wrapping for commas inside object init fields
        if (parentType == StrideTypes.OBJECT_INIT_FIELDS) {
            val child1Type = child1Node.elementType

            // Find the OBJECT_INITIALIZATION ancestor
            val objInitNode = myNode.treeParent
            if (objInitNode != null && objInitNode.elementType == StrideTypes.OBJECT_INITIALIZATION) {
                val wrap = shouldWrap(objInitNode)
                if (wrap && child1Type == StrideTypes.COMMA) {
                    return lineBreakSpacing()
                }
                // Single-line comma spacing is handled by the SpacingBuilder
            }
        }

        // Dynamic wrapping for function calls
        if (parentType == StrideTypes.FUNCTION_CALL_EXPRESSION) {
            val wrap = shouldWrap(myNode)
            val child1Type = child1Node.elementType
            val child2Type = child2Node.elementType

            if (wrap) {
                // Multi-line: line breaks after ( and before )
                if (child1Type == StrideTypes.LPAREN && child2Type == StrideTypes.ARGUMENT_LIST) {
                    return lineBreakSpacing()
                }
                if (child1Type == StrideTypes.ARGUMENT_LIST && child2Type == StrideTypes.RPAREN) {
                    return lineBreakSpacing()
                }
            }
            // Single-line: default spacing from SpacingBuilder handles this
        }

        // Dynamic wrapping for commas inside argument lists
        if (parentType == StrideTypes.ARGUMENT_LIST) {
            val child1Type = child1Node.elementType

            val funcCallNode = myNode.treeParent
            if (funcCallNode != null && funcCallNode.elementType == StrideTypes.FUNCTION_CALL_EXPRESSION) {
                val wrap = shouldWrap(funcCallNode)
                if (wrap && child1Type == StrideTypes.COMMA) {
                    return lineBreakSpacing()
                }
            }
        }

        return spacingBuilder.getSpacing(this, child1, child2)
    }

    override fun isLeaf(): Boolean = myNode.firstChildNode == null

    private fun shouldWrap(node: ASTNode): Boolean {
        val maxWidth = strideSettings.MAX_LINE_WIDTH
        val textLen = computeSingleLineLength(node)
        return textLen > maxWidth
    }

    companion object {
        fun computeSingleLineLength(node: ASTNode): Int {
            // Compute what the text length would be if rendered on a single line
            // with standard spacing (one space after commas, spaces around braces, etc.)
            return node.text.replace(Regex("\\s+"), " ").trim().length
        }

        private fun lineBreakSpacing(): Spacing {
            return Spacing.createSpacing(0, 0, 1, true, 0)
        }

        private fun singleSpacing(): Spacing {
            return Spacing.createSpacing(1, 1, 0, false, 0)
        }

        private fun noSpacing(): Spacing {
            return Spacing.createSpacing(0, 0, 0, false, 0)
        }
    }
}
