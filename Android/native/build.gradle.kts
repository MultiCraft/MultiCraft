import java.net.URI

plugins {
	id("com.android.library")
}

android {
	buildToolsVersion = "36.0.0"
	compileSdk = 36
	ndkVersion = "29.0.14206865"
	namespace = "com.multicraft"

	val versionMajor = rootProject.extra["versionMajor"] as Int
	val versionMinor = rootProject.extra["versionMinor"] as Int
	val versionPatch = rootProject.extra["versionPatch"] as Int
	val versionExtra = rootProject.extra["versionExtra"] as String
	val developmentBuild = rootProject.extra["developmentBuild"] as Int

	defaultConfig {
		minSdk = 23
		lint.targetSdk = 36

		@Suppress("UnstableApiUsage")
		externalNativeBuild {
			ndkBuild {
				arguments(
					"-j${Runtime.getRuntime().availableProcessors()}",
					"--output-sync=none",
					"versionMajor=$versionMajor",
					"versionMinor=$versionMinor",
					"versionPatch=$versionPatch",
					"versionExtra=$versionExtra",
					"developmentBuild=$developmentBuild"
				)
			}
		}
	}

	externalNativeBuild {
		ndkBuild {
			path = file("jni/Android.mk")
		}
	}

	splits {
		abi {
			isEnable = true
			reset()
			include("armeabi-v7a", "arm64-v8a", "x86_64")
		}
	}

	buildTypes {
		getByName("release") {
			@Suppress("UnstableApiUsage")
			externalNativeBuild {
				ndkBuild {
					arguments("NDEBUG=1")
				}
			}
		}
	}
}

val buildDirFile: File = layout.buildDirectory.asFile.get()
val depsZip = File(buildDirFile, "deps.zip")
val depsDir = file("deps")

val downloadDeps = tasks.register("downloadDeps") {
	doLast {
		if (!depsZip.exists()) {
			println("Downloading dependencies...")
			depsZip.parentFile.mkdirs()
			val depsUrl =
				URI("https://github.com/MultiCraft/deps_android/releases/latest/download/deps_android.zip").toURL()

			depsUrl.openStream().use { input ->
				depsZip.outputStream().use { output ->
					input.copyTo(output)
				}
			}

			println("Downloaded to $depsZip")
		} else {
			println("Dependencies already downloaded")
		}
	}
}

val getDeps = tasks.register<Copy>("getDeps") {
	dependsOn(downloadDeps)

	from(zipTree(depsZip))
	into(depsDir)

	onlyIf { !depsDir.exists() }

	doLast { println("Dependencies unpacked into $depsDir") }
}

tasks.named("preBuild") {
	dependsOn(getDeps)
}

android.defaultConfig.externalNativeBuild.ndkBuild.apply {
	arguments("prebuilt=$(if $(strip $(wildcard $(prebuilt_path))),$(prebuilt_path),.)")
}
