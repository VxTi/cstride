package com.stride.intellij

import com.intellij.lang.Language
import com.intellij.psi.codeStyle.CodeStyleSettingsCustomizable
import com.intellij.psi.codeStyle.CommonCodeStyleSettings
import com.intellij.psi.codeStyle.LanguageCodeStyleSettingsProvider

class StrideLanguageCodeStyleSettingsProvider : LanguageCodeStyleSettingsProvider() {
    override fun getLanguage(): Language = StrideLanguage

    override fun customizeSettings(consumer: CodeStyleSettingsCustomizable, settingsType: SettingsType) {
        if (settingsType == SettingsType.INDENT_SETTINGS) {
            consumer.showStandardOptions("INDENT_SIZE", "TAB_SIZE")
        }
        if (settingsType == SettingsType.WRAPPING_AND_BRACES_SETTINGS) {
            consumer.showStandardOptions("RIGHT_MARGIN")
        }
    }

    override fun customizeDefaults(commonSettings: CommonCodeStyleSettings, indentOptions: CommonCodeStyleSettings.IndentOptions) {
        indentOptions.INDENT_SIZE = 4
        indentOptions.TAB_SIZE = 4
        indentOptions.USE_TAB_CHARACTER = false
    }

    override fun getCodeSample(settingsType: SettingsType): String {
        return """
            package example;

            type Vector3 = {
                x: f64;
                y: f64;
                z: f64;
            };

            fn create_vector(x: f64, y: f64, z: f64): Vector3 {
                const v = Vector3::{ x: x, y: y, z: z };
                return v;
            }

            fn main(): void {
                const short = Vector3::{ x: 1, y: 2, z: 3 };
                IO::Print("x: %f, y: %f, z: %f", short.x, short.y, short.z);
            }
        """.trimIndent()
    }
}
