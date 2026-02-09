package com.stride.intellij

import com.intellij.openapi.fileTypes.LanguageFileType
import javax.swing.Icon

object StrideFileType : LanguageFileType(StrideLanguage) {
    val INSTANCE = this

    override fun getName(): String = "Stride File"

    override fun getDescription(): String = "Stride language file"

    override fun getDefaultExtension(): String = "sr"

    override fun getIcon(): Icon? = StrideIcons.FILE
}
