/*
MultiCraft
Copyright (C) 2014-2021 MoNTE48, Maksim Gamarnik <MoNTE48@mail.ua>
Copyright (C) 2014-2021 ubulem,  Bektur Mambetov <berkut87@gmail.com>

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
import android.app.PendingIntent.FLAG_CANCEL_CURRENT
import android.app.PendingIntent.FLAG_IMMUTABLE
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Bitmap
import android.graphics.drawable.BitmapDrawable
import android.net.ConnectivityManager
import android.net.NetworkCapabilities.NET_CAPABILITY_VALIDATED
import android.view.Window
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat.Type.navigationBars
import androidx.core.view.WindowInsetsCompat.Type.statusBars
import androidx.core.view.WindowInsetsControllerCompat
import androidx.core.view.WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
import com.multicraft.game.MainActivity
import com.multicraft.game.R
import com.multicraft.game.databinding.RestartDialogBinding
import com.multicraft.game.helpers.ApiLevelHelper.isAndroid12
import com.multicraft.game.helpers.ApiLevelHelper.isMarshmallow
import com.multicraft.game.helpers.ApiLevelHelper.isOreo
import com.multicraft.game.helpers.PreferenceHelper.TAG_SHORTCUT_EXIST
import com.multicraft.game.helpers.PreferenceHelper.set
import java.io.File
import java.io.InputStream
import kotlin.system.exitProcess

object Utilities {

	fun makeFullScreen(window: Window) {
		WindowCompat.setDecorFitsSystemWindows(window, false)
		WindowInsetsControllerCompat(window, window.decorView).let {
			it.hide(statusBars() or navigationBars())
			it.systemBarsBehavior = BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
		}
	}

	fun getIcon(activity: Activity) = try {
		activity.packageManager.getApplicationIcon(activity.packageName)
	} catch (e: PackageManager.NameNotFoundException) {
		ContextCompat.getDrawable(activity, R.mipmap.ic_launcher)
	}

	@Suppress("DEPRECATION")
	fun addShortcut(activity: AppCompatActivity) {
		if (isOreo()) return
		val activityManager = activity.getSystemService(Context.ACTIVITY_SERVICE) as ActivityManager
		val size = activityManager.launcherLargeIconSize
		var shortcutIconBitmap = (getIcon(activity) as BitmapDrawable).bitmap
		if (shortcutIconBitmap.width != size || shortcutIconBitmap.height != size)
			shortcutIconBitmap = Bitmap.createScaledBitmap(shortcutIconBitmap, size, size, true)
		val shortcutIntent = Intent(activity, MainActivity::class.java)
		shortcutIntent.action = Intent.ACTION_MAIN
		val addIntent = Intent()
		addIntent.putExtra("duplicate", false)
		addIntent.putExtra(Intent.EXTRA_SHORTCUT_INTENT, shortcutIntent)
		addIntent.putExtra(Intent.EXTRA_SHORTCUT_NAME, R.string.app_name)
		addIntent.putExtra(Intent.EXTRA_SHORTCUT_ICON, shortcutIconBitmap)
		addIntent.action = "com.android.launcher.action.INSTALL_SHORTCUT"
		activity.applicationContext.sendBroadcast(addIntent)
		// save preference
		PreferenceHelper.init(activity)[TAG_SHORTCUT_EXIST] = true
	}

	fun finishApp(restart: Boolean, activity: Activity) {
		if (restart) {
			val intent = Intent(activity, activity::class.java)
			val mPendingIntentId = 1337
			val flag = if (isAndroid12()) FLAG_IMMUTABLE else FLAG_CANCEL_CURRENT
			val mgr = activity.getSystemService(Context.ALARM_SERVICE) as AlarmManager
			mgr.set(
				AlarmManager.RTC, System.currentTimeMillis(), PendingIntent.getActivity(
					activity, mPendingIntentId, intent, flag
				)
			)
		}
		exitProcess(0)
	}

	fun showRestartDialog(activity: Activity, isRestart: Boolean = true) {
		val message =
			if (isRestart) activity.getString(R.string.restart) else activity.getString(R.string.no_space)
		val builder = AlertDialog.Builder(activity)
		builder.setIcon(getIcon(activity))
		val binding = RestartDialogBinding.inflate(activity.layoutInflater)
		builder.setView(binding.root)
		val dialog = builder.create()
		binding.errorDesc.text = message
		binding.close.setOnClickListener {
			dialog.dismiss()
			finishApp(!isRestart, activity)
		}
		binding.restart.setOnClickListener { finishApp(isRestart, activity) }
		dialog.window?.setBackgroundDrawableResource(R.drawable.custom_dialog_rounded_daynight)
		makeFullScreen(dialog.window!!)
		dialog.show()
	}

	fun File.copyInputStreamToFile(inputStream: InputStream) =
		this.outputStream().use { fileOut -> inputStream.copyTo(fileOut, 8192) }

	fun isConnected(context: Context): Boolean {
		val cm = context.getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager
		if (isMarshmallow()) {
			val activeNetwork = cm.activeNetwork ?: return false
			val capabilities = cm.getNetworkCapabilities(activeNetwork) ?: return false
			return capabilities.hasCapability(NET_CAPABILITY_VALIDATED)
		} else @Suppress("DEPRECATION") {
			val activeNetworkInfo = cm.activeNetworkInfo ?: return false
			return activeNetworkInfo.isConnected
		}
	}
}
