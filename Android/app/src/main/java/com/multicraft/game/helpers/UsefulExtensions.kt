/*
MultiCraft
Copyright (C) 2014-2023 MoNTE48, Maksim Gamarnik <Maksym48@pm.me>
Copyright (C) 2014-2023 ubulem,  Bektur Mambetov <berkut87@gmail.com>

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

import android.app.*
import android.content.Context
import android.content.Intent
import android.content.res.Configuration
import android.net.*
import android.os.*
import android.view.Window
import androidx.activity.result.ActivityResultLauncher
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.*
import com.multicraft.game.*
import com.multicraft.game.databinding.*
import com.multicraft.game.dialogs.RestartDialog
import com.multicraft.game.helpers.ApiLevelHelper.isAndroid12
import com.multicraft.game.helpers.ApiLevelHelper.isMarshmallow
import java.io.*
import java.util.*

// Activity extensions
fun Activity.finishApp(restart: Boolean) {
	if (restart) {
		val intent = Intent(this, this::class.java)
		val mPendingIntentId = 1337
		val flag =
			if (isAndroid12()) PendingIntent.FLAG_IMMUTABLE else PendingIntent.FLAG_CANCEL_CURRENT
		val mgr = getSystemService(Context.ALARM_SERVICE) as AlarmManager
		mgr.set(
			AlarmManager.RTC, System.currentTimeMillis(), PendingIntent.getActivity(
				this, mPendingIntentId, intent, flag
			)
		)
	}
	finish()
}

fun Activity.isConnected(): Boolean {
	val cm = getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager
	if (isMarshmallow()) {
		val activeNetwork = cm.activeNetwork ?: return false
		val capabilities = cm.getNetworkCapabilities(activeNetwork) ?: return false
		return capabilities.hasCapability(NetworkCapabilities.NET_CAPABILITY_VALIDATED)
	} else @Suppress("DEPRECATION") {
		val activeNetworkInfo = cm.activeNetworkInfo ?: return false
		return activeNetworkInfo.isConnected
	}
}

fun AppCompatActivity.showRestartDialog(
	startForResult: ActivityResultLauncher<Intent>,
	isRestart: Boolean = true
) {
	val message =
		if (isRestart) getString(R.string.restart) else getString(R.string.no_space)
	val intent = Intent(this, RestartDialog::class.java)
	intent.putExtra("message", message)
	startForResult.launch(intent)
}

fun Activity.hasHardKeyboard() =
	resources.configuration.hardKeyboardHidden == Configuration.HARDKEYBOARDHIDDEN_NO

// Other extensions
fun File.copyInputStreamToFile(inputStream: InputStream) =
	outputStream().use { fileOut -> inputStream.copyTo(fileOut, 8192) }

fun Window.makeFullScreen() {
	WindowCompat.setDecorFitsSystemWindows(this, false)
	WindowInsetsControllerCompat(this, decorView).let {
		it.hide(WindowInsetsCompat.Type.statusBars() or WindowInsetsCompat.Type.navigationBars())
		it.systemBarsBehavior =
			WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
	}
}

fun Window.makeFullScreenAlert() {
	WindowInsetsControllerCompat(this, decorView).let {
		it.hide(WindowInsetsCompat.Type.statusBars() or WindowInsetsCompat.Type.navigationBars())
		it.systemBarsBehavior =
			WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
	}
}
