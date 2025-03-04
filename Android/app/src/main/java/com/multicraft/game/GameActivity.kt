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

import android.content.res.Configuration
import android.net.Uri
import android.os.Bundle
import android.text.InputType
import android.view.*
import android.view.WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_HIDDEN
import android.view.WindowManager.LayoutParams.SOFT_INPUT_STATE_VISIBLE
import android.view.inputmethod.EditorInfo
import android.view.inputmethod.InputMethodManager
import android.widget.TextView
import androidx.appcompat.app.AlertDialog
import androidx.browser.customtabs.CustomTabsIntent
import androidx.browser.customtabs.CustomTabsIntent.SHARE_STATE_OFF
import com.multicraft.game.MainActivity.Companion.radius
import com.multicraft.game.databinding.*
import com.multicraft.game.helpers.*
import com.multicraft.game.helpers.ApiLevelHelper.isOreo
import org.libsdl.app.SDLActivity
import kotlin.system.exitProcess

class GameActivity : SDLActivity() {
	companion object {
		var isMultiPlayer = false
		var isInputActive = false

		@JvmStatic
		external fun pauseGame()

		@JvmStatic
		external fun keyboardEvent(keyboard: Boolean)
	}

	private var messageReturnValue = ""
	private var hasKeyboard = false
	override fun getLibraries() = arrayOf("MultiCraft")

	override fun getMainSharedObject() =
		"${getContext().applicationInfo.nativeLibraryDir}/libMultiCraft.so"

	override fun onCreate(savedInstanceState: Bundle?) {
		try {
			super.onCreate(savedInstanceState)
		} catch (e: Error) {
			exitProcess(0)
		} catch (e: Exception) {
			exitProcess(0)
		}
		window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
		hasKeyboard = hasHardKeyboard()
	}

	override fun onWindowFocusChanged(hasFocus: Boolean) {
		super.onWindowFocusChanged(hasFocus)
		if (hasFocus) window.makeFullScreen()
	}

	@Deprecated("Deprecated in Java")
	override fun onBackPressed() {
		// Ignore the back press so MultiCraft can handle it
	}

	override fun onPause() {
		super.onPause()
		pauseGame()
	}

	override fun onResume() {
		super.onResume()
		if (hasKeyboard) keyboardEvent(true)
		window.makeFullScreen()
	}

	override fun onConfigurationChanged(newConfig: Configuration) {
		super.onConfigurationChanged(newConfig)
		val statusKeyboard = hasHardKeyboard()
		if (hasKeyboard != statusKeyboard) {
			hasKeyboard = statusKeyboard
			keyboardEvent(hasKeyboard)
		}
	}

	@Suppress("unused")
	fun showDialog(hint: String?, current: String?, editType: Int) {
		isInputActive = true
		messageReturnValue = ""
		if (editType == 1)
			runOnUiThread { showMultiLineDialog(hint, current) }
		else
			runOnUiThread { showSingleDialog(hint, current, editType) }
	}

	private fun showSingleDialog(hint: String?, current: String?, editType: Int) {
		val builder = AlertDialog.Builder(this, R.style.FullScreenDialogStyle)
		val binding = InputTextBinding.inflate(layoutInflater)
		var hintText: String = hint?.ifEmpty {
			resources.getString(if (editType == 3) R.string.input_password else R.string.input_text)
		}.toString()
		hintText = hintText.replace(":$".toRegex(), "")
		binding.input.hint = hintText
		builder.setView(binding.root)
		val alertDialog = builder.create()
		val editText = binding.editText
		editText.requestFocus()
		editText.setText(current.toString())
		editText.imeOptions = EditorInfo.IME_FLAG_NO_FULLSCREEN
		val imm = getSystemService(INPUT_METHOD_SERVICE) as InputMethodManager
		var inputType = InputType.TYPE_CLASS_TEXT
		if (editType == 3) {
			inputType = inputType or InputType.TYPE_TEXT_VARIATION_PASSWORD
			if (isOreo())
				editText.importantForAutofill = View.IMPORTANT_FOR_AUTOFILL_NO
		}
		editText.inputType = inputType
		editText.setSelection(editText.text?.length ?: 0)
		// for Android OS
		editText.setOnEditorActionListener { _: TextView?, keyCode: Int, _: KeyEvent? ->
			if (keyCode == KeyEvent.KEYCODE_ENTER || keyCode == KeyEvent.KEYCODE_ENDCALL) {
				imm.hideSoftInputFromWindow(editText.windowToken, 0)
				messageReturnValue = editText.text.toString()
				alertDialog.dismiss()
				isInputActive = false
				return@setOnEditorActionListener true
			}
			return@setOnEditorActionListener false
		}
		if (isChromebook()) {
			editText.setOnKeyListener { _: View?, keyCode: Int, _: KeyEvent? ->
				if (keyCode == KeyEvent.KEYCODE_ENTER || keyCode == KeyEvent.KEYCODE_ENDCALL) {
					imm.hideSoftInputFromWindow(editText.windowToken, 0)
					messageReturnValue = editText.text.toString()
					alertDialog.dismiss()
					isInputActive = false
					return@setOnKeyListener true
				}
				return@setOnKeyListener false
			}
		}
		binding.input.setEndIconOnClickListener {
			imm.hideSoftInputFromWindow(editText.windowToken, 0)
			messageReturnValue = editText.text.toString()
			alertDialog.dismiss()
			isInputActive = false
		}
		binding.rl.setOnClickListener {
			window.setSoftInputMode(SOFT_INPUT_STATE_ALWAYS_HIDDEN)
			messageReturnValue = current.toString()
			alertDialog.dismiss()
			isInputActive = false
		}
		val alertWindow = alertDialog.window!!
		// should be above `show()`
		alertWindow.setSoftInputMode(SOFT_INPUT_STATE_VISIBLE)
		alertDialog.show()
		if (!isTablet())
			alertWindow.makeFullScreenAlert()
		alertDialog.setOnCancelListener {
			window.setSoftInputMode(SOFT_INPUT_STATE_ALWAYS_HIDDEN)
			messageReturnValue = current.toString()
			isInputActive = false
		}
	}

