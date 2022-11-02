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

package com.multicraft.game

import android.content.*
import android.graphics.Color
import android.graphics.drawable.LayerDrawable
import android.os.Bundle
import android.provider.Settings.ACTION_WIFI_SETTINGS
import android.provider.Settings.ACTION_WIRELESS_SETTINGS
import android.view.*
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.core.graphics.BlendModeColorFilterCompat
import androidx.core.graphics.BlendModeCompat
import androidx.lifecycle.*
import androidx.work.WorkInfo
import com.multicraft.game.databinding.ActivityMainBinding
import com.multicraft.game.helpers.ApiLevelHelper.isAndroid12
import com.multicraft.game.helpers.PreferenceHelper
import com.multicraft.game.helpers.PreferenceHelper.TAG_BUILD_VER
import com.multicraft.game.helpers.PreferenceHelper.TAG_LAUNCH_TIMES
import com.multicraft.game.helpers.PreferenceHelper.TAG_SHORTCUT_EXIST
import com.multicraft.game.helpers.PreferenceHelper.getBoolValue
import com.multicraft.game.helpers.PreferenceHelper.getIntValue
import com.multicraft.game.helpers.PreferenceHelper.getStringValue
import com.multicraft.game.helpers.PreferenceHelper.set
import com.multicraft.game.helpers.Utilities.addShortcut
import com.multicraft.game.helpers.Utilities.copyInputStreamToFile
import com.multicraft.game.helpers.Utilities.getIcon
import com.multicraft.game.helpers.Utilities.isConnected
import com.multicraft.game.helpers.Utilities.makeFullScreen
import com.multicraft.game.helpers.Utilities.showRestartDialog
import com.multicraft.game.workmanager.UnzipWorker.Companion.PROGRESS
import com.multicraft.game.workmanager.WorkerViewModel
import com.multicraft.game.workmanager.WorkerViewModelFactory
import kotlinx.coroutines.launch
import java.io.File
import java.io.IOException

class MainActivity : AppCompatActivity() {
	private lateinit var binding: ActivityMainBinding
	private var externalStorage: File? = null
	private val sep = File.separator
	private lateinit var prefs: SharedPreferences
	private val versionName = BuildConfig.VERSION_NAME
	private val requestConnection = 104

	companion object {
		var radius = 0
	}

	override fun onCreate(savedInstanceState: Bundle?) {
		super.onCreate(savedInstanceState)
		window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
		binding = ActivityMainBinding.inflate(layoutInflater)
		setContentView(binding.root)
		prefs = PreferenceHelper.init(this)
		try {
			externalStorage = getExternalFilesDir(null)
			if (filesDir == null || cacheDir == null || externalStorage == null)
				throw IOException("Bad disk space state")
			lateInit()
		} catch (e: IOException) {
			showRestartDialog(this, !e.message!!.contains("ENOSPC"))
		}
	}

