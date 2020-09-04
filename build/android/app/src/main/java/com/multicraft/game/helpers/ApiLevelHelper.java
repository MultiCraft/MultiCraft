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

import static android.os.Build.VERSION.SDK_INT;
import static android.os.Build.VERSION_CODES.JELLY_BEAN_MR1;
import static android.os.Build.VERSION_CODES.KITKAT;
import static android.os.Build.VERSION_CODES.LOLLIPOP;
import static android.os.Build.VERSION_CODES.O;
import static android.os.Build.VERSION_CODES.Q;

public class ApiLevelHelper {
	public static boolean isGreaterOrEqual(int versionCode) {
		return SDK_INT >= versionCode;
	}

	public static boolean isJellyBeanMR1() {
		return isGreaterOrEqual(JELLY_BEAN_MR1);
	}

	public static boolean isKitkat() {
		return isGreaterOrEqual(KITKAT);
	}

	public static boolean isLollipop() {
		return isGreaterOrEqual(LOLLIPOP);
	}

	public static boolean isOreo() {
		return isGreaterOrEqual(O);
	}

	public static boolean isAndroidQ() {
		return isGreaterOrEqual(Q);
	}
}
