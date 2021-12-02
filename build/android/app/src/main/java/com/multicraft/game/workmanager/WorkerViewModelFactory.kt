package com.multicraft.game.workmanager

import android.app.Application
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider


class WorkerViewModelFactory(
	private val application: Application,
	private val zips: Array<String>
) :
	ViewModelProvider.Factory {
	override fun <T : ViewModel?> create(modelClass: Class<T>): T {
		if (modelClass.isAssignableFrom(WorkerViewModel::class.java)) {
			@Suppress("UNCHECKED_CAST")
			return WorkerViewModel(application, zips) as T
		}
		throw IllegalArgumentException("Unknown ViewModel class")
	}
}
