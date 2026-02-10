import org.jetbrains.grammarkit.tasks.*

plugins {
    id("java")
    id("org.jetbrains.kotlin.jvm") version "1.9.22"
    id("org.jetbrains.intellij.platform") version "2.1.0"
    id("org.jetbrains.grammarkit") version "2022.3.2.2"
}

group = "com.stride"
version = "1.0-SNAPSHOT"

kotlin {
    jvmToolchain(17)
}

repositories {
    mavenCentral()

    intellijPlatform {
        defaultRepositories()
    }
}

dependencies {
    intellijPlatform {
        intellijIdeaCommunity("2023.3.6")
        instrumentationTools()
        pluginVerifier()
        zipSigner()
    }
}

// Configure Gradle IntelliJ Platform Plugin
// Read more: https://plugins.jetbrains.com/docs/intellij/tools-gradle-intellij-platform-plugin.html
intellijPlatform {
    buildSearchableOptions = false

    pluginConfiguration {
        name = "Stride"
        version = "1.0-SNAPSHOT"

        ideaVersion {
            sinceBuild = "233"
            untilBuild = "242.*"
        }
    }
}

sourceSets {
    main {
        java {
            srcDirs("src/main/gen")
        }
    }
}



val generateStrideParser = tasks.register<GenerateParserTask>("generateStrideParser") {
    sourceFile.set(file("src/main/grammars/Stride.bnf"))
    targetRootOutputDir.set(layout.projectDirectory.dir("src/main/gen"))
    pathToParser.set("com/stride/intellij/parser/StrideParser.java")
    pathToPsiRoot.set("com/stride/intellij/psi")
    purgeOldFiles.set(true)
}

val generateStrideLexer = tasks.register<GenerateLexerTask>("generateStrideLexer") {
    sourceFile.set(file("src/main/grammars/Stride.flex"))
    targetOutputDir.set(layout.projectDirectory.dir("src/main/gen/com/stride/intellij/lexer"))
    purgeOldFiles.set(true)
}

tasks.withType<org.jetbrains.kotlin.gradle.tasks.KotlinCompile> {
    dependsOn(generateStrideParser, generateStrideLexer)
}
