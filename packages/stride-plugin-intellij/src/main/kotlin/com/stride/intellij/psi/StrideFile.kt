package com.stride.intellij.psi

import com.intellij.extapi.psi.PsiFileBase
import com.intellij.openapi.fileTypes.FileType
import com.intellij.psi.FileViewProvider
import com.stride.intellij.StrideFileType
import com.stride.intellij.StrideLanguage

class StrideFile(viewProvider: FileViewProvider) : PsiFileBase(viewProvider, StrideLanguage) {
    override fun getFileType(): FileType = StrideFileType
    override fun toString(): String = "Stride File"
}
