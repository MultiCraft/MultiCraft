/*
MultiCraft
Copyright (C) 2014-2020 MoNTE48, Maksim Gamarnik <MoNTE48@mail.ua>
Copyright (C) 2014-2020 ubulem,  Bektur Mambetov <berkut87@gmail.com>

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

package com.multicraft.game.helpers;

import android.os.Build;

public class ApiLevelHelper {
	public static boolean isGreaterOrEqual(int versionCode) {
		return Build.VERSION.SDK_INT >= versionCode;
	}

	public static boolean isJellyBeanMR1() {
		return isGreaterOrEqual(Build.VERSION_CODES.JELLY_BEAN_MR1);
	}

	public static boolean isKitkat() {
		return isGreaterOrEqual(Build.VERSION_CODES.KITKAT);
	}

	public static boolean isLollipop() {
		return isGreaterOrEqual(Build.VERSION_CODES.LOLLIPOP);
	}

	public static boolean isOreo() {
		return isGreaterOrEqual(Build.VERSION_CODES.O);
	}

	public static boolean isAndroidQ() {
		return isGreaterOrEqual(Build.VERSION_CODES.Q);
	}
}
