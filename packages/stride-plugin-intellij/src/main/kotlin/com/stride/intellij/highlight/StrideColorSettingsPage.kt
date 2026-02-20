package com.stride.intellij.highlight

import com.intellij.openapi.editor.colors.TextAttributesKey
import com.intellij.openapi.fileTypes.SyntaxHighlighter
import com.intellij.openapi.options.colors.AttributesDescriptor
import com.intellij.openapi.options.colors.ColorDescriptor
import com.intellij.openapi.options.colors.ColorSettingsPage
import com.stride.intellij.StrideIcons
import javax.swing.Icon

class StrideColorSettingsPage : ColorSettingsPage {
    override fun getAttributeDescriptors(): Array<AttributesDescriptor> = DESCRIPTORS

    override fun getColorDescriptors(): Array<ColorDescriptor> = ColorDescriptor.EMPTY_ARRAY

    override fun getDisplayName(): String = "Stride"

    override fun getIcon(): Icon = StrideIcons.FILE

    override fun getHighlighter(): SyntaxHighlighter = StrideSyntaxHighlighter()

    override fun getDemoText(): String = """
            // Stride example
            extern fn <func>print</func>(msg: string): void;

            struct Point {
                x: float32,
                y: float32
            }

            fn <func>main</func>(): int32 {
                const p: Point = Point::{ x: 1.0, y: 2.0 };
                let msg: string = "Hello, Stride!";
                if (p.x > 0.0 && true) {
                    <func>print</func>(msg);
                }
                return 0;
            }
        """.trimIndent()

    override fun getAdditionalHighlightingTagToDescriptorMap(): Map<String, TextAttributesKey>? = mapOf(
            "func" to StrideSyntaxHighlighter.FUNCTION_CALL
        )

    companion object {
        private val DESCRIPTORS = arrayOf(
            AttributesDescriptor("Keyword", StrideSyntaxHighlighter.KEYWORD),
            AttributesDescriptor("Number", StrideSyntaxHighlighter.NUMBER),
            AttributesDescriptor("String", StrideSyntaxHighlighter.STRING),
            AttributesDescriptor("Comment", StrideSyntaxHighlighter.COMMENT),
            AttributesDescriptor("Operator", StrideSyntaxHighlighter.OPERATOR),
            AttributesDescriptor("Parentheses", StrideSyntaxHighlighter.PARENTHESES),
            AttributesDescriptor("Braces", StrideSyntaxHighlighter.BRACES),
            AttributesDescriptor("Semicolon", StrideSyntaxHighlighter.SEMICOLON),
            AttributesDescriptor("Comma", StrideSyntaxHighlighter.COMMA),
            AttributesDescriptor("Dot", StrideSyntaxHighlighter.DOT),
            AttributesDescriptor("Type", StrideSyntaxHighlighter.TYPE),
            AttributesDescriptor("Function call", StrideSyntaxHighlighter.FUNCTION_CALL)

        )
    }
}
