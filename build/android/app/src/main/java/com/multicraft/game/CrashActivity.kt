package com.multicraft.game

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import com.multicraft.game.helpers.Utilities.showRestartDialog

class CrashActivity : AppCompatActivity() {
	override fun onCreate(savedInstanceState: Bundle?) {
		super.onCreate(savedInstanceState)
		showRestartDialog(this)
	}
}
