package com.stride.intellij

import com.intellij.psi.codeStyle.CodeStyleSettingsManager
import com.intellij.testFramework.fixtures.BasePlatformTestCase

class StrideLineWidthFormatterTest : BasePlatformTestCase() {

    private fun getStrideSettings(): StrideCodeStyleSettings {
        val settings = CodeStyleSettingsManager.getSettings(project)
        return settings.getCustomSettings(StrideCodeStyleSettings::class.java)
    }

    private fun setIndentSize(size: Int) {
        val settings = CodeStyleSettingsManager.getSettings(project)
        val indentOptions = settings.getCommonSettings(StrideLanguage).indentOptions
        indentOptions?.INDENT_SIZE = size
    }

    fun testObjectInitFitsOnOneLine() {
        getStrideSettings().MAX_LINE_WIDTH = 120
        val before = """
            const obj = SomeObject::{ field1, field2, field3 };
        """.trimIndent()
        val after = """
            const obj = SomeObject::{ field1, field2, field3 };
        """.trimIndent()
        myFixture.configureByText("test.sr", before)
        myFixture.performEditorAction("ReformatCode")
        myFixture.checkResult(after)
    }

    fun testObjectInitExceedsLineWidth() {
        getStrideSettings().MAX_LINE_WIDTH = 40
        val before = """
            const obj = SomeObject::{ field1, field2, field3, field4, field5 };
        """.trimIndent()
        val after = """
            const obj = SomeObject::{
                field1,
                field2,
                field3,
                field4,
                field5
            };
        """.trimIndent()
        myFixture.configureByText("test.sr", before)
        myFixture.performEditorAction("ReformatCode")
        myFixture.checkResult(after)
    }

    fun testFunctionCallFitsOnOneLine() {
        getStrideSettings().MAX_LINE_WIDTH = 120
        val before = """
            fn main(): void {
                some::function_call(param1, param2, param3);
            }
        """.trimIndent()
        val after = """
            fn main(): void {
                some::function_call(param1, param2, param3);
            }
        """.trimIndent()
        myFixture.configureByText("test.sr", before)
        myFixture.performEditorAction("ReformatCode")
        myFixture.checkResult(after)
    }

    fun testFunctionCallExceedsLineWidth() {
        getStrideSettings().MAX_LINE_WIDTH = 30
        val before = """
            fn main(): void {
                some::function_call(param1, param2, param3);
            }
        """.trimIndent()
        val after = """
            fn main(): void {
                some::function_call(
                    param1,
                    param2,
                    param3
                );
            }
        """.trimIndent()
        myFixture.configureByText("test.sr", before)
        myFixture.performEditorAction("ReformatCode")
        myFixture.checkResult(after)
    }

    fun testNestedObjectInitMixedWrapping() {
        getStrideSettings().MAX_LINE_WIDTH = 60
        val before = """
            const player = Entity::{ id: 1, name: "Player1", position: Vec3::{ x: 10, y: 20, z: 30 } };
        """.trimIndent()
        val after = """
            const player = Entity::{
                id: 1,
                name: "Player1",
                position: Vec3::{ x: 10, y: 20, z: 30 }
            };
        """.trimIndent()
        myFixture.configureByText("test.sr", before)
        myFixture.performEditorAction("ReformatCode")
        myFixture.checkResult(after)
    }

    fun testObjectInitWithValuesFitsOnOneLine() {
        getStrideSettings().MAX_LINE_WIDTH = 120
        val before = """
            const obj = SomeType::{ field: value, field2: value2, field3: value3 };
        """.trimIndent()
        val after = """
            const obj = SomeType::{ field: value, field2: value2, field3: value3 };
        """.trimIndent()
        myFixture.configureByText("test.sr", before)
        myFixture.performEditorAction("ReformatCode")
        myFixture.checkResult(after)
    }

    fun testObjectInitWithValuesExceedsLineWidth() {
        getStrideSettings().MAX_LINE_WIDTH = 40
        val before = """
            const obj = SomeType::{ field: value, field2: value2, field3: value3 };
        """.trimIndent()
        val after = """
            const obj = SomeType::{
                field: value,
                field2: value2,
                field3: value3
            };
        """.trimIndent()
        myFixture.configureByText("test.sr", before)
        myFixture.performEditorAction("ReformatCode")
        myFixture.checkResult(after)
    }

    fun testGenericObjectInitWrapping() {
        getStrideSettings().MAX_LINE_WIDTH = 120
        val before = """
            pub fn arrayOf<T>(members: T[]): Array<T> {
                return Array<T>::{
                    length: members.length,
                    data: members
                };
            }
        """.trimIndent()
        val after = """
            pub fn arrayOf<T>(members: T[]): Array<T> {
                return Array<T>::{ length: members.length, data: members };
            }
        """.trimIndent()
        myFixture.configureByText("test.sr", before)
        myFixture.performEditorAction("ReformatCode")
        myFixture.checkResult(after)
    }

    fun testCustomIndentSize() {
        getStrideSettings().MAX_LINE_WIDTH = 30
        setIndentSize(2)
        val before = """
            const obj = SomeObject::{ field1, field2, field3 };
        """.trimIndent()
        val after = """
            const obj = SomeObject::{
              field1,
              field2,
              field3
            };
        """.trimIndent()
        myFixture.configureByText("test.sr", before)
        myFixture.performEditorAction("ReformatCode")
        myFixture.checkResult(after)
    }
}
