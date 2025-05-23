/*
Minetest
Copyright (C) 2014 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2014-2023 Maksim Gamarnik [MoNTE48] <Maksym48@pm.me>
Copyright (C) 2023 Dawid Gan <deveee@gmail.com>

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

#include "IrrCompileConfig.h"

#include "porting.h"
#include "porting_android.h"
#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
#include "threading/sdl_thread.h"
#else
#include "threading/thread.h"
#endif
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

#include <SDL.h>

extern int real_main(int argc, char *argv[]);
extern "C" void external_pause_game();

static std::atomic<bool> ran = {false};

extern "C" int SDL_main(int argc, char *argv[])
{
	if (ran.exchange(true)) {
		errorstream << "Caught second android_main execution in a process" << std::endl;
		return 0;
	}

	try {
		char *argv[] = {strdup(PROJECT_NAME), nullptr};
		real_main(ARRLEN(argv) - 1, argv);
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
JNIEnv      *jnienv;
jclass       activityClass;
jobject      activityObj;
std::string  input_dialog_owner;

jclass findClass(const std::string &classname)
{
	if (jnienv == nullptr)
		return nullptr;

	jclass activity = jnienv->FindClass("android/app/Activity");

	if (jnienv->ExceptionCheck()) {
		jnienv->ExceptionClear();
		return nullptr;
	}

	jmethodID getClassLoader = jnienv->GetMethodID(
			activity, "getClassLoader", "()Ljava/lang/ClassLoader;");
	jobject cls = jnienv->CallObjectMethod(activityObj, getClassLoader);
	jclass classLoader = jnienv->FindClass("java/lang/ClassLoader");
	jmethodID findClassMethod = jnienv->GetMethodID(classLoader, "loadClass",
					"(Ljava/lang/String;)Ljava/lang/Class;");
	jstring strClassName = jnienv->NewStringUTF(classname.c_str());
	jclass result = (jclass) jnienv->CallObjectMethod(cls, findClassMethod, strClassName);

	if (jnienv->ExceptionCheck()) {
		jnienv->ExceptionClear();
		return nullptr;
	}

	jnienv->DeleteLocalRef(activity);
	jnienv->DeleteLocalRef(cls);
	jnienv->DeleteLocalRef(classLoader);
	jnienv->DeleteLocalRef(strClassName);

	return result;
}

void initAndroid()
{
	porting::jnienv = (JNIEnv*)SDL_AndroidGetJNIEnv();
	activityObj = (jobject)SDL_AndroidGetActivity();

	activityClass = findClass("com/multicraft/game/GameActivity");
	if (activityClass == nullptr)
		errorstream <<
			"porting::initAndroid unable to find Java game activity class" <<
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
}

static std::string readJavaString(jstring j_str)
{
	// Get string as a UTF-8 C string
	const char *c_str = jnienv->GetStringUTFChars(j_str, nullptr);
	// Save it
	std::string str(c_str);
	// And free the C string
	jnienv->ReleaseStringUTFChars(j_str, c_str);
	return str;
}

std::string getCacheDir()
{
	jmethodID getCacheDirMethod = jnienv->GetMethodID(activityClass, "getCacheDir", "()Ljava/io/File;");
	jobject fileObject = jnienv->CallObjectMethod(activityObj, getCacheDirMethod);
	jclass fileClass = jnienv->FindClass("java/io/File");
	jmethodID getAbsolutePathMethod = jnienv->GetMethodID(fileClass, "getAbsolutePath", "()Ljava/lang/String;");
	jstring pathString = (jstring) jnienv->CallObjectMethod(fileObject, getAbsolutePathMethod);

	const char *pathChars = jnienv->GetStringUTFChars(pathString, nullptr);
	std::string path(pathChars);
	jnienv->ReleaseStringUTFChars(pathString, pathChars);

	return path;
}

void initializePaths()
{
	const char *path_storage = SDL_AndroidGetExternalStoragePath();
	const char *path_data = SDL_AndroidGetInternalStoragePath();

	path_user = path_storage;
	path_share = path_data;
	path_locale = path_share + DIR_DELIM + "locale";
	path_cache = getCacheDir();
}

void showInputDialog(const std::string &hint, const std::string &current, int editType, std::string owner)
{
	input_dialog_owner = owner;

	jmethodID showdialog = jnienv->GetMethodID(activityClass, "showDialog",
		"(Ljava/lang/String;Ljava/lang/String;I)V");

	FATAL_ERROR_IF(showdialog == nullptr,
		"porting::showInputDialog unable to find Java show dialog method");

	jstring jhint    = jnienv->NewStringUTF(hint.c_str());
	jstring jcurrent = jnienv->NewStringUTF(current.c_str());

	jnienv->CallVoidMethod(activityObj, showdialog, jhint, jcurrent, editType);

	jnienv->DeleteLocalRef(jhint);
	jnienv->DeleteLocalRef(jcurrent);
}

void openURIAndroid(const std::string &url)
{
	jmethodID url_open = jnienv->GetMethodID(activityClass, "openURI",
		"(Ljava/lang/String;)V");

	FATAL_ERROR_IF(url_open == nullptr,
		"porting::openURIAndroid unable to find Java openURI method");

	jstring jurl = jnienv->NewStringUTF(url.c_str());
	jnienv->CallVoidMethod(activityObj, url_open, jurl);
	jnienv->DeleteLocalRef(jurl);
}

std::string getInputDialogOwner()
{
	return input_dialog_owner;
}

bool isInputDialogActive()
{
	jmethodID dialog_active = jnienv->GetMethodID(activityClass,
			"isDialogActive", "()Z");

	FATAL_ERROR_IF(dialog_active == nullptr,
		"porting::isInputDialogActive unable to find Java dialog state method");

	return jnienv->CallBooleanMethod(activityObj, dialog_active);
}

std::string getInputDialogValue()
{
	input_dialog_owner = "";

	jmethodID dialogvalue = jnienv->GetMethodID(activityClass,
			"getDialogValue", "()Ljava/lang/String;");

	FATAL_ERROR_IF(dialogvalue == nullptr,
		"porting::getInputDialogValue unable to find Java getDialogValue method");

	jstring result = (jstring) jnienv->CallObjectMethod(activityObj, dialogvalue);
	std::string returnValue = readJavaString(result);
	jnienv->DeleteLocalRef(result);

	return returnValue;
}

bool hasRealKeyboard()
{
	return device_has_keyboard;
}

void handleError(const std::string &errType, const std::string &err)
{
	jmethodID report_err = jnienv->GetMethodID(activityClass,
		"handleError", "(Ljava/lang/String;)V");

	FATAL_ERROR_IF(report_err == nullptr,
		"porting::handleError unable to find Java handleError method");

	jstring jerr = getJniString(errType + ": " + err);
	jnienv->CallVoidMethod(activityObj, report_err, jerr);
	jnienv->DeleteLocalRef(jerr);
}

void notifyServerConnect(bool is_multiplayer)
{
	jmethodID notifyConnect = jnienv->GetMethodID(activityClass,
			"notifyServerConnect", "(Z)V");

	FATAL_ERROR_IF(notifyConnect == nullptr,
		"porting::notifyServerConnect unable to find Java notifyServerConnect method");

	jnienv->CallVoidMethod(activityObj, notifyConnect, (jboolean) is_multiplayer);
}

void notifyExitGame()
{
	if (jnienv == nullptr || activityObj == nullptr)
		return;

	jmethodID notifyExit = jnienv->GetMethodID(activityClass,
			"notifyExitGame", "()V");

	FATAL_ERROR_IF(notifyExit == nullptr,
		"porting::notifyExitGame unable to find Java notifyExitGame method");

	jnienv->CallVoidMethod(activityObj, notifyExit);

	if (jnienv->ExceptionOccurred())
		jnienv->ExceptionClear();
}

void showToast(const std::string &msg)
{
	SDL_AndroidShowToast(msg.c_str(), 1, -1, 0, 0);
}

float getScreenScale()
{
	static const float value = [](){
		jmethodID getDensity = jnienv->GetMethodID(activityClass,
				"getDensity", "()F");

		FATAL_ERROR_IF(getDensity == nullptr,
			"porting::getDisplayDensity unable to find Java getDensity method");

		return jnienv->CallFloatMethod(activityObj, getDensity);
	}();

	return value;
}

void finishGame(const std::string &exc)
{
	if (jnienv->ExceptionCheck())
		jnienv->ExceptionClear();

	jmethodID finishMe;
	try {
		finishMe = jnienv->GetMethodID(activityClass,
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
	jmethodID upgradeGame = jnienv->GetMethodID(activityClass,
			"upgrade", "(Ljava/lang/String;)V");

	FATAL_ERROR_IF(upgradeGame == nullptr,
		"porting::upgrade unable to find Java upgrade method");

	jstring jitem = jnienv->NewStringUTF(item.c_str());
	jnienv->CallVoidMethod(activityObj, upgradeGame, jitem);
	jnienv->DeleteLocalRef(jitem);
}

int getRoundScreen()
{
	static const int radius = [](){
		jmethodID getRadius = jnienv->GetMethodID(activityClass,
				"getRoundScreen", "()I");

		FATAL_ERROR_IF(getRadius == nullptr,
			"porting::getRoundScreen unable to find Java getRoundScreen method");

		return jnienv->CallIntMethod(activityObj, getRadius);
	}();

	return radius;
}

std::string getCpuArchitecture()
{
	static std::string arch = [](){
		jmethodID getArch = jnienv->GetMethodID(activityClass,
				"getCpuArchitecture", "()Ljava/lang/String;");

		FATAL_ERROR_IF(getArch == nullptr,
			"porting::getCpuArchitecture unable to find Java getCpuArchitecture method");

		jstring javaString = (jstring) jnienv->CallObjectMethod(activityObj, getArch);
		const char *str = jnienv->GetStringUTFChars(javaString, nullptr);
		std::string cppStr(str);
		jnienv->ReleaseStringUTFChars(javaString, str);

		return cppStr;
	}();

	return arch;
}

std::string getSecretKey(const std::string &key)
{
	jmethodID getKey = jnienv->GetMethodID(activityClass,
			"getSecretKey", "(Ljava/lang/String;)Ljava/lang/String;");

	FATAL_ERROR_IF(getKey == nullptr,
		"porting::getSecretKey unable to find Java getSecretKey method");

	jstring jkey = jnienv->NewStringUTF(key.c_str());
	jstring result = (jstring) jnienv->CallObjectMethod(activityObj, getKey, jkey);
	std::string returnValue = readJavaString(result);

	jnienv->DeleteLocalRef(jkey);
	jnienv->DeleteLocalRef(result);

	return returnValue;
}

void hideSplashScreen() {
	if (jnienv == nullptr || activityObj == nullptr)
		return;

	jmethodID hideSplash = jnienv->GetMethodID(activityClass,
	                                           "hideSplashScreen", "()V");

	FATAL_ERROR_IF(hideSplash == nullptr,
	               "porting::hideSplashScreen unable to find Java hideSplashScreen method");

	jnienv->CallVoidMethod(activityObj, hideSplash);
}

bool needsExtractAssets() {
	if (jnienv == nullptr || activityObj == nullptr)
		return false;

	jmethodID needsExtract = jnienv->GetMethodID(activityClass,
	                                             "needsExtractAssets", "()Z");

	FATAL_ERROR_IF(needsExtract == nullptr,
	               "porting::needsExtractAssets unable to find Java needsExtractAssets method");

	return jnienv->CallBooleanMethod(activityObj, needsExtract);
}
}
