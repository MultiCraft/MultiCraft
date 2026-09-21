import java.io.ByteArrayOutputStream
import java.io.FileInputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.util.Properties
import javax.inject.Inject

plugins {
	id("com.android.application")
}

// Gradle 9 runs a command from a task only through ExecOperations
interface Injected {
	@get:Inject
	val exec: ExecOperations
}
val execOps = objects.newInstance<Injected>().exec

android {
	buildToolsVersion = "37.0.0"
	compileSdk = 37
	ndkVersion = "30.0.16248370"
	namespace = "com.multicraft.game"

	val versionMajor = rootProject.extra["versionMajor"] as Int
	val versionMinor = rootProject.extra["versionMinor"] as Int
	val versionPatch = rootProject.extra["versionPatch"] as Int

	defaultConfig {
		applicationId = "com.multicraft.game"
		minSdk = 23
		targetSdk = 37
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

	buildFeatures {
		viewBinding = true
		buildConfig = true
	}

	androidResources {
		// zipAssetsFiles already deflated it
		noCompress += "assets.zip"
	}
}

// Builds a GLSL stage to SPIR-V and shows what glslang says only when it fails
fun spirv(source: File, output: File, vararg flags: String) {
	val log = ByteArrayOutputStream()
	val result = execOps.exec {
		commandLine("glslangValidator", "-V", *flags, "-o", output.path, source.path)
		standardOutput = log
		errorOutput = log
		isIgnoreExitValue = true
	}
	if (result.exitValue != 0)
		throw GradleException("GLSL did not compile for ${source.name}:\n$log")
}

// ES 3 dropped the words the game's shaders are written in
fun prepare(source: File, word: String): String {
	val text = source.readText()
	// A varying OpenGL reads at the centroid keeps centroid
	val centroid = if (Regex("^centroid varying vec2 varTexCoord;$", RegexOption.MULTILINE).containsMatchIn(text))
		"centroid " else ""
	return text.lines().joinToString("\n") {
		it.replace(Regex("^varying (?=.*varTexCoord;$)"), "$centroid$word ")
			// Both stages agree on the precision of a varying: PowerVR draws garbage when they differ
			.replace(Regex("^varying (?!(lowp|mediump|highp) )"), "$word mediump ")
			.replace(Regex("^varying "), "$word ")
			.replace("texture2D(", "texture(")
			.replace("textureCube(", "texture(")
			.replace("gl_FragColor", "fragColor")
	}
}

// The game's programs as one SPIR-V library, named the way shaderVariant() names them
fun buildVulkanPrograms(root: File, work: File, dest: File) {
	// How many lights the client sends
	val lights = Regex("^constexpr u32 MAX_POINT_LIGHTS = (\\d+);", RegexOption.MULTILINE)
		.find(File(root, "src/client/shader.h").readText())?.groupValues?.get(1)
		?: throw GradleException("MAX_POINT_LIGHTS not found in src/client/shader.h")

	// The constants the game bakes into its source
	val drawTypes = listOf("NORMAL", "AIRLIKE", "LIQUID", "FLOWINGLIQUID", "GLASSLIKE", "ALLFACES",
		"ALLFACES_OPTIONAL", "TORCHLIKE", "SIGNLIKE", "PLANTLIKE", "FENCELIKE", "RAILLIKE", "NODEBOX",
		"GLASSLIKE_FRAMED", "FIRELIKE", "GLASSLIKE_FRAMED_OPTIONAL", "MESH", "PLANTLIKE_ROOTED")
	val materials = listOf("BASIC", "ALPHA", "LIQUID_TRANSPARENT", "LIQUID_OPAQUE", "WAVING_LEAVES",
		"WAVING_PLANTS", "OPAQUE", "WAVING_LIQUID_BASIC", "WAVING_LIQUID_TRANSPARENT",
		"WAVING_LIQUID_OPAQUE", "PLAIN", "PLAIN_ALPHA")
	val constants = "#define SETTINGS_UNIFORMS 1\n" +
		drawTypes.withIndex().joinToString("") { "#define NDT_${it.value} ${it.index}\n" } +
		materials.withIndex().joinToString("") { "#define TILE_MATERIAL_${it.value} ${it.index}\n" }

	// Slots as the driver declares the vertex layout
	val vertexHead = """
		#version 310 es
		precision highp float;

		// Shared edges land on the same pixels
		invariant gl_Position;

		// Turn of a screen drawn pre-rotated: 1 by 90 degrees, -1 by 270
		layout(push_constant) uniform PreRotation
		{
			int turn;
		} preRotation;
		#define main gameMain

		layout(location = 0) in highp vec4 inVertexPosition;
		layout(location = 1) in mediump vec3 inVertexNormal;
		layout(location = 2) in lowp vec4 inVertexColor;
		layout(location = 3) in highp vec4 inTexCoord0;
		layout(location = 4) in mediump vec4 inTexCoord1;
		layout(location = 5) in mediump vec4 inVertexTangent;
		layout(location = 6) in mediump vec4 inVertexBinormal;
	""".trimIndent() + "\n"
	// Runs the game's stage, then turns the position for a screen drawn pre-rotated
	val vertexTail = """
		#undef main
		void main()
		{
			gameMain();
			if (preRotation.turn != 0)
				gl_Position.xy = gl_Position.yx * vec2(preRotation.turn, -preRotation.turn);
		}
	""".trimIndent() + "\n"
	// The matrices the client sets, each stage declaring the ones it names
	val vertexUniforms = listOf("mWorldView", "mWorldViewProj", "mTexture")
	val fragmentHead = "#version 310 es\nprecision mediump float;\nlayout(location = 0) out vec4 fragColor;\n"

	fun join(separator: String, vararg parts: String) = parts.filter { it.isNotEmpty() }.joinToString(separator)
	// waving names the branch that waves
	fun defines(material: Int, waving: String, count: String) =
		"#define MATERIAL_TYPE $material\n#define ENABLE_WAVING_WATER ${if (waving == "liquid") 1 else 0}\n" +
		"#define ENABLE_WAVING_LEAVES ${if (waving == "leaves") 1 else 0}\n" +
		"#define ENABLE_WAVING_PLANTS ${if (waving == "plants") 1 else 0}\n" +
		"#define POINT_LIGHTS $count\n#define ENABLE_TONE_MAPPING 0\n"
	val discard = mapOf("blend" to "#define USE_ALPHA_REF_DISCARD 1\n", "ref" to "#define USE_ALPHA_TEST_DISCARD 1\n")

	// Each stage under its name in the library, with the shader it comes from and what it is built with
	val stages = sortedMapOf<String, Pair<String, String>>()
	val programs = StringBuilder()
	// A branch of a shader, the material type it builds with and the discard modes of the materials it covers
	fun branch(shader: String, name: String, material: Int, modes: String, count: String) {
		val light = if (count == "0") "" else "lights"
		val vertex = join("_", shader, name, light)
		stages["vs_$vertex"] = "${shader}_shader" to defines(material, name, count)
		for (mode in modes.split(" ")) {
			val fragment = join("_", shader, mode, light)
			stages["fs_$fragment"] = "${shader}_shader" to (discard[mode] ?: "") + defines(0, "", count)
			programs.append(join(":", "${shader}_shader", name, if (mode == "solid") "" else mode, light))
				.append("|$vertex|$fragment\n")
		}
	}
	for ((name, material, modes) in listOf(Triple("", 0, "solid blend ref"), Triple("leaves", 4, "ref"),
			Triple("plants", 5, "ref"), Triple("liquid", 8, "solid blend ref")))
		for (count in listOf("0", lights))
			branch("nodes", name, material, modes, count)
	branch("object", "", 0, "solid blend ref", "0")
	branch("object", "plain", 10, "blend ref", "0")
	for (one in listOf("selection", "stars", "minimap")) {
		stages["vs_$one"] = "${one}_shader" to defines(0, "", "0")
		stages["fs_$one"] = "${one}_shader" to defines(0, "", "0")
		programs.append("${one}_shader|$one|$one\n")
	}

	// Every stage in one file, each as its name and its words
	val library = ByteArrayOutputStream()
	fun word(value: Int) = library.write(ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(value).array())
	for ((stage, build) in stages) {
		val vertex = stage.startsWith("vs_")
		val glsl = File(work, "$stage.glsl")
		val source = prepare(File(root, "client/shaders/${build.first}/opengl_${if (vertex) "vertex" else "fragment"}.glsl"),
			if (vertex) "out" else "in")
		val head = if (!vertex) fragmentHead else vertexHead + vertexUniforms
			.filter { Regex("\\b$it\\b").containsMatchIn(source) }
			.joinToString("") { "uniform highp mat4 $it;\n" }
		glsl.writeText(head + build.second + constants + source +
			if (vertex) "\n" + vertexTail else "")
		val spv = File(work, "$stage.spv")
		// -R gathers the loose constants of a stage into one block
		spirv(glsl, spv, "-O", "-R", "--auto-map-bindings", "-S", if (vertex) "vert" else "frag", "--auto-map-locations")
		val name = stage.toByteArray()
		val words = spv.readBytes()
		word(name.size)
		library.write(name)
		library.write(ByteArray((4 - name.size % 4) % 4))
		word(words.size / 4)
		library.write(words)
	}
	dest.mkdirs()
	File(dest, "vulkan_shaders.spvlib").writeBytes(library.toByteArray())
	File(dest, "programs.txt").writeText(programs.toString())
}

val prepareAssetsFiles = tasks.register("prepareAssetsFiles") {
	description = "Prepare assets"
	val assetsFolder = "build/assets/Files"
	val projRoot = "../../"

	doLast {
		copy {
			from("$projRoot/builtin")
			into("$assetsFolder/builtin")
		}
		copy {
			from("$projRoot/client/shaders")
			into("$assetsFolder/client/shaders")
		}
		// The GLES2 driver's shaders, anew each build
		delete("$assetsFolder/client/shaders/Irrlicht")
		copy {
			from("../native/deps/irrlicht/shaders")
			into("$assetsFolder/client/shaders/Irrlicht")
			include("*.vsh", "*.fsh")
		}
		// The Vulkan driver reads SPIR-V: Irrlicht's own stages and the game's library
		fileTree("../native/deps/irrlicht/shaders/Common") { include("*.vert", "*.frag") }.forEach {
			spirv(it, file("$assetsFolder/client/shaders/Irrlicht/${it.name}.spv"),
				"-O", "--target-env", "vulkan1.0", "-DIRR_PRE_ROTATION")
		}
		buildVulkanPrograms(file(projRoot), temporaryDir, file("$assetsFolder/client/shaders/Vulkan"))
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

val zipAssetsFiles = tasks.register<Zip>("zipAssetsFiles") {
	description = "Compress assets"
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
	implementation("androidx.appcompat:appcompat:1.8.0")
	implementation("androidx.appcompat:appcompat-resources:1.8.0")
	implementation("androidx.browser:browser:1.10.0")
	implementation("androidx.lifecycle:lifecycle-runtime-ktx:2.11.0")
	implementation("com.google.android.material:material:1.14.0")
}
