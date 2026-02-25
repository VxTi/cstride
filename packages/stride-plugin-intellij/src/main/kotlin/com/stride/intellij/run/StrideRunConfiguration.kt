package com.stride.intellij.run

import com.intellij.execution.Executor
import com.intellij.execution.configurations.*
import com.intellij.execution.process.ProcessHandler
import com.intellij.execution.process.ProcessHandlerFactory
import com.intellij.execution.process.ProcessTerminatedListener
import com.intellij.execution.runners.ExecutionEnvironment
import com.intellij.openapi.components.BaseState
import com.intellij.openapi.fileChooser.FileChooserDescriptorFactory
import com.intellij.openapi.options.SettingsEditor
import com.intellij.openapi.project.Project
import com.intellij.openapi.ui.TextFieldWithBrowseButton
import com.intellij.util.xmlb.annotations.Attribute
import javax.swing.BoxLayout
import javax.swing.JComponent
import javax.swing.JLabel
import javax.swing.JPanel

class StrideRunConfigurationOptions : RunConfigurationOptions() {
    @get:Attribute("scriptPath")
    var scriptPath by string("")
}

class StrideRunConfiguration(
    project: Project,
    factory: ConfigurationFactory,
    name: String
) : RunConfigurationBase<StrideRunConfigurationOptions>(project, factory, name) {

    override fun getOptions(): StrideRunConfigurationOptions =
        super.getOptions() as StrideRunConfigurationOptions

    var scriptPath: String
        get() = options.scriptPath ?: ""
        set(value) { options.scriptPath = value }

    override fun getConfigurationEditor(): SettingsEditor<out RunConfiguration> =
        StrideSettingsEditor(project)

    override fun getState(executor: Executor, environment: ExecutionEnvironment): RunProfileState {
        return object : CommandLineState(environment) {
            override fun startProcess(): ProcessHandler {
                val cmd = GeneralCommandLine("cstride", "-c", scriptPath)
                cmd.setWorkDirectory(project.basePath)
                val handler = ProcessHandlerFactory.getInstance().createColoredProcessHandler(cmd)
                ProcessTerminatedListener.attach(handler)
                return handler
            }
        }
    }
}

class StrideSettingsEditor(private val project: Project) : SettingsEditor<StrideRunConfiguration>() {
    private val scriptPathField = TextFieldWithBrowseButton()

    override fun createEditor(): JComponent {
        val descriptor = FileChooserDescriptorFactory.createSingleFileDescriptor("sr")
        scriptPathField.addBrowseFolderListener("Select Stride Script", null, project, descriptor)

        val panel = JPanel()
        panel.layout = BoxLayout(panel, BoxLayout.Y_AXIS)
        panel.add(JLabel("Script path (.sr):"))
        panel.add(scriptPathField)
        return panel
    }

    override fun applyEditorTo(config: StrideRunConfiguration) {
        config.scriptPath = scriptPathField.text
    }

    override fun resetEditorFrom(config: StrideRunConfiguration) {
        scriptPathField.text = config.scriptPath
    }
}