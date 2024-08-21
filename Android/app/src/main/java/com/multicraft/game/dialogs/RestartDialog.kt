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

package com.multicraft.game.dialogs

import android.app.Activity
import android.content.Context
import android.content.res.Configuration
import android.os.Bundle
import android.widget.LinearLayout
import androidx.activity.OnBackPressedCallback
import androidx.appcompat.app.AppCompatActivity
import com.multicraft.game.databinding.RestartDialogBinding
import com.multicraft.game.helpers.makeFullScreen
import org.libsdl.app.SDLActivity

class RestartDialog : AppCompatActivity() {
	override fun onCreate(savedInstanceState: Bundle?) {
		super.onCreate(savedInstanceState)
		val binding = RestartDialogBinding.inflate(layoutInflater)
		if (SDLActivity.isTablet()) {
			val param = LinearLayout.LayoutParams(
				0,
				LinearLayout.LayoutParams.WRAP_CONTENT,
				0.5f
			)
			binding.restartRoot.layoutParams = param
		}
		setContentView(binding.root)

		onBackPressedDispatcher.addCallback(object : OnBackPressedCallback(true) {
			override fun handleOnBackPressed() {
			}
		})

		val message = intent.getStringExtra("message")!!
		binding.errorDesc.text = message
		binding.restart.setOnClickListener {
			setResult(Activity.RESULT_OK)
			finish()
		}
		binding.close.setOnClickListener {
			setResult(Activity.RESULT_CANCELED)
			finish()
		}
	}

	override fun onResume() {
		super.onResume()
		window.makeFullScreen()
	}

	override fun attachBaseContext(base: Context?) {
		val configuration = Configuration(base?.resources?.configuration)
		configuration.fontScale = 1.0f
		applyOverrideConfiguration(configuration)
		super.attachBaseContext(base)
	}
}
