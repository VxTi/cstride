package com.stride.intellij

import com.intellij.testFramework.fixtures.BasePlatformTestCase

class StrideModuleFormatterTest : BasePlatformTestCase() {
    fun testModuleNesting() {
        val before = "module outer { module inner { fn somefn(): void { } } }".trimIndent()
        myFixture.configureByText("test.sr", before)
        myFixture.performEditorAction("ReformatCode")
        val after = """
            module outer {
                module inner {
                    fn somefn(): void {
                    }
                }
            }
        """.trimIndent()
        myFixture.checkResult(after)
    }
}
