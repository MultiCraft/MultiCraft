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

package com.multicraft.game.workmanager

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.LiveData
import androidx.work.*
import com.multicraft.game.workmanager.UnzipWorker.Companion.EXTRA_KEY_IN_FILE

class WorkerViewModel(
	application: Application,
	private val zips: Array<String>
) :
	AndroidViewModel(application) {

	private val mWorkManager: WorkManager by lazy { WorkManager.getInstance(application) }

	val unzippingWorkObserver: LiveData<WorkInfo> by lazy {
		mWorkManager.getWorkInfoByIdLiveData(
			unzippingWorkReq.id
		)
	}

	private val unzippingWorkReq: OneTimeWorkRequest by lazy {
		OneTimeWorkRequest.Builder(UnzipWorker::class.java)
			.setInputData(workDataOf(EXTRA_KEY_IN_FILE to zips))
			.build()
	}

	fun startOneTimeWorkRequest() {
		mWorkManager.enqueue(unzippingWorkReq)
	}
}
