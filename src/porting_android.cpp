/*
Minetest
Copyright (C) 2014 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2014-2022 Maksim Gamarnik [MoNTE48] Maksym48@pm.me

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

#include <atomic>
#include <sstream>
#include <exception>
#include <cstdlib>

#ifdef GPROF
#include "prof.h"
#endif

extern int main(int argc, char *argv[]);
extern "C" void external_pause_game();

static std::atomic<bool> ran = {false};

void android_main(android_app *app)
{
	if (ran.exchange(true)) {
		errorstream << "Caught second android_main execution in a process" << std::endl;
		return;
	}

	porting::app_global = app;

	Thread::setName("Main");

	try {
		char *argv[] = {strdup(PROJECT_NAME), nullptr};
		main(ARRLEN(argv) - 1, argv);
		free(argv[0]);
	} catch (std::exception &e) {
		errorstream << "Uncaught exception in main thread: " << e.what() << std::endl;
		porting::finishGame(e.what());
	} catch (...) {
		errorstream << "Uncaught exception in main thread!" << std::endl;
		porting::finishGame("Unknown error");
	}

	infostream << "Shutting down." << std::endl;
	porting::cleanupAndroid();
	_Exit(0);
}

/**
 * Handler for finished message box input
 * Intentionally NOT in namespace porting
 * ToDo: this doesn't work as expected, there's a workaround for it right now
 */
