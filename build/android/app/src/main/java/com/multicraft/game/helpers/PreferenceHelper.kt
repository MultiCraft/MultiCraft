/*
MultiCraft
Copyright (C) 2014-2022 MoNTE48, Maksim Gamarnik <MoNTE48@mail.ua>
Copyright (C) 2014-2022 ubulem,  Bektur Mambetov <berkut87@gmail.com>

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

package com.multicraft.game.helpers

import android.content.Context
import android.content.SharedPreferences

object PreferenceHelper {
	const val TAG_BUILD_VER = "buildVer"

	fun init(context: Context): SharedPreferences =
		context.getSharedPreferences("MultiCraftSettings", Context.MODE_PRIVATE)

	private inline fun SharedPreferences.edit(operation: (SharedPreferences.Editor) -> Unit) {
		val editor = this.edit()
		operation(editor)
		editor.apply()
	}

	operator fun SharedPreferences.set(key: String, value: Any?) = when (value) {
		is String? -> edit { it.putString(key, value) }
		is Int -> edit { it.putInt(key, value) }
		is Boolean -> edit { it.putBoolean(key, value) }
		is Float -> edit { it.putFloat(key, value) }
		is Long -> edit { it.putLong(key, value) }
		else -> throw UnsupportedOperationException("Not yet implemented")
	}

	fun SharedPreferences.getStringValue(key: String) = when (key) {
		TAG_BUILD_VER -> getString(key, "0") as String
		else -> throw UnsupportedOperationException("Not yet implemented")
	}
}
