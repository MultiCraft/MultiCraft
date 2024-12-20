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

package com.multicraft.game

import android.content.Intent
import android.content.SharedPreferences
import android.graphics.drawable.AnimationDrawable
import android.os.Bundle
import android.provider.Settings.ACTION_WIFI_SETTINGS
import android.provider.Settings.ACTION_WIRELESS_SETTINGS
import android.view.RoundedCorner
import android.view.WindowManager
import androidx.activity.OnBackPressedCallback
import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.*
import androidx.work.WorkInfo
import com.multicraft.game.databinding.ActivityMainBinding
import com.multicraft.game.dialogs.ConnectionDialog
import com.multicraft.game.helpers.*
import com.multicraft.game.helpers.ApiLevelHelper.isAndroid12
import com.multicraft.game.helpers.ApiLevelHelper.isPie
import com.multicraft.game.helpers.PreferenceHelper.TAG_BUILD_VER
import com.multicraft.game.helpers.PreferenceHelper.getStringValue
import com.multicraft.game.helpers.PreferenceHelper.set
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
	private lateinit var restartStartForResult: ActivityResultLauncher<Intent>
	private lateinit var connStartForResult: ActivityResultLauncher<Intent>
	private val versionCode = BuildConfig.VERSION_CODE
	private val versionName = "${BuildConfig.VERSION_NAME}+$versionCode"

	companion object {
		var radius = 0
		const val NO_SPACE_LEFT = "ENOSPC"
	}

	override fun onCreate(savedInstanceState: Bundle?) {
		super.onCreate(savedInstanceState)
		window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
		binding = ActivityMainBinding.inflate(layoutInflater)
		setContentView(binding.root)
		onBackPressedDispatcher.addCallback(object : OnBackPressedCallback(true) {
			override fun handleOnBackPressed() {
			}
		})
		connStartForResult = registerForActivityResult(
			ActivityResultContracts.StartActivityForResult()
		) {
			checkAppVersion()
		}
		restartStartForResult = registerForActivityResult(
			ActivityResultContracts.StartActivityForResult()
		) {
			if (it.resultCode == RESULT_OK)
				finishApp(true)
			else
				finishApp(false)
		}
		try {
			prefs = PreferenceHelper.init(this)
			externalStorage = getExternalFilesDir(null)
			listOf(filesDir, cacheDir, externalStorage).requireNoNulls()
			checkConnection()
		} catch (e: Exception) {
			val isRestart = e.message?.contains(NO_SPACE_LEFT) != true
			showRestartDialog(restartStartForResult, isRestart)
		}
	}

	override fun onResume() {
		super.onResume()
		window.makeFullScreen()
	}

	override fun onWindowFocusChanged(hasFocus: Boolean) {
		super.onWindowFocusChanged(hasFocus)
		if (hasFocus) window.makeFullScreen()
	}

	override fun onAttachedToWindow() {
		super.onAttachedToWindow()
		if (isPie()) {
			val cutout = window.decorView.rootWindowInsets.displayCutout
			if (cutout != null) {
				radius = 40
			}
			if (isAndroid12()) {
				val insets = window.decorView.rootWindowInsets
				if (insets != null) {
					val tl = insets.getRoundedCorner(RoundedCorner.POSITION_TOP_LEFT)
					radius = tl?.radius ?: if (cutout != null) 40 else 0
				}
			}
		}
		val animation = binding.loadingAnim.drawable as AnimationDrawable
		animation.start()
	}

	private fun startNative() {
		val initLua = File(filesDir, "builtin${sep}mainmenu${sep}init.lua")
		if (initLua.exists() && initLua.canRead()) {
			val intent = Intent(this, GameActivity::class.java)
			intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK)
			intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
			startActivity(intent)
		} else {
			prefs[TAG_BUILD_VER] = "0"
			showRestartDialog(restartStartForResult)
		}
	}

	private fun prepareToRun() {
		val filesList = mutableListOf<File>().apply {
			addAll(listOf(
				"builtin",
				"client${sep}shaders",
				"fonts",
				"games${sep}default",
				"textures${sep}base"
			).map { File(filesDir, it) })
		}

		val zips = mutableListOf("assets.zip")

		lifecycleScope.launch {
			filesList.forEach { it.deleteRecursively() }
			zips.forEach {
				try {
					assets.open(it).use { input ->
						File(cacheDir, it).copyInputStreamToFile(input)
					}
				} catch (e: IOException) {
					val isNotEnoughSpace = e.message!!.contains(NO_SPACE_LEFT)
					runOnUiThread { showRestartDialog(restartStartForResult, !isNotEnoughSpace) }
					return@forEach
				}
			}
			try {
				startUnzipWorker(zips.toTypedArray())
			} catch (e: Exception) {
				runOnUiThread { showRestartDialog(restartStartForResult) }
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

	private fun showConnectionDialog() {
		val intent = Intent(this, ConnectionDialog::class.java)
		val startForResult = registerForActivityResult(
			ActivityResultContracts.StartActivityForResult()
		) {
			when (it.resultCode) {
				RESULT_OK -> connStartForResult.launch(Intent(ACTION_WIFI_SETTINGS))
				RESULT_FIRST_USER -> connStartForResult.launch(Intent(ACTION_WIRELESS_SETTINGS))
				else -> checkAppVersion()
			}
		}
		startForResult.launch(intent)
	}

	// check connection available
	private fun checkConnection() = lifecycleScope.launch {
		if (isConnected()) checkAppVersion()
		else try {
			showConnectionDialog()
		} catch (e: Exception) {
			checkAppVersion()
		}
	}

	private fun startUnzipWorker(file: Array<String>) {
		val viewModelFactory = WorkerViewModelFactory(application, file)
		val viewModel = ViewModelProvider(this, viewModelFactory)[WorkerViewModel::class.java]
		viewModel.unzippingWorkObserver
			.observe(this, Observer { workInfo ->
				if (workInfo == null)
					return@Observer
				val progress = workInfo.progress.getInt(PROGRESS, 0)
				if (progress > 0) {
					val progressMessage = "${getString(R.string.loading)} $progress%"
					binding.tvProgress.text = progressMessage
				}

				if (workInfo.state.isFinished) {
					if (workInfo.state == WorkInfo.State.FAILED) {
						val isRestart = workInfo.outputData.getBoolean("restart", true)
						showRestartDialog(restartStartForResult, isRestart)
					} else if (workInfo.state == WorkInfo.State.SUCCEEDED) {
						prefs[TAG_BUILD_VER] = versionName
						startNative()
					}
				}
			})
		viewModel.startOneTimeWorkRequest()
	}
}
