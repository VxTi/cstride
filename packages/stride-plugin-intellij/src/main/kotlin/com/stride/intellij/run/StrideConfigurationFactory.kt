package com.stride.intellij.run

import com.intellij.execution.configurations.ConfigurationFactory
import com.intellij.execution.configurations.ConfigurationType
import com.intellij.execution.configurations.RunConfiguration
import com.intellij.openapi.components.BaseState
import com.intellij.openapi.project.Project

class StrideConfigurationFactory(type: ConfigurationType) : ConfigurationFactory(type) {
    override fun getId(): String = "StrideConfigurationFactory"

    override fun createTemplateConfiguration(project: Project): RunConfiguration =
        StrideRunConfiguration(project, this, "Stride")

    override fun getOptionsClass(): Class<out BaseState> = StrideRunConfigurationOptions::class.java
}