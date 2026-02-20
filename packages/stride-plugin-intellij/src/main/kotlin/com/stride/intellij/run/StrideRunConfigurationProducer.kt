package com.stride.intellij.run

import com.intellij.execution.actions.ConfigurationContext
import com.intellij.execution.actions.LazyRunConfigurationProducer
import com.intellij.execution.configurations.ConfigurationFactory
import com.intellij.execution.configurations.ConfigurationTypeUtil
import com.intellij.openapi.util.Ref
import com.intellij.psi.PsiElement
import com.stride.intellij.StrideFileType

class StrideRunConfigurationProducer : LazyRunConfigurationProducer<StrideRunConfiguration>() {

    override fun getConfigurationFactory(): ConfigurationFactory =
        ConfigurationTypeUtil.findConfigurationType(StrideRunConfigurationType::class.java)
            .configurationFactories[0]

    override fun isConfigurationFromContext(
        configuration: StrideRunConfiguration,
        context: ConfigurationContext
    ): Boolean {
        val file = context.location?.virtualFile ?: return false
        return file.fileType == StrideFileType && configuration.scriptPath == file.path
    }

    override fun setupConfigurationFromContext(
        configuration: StrideRunConfiguration,
        context: ConfigurationContext,
        sourceElement: Ref<PsiElement>
    ): Boolean {
        val file = context.location?.virtualFile ?: return false
        if (file.fileType != StrideFileType) return false

        val basePath = context.project.basePath ?: return false

        configuration.scriptPath = file.path
        configuration.compilerPath = "$basePath/packages/compiler/cmake-build-debug/cstride"
        configuration.name = "Run ${file.name}"

        return true
    }
}