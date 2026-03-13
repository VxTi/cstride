package com.stride.intellij

import com.intellij.testFramework.fixtures.BasePlatformTestCase

class StrideFormatterTest : BasePlatformTestCase() {
    fun testSimpleFormatting() {
        val before = """
            type First = { member: i32; };
            type Second = First;
            fn main(): void { const s: Second = First::{ member: 123 }; }
        """.trimIndent()
        val after = """
            type First = {
                member: i32;
            };

            type Second = First;

            fn main(): void {
                const s: Second = First::{ member: 123 };
            }
        """.trimIndent()
        myFixture.configureByText("test.sr", before)
        myFixture.performEditorAction("ReformatCode")
        myFixture.checkResult(after)
    }

    fun testMultipleStatementsFormatting() {
        val before = """
            fn test(): void { const a: i32 = 1;const b: i32 = 2;const c: i32 = 3; }
        """.trimIndent()
        val after = """
            fn test(): void {
                const a: i32 = 1;
                const b: i32 = 2;
                const c: i32 = 3;
            }
        """.trimIndent()
        myFixture.configureByText("test.sr", before)
        myFixture.performEditorAction("ReformatCode")
        myFixture.checkResult(after)
    }
}
