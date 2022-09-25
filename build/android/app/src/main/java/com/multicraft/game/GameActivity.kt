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

package com.multicraft.game

import android.app.NativeActivity
import android.content.Intent
import android.content.res.Configuration
import android.content.res.Configuration.HARDKEYBOARDHIDDEN_NO
import android.net.Uri
import android.os.Bundle
import android.text.InputType
import android.view.*
import android.view.inputmethod.EditorInfo
import android.view.inputmethod.InputMethodManager
import android.widget.TextView
import androidx.appcompat.app.AlertDialog
import com.multicraft.game.MainActivity.Companion.radius
import com.multicraft.game.databinding.InputTextBinding
import com.multicraft.game.helpers.Utilities.finishApp
import com.multicraft.game.helpers.Utilities.makeFullScreen
import kotlin.system.exitProcess

class GameActivity : NativeActivity() {
	companion object {
		var isMultiPlayer = false
		var isInputActive = false

		@JvmStatic
		external fun pauseGame()

		@JvmStatic
		external fun keyboardEvent(keyboard: Boolean)

		init {
			try {
				System.loadLibrary("MultiCraft")
			} catch (e: UnsatisfiedLinkError) {
				exitProcess(0)
			}
		}
	}

	private var messageReturnCode = -1
	private var messageReturnValue = ""
	private var hasKeyboard = false

	public override fun onCreate(savedInstanceState: Bundle?) {
		super.onCreate(savedInstanceState)
		window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
		hasKeyboard = resources.configuration.hardKeyboardHidden == HARDKEYBOARDHIDDEN_NO
		keyboardEvent(hasKeyboard)
	}

	override fun onWindowFocusChanged(hasFocus: Boolean) {
		super.onWindowFocusChanged(hasFocus)
		if (hasFocus) makeFullScreen(window)
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
		makeFullScreen(window)
	}

	override fun onConfigurationChanged(newConfig: Configuration) {
		super.onConfigurationChanged(newConfig)
		val statusKeyboard =
			resources.configuration.hardKeyboardHidden == HARDKEYBOARDHIDDEN_NO
		if (hasKeyboard != statusKeyboard) {
			hasKeyboard = statusKeyboard
			keyboardEvent(hasKeyboard)
		}
	}

	@Suppress("unused")
	fun showDialog(
		@Suppress("UNUSED_PARAMETER") s: String?,
		hint: String?, current: String?, editType: Int
	) {
		runOnUiThread { showDialogUI(hint, current, editType) }
	}

	private fun showDialogUI(hint: String?, current: String?, editType: Int) {
		isInputActive = true
		val builder = AlertDialog.Builder(this)
		if (editType == 1) builder.setPositiveButton(R.string.done, null)
		val binding = InputTextBinding.inflate(layoutInflater)
		val hintText = hint?.ifEmpty {
			resources.getString(if (editType == 3) R.string.input_password else R.string.input_text)
		}
		binding.inputLayout.hint = hintText
		builder.setView(binding.root)
		val alertDialog = builder.create()
		val editText = binding.editText
		editText.requestFocus()
		editText.setText(current.toString())
		if (editType != 1) editText.imeOptions = EditorInfo.IME_FLAG_NO_FULLSCREEN
		val imm = getSystemService(INPUT_METHOD_SERVICE) as InputMethodManager
		var inputType = InputType.TYPE_CLASS_TEXT
		when (editType) {
			1 -> {
				inputType = inputType or InputType.TYPE_TEXT_FLAG_MULTI_LINE
				editText.maxLines = 8
			}
			3 -> inputType = inputType or InputType.TYPE_TEXT_VARIATION_PASSWORD
		}
		editText.inputType = inputType
		editText.setSelection(editText.text?.length ?: 0)
		// for Android OS
		editText.setOnEditorActionListener { _: TextView?, KeyCode: Int, _: KeyEvent? ->
			if (KeyCode == KeyEvent.KEYCODE_ENTER || KeyCode == KeyEvent.KEYCODE_ENDCALL) {
				imm.hideSoftInputFromWindow(editText.windowToken, 0)
				messageReturnCode = 0
				messageReturnValue = editText.text.toString()
				alertDialog.dismiss()
				isInputActive = false
				return@setOnEditorActionListener true
			}
			return@setOnEditorActionListener false
		}
		// for Chrome OS
		editText.setOnKeyListener { _: View?, KeyCode: Int, _: KeyEvent? ->
			if (KeyCode == KeyEvent.KEYCODE_ENTER || KeyCode == KeyEvent.KEYCODE_ENDCALL) {
				imm.hideSoftInputFromWindow(editText.windowToken, 0)
				messageReturnCode = 0
				messageReturnValue = editText.text.toString()
				alertDialog.dismiss()
				isInputActive = false
				return@setOnKeyListener true
			}
			return@setOnKeyListener false
		}
		// should be above `show()`
		alertDialog.window!!.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_VISIBLE)
		alertDialog.show()
		val button = alertDialog.getButton(AlertDialog.BUTTON_POSITIVE)
		button?.setOnClickListener {
			imm.hideSoftInputFromWindow(editText.windowToken, 0)
			messageReturnCode = 0
			messageReturnValue = editText.text.toString()
			alertDialog.dismiss()
			isInputActive = false
		}
		alertDialog.setOnCancelListener {
			window.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_HIDDEN)
			messageReturnValue = current.toString()
			messageReturnCode = -1
			isInputActive = false
		}
	}

	@Suppress("unused")
	fun getDialogState() = messageReturnCode

	@Suppress("unused")
	fun getDialogValue(): String {
		messageReturnCode = -1
		return messageReturnValue
	}

	@Suppress("unused")
	fun getDensity() = resources.displayMetrics.density

	@Suppress("unused")
	fun notifyServerConnect(multiplayer: Boolean) {
		isMultiPlayer = multiplayer
	}

	@Suppress("unused")
	fun notifyExitGame() {
	}

	@Suppress("unused")
	fun openURI(uri: String?) {
		try {
			val browserIntent = Intent(Intent.ACTION_VIEW, Uri.parse(uri))
			startActivity(browserIntent)
		} catch (ignored: Exception) {
		}
	}

	@Suppress("unused")
	fun finishGame(exc: String?) {
		finishApp(true, this)
	}

	@Suppress("unused")
	fun handleError(exc: String?) {
	}

	@Suppress("unused")
	fun upgrade(item: String) {
	}

	fun getSecretKey(key: String): String {
		return "Stub"
 }
  
	@Suppress("unused")
	fun getRoundScreen(): Int {
		return radius
	}
}
