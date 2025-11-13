/*
Minetest
Copyright (C) 2014 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2014-2025 Maksim Gamarnik [MoNTE48] <Maksym48@pm.me>
Copyright (C) 2023-2025 Dawid Gan <deveee@gmail.com>

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

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

extern int real_main(int argc, char *argv[]);
extern "C" void external_pause_game();
extern "C" void external_update(const char* key, const char* value);

static std::atomic<bool> ran = {false};

int main(int argc, char *argv[])
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
	JNIEXPORT void JNICALL Java_com_multicraft_game_GameActivity_update(
		JNIEnv *env, jclass clazz, jstring key, jstring value)
	{
		const std::string key_str = porting::readJavaString(key);
		const std::string value_str = porting::readJavaString(value);
		external_update(key_str.c_str(), value_str.c_str());
	}
}

namespace porting {
JNIEnv      *jnienv;
jclass       activityClass;
jobject      activityObj;
std::string  input_dialog_owner;
AAssetManager *asset_manager = NULL;
jobject java_asset_manager_ref = 0;

void initAndroid()
{
	porting::jnienv = (JNIEnv*)SDL_GetAndroidJNIEnv();
	activityObj = (jobject)SDL_GetAndroidActivity();

	activityClass = jnienv->GetObjectClass(activityObj);
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

void initializePaths()
{
	const char *path_storage = SDL_GetAndroidExternalStoragePath();
	const char *path_data = SDL_GetAndroidInternalStoragePath();
	const char *path_acache = SDL_GetAndroidCachePath(); // not a typo

	path_user = path_storage;
	path_share = path_data;
	path_locale = path_share + DIR_DELIM + "locale";
	path_cache = path_acache;
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

bool openURIAndroid(const std::string &url, bool untrusted)
{
	jmethodID url_open = jnienv->GetMethodID(activityClass, "openURI",
		"(Ljava/lang/String;Z)Z");

	FATAL_ERROR_IF(url_open == nullptr,
		"porting::openURIAndroid unable to find Java openURI method");

	jstring jurl = jnienv->NewStringUTF(url.c_str());
	jboolean result = jnienv->CallBooleanMethod(activityObj, url_open, jurl, (jboolean) untrusted);
	jnienv->DeleteLocalRef(jurl);

	return result == JNI_TRUE;
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
	return isGooglePC() || device_has_keyboard;
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
	const int ver = SDL_GetAndroidSDKVersion();
	if (ver >= 33 && msg == "Copied to clipboard")
		return;

	SDL_ShowAndroidToast(msg.c_str(), 1, -1, 0, 0);
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

	jclass charsetClass = jnienv->FindClass("java/nio/charset/Charset");
	jmethodID forName = jnienv->GetStaticMethodID(
			charsetClass, "forName", "(Ljava/lang/String;)Ljava/nio/charset/Charset;");
	jstring utf8 = jnienv->NewStringUTF("UTF-8");
	jobject charset = jnienv->CallStaticObjectMethod(charsetClass, forName, utf8);

	jclass stringClass = jnienv->FindClass("java/lang/String");
	jmethodID ctor = jnienv->GetMethodID(
			stringClass, "<init>", "([BLjava/nio/charset/Charset;)V");

	jstring jMessage = (jstring) jnienv->NewObject(stringClass, ctor, bytes, charset);

	jnienv->DeleteLocalRef(bytes);
	jnienv->DeleteLocalRef(utf8);
	jnienv->DeleteLocalRef(charsetClass);
	jnienv->DeleteLocalRef(stringClass);

	return jMessage;
}

bool upgrade(const std::string &item)
{
	jmethodID upgradeGame = jnienv->GetMethodID(activityClass,
			"upgrade", "(Ljava/lang/String;)Z");

	FATAL_ERROR_IF(upgradeGame == nullptr,
		"porting::upgrade unable to find Java upgrade method");

	jstring jitem = jnienv->NewStringUTF(item.c_str());
	jboolean res = jnienv->CallBooleanMethod(activityObj, upgradeGame, jitem);
	jnienv->DeleteLocalRef(jitem);

	return res == JNI_TRUE;
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

bool isGooglePC()
{
	if (jnienv == nullptr || activityObj == nullptr)
		return false;

	static const bool value = [](){
		jmethodID googlePC = jnienv->GetMethodID(activityClass,
				"getPackageManager", "()Landroid/content/pm/PackageManager;");

		if (googlePC == nullptr) {
			errorstream << "porting::isGooglePC unable to find Java getPackageManager method" << std::endl;
			return false;
		}

		jobject pm = jnienv->CallObjectMethod(activityObj, googlePC);
		jclass pmCls = jnienv->GetObjectClass(pm);
		jmethodID hasFeat = jnienv->GetMethodID(pmCls, "hasSystemFeature", "(Ljava/lang/String;)Z");
		jstring feat = jnienv->NewStringUTF("com.google.android.play.feature.HPE_EXPERIENCE");
		jboolean result = jnienv->CallBooleanMethod(pm, hasFeat, feat);

		jnienv->DeleteLocalRef(feat);
		jnienv->DeleteLocalRef(pmCls);
		jnienv->DeleteLocalRef(pm);

		return result == JNI_TRUE;
	}();

	return value;
}

void hideSplashScreen()
{
	if (jnienv == nullptr || activityObj == nullptr)
		return;

	jmethodID hideSplash = jnienv->GetMethodID(activityClass,
			"hideSplashScreen", "()V");

	FATAL_ERROR_IF(hideSplash == nullptr,
		"porting::hideSplashScreen unable to find Java hideSplashScreen method");

	jnienv->CallVoidMethod(activityObj, hideSplash);
}

bool needsExtractAssets()
{
	if (jnienv == nullptr || activityObj == nullptr)
		return false;

	jmethodID needsExtract = jnienv->GetMethodID(activityClass,
			"needsExtractAssets", "()Z");

	FATAL_ERROR_IF(needsExtract == nullptr,
		"porting::needsExtractAssets unable to find Java needsExtractAssets method");

	return jnienv->CallBooleanMethod(activityObj, needsExtract);
}

bool createAssetManager()
{
	jmethodID midGetContext = jnienv->GetStaticMethodID(activityClass,
			"getContext", "()Landroid/content/Context;");

	jobject context = jnienv->CallStaticObjectMethod(activityClass, midGetContext);

	jmethodID mid = jnienv->GetMethodID(jnienv->GetObjectClass(context),
			"getAssets", "()Landroid/content/res/AssetManager;");
	jobject javaAssetManager = jnienv->CallObjectMethod(context, mid);

	java_asset_manager_ref = jnienv->NewGlobalRef(javaAssetManager);
	asset_manager = AAssetManager_fromJava(jnienv, java_asset_manager_ref);

	if (!asset_manager) {
		jnienv->DeleteGlobalRef(java_asset_manager_ref);
		return false;
	}

	return true;
}

void destroyAssetManager()
{
	if (asset_manager) {
		jnienv->DeleteGlobalRef(java_asset_manager_ref);
		asset_manager = NULL;
	}
}
}
