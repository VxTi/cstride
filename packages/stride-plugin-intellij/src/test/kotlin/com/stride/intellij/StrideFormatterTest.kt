package com.stride.intellij

import com.intellij.testFramework.fixtures.BasePlatformTestCase

class StrideFormatterTest : BasePlatformTestCase() {
    fun testSimpleFormatting() {
        val before = """
            type First = { member: int32; };
            type Second = First;
            fn main(): void { const s: Second = First::{ member: 123 }; }
        """.trimIndent()
        val after = """
            type First = {
                member: int32;
            };

            type Second = First;

            fn main(): void {
                const s: Second = First::{
                    member: 123
                };
            }
        """.trimIndent()
        myFixture.configureByText("test.sr", before)
        myFixture.performEditorAction("ReformatCode")
        myFixture.checkResult(after)
    }

    fun testMultipleStatementsFormatting() {
        val before = """
            fn test(): void { const a: int32 = 1;const b: int32 = 2;const c: int32 = 3; }
        """.trimIndent()
        val after = """
            fn test(): void {
                const a: int32 = 1;
                const b: int32 = 2;
                const c: int32 = 3;
            }
        """.trimIndent()
        myFixture.configureByText("test.sr", before)
        myFixture.performEditorAction("ReformatCode")
        myFixture.checkResult(after)
    }
}
