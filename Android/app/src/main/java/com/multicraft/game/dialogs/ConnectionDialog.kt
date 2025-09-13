/*
MultiCraft
Copyright (C) 2014-2025 MoNTE48, Maksim Gamarnik <Maksym48@pm.me>
Copyright (C) 2014-2025 ubulem,  Bektur Mambetov <berkut87@gmail.com>

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

import android.telephony.TelephonyManager
import android.telephony.TelephonyManager.SIM_STATE_READY
import android.view.View
import com.multicraft.game.databinding.ConnectionDialogBinding
import com.multicraft.game.helpers.ApiLevelHelper.isOreo

class ConnectionDialog : StandardDialog<ConnectionDialogBinding>() {
	private fun isSimCardPresent(): Boolean {
		val telephonyManager = getSystemService(TELEPHONY_SERVICE) as TelephonyManager

		if (!isOreo())
			return telephonyManager.simState == SIM_STATE_READY

		val isFirstSimPresent = telephonyManager.getSimState(0) == SIM_STATE_READY
		val isSecondSimPresent = telephonyManager.getSimState(1) == SIM_STATE_READY

		return isFirstSimPresent || isSecondSimPresent
	}

	override fun initBinding() {
		binding = ConnectionDialogBinding.inflate(layoutInflater)
		topRoot = binding.connRoot
		headerIcon = binding.headerIcon
	}

	override fun setupLayout() {
		if (isSimCardPresent())
			binding.mobile.visibility = View.VISIBLE

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
}
