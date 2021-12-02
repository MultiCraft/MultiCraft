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

import android.annotation.SuppressLint
import android.app.Activity
import android.app.ActivityManager
import android.app.AlarmManager
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Bitmap
import android.graphics.drawable.BitmapDrawable
import android.net.ConnectivityManager
import android.net.NetworkCapabilities.NET_CAPABILITY_VALIDATED
import android.view.View
import android.view.Window
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat.Type.navigationBars
import androidx.core.view.WindowInsetsCompat.Type.statusBars
import androidx.core.view.WindowInsetsControllerCompat
import androidx.core.view.WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
import com.multicraft.game.MainActivity
import com.multicraft.game.R
import com.multicraft.game.helpers.ApiLevelHelper.isLollipop
import com.multicraft.game.helpers.ApiLevelHelper.isMarshmallow
import com.multicraft.game.helpers.ApiLevelHelper.isOreo
import com.multicraft.game.helpers.PreferenceHelper.TAG_SHORTCUT_EXIST
import com.multicraft.game.helpers.PreferenceHelper.set
import java.io.File
import java.io.InputStream
import kotlin.math.roundToInt
import kotlin.system.exitProcess

object Utilities {

	@JvmStatic
	fun getTotalMem(context: Context): Float {
		val actManager = context.getSystemService(Context.ACTIVITY_SERVICE) as ActivityManager
		val memInfo = ActivityManager.MemoryInfo()
		actManager.getMemoryInfo(memInfo)
		var memory = memInfo.totalMem * 1.0f / (1024 * 1024 * 1024)
		memory = (memory * 100).roundToInt() / 100.0f
		return memory
	}

	@JvmStatic
	fun makeFullScreen(window: Window) {
		if (isLollipop()) {
			WindowCompat.setDecorFitsSystemWindows(window, false)
			WindowInsetsControllerCompat(window, window.decorView).let {
				it.hide(statusBars() or navigationBars())
				it.systemBarsBehavior = BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
			}
		} else @Suppress("DEPRECATION") {
			val decor = View.SYSTEM_UI_FLAG_HIDE_NAVIGATION or
					View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY or
					View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
			window.decorView.systemUiVisibility = decor
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

	@JvmStatic
	fun finishApp(restart: Boolean, activity: Activity) {
		if (restart) @SuppressLint("UnspecifiedImmutableFlag") {
			val intent = Intent(activity, activity::class.java)
			val mPendingIntentId = 1337
			val mgr = activity.getSystemService(Context.ALARM_SERVICE) as AlarmManager
			mgr.set(
				AlarmManager.RTC, System.currentTimeMillis(), PendingIntent.getActivity(
					activity, mPendingIntentId, intent, PendingIntent.FLAG_CANCEL_CURRENT
				)
			)
		}
		exitProcess(0)
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
