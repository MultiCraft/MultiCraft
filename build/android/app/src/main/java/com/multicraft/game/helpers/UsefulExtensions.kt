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

import android.app.*
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.net.ConnectivityManager
import android.net.NetworkCapabilities
import android.provider.Settings
import android.view.View
import android.view.Window
import androidx.appcompat.app.AlertDialog
import androidx.core.content.ContextCompat
import androidx.core.view.*
import com.multicraft.game.R
import com.multicraft.game.databinding.ConnDialogBinding
import com.multicraft.game.databinding.RestartDialogBinding
import com.multicraft.game.helpers.ApiLevelHelper.isAndroid12
import java.io.File
import java.io.InputStream
import kotlin.system.exitProcess

// Activity extensions
fun Activity.getIcon() = try {
	packageManager.getApplicationIcon(packageName)
} catch (e: PackageManager.NameNotFoundException) {
	ContextCompat.getDrawable(this, R.mipmap.ic_launcher)
}

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
	exitProcess(0)
}

fun Activity.defaultDialog(title: Int, view: View, style: Int = R.style.CustomDialog): AlertDialog {
	val builder = AlertDialog.Builder(this, style)
		.setIcon(getIcon())
		.setTitle(title)
		.setCancelable(false)
		.setView(view)
	return builder.create()
}

fun Activity.headlessDialog(
	view: View,
	style: Int = R.style.LightTheme,
	isCancelable: Boolean = false
): AlertDialog {
	val builder = AlertDialog.Builder(this, style)
		.setCancelable(isCancelable)
		.setView(view)
	return builder.create()
}

fun Activity.show(dialog: AlertDialog) {
	window?.makeFullScreen()
	if (!isFinishing) dialog.show()
}

fun Activity.showConnectionDialog(listener: (() -> Unit)? = null) {
	val binding = ConnDialogBinding.inflate(layoutInflater)
	val dialog = defaultDialog(R.string.conn_title, binding.root)
	binding.wifi.setOnClickListener {
		@Suppress("DEPRECATION")
		dialog.dismiss()
		startActivityForResult(
			Intent(Settings.ACTION_WIFI_SETTINGS),
			104
		)
	}
	binding.mobile.setOnClickListener {
		@Suppress("DEPRECATION")
		dialog.dismiss()
		startActivityForResult(
			Intent(Settings.ACTION_WIRELESS_SETTINGS),
			104
		)
	}
	binding.ignore.setOnClickListener {
		dialog.dismiss()
		listener?.invoke()
	}
	show(dialog)
}

fun Activity.showRestartDialog(isRestart: Boolean = true) {
	val message =
		if (isRestart) getString(R.string.restart) else getString(R.string.no_space)
	val binding = RestartDialogBinding.inflate(layoutInflater)
	val dialog = headlessDialog(binding.root, R.style.CustomDialog)
	binding.errorDesc.text = message
	binding.close.setOnClickListener { finishApp(false) }
	binding.restart.setOnClickListener { finishApp(true) }
	show(dialog)
}

fun Activity.isConnected(): Boolean {
	val cm = getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager
	if (ApiLevelHelper.isMarshmallow()) {
		val activeNetwork = cm.activeNetwork ?: return false
		val capabilities = cm.getNetworkCapabilities(activeNetwork) ?: return false
		return capabilities.hasCapability(NetworkCapabilities.NET_CAPABILITY_VALIDATED)
	} else @Suppress("DEPRECATION") {
		val activeNetworkInfo = cm.activeNetworkInfo ?: return false
		return activeNetworkInfo.isConnected
	}
}

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
