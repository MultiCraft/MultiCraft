package com.multicraft.game.workmanager

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.LiveData
import androidx.work.OneTimeWorkRequest
import androidx.work.WorkInfo
import androidx.work.WorkManager
import androidx.work.workDataOf
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
