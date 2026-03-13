package com.stride.intellij

import com.intellij.psi.codeStyle.CodeStyleSettings
import com.intellij.psi.codeStyle.CustomCodeStyleSettings

class StrideCodeStyleSettings(container: CodeStyleSettings) :
    CustomCodeStyleSettings("StrideCodeStyleSettings", container) {

    @JvmField
    var MAX_LINE_WIDTH: Int = 120
}
