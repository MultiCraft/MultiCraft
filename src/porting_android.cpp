/*
Minetest
Copyright (C) 2014 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3.0 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef __ANDROID__
#error This file may only be compiled for android!
#endif

#include "util/numeric.h"
#include "porting.h"
#include "porting_android.h"
#include "threading/thread.h"
#include "config.h"
#include "filesys.h"
#include "log.h"

#include <sstream>
#include <exception>
#include <cstdlib>

#ifdef GPROF
#include "prof.h"
#endif

extern int main(int argc, char *argv[]);
extern "C" void external_pause_game();

void android_main(android_app *app)
{
	int retval = 0;
	porting::app_global = app;

	Thread::setName("Main");

	try {
		char *argv[] = {strdup(PROJECT_NAME), nullptr};
		main(ARRLEN(argv) - 1, argv);
		free(argv[0]);
	} catch (std::exception &e) {
		errorstream << "Uncaught exception in main thread: " << e.what() << std::endl;
		retval = -1;
	} catch (...) {
		errorstream << "Uncaught exception in main thread!" << std::endl;
		retval = -1;
	}

	porting::cleanupAndroid();
	infostream << "Shutting down." << std::endl;
	exit(retval);
}

/**
 * Handler for finished message box input
 * Intentionally NOT in namespace porting
 * ToDo: this doesn't work as expected, there's a workaround for it right now
 */
extern "C" {
	JNIEXPORT void JNICALL Java_com_multicraft_game_GameActivity_putMessageBoxResult(
			JNIEnv *env, jclass thiz, jstring text)
	{
		errorstream <<
			"Java_com_multicraft_game_GameActivity_putMessageBoxResult got: " <<
			std::string((const char*) env->GetStringChars(text, nullptr)) << std::endl;
	}
	JNIEXPORT void JNICALL Java_com_multicraft_game_GameActivity_pauseGame(
			JNIEnv *env, jclass clazz)
	{
		external_pause_game();
	}
	bool device_has_keyboard = false;
	JNIEXPORT void JNICALL Java_com_multicraft_game_GameActivity_keyboardEvent(
			JNIEnv *env, jclass clazz, jboolean hasKeyboard)
	{
		device_has_keyboard = hasKeyboard;
	}
}

