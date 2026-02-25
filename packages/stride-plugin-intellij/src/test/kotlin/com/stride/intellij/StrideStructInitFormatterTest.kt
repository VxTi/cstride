package com.stride.intellij

import com.intellij.testFramework.fixtures.BasePlatformTestCase

class StrideStructInitFormatterTest : BasePlatformTestCase() {
    fun testStructInitFormatting() {
        val before = """
            const player = Entity::{ id: 1, name: "Player1", position: Vec3::{ x: 10, y: 20, z: 30 }, };
        """.trimIndent()
        val after = """
            const player = Entity::{
                id: 1,
                name: "Player1",
                position: Vec3::{
                    x: 10,
                    y: 20,
                    z: 30
                },
            };
        """.trimIndent()
        myFixture.configureByText("test.sr", before)
        myFixture.performEditorAction("ReformatCode")
        myFixture.checkResult(after)
    }
}
