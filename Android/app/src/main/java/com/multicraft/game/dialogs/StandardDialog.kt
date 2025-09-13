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

import android.content.Context
import android.content.res.Configuration
import android.os.Bundle
import android.widget.LinearLayout
import androidx.activity.OnBackPressedCallback
import androidx.appcompat.app.AppCompatActivity
import androidx.viewbinding.ViewBinding
import com.google.android.material.imageview.ShapeableImageView
import com.multicraft.game.helpers.*
import com.multicraft.game.helpers.ApiLevelHelper.isOreo

abstract class StandardDialog<B : ViewBinding> : AppCompatActivity() {
	lateinit var binding: B
	var topRoot: LinearLayout? = null
	lateinit var headerIcon: ShapeableImageView

	override fun onCreate(savedInstanceState: Bundle?) {
		super.onCreate(savedInstanceState)
		initBinding()
		if (isTabletDp()) {
			val param = LinearLayout.LayoutParams(
				0,
				LinearLayout.LayoutParams.WRAP_CONTENT,
				0.5f
			)
			topRoot?.layoutParams = param
		}
		if (isOreo())
			headerIcon.setImageDrawable(getIcon())
		setContentView(binding.root)
		setupLayout()

		onBackPressedDispatcher.addCallback(object : OnBackPressedCallback(true) {
			override fun handleOnBackPressed() {
			}
		})
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

	abstract fun initBinding()

	abstract fun setupLayout()
}
