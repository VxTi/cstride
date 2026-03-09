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
            package System;

            import System::{
                IO::Print,
                Time::Sleep
            };

            type <usertype>Array</usertype>&lt;<generic>T</generic>&gt; = <generic>T</generic>[];
            type <usertype>IArray</usertype> = <usertype>Array</usertype>&lt;i32&gt;;

            /**
             * Prints the given string and sleeps for the specified duration.
             */
            pub async fn <func>delayed_print</func>(msg: string, ms: i32): void {
                IO::<func>Print</func>(msg);
                Time::<func>Sleep</func>(ms);
            }

            struct Point {
                x: f32,
                y: f32
            }

            async fn <func>main</func>(): i32 {
                const p: Point = Point::{ x: 1.0, y: 2.0 };
                const numbers: <usertype>IArray</usertype> = [1, 2, 3, 4, 5];
                let msg: string = "Hello, Stride!";

                if (p.x > 0.0 && true) {
                    <func>delayed_print</func>(msg, 1000);
                }
                return 0;
            }
        """.trimIndent()

    override fun getAdditionalHighlightingTagToDescriptorMap(): Map<String, TextAttributesKey>? = mapOf(
            "func" to StrideSyntaxHighlighter.FUNCTION_CALL,
            "usertype" to StrideSyntaxHighlighter.USER_TYPE,
            "generic" to StrideSyntaxHighlighter.GENERIC_TYPE_PARAMETER
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
            AttributesDescriptor("Brackets", StrideSyntaxHighlighter.BRACKETS),
            AttributesDescriptor("Semicolon", StrideSyntaxHighlighter.SEMICOLON),
            AttributesDescriptor("Comma", StrideSyntaxHighlighter.COMMA),
            AttributesDescriptor("Dot", StrideSyntaxHighlighter.DOT),
            AttributesDescriptor("Primitive types", StrideSyntaxHighlighter.TYPE),
            AttributesDescriptor("Function call", StrideSyntaxHighlighter.FUNCTION_CALL),
            AttributesDescriptor("Custom types", StrideSyntaxHighlighter.USER_TYPE),
            AttributesDescriptor("Generic type parameter", StrideSyntaxHighlighter.GENERIC_TYPE_PARAMETER)

        )
    }
}