extern "C" {
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
jobject      activityObj;

jclass findClass(const std::string &classname)
{
	if (jnienv == nullptr)
		return nullptr;

	jclass nativeactivity = jnienv->FindClass("android/app/NativeActivity");

	if (nativeactivity == nullptr)
		finishGame("findClass");

	jmethodID getClassLoader = jnienv->GetMethodID(
			nativeactivity, "getClassLoader", "()Ljava/lang/ClassLoader;");
	jobject cls = jnienv->CallObjectMethod(activityObj, getClassLoader);
	jclass classLoader = jnienv->FindClass("java/lang/ClassLoader");
	jmethodID findClass = jnienv->GetMethodID(classLoader, "loadClass",
					"(Ljava/lang/String;)Ljava/lang/Class;");
	jstring strClassName = jnienv->NewStringUTF(classname.c_str());
	jclass result = (jclass) jnienv->CallObjectMethod(cls, findClass, strClassName);

	if (jnienv->ExceptionCheck()) {
		jnienv->ExceptionClear();
		return nullptr;
	}

	return result;
}

void initAndroid()
{
	porting::jnienv = nullptr;
	JavaVM *jvm = app_global->activity->vm;
	JavaVMAttachArgs lJavaVMAttachArgs;
	lJavaVMAttachArgs.version = JNI_VERSION_1_6;
	lJavaVMAttachArgs.name = PROJECT_NAME_C "NativeThread";
	lJavaVMAttachArgs.group = nullptr;
	activityObj = app_global->activity->clazz;

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
	ANativeActivity_finish(porting::app_global->activity);
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
	std::string path_data = getAndroidPath(nativeActivity, activityObj, mt_getAbsPath,
				"getFilesDir");

	path_user    = path_storage + DIR_DELIM + "Android/data/com.multicraft.game/files";
	path_share   = path_data;
	path_locale  = path_data + DIR_DELIM + "locale";
	path_cache   = getAndroidPath(nativeActivity,
			activityObj, mt_getAbsPath, "getCacheDir");
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

	jnienv->CallVoidMethod(activityObj, showdialog,
			jacceptButton, jhint, jcurrent, jeditType);
}

void openURIAndroid(const std::string &url)
{
	jmethodID url_open = jnienv->GetMethodID(nativeActivity, "openURI",
		"(Ljava/lang/String;)V");

	FATAL_ERROR_IF(url_open == nullptr,
		"porting::openURIAndroid unable to find java openURI method");

	jstring jurl = jnienv->NewStringUTF(url.c_str());
	jnienv->CallVoidMethod(activityObj, url_open, jurl);
}

int getInputDialogState()
{
	jmethodID dialogstate = jnienv->GetMethodID(nativeActivity,
			"getDialogState", "()I");

	FATAL_ERROR_IF(dialogstate == nullptr,
		"porting::getInputDialogState unable to find java dialog state method");

	return jnienv->CallIntMethod(activityObj, dialogstate);
}

std::string getInputDialogValue()
{
	jmethodID dialogvalue = jnienv->GetMethodID(nativeActivity,
			"getDialogValue", "()Ljava/lang/String;");

	FATAL_ERROR_IF(dialogvalue == nullptr,
		"porting::getInputDialogValue unable to find java dialog value method");

	jobject result = jnienv->CallObjectMethod(activityObj, dialogvalue);

	const char *javachars = jnienv->GetStringUTFChars((jstring) result, nullptr);
	std::string text(javachars);
	jnienv->ReleaseStringUTFChars((jstring) result, javachars);

	return text;
}

float getTotalSystemMemory()
{
	long pages = sysconf(_SC_PHYS_PAGES);
	long page_size = sysconf(_SC_PAGE_SIZE);
	int divisor = 1024 * 1024 * 1024;
	return (float) (pages * page_size) / (float) divisor;
}

bool hasRealKeyboard()
{
	return device_has_keyboard;
}

void handleError(const std::string &errType, const std::string &err)
{
	jmethodID report_err = jnienv->GetMethodID(nativeActivity,
		"handleError","(Ljava/lang/String;)V");

	FATAL_ERROR_IF(report_err == nullptr,
		"porting::handleError unable to find java handleError method");

	std::string errorMessage = errType + ": " + err;
	jstring jerr = porting::getJniString(errorMessage);
	jnienv->CallVoidMethod(activityObj, report_err, jerr);
}

void notifyServerConnect(bool is_multiplayer)
{
	jmethodID notifyConnect = jnienv->GetMethodID(nativeActivity,
			"notifyServerConnect", "(Z)V");

	FATAL_ERROR_IF(notifyConnect == nullptr,
		"porting::notifyServerConnect unable to find java notifyServerConnect method");

	auto param = (jboolean) is_multiplayer;

	jnienv->CallVoidMethod(activityObj, notifyConnect, param);
}

void notifyExitGame()
{
	if (jnienv == nullptr || activityObj == nullptr)
		return;

	jclass _nativeActivity = findClass("com/multicraft/game/GameActivity");
	FATAL_ERROR_IF(_nativeActivity == nullptr,
	               "porting::notifyExitGame unable to find java native activity class");

	jmethodID notifyExit = jnienv->GetMethodID(_nativeActivity, "notifyExitGame", "()V");
	FATAL_ERROR_IF(notifyExit == nullptr,
	               "porting::notifyExitGame unable to find java notifyExit method");

	jnienv->CallVoidMethod(activityObj, notifyExit);

	if (jnienv->ExceptionOccurred())
		jnienv->ExceptionClear();
}

#ifndef SERVER
float getDisplayDensity()
{
	static bool firstRun = true;
	static float value = 0;

	if (firstRun) {
		jmethodID getDensity = jnienv->GetMethodID(nativeActivity,
				"getDensity", "()F");

		FATAL_ERROR_IF(getDensity == nullptr,
			"porting::getDisplayDensity unable to find java getDensity method");

		value = jnienv->CallFloatMethod(activityObj, getDensity);
		firstRun = false;
	}
	return value;
}
#endif // ndef SERVER

void finishGame(const std::string &exc)
{
	if (jnienv->ExceptionCheck())
		jnienv->ExceptionClear();

	jmethodID finishMe;
	try {
		finishMe = jnienv->GetMethodID(nativeActivity,
			"finishGame", "(Ljava/lang/String;)V");
	} catch (...) {
		exit(-1);
	}

	// Don't use `FATAL_ERROR_IF` to avoid creating a loop
	if (finishMe == nullptr)
		exit(-1);

	jstring jexc = jnienv->NewStringUTF(exc.c_str());
	jnienv->CallVoidMethod(activityObj, finishMe, jexc);
	exit(0);
}

jstring getJniString(const std::string &message)
{
	int byteCount = message.length();
	const jbyte *pNativeMessage = (const jbyte*) message.c_str();
	jbyteArray bytes = jnienv->NewByteArray(byteCount);
	jnienv->SetByteArrayRegion(bytes, 0, byteCount, pNativeMessage);

	jclass charsetClass = findClass("java/nio/charset/Charset");
	jmethodID forName = jnienv->GetStaticMethodID(
			charsetClass, "forName", "(Ljava/lang/String;)Ljava/nio/charset/Charset;");
	jstring utf8 = jnienv->NewStringUTF("UTF-8");
	jobject charset = jnienv->CallStaticObjectMethod(charsetClass, forName, utf8);

	jclass stringClass = findClass("java/lang/String");
	jmethodID ctor = jnienv->GetMethodID(
			stringClass, "<init>", "([BLjava/nio/charset/Charset;)V");

	jstring jMessage = (jstring) jnienv->NewObject(stringClass, ctor, bytes, charset);

	return jMessage;
}

void upgrade(const std::string &item)
{
	jmethodID upgradeGame = jnienv->GetMethodID(nativeActivity,
			"upgrade","(Ljava/lang/String;)V");

	FATAL_ERROR_IF(upgradeGame == nullptr,
		"porting::upgradeGame unable to find java upgrade method");

	jstring jitem = jnienv->NewStringUTF(item.c_str());
	jnienv->CallVoidMethod(activityObj, upgradeGame, jitem);
}

std::string getSecretKey(const std::string &key)
{
	jmethodID getKey = jnienv->GetMethodID(nativeActivity,
				"getSecretKey","(Ljava/lang/String;)Ljava/lang/String;");

	FATAL_ERROR_IF(getKey == nullptr,
				   "porting::getSecretKey unable to find java getSecretKey method");

	jstring jkey = jnienv->NewStringUTF(key.c_str());
	auto result = (jstring) jnienv->CallObjectMethod(activityObj, getKey, jkey);
	return javaStringToUTF8(result);

int getRoundScreen()
{
	static bool firstRun = true;
	static int radius = 0;

	if (firstRun) {
		jmethodID getRadius = jnienv->GetMethodID(nativeActivity,
				"getRoundScreen", "()I");

		FATAL_ERROR_IF(getRadius == nullptr,
			"porting::getRadius unable to find java getRoundScreen method");

		radius = jnienv->CallIntMethod(activityObj, getRadius);
		firstRun = false;
	}
	return radius;

}
}
