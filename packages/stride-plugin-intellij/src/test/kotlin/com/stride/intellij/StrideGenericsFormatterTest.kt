package com.stride.intellij

import com.intellij.testFramework.fixtures.BasePlatformTestCase

class StrideGenericsFormatterTest : BasePlatformTestCase() {
    fun testGenericFormatting() {
        val before = """
            type SomeStruct< A, B, C > = {
                field1: A;
                field2: B;
                field3: C;
            };

            fn some_fn(nested: SomeStruct< i32, i32, i32 >): void {
                IO::Print("Field1: %d", nested.field1);
            }
        """.trimIndent()

        val after = """
            type SomeStruct<A, B, C> = {
                field1: A;
                field2: B;
                field3: C;
            };

            fn some_fn(nested: SomeStruct<i32, i32, i32>): void {
                IO::Print("Field1: %d", nested.field1);
            }
        """.trimIndent()

        myFixture.configureByText("test.sr", before)
        myFixture.performEditorAction("ReformatCode")
        myFixture.checkResult(after)
    }

    fun testNestedGenericFormatting() {
        val before = """
            type Nested< T > = T;
            type Outer< T > = Nested< T >;
        """.trimIndent()

        val after = """
            type Nested<T> = T;
            
            type Outer<T> = Nested<T>;
        """.trimIndent()

        myFixture.configureByText("test.sr", before)
        myFixture.performEditorAction("ReformatCode")
        myFixture.checkResult(after)
    }
    fun testFunctionCallGenericFormatting() {
        val before = """
            fn main(): void {
                some_fn< i32, i32 >(1, 2);
            }
        """.trimIndent()

        val after = """
            fn main(): void {
                some_fn<i32, i32>(1, 2);
            }
        """.trimIndent()

        myFixture.configureByText("test.sr", before)
        myFixture.performEditorAction("ReformatCode")
        myFixture.checkResult(after)
    }
}
