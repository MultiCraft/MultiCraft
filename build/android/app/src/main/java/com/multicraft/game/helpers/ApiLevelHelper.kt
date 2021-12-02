package com.multicraft.game.helpers

import android.os.Build.VERSION.SDK_INT
import android.os.Build.VERSION_CODES.*

object ApiLevelHelper {
	private fun isGreaterOrEqual(versionCode: Int) = SDK_INT >= versionCode

	fun isKitKat() = isGreaterOrEqual(KITKAT)

	fun isMarshmallow() = isGreaterOrEqual(M)

	fun isOreo() = isGreaterOrEqual(O)

	fun isAndroid11() = isGreaterOrEqual(R)
}
