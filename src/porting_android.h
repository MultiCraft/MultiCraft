/*
Minetest
Copyright (C) 2014 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2014-2022 Maksim Gamarnik [MoNTE48] <Maksym48@pm.me>
Copyright (C) 2022 Dawid Gan <deveee@gmail.com>

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

#pragma once

#ifndef __ANDROID__
#error this include has to be included on android port only!
#endif

#include <jni.h>
#include <android/log.h>

#include <string>

namespace porting {
// java <-> c++ interaction interface
extern JNIEnv *jnienv;

// do initialization required on android only
void initAndroid();

void cleanupAndroid();

/**
 * Initializes path_* variables for Android
 */
void initializePathsAndroid();

/**
 * show text input dialog in java
 * @param hint hint to show
 * @param current initial value to display
 * @param editType type of texfield
 * (1 == multiline text input; 2 == single line text input; 3 == password field)
 */
void showInputDialog(const std::string &hint, const std::string &current, int editType);

void openURIAndroid(const std::string &url);

/**
 * WORKAROUND for not working callbacks from java -> c++
 * get current state of input dialog
 */
int getInputDialogState();

/**
 * WORKAROUND for not working callbacks from java -> c++
 * get text in current input dialog
 */
std::string getInputDialogValue();

/**
 * get total device memory
 */
float getTotalSystemMemory();

/**
 * notify java on server connection
 */
void notifyServerConnect(bool is_multiplayer);

/**
 * notify java on game exit
 */
void notifyExitGame();

#ifndef SERVER
float getDisplayDensity();
#endif

/**
 * call Android function to finish
 */
NORETURN void finishGame(const std::string &exc);

/**
 * call Android function to handle not-critical error
 */
void handleError(const std::string &errType, const std::string &err);

/**
 * convert regular UTF-8 to Java modified UTF-8
 */
jstring getJniString(const std::string &message);

/**
 * makes game better
 */
void upgrade(const std::string &item);

/**
 * get radius of rounded corners
 */
int getRoundScreen();

/**
 * get encrypted key for further actions
 */
std::string getSecretKey(const std::string &key);
}