	private fun showMultiLineDialog(hint: String?, current: String?) {
		val builder = AlertDialog.Builder(this, R.style.FullScreenDialogStyle)
		val binding = MultilineInputBinding.inflate(layoutInflater)
		var hintText: String = hint?.ifEmpty {
			resources.getString(R.string.input_text)
		}.toString()
		hintText = hintText.replace(":$".toRegex(), "")
		binding.multiInput.hint = hintText
		builder.setView(binding.root)
		val alertDialog = builder.create()
		val editText = binding.multiEditText
		editText.requestFocus()
		editText.setText(current.toString())
		editText.imeOptions = EditorInfo.IME_FLAG_NO_FULLSCREEN
		editText.setSelection(editText.text?.length ?: 0)
		val imm = getSystemService(INPUT_METHOD_SERVICE) as InputMethodManager
		// for Android OS
		editText.setOnEditorActionListener { _: TextView?, keyCode: Int, _: KeyEvent? ->
			if (keyCode == KeyEvent.KEYCODE_ENTER || keyCode == KeyEvent.KEYCODE_ENDCALL) {
				imm.hideSoftInputFromWindow(editText.windowToken, 0)
				messageReturnValue = editText.text.toString()
				alertDialog.dismiss()
				isInputActive = false
				return@setOnEditorActionListener true
			}
			return@setOnEditorActionListener false
		}
		if (isChromebook()) {
			editText.setOnKeyListener { _: View?, keyCode: Int, _: KeyEvent? ->
				if (keyCode == KeyEvent.KEYCODE_ENTER || keyCode == KeyEvent.KEYCODE_ENDCALL) {
					imm.hideSoftInputFromWindow(editText.windowToken, 0)
					messageReturnValue = editText.text.toString()
					alertDialog.dismiss()
					isInputActive = false
					return@setOnKeyListener true
				}
				return@setOnKeyListener false
			}
		}
		binding.multiInput.setEndIconOnClickListener {
			imm.hideSoftInputFromWindow(editText.windowToken, 0)
			messageReturnValue = editText.text.toString()
			alertDialog.dismiss()
			isInputActive = false
		}
		binding.multiRl.setOnClickListener {
			window.setSoftInputMode(SOFT_INPUT_STATE_ALWAYS_HIDDEN)
			messageReturnValue = current.toString()
			alertDialog.dismiss()
			isInputActive = false
		}
		// should be above `show()`
		val alertWindow = alertDialog.window!!
		alertWindow.setSoftInputMode(SOFT_INPUT_STATE_VISIBLE)
		alertDialog.show()
		if (!isTablet())
			alertWindow.makeFullScreenAlert()
		alertDialog.setOnCancelListener {
			window.setSoftInputMode(SOFT_INPUT_STATE_ALWAYS_HIDDEN)
			messageReturnValue = current.toString()
			isInputActive = false
		}
	}

	@Suppress("unused")
	fun isDialogActive() = isInputActive

	@Suppress("unused")
	fun getDialogValue(): String {
		val value = messageReturnValue
		messageReturnValue = ""
		return value
	}

	fun getDensity() = resources.displayMetrics.density

	fun notifyServerConnect(multiplayer: Boolean) {
		isMultiPlayer = multiplayer
	}

	fun notifyExitGame() {
	}

	fun openURI(uri: String?) {
		val builder = CustomTabsIntent.Builder()
		builder.setShareState(SHARE_STATE_OFF)
			.setStartAnimations(this, R.anim.slide_in_bottom, R.anim.slide_out_top)
			.setExitAnimations(this, R.anim.slide_in_top, R.anim.slide_out_bottom)
		val customTabsIntent = builder.build()
		try {
			customTabsIntent.launchUrl(this, Uri.parse(uri))
		} catch (ignored: Exception) {
		}
	}

	fun finishGame(exc: String?) {
		print(exc)
		finishApp(true)
	}

	fun handleError(exc: String?) {
		print(exc)
	}

	fun upgrade(item: String) {
		print(item)
	}

	fun getSecretKey(key: String): String {
		return key
	}

	fun getRoundScreen(): Int {
		return radius
	}

	fun getCpuArchitecture(): String {
		return System.getProperty("os.arch") ?: "null"
	}
}
