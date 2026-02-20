package com.stride.intellij.run

import com.intellij.execution.configurations.ConfigurationFactory
import com.intellij.execution.configurations.ConfigurationType
import com.stride.intellij.StrideIcons
import javax.swing.Icon

class StrideRunConfigurationType : ConfigurationType {
    override fun getDisplayName(): String = "Stride"
    override fun getConfigurationTypeDescription(): String = "Run a Stride (.sr) file"
    override fun getIcon(): Icon = StrideIcons.FILE
    override fun getId(): String = "StrideRunConfiguration"
    override fun getConfigurationFactories(): Array<ConfigurationFactory> =
        arrayOf(StrideConfigurationFactory(this))
}