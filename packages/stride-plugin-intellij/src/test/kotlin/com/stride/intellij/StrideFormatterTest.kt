package com.stride.intellij

import com.intellij.testFramework.fixtures.BasePlatformTestCase

class StrideFormatterTest : BasePlatformTestCase() {
    fun testSimpleFormatting() {
        val before = """
            struct First { member: i32; }
            struct Second = First;
            fn main(): void { const s: Second = First::{ member: 123 }; }
        """.trimIndent()
        val after = """
            struct First {
                member: i32;
            }

            struct Second = First;

            fn main(): void {
                const s: Second = First::{ member: 123 };
            }
        """.trimIndent()
        myFixture.configureByText("test.sr", before)
        myFixture.performEditorAction("ReformatCode")
        myFixture.checkResult(after)
    }
}

