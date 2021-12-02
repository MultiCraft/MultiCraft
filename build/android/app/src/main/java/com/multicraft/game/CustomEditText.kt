package com.multicraft.game

import android.content.Context
import android.util.AttributeSet
import android.view.KeyEvent
import android.view.inputmethod.InputMethodManager

class CustomEditText constructor(context: Context, attrs: AttributeSet) :
	com.google.android.material.textfield.TextInputEditText(context, attrs) {
	override fun onKeyPreIme(keyCode: Int, event: KeyEvent): Boolean {
		if (keyCode == KeyEvent.KEYCODE_BACK) {
			val mgr = context.getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
			mgr.hideSoftInputFromWindow(this.windowToken, 0)
		}
		return false
	}
}