namespace porting {
android_app *app_global;
JNIEnv      *jnienv;
jclass       nativeActivity;

static float device_memory_max = 0;

jclass findClass(const std::string &classname)
{
	if (jnienv == nullptr)
		return nullptr;

	jclass nativeactivity = jnienv->FindClass("android/app/NativeActivity");
	jmethodID getClassLoader = jnienv->GetMethodID(
			nativeactivity, "getClassLoader", "()Ljava/lang/ClassLoader;");
	jobject cls = jnienv->CallObjectMethod(
						app_global->activity->clazz, getClassLoader);
	jclass classLoader = jnienv->FindClass("java/lang/ClassLoader");
	jmethodID findClass = jnienv->GetMethodID(classLoader, "loadClass",
					"(Ljava/lang/String;)Ljava/lang/Class;");
	jstring strClassName = jnienv->NewStringUTF(classname.c_str());
	return (jclass) jnienv->CallObjectMethod(cls, findClass, strClassName);
}

void initAndroid()
{
	porting::jnienv = nullptr;
	JavaVM *jvm = app_global->activity->vm;
	JavaVMAttachArgs lJavaVMAttachArgs;
	lJavaVMAttachArgs.version = JNI_VERSION_1_6;
	lJavaVMAttachArgs.name = PROJECT_NAME_C "NativeThread";
	lJavaVMAttachArgs.group = nullptr;

	if (jvm->AttachCurrentThread(&porting::jnienv, &lJavaVMAttachArgs) == JNI_ERR) {
		errorstream << "Failed to attach native thread to jvm" << std::endl;
		exit(-1);
	}

	nativeActivity = findClass("com/multicraft/game/GameActivity");
	if (nativeActivity == nullptr)
		errorstream <<
			"porting::initAndroid unable to find java native activity class" <<
			std::endl;

#ifdef GPROF
	// in the start-up code
	__android_log_print(ANDROID_LOG_ERROR, PROJECT_NAME_C,
			"Initializing GPROF profiler");
	monstartup("libMultiCraft.so");
#endif
}

void cleanupAndroid()
{
#ifdef GPROF
	errorstream << "Shutting down GPROF profiler" << std::endl;
	setenv("CPUPROFILE", (path_user + DIR_DELIM + "gmon.out").c_str(), 1);
	moncleanup();
#endif

	JavaVM *jvm = app_global->activity->vm;
	jvm->DetachCurrentThread();
}

static std::string javaStringToUTF8(jstring js)
{
	std::string str;
	// Get string as a UTF-8 c-string
	const char *c_str = jnienv->GetStringUTFChars(js, nullptr);
	// Save it
	str = c_str;
	// And free the c-string
	jnienv->ReleaseStringUTFChars(js, c_str);
	return str;
}

// Calls static method if obj is NULL
static std::string getAndroidPath(
		jclass cls, jobject obj, jmethodID mt_getAbsPath, const char *getter)
{
	// Get getter method
	jmethodID mt_getter;
	if (obj)
		mt_getter = jnienv->GetMethodID(cls, getter, "()Ljava/io/File;");
	else
		mt_getter = jnienv->GetStaticMethodID(cls, getter, "()Ljava/io/File;");

	// Call getter
	jobject ob_file;
	if (obj)
		ob_file = jnienv->CallObjectMethod(obj, mt_getter);
	else
		ob_file = jnienv->CallStaticObjectMethod(cls, mt_getter);

	// Call getAbsolutePath
	auto js_path = (jstring) jnienv->CallObjectMethod(ob_file, mt_getAbsPath);

	return javaStringToUTF8(js_path);
}

void initializePathsAndroid()
{
	// Get Environment class
	jclass cls_Env = jnienv->FindClass("android/os/Environment");
	// Get File class
	jclass cls_File = jnienv->FindClass("java/io/File");
	// Get getAbsolutePath method
	jmethodID mt_getAbsPath = jnienv->GetMethodID(cls_File,
				"getAbsolutePath", "()Ljava/lang/String;");
	std::string path_storage = getAndroidPath(cls_Env, nullptr,
				mt_getAbsPath, "getExternalStorageDirectory");
	std::string path_data = getAndroidPath(nativeActivity, app_global->activity->clazz, mt_getAbsPath,
				"getFilesDir");

	path_user    = path_storage + DIR_DELIM + "Android/data/com.multicraft.game/files";
	path_share   = path_data;
	path_locale  = path_data + DIR_DELIM + "locale";
	path_cache   = getAndroidPath(nativeActivity,
			app_global->activity->clazz, mt_getAbsPath, "getCacheDir");
}

void showInputDialog(const std::string &acceptButton, const std::string &hint,
		const std::string &current, int editType)
{
	jmethodID showdialog = jnienv->GetMethodID(nativeActivity, "showDialog",
		"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V");

	FATAL_ERROR_IF(showdialog == nullptr,
		"porting::showInputDialog unable to find java show dialog method");

	jstring jacceptButton = jnienv->NewStringUTF(acceptButton.c_str());
	jstring jhint         = jnienv->NewStringUTF(hint.c_str());
	jstring jcurrent      = jnienv->NewStringUTF(current.c_str());
	jint    jeditType     = editType;

	jnienv->CallVoidMethod(app_global->activity->clazz, showdialog,
			jacceptButton, jhint, jcurrent, jeditType);
}

void openURIAndroid(const std::string &url)
{
	jmethodID url_open = jnienv->GetMethodID(nativeActivity, "openURI",
		"(Ljava/lang/String;)V");

	FATAL_ERROR_IF(url_open == nullptr,
		"porting::openURIAndroid unable to find java openURI method");

	jstring jurl = jnienv->NewStringUTF(url.c_str());
	jnienv->CallVoidMethod(app_global->activity->clazz, url_open, jurl);
}

int getInputDialogState()
{
	jmethodID dialogstate = jnienv->GetMethodID(nativeActivity,
			"getDialogState", "()I");

	FATAL_ERROR_IF(dialogstate == nullptr,
		"porting::getInputDialogState unable to find java dialog state method");

	return jnienv->CallIntMethod(app_global->activity->clazz, dialogstate);
}

std::string getInputDialogValue()
{
	jmethodID dialogvalue = jnienv->GetMethodID(nativeActivity,
			"getDialogValue", "()Ljava/lang/String;");

	FATAL_ERROR_IF(dialogvalue == nullptr,
		"porting::getInputDialogValue unable to find java dialog value method");

	jobject result = jnienv->CallObjectMethod(app_global->activity->clazz,
			dialogvalue);

	const char *javachars = jnienv->GetStringUTFChars((jstring) result, nullptr);
	std::string text(javachars);
	jnienv->ReleaseStringUTFChars((jstring) result, javachars);

	return text;
}

float getMemoryMax()
{
	if (device_memory_max == 0) {
		jmethodID getMemory = jnienv->GetMethodID(nativeActivity,
				"getMemoryMax", "()F");

		if (getMemory == nullptr)
			assert("porting::getMemoryMax unable to find java method" == nullptr);

		device_memory_max = jnienv->CallFloatMethod(
				app_global->activity->clazz, getMemory);
	}

	return device_memory_max;
}

bool hasRealKeyboard()
{
	return device_has_keyboard;
}

void notifyServerConnect(bool is_multiplayer)
{
	jmethodID notifyConnect = jnienv->GetMethodID(nativeActivity,
			"notifyServerConnect", "(Z)V");

	FATAL_ERROR_IF(notifyConnect == nullptr,
			"porting::notifyServerConnect unable to find java getDensity method");

	auto param = (jboolean) is_multiplayer;

	jnienv->CallVoidMethod(app_global->activity->clazz, notifyConnect, param);
}

void notifyExitGame()
{
	jmethodID notifyExit = jnienv->GetMethodID(nativeActivity,
			"notifyExitGame", "()V");

	FATAL_ERROR_IF(notifyExit == nullptr,
		"porting::notifyExit unable to find java notifyExit method");

	jnienv->CallVoidMethod(app_global->activity->clazz, notifyExit);
}

#ifndef SERVER
float getDisplayDensity()
{
	static bool firstrun = true;
	static float value = 0;

	if (firstrun) {
		jmethodID getDensity = jnienv->GetMethodID(nativeActivity,
				"getDensity", "()F");

		FATAL_ERROR_IF(getDensity == nullptr,
			"porting::getDisplayDensity unable to find java getDensity method");

		value = jnienv->CallFloatMethod(app_global->activity->clazz, getDensity);
		firstrun = false;
	}
	return value;
}
#endif // ndef SERVER
}
