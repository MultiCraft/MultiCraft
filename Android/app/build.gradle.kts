import org.jetbrains.kotlin.gradle.dsl.JvmTarget
import java.io.FileInputStream
import java.util.Properties

plugins {
	id("com.android.application")
	kotlin("android")
}

android {
	buildToolsVersion = "36.1.0"
	compileSdk = 36
	ndkVersion = "29.0.14206865"
	namespace = "com.multicraft.game"

	val versionMajor = rootProject.extra["versionMajor"] as Int
	val versionMinor = rootProject.extra["versionMinor"] as Int
	val versionPatch = rootProject.extra["versionPatch"] as Int

	defaultConfig {
		applicationId = "com.multicraft.game"
		minSdk = 23
		targetSdk = 36
		versionName = "$versionMajor.$versionMinor.$versionPatch"
		versionCode = rootProject.extra["versionCode"] as Int
	}

	// Load keystore properties if present
	val props = Properties()
	val propFile = rootProject.file("local.properties")
	if (propFile.exists()) {
		props.load(FileInputStream(propFile))
	}

	if (props.getProperty("keystore") != null) {
		signingConfigs {
			create("release") {
				storeFile = file(props["keystore"]!!)
				storePassword = props["keystore.password"] as String?
				keyAlias = props["key"] as String?
				keyPassword = props["key.password"] as String?
			}
		}

		buildTypes {
			getByName("release") {
				isMinifyEnabled = true
				isShrinkResources = true
				signingConfig = signingConfigs.getByName("release")
				proguardFiles(
					getDefaultProguardFile("proguard-android-optimize.txt"),
					"proguard-rules.pro",
					"proguard-rules-sdl.pro"
				)
			}
			getByName("debug") {
				isDebuggable = true
				isMinifyEnabled = false
			}
		}
	}

	splits {
		abi {
			isEnable = true
			reset()
			include("armeabi-v7a", "arm64-v8a", "x86_64")
		}
	}

	compileOptions {
		sourceCompatibility = JavaVersion.VERSION_17
		targetCompatibility = JavaVersion.VERSION_17
	}

	kotlin {
		compilerOptions {
			jvmTarget.set(JvmTarget.JVM_17)
		}
	}

	buildFeatures {
		viewBinding = true
		buildConfig = true
	}
}

val prepareAssetsFiles by tasks.registering {
	val assetsFolder = "build/assets/Files"
	val projRoot = "../../"

	doLast {
		copy {
			from("$projRoot/builtin")
			into("$assetsFolder/builtin")
			exclude("*.txt")
		}
		copy {
			from("$projRoot/client/shaders")
			into("$assetsFolder/client/shaders")
		}
		copy {
			from("../native/deps/irrlicht/shaders")
			into("$assetsFolder/client/shaders/Irrlicht")
		}
		copy {
			from(
				"$projRoot/fonts/DroidSansFallback.ttf",
				"$projRoot/fonts/MultiCraftFont.ttf"
			)
			into("$assetsFolder/fonts")
		}
		copy {
			from("$projRoot/textures")
			into("$assetsFolder/textures")
			exclude("*.txt")
		}
	}
}

val zipAssetsFiles by tasks.registering(Zip::class) {
	dependsOn(prepareAssetsFiles)
	archiveFileName.set("assets.zip")
	destinationDirectory.set(file("src/main/assets"))
	from("build/assets/Files")

	val projRoot = "../../"
	copy {
		from("$projRoot/client/cacert.pem")
		into(file("src/main/assets"))
	}
}

tasks.named("preBuild") {
	dependsOn(zipAssetsFiles)
}

dependencies {
	/* MultiCraft Native */
	implementation(project(":native"))

	/* Third-party libraries */
	implementation("androidx.appcompat:appcompat:1.7.1")
	implementation("androidx.appcompat:appcompat-resources:1.7.1")
	implementation("androidx.browser:browser:1.9.0")
	implementation("androidx.lifecycle:lifecycle-runtime-ktx:2.10.0")
	implementation("com.google.android.material:material:1.13.0")
}
