plugins {
	id("com.android.application") version "9.0.1" apply false
	id("com.android.library") version "9.0.1" apply false
	id("org.jetbrains.kotlin.android") version "2.3.10" apply false
}

extra.apply {
	set("versionMajor", 2)		// Version Major
	set("versionMinor", 0)		// Version Minor
	set("versionPatch", 13)		// Version Patch
	set("versionExtra", "")		// Version Extra
	set("versionCode", 200)		// Android Version Code
	set("developmentBuild", 0)	// Development or release build flag
}

tasks.register<Delete>("clean") {
	delete(rootProject.layout.buildDirectory)
	delete("native/deps")
}
