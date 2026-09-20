import io.gitlab.arturbosch.detekt.Detekt
import java.util.Properties

plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.kotlin.compose)
    alias(libs.plugins.kotlin.serialization)
    alias(libs.plugins.detekt)
    alias(libs.plugins.ktfmt)
}

private val defaultVersionName = "1.0.0"

private fun resolveVersionName(): String {
    providers
        .environmentVariable("APP_VERSION_NAME")
        .orNull
        ?.takeIf { it.isNotBlank() }
        ?.let {
            return it.removePrefix("v")
        }
    val tag = runCatching {
        providers
            .exec {
                commandLine("git", "describe", "--tags", "--abbrev=0")
                isIgnoreExitValue = true
            }
            .standardOutput
            .asText
            .get()
            .trim()
    }
        .getOrDefault("")
    return tag.takeIf { it.isNotEmpty() }?.removePrefix("v") ?: defaultVersionName
}

private fun resolveVersionCode(versionName: String): Int = runCatching {
    val parts = versionName.split('.')
    fun part(i: Int) = parts.getOrNull(i)?.takeWhile(Char::isDigit)?.toIntOrNull() ?: 0
    part(0) * 10_000 + part(1) * 100 + part(2)
}
    .getOrDefault(1)

val appVersionName = resolveVersionName()
val appVersionCode = resolveVersionCode(appVersionName)

val localProps =
    Properties().apply {
        val file = rootProject.file("local.properties")
        if (file.exists()) file.inputStream().use(::load)
    }

fun signingSecret(key: String): String? =
    providers.environmentVariable(key).orNull
        ?: localProps.getProperty(key.lowercase().replace('_', '.'))

android {
    namespace = "org.gozaltech.nvdaremotecompanion.android"
    compileSdk = 37

    defaultConfig {
        applicationId = "org.gozaltech.nvdaremotecompanion.android"
        minSdk = 30
        targetSdk = 37
        versionCode = appVersionCode
        versionName = appVersionName

        ndk { abiFilters += "arm64-v8a" }

        externalNativeBuild {
            cmake {
                cppFlags += "-std=c++20"
                arguments += listOf("-DANDROID_STL=c++_static", "-DANDROID_PLATFORM=android-30")
            }
        }
    }

    signingConfigs {
        create("release") {
            signingSecret("KEYSTORE_FILE")?.let { keystore ->
                storeFile = file(keystore)
                storePassword = signingSecret("KEYSTORE_PASSWORD")
                keyAlias = signingSecret("KEY_ALIAS")
                keyPassword = signingSecret("KEY_PASSWORD")
            }
        }
    }

    buildTypes {
        release {
            if (signingSecret("KEYSTORE_FILE") != null) {
                signingConfig = signingConfigs.getByName("release")
            }
            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro",
            )
        }
    }

    buildFeatures { compose = true }

    packaging {
        jniLibs { useLegacyPackaging = true }
        dex { useLegacyPackaging = true }
    }

    bundle {
        language { enableSplit = false }
        density { enableSplit = true }
        abi { enableSplit = true }
    }

    androidResources { generateLocaleConfig = true }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    testOptions { unitTests { isReturnDefaultValues = true } }

    lint {
        warningsAsErrors = false
        abortOnError = true
        disable += "GradleDependency"
        disable += "ObsoleteSdkInt"
    }
}

ktfmt {
    kotlinLangStyle()
    maxWidth.set(100)
}

detekt {
    buildUponDefaultConfig = true
    parallel = false
    config.setFrom(rootProject.file("config/detekt/detekt.yml"))
    source.setFrom(files("src/main/java", "src/test/java"))
}

tasks.withType<Detekt>().configureEach {
    jvmTarget = JavaVersion.VERSION_17.toString()
    reports {
        html.required.set(true)
        sarif.required.set(true)
        xml.required.set(false)
        txt.required.set(false)
    }
}

tasks.register("staticAnalysis") {
    group = "verification"
    description = "Runs the ktfmt formatting check and detekt."
    dependsOn("ktfmtCheck", "detekt")
}

androidComponents {
    onVariants { variant ->
        variant.outputs.forEach { output ->
            output.outputFileName.set("NVDARemoteCompanion-v$appVersionName.apk")
        }
    }
}

dependencies {
    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.activity.compose)

    implementation(platform(libs.compose.bom))
    implementation(libs.bundles.compose)
    debugImplementation(libs.compose.ui.tooling)

    implementation(libs.androidx.lifecycle.runtime.compose)
    implementation(libs.androidx.lifecycle.viewmodel.compose)
    implementation(libs.bundles.navigation3)

    implementation(libs.androidx.datastore.preferences)
    implementation(libs.kotlinx.coroutines.android)
    implementation(libs.kotlinx.serialization.json)

    implementation(platform(libs.koin.bom))
    implementation(libs.koin.android)
    implementation(libs.koin.androidx.compose)

    testImplementation(libs.junit)
    testImplementation(libs.kotlin.test.junit)
    testImplementation(libs.kotlinx.coroutines.test)
}
