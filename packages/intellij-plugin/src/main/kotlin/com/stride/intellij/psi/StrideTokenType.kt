package com.stride.intellij.psi

import com.intellij.psi.tree.IElementType
import com.stride.intellij.StrideLanguage

class StrideTokenType(debugName: String) : IElementType(debugName, StrideLanguage) {
    override fun toString(): String = "StrideTokenType." + super.toString()
}
