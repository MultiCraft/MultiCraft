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

import com.multicraft.game.R
import com.multicraft.game.databinding.RestartDialogBinding

class RestartDialog : StandardDialog<RestartDialogBinding>() {
	override fun initBinding() {
		binding = RestartDialogBinding.inflate(layoutInflater)
		topRoot = binding.restartRoot
		headerIcon = binding.headerIcon
	}

	override fun setupLayout() {
		val message = intent.getStringExtra("message")
		binding.errorDesc.text = message ?: getString(R.string.restart)
		binding.restart.setOnClickListener {
			setResult(RESULT_OK)
			finish()
		}
		binding.close.setOnClickListener {
			setResult(RESULT_CANCELED)
			finish()
		}
	}
}
