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
import androidx.lifecycle.lifecycleScope
import com.multicraft.game.databinding.ActivityMainBinding
import com.multicraft.game.dialogs.ConnectionDialog
import com.multicraft.game.helpers.*
import com.multicraft.game.helpers.ApiLevelHelper.isAndroid12
import com.multicraft.game.helpers.ApiLevelHelper.isPie
import com.multicraft.game.helpers.PreferenceHelper.TAG_BUILD_VER
import com.multicraft.game.helpers.PreferenceHelper.getIntValue
import kotlinx.coroutines.launch
import java.io.File

class MainActivity : AppCompatActivity() {
	private lateinit var binding: ActivityMainBinding
	private var externalStorage: File? = null
	private val sep = File.separator
	private lateinit var prefs: SharedPreferences
	private lateinit var restartStartForResult: ActivityResultLauncher<Intent>
	private lateinit var connStartForResult: ActivityResultLauncher<Intent>

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

	private fun startNative(isExtract: Boolean = false) {
		val intent = Intent(this, GameActivity::class.java).apply {
			addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK)
			addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
			val update = if (isExtract) {
				true
			} else {
				val initLua = File(filesDir, "builtin${sep}mainmenu${sep}init.lua")
				!(initLua.exists() && initLua.canRead())
			}

			putExtra("update", update)
		}
		startActivity(intent)
	}

	private fun prepareToRun() {
		val filesList = mutableListOf<File>().apply {
			addAll(
				listOf(
					"builtin",
					"client${sep}shaders",
					"fonts",
					"games${sep}default",
					"textures${sep}base"
				).map { File(filesDir, it) })
		}

		lifecycleScope.launch {
			filesList.forEach { it.deleteRecursively() }
			startNative(true)
		}
	}

	private fun checkAppVersion() {
		var prefVersion = 0
		try {
			prefVersion = prefs.getIntValue(TAG_BUILD_VER)
		} catch (_: ClassCastException) {

		}
		if (prefVersion == BuildConfig.VERSION_CODE)
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
		} catch (_: Exception) {
			checkAppVersion()
		}
	}
}