	@Deprecated("Deprecated in Java")
	override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
		@Suppress("DEPRECATION")
		super.onActivityResult(requestCode, resultCode, data)
		if (requestCode == requestConnection)
			checkAppVersion()
	}

	@Deprecated("Deprecated in Java")
	override fun onBackPressed() {
		// Prevent abrupt interruption when copy game files from assets
	}

	override fun onResume() {
		super.onResume()
		makeFullScreen(window)
	}

	override fun onWindowFocusChanged(hasFocus: Boolean) {
		super.onWindowFocusChanged(hasFocus)
		if (hasFocus) makeFullScreen(window)
	}

	override fun onAttachedToWindow() {
		super.onAttachedToWindow()
		if (isAndroid12()) {
			val insets = window.decorView.rootWindowInsets
			if (insets != null) {
				val tl = insets.getRoundedCorner(RoundedCorner.POSITION_TOP_LEFT)
				radius = tl?.radius ?: 0
			}
		}
	}

	private fun addLaunchTimes() {
		val launchTimes = prefs.getIntValue(TAG_LAUNCH_TIMES) + 1
		prefs[TAG_LAUNCH_TIMES] = launchTimes
	}

	// interface
	private fun showProgress(textMessage: Int, progressMessage: Int, progress: Int) {
		if (binding.progressBar.visibility == View.GONE) {
			binding.tvProgress.setText(textMessage)
			binding.progressCircle.visibility = View.GONE
			binding.progressBar.visibility = View.VISIBLE
			binding.progressBar.progress = 0
		} else if (progress > 0) {
			binding.tvProgress.text = String.format(getString(progressMessage), progress)
			binding.progressBar.progress = progress
			// colorize the progress bar
			val progressBarDrawable =
				(binding.progressBar.progressDrawable as LayerDrawable).getDrawable(0)
			val progressDrawable = (progressBarDrawable as LayerDrawable).getDrawable(1)
			val color = Color.rgb(255 - progress * 2, progress * 2, 25)
			progressDrawable.colorFilter =
				BlendModeColorFilterCompat.createBlendModeColorFilterCompat(
					color, BlendModeCompat.SRC_IN
				)
		}
	}

	private fun lateInit() {
		addLaunchTimes()
		if (!prefs.getBoolValue(TAG_SHORTCUT_EXIST)) try {
			addShortcut(this)
		} catch (ignored: Exception) {
		}
		checkConnection()
	}

	private fun startNative() {
		val initLua = File(filesDir, "builtin${sep}mainmenu${sep}init.lua")
		if (initLua.exists() && initLua.canRead()) {
			val intent = Intent(this, GameActivity::class.java)
			intent.flags = Intent.FLAG_ACTIVITY_CLEAR_TOP or Intent.FLAG_ACTIVITY_CLEAR_TASK
			startActivity(intent)
		} else {
			prefs[TAG_BUILD_VER] = "0"
			showRestartDialog(this)
		}
	}

	private fun prepareToRun() {
		binding.tvProgress.setText(R.string.preparing)
		binding.progressCircle.visibility = View.VISIBLE
		binding.progressBar.visibility = View.GONE
		val filesList = listOf(
			File(externalStorage, "builtin"),
			File(externalStorage, "cache"), // ToDo: remove me!
			File(externalStorage, "textures${sep}base"),
			File(externalStorage, "debug.txt"),
			File(filesDir, "builtin"),
			File(filesDir, "textures${sep}base")
		)
		val zips = assets.list("data")!!.toList()
		lifecycleScope.launch {
			filesList.forEach { it.deleteRecursively() }
			zips.forEach {
				try {
					assets.open("data$sep$it").use { input ->
						File(cacheDir, it).copyInputStreamToFile(input)
					}
				} catch (e: IOException) {
					runOnUiThread {
						showRestartDialog(
							this@MainActivity,
							!e.message!!.contains("ENOSPC")
						)
					}
					return@forEach
				}
			}
			try {
				startUnzipWorker(zips)
			} catch (e: Exception) {
				runOnUiThread { showRestartDialog(this@MainActivity) }
			}
		}
	}

	private fun checkAppVersion() {
		val prefVersion = prefs.getStringValue(TAG_BUILD_VER)
		if (prefVersion == versionName)
			startNative()
		else
			prepareToRun()
	}

	// check connection available
	private fun checkConnection() = lifecycleScope.launch {
		val result = isConnected(this@MainActivity)
		if (result) checkAppVersion()
		else try {
			showConnectionDialog()
		} catch (e: Exception) {
			checkAppVersion()
		}
	}

	private fun startUnzipWorker(file: List<String>) {
		val viewModelFactory = WorkerViewModelFactory(application, file.toTypedArray())
		val viewModel = ViewModelProvider(this, viewModelFactory)[WorkerViewModel::class.java]
		viewModel.unzippingWorkObserver
			.observe(this, Observer { workInfo ->
				if (workInfo == null)
					return@Observer
				val progress = workInfo.progress.getInt(PROGRESS, 0)
				showProgress(R.string.loading, R.string.loadingp, progress)

				if (workInfo.state.isFinished) {
					if (workInfo.state == WorkInfo.State.FAILED) {
						showRestartDialog(this)
					} else if (workInfo.state == WorkInfo.State.SUCCEEDED) {
						prefs[TAG_BUILD_VER] = versionName
						startNative()
					}
				}
			})
		viewModel.startOneTimeWorkRequest()
	}

	// connection dialog
	private fun showConnectionDialog() {
		val builder = AlertDialog.Builder(this)
		builder.setIcon(getIcon(this))
			.setTitle(R.string.conn_title)
			.setMessage(R.string.conn_message)
			.setPositiveButton(R.string.conn_wifi) { _, _ ->
				@Suppress("DEPRECATION")
				startActivityForResult(Intent(ACTION_WIFI_SETTINGS), requestConnection)
			}
			.setNegativeButton(R.string.conn_mobile) { _, _ ->
				@Suppress("DEPRECATION")
				startActivityForResult(Intent(ACTION_WIRELESS_SETTINGS), requestConnection)
			}
			.setNeutralButton(R.string.ignore) { _, _ -> checkAppVersion() }
			.setCancelable(false)
		val dialog = builder.create()
		makeFullScreen(dialog.window!!)
		if (!isFinishing) {
			dialog.show()
			dialog.getButton(DialogInterface.BUTTON_NEUTRAL)?.setTextColor(Color.RED)
		}
	}
}
