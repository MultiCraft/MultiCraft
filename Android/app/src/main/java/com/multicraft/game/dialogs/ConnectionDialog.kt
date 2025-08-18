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

import android.content.Context
import android.content.res.Configuration
import android.os.Bundle
import android.telephony.TelephonyManager
import android.telephony.TelephonyManager.SIM_STATE_READY
import android.view.View
import android.widget.LinearLayout
import androidx.activity.OnBackPressedCallback
import androidx.appcompat.app.AppCompatActivity
import com.multicraft.game.databinding.ConnectionDialogBinding
import com.multicraft.game.helpers.ApiLevelHelper.isOreo
import com.multicraft.game.helpers.makeFullScreen
import org.libsdl.app.SDLActivity

class ConnectionDialog : AppCompatActivity() {
	private fun isSimCardPresent(): Boolean {
		val telephonyManager = getSystemService(TELEPHONY_SERVICE) as TelephonyManager

		if (!isOreo())
			return telephonyManager.simState == SIM_STATE_READY

		val isFirstSimPresent = telephonyManager.getSimState(0) == SIM_STATE_READY
		val isSecondSimPresent = telephonyManager.getSimState(1) == SIM_STATE_READY

		return isFirstSimPresent || isSecondSimPresent
	}

	override fun onCreate(savedInstanceState: Bundle?) {
		super.onCreate(savedInstanceState)
		val binding = ConnectionDialogBinding.inflate(layoutInflater)
		if (SDLActivity.isTablet()) {
			val param = LinearLayout.LayoutParams(
				0,
				LinearLayout.LayoutParams.WRAP_CONTENT,
				0.5f
			)
			binding.connRoot.layoutParams = param
		}
		if (isSimCardPresent())
			binding.mobile.visibility = View.VISIBLE
		setContentView(binding.root)

		onBackPressedDispatcher.addCallback(object : OnBackPressedCallback(true) {
			override fun handleOnBackPressed() {
			}
		})

		binding.wifi.setOnClickListener {
			setResult(RESULT_OK)
			finish()
		}
		binding.mobile.setOnClickListener {
			setResult(RESULT_FIRST_USER)
			finish()
		}
		binding.ignore.setOnClickListener {
			setResult(RESULT_CANCELED)
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
