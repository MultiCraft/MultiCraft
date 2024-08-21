/*
MultiCraft
Copyright (C) 2014-2024 MoNTE48, Maksim Gamarnik <Maksym48@pm.me>
Copyright (C) 2014-2024 ubulem,  Bektur Mambetov <berkut87@gmail.com>

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

import android.os.Build.VERSION.SDK_INT
import android.os.Build.VERSION_CODES.*

object ApiLevelHelper {
	private fun isGreaterOrEqual(versionCode: Int) = SDK_INT >= versionCode

	fun isMarshmallow() = isGreaterOrEqual(M)

	fun isOreo() = isGreaterOrEqual(O)

	fun isPie() = isGreaterOrEqual(P)

	fun isAndroid12() = isGreaterOrEqual(S)

	fun isAndroid14() = isGreaterOrEqual(UPSIDE_DOWN_CAKE)
}
