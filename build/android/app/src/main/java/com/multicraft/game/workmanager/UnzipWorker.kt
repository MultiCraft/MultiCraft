/*
MultiCraft
Copyright (C) 2014-2022 MoNTE48, Maksim Gamarnik <Maksym48@pm.me>
Copyright (C) 2014-2022 ubulem,  Bektur Mambetov <berkut87@gmail.com>

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

import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.Context
import android.content.Context.NOTIFICATION_SERVICE
import androidx.core.app.NotificationCompat
import androidx.work.*
import com.multicraft.game.R
import com.multicraft.game.helpers.ApiLevelHelper.isOreo
import com.multicraft.game.helpers.copyInputStreamToFile
import java.io.File
import java.io.IOException
import java.util.zip.ZipFile

class UnzipWorker(private val appContext: Context, workerParams: WorkerParameters) :
	CoroutineWorker(appContext, workerParams) {

	private fun createForegroundInfo(): ForegroundInfo {
		if (isOreo()) {
			val importance = NotificationManager.IMPORTANCE_DEFAULT
			val mChannel = NotificationChannel(CHANNEL_ID, CHANNEL_NAME, importance)
			mChannel.setSound(null, null)
			mChannel.description = CHANNEL_DESC
			val notificationManager =
				appContext.getSystemService(NOTIFICATION_SERVICE) as NotificationManager
			notificationManager.createNotificationChannel(mChannel)
		}

		val notification = NotificationCompat.Builder(applicationContext, CHANNEL_ID)
			.setContentTitle(appContext.getString(R.string.notification_title))
			.setContentText(appContext.getString(R.string.notification_description))
			.setSmallIcon(R.drawable.update)
			.build()
		return ForegroundInfo(NOTIFICATION_ID, notification)
	}

	override suspend fun doWork(): Result {
		val zips = inputData.getStringArray(EXTRA_KEY_IN_FILE)!!
		setForeground(createForegroundInfo())
		var previousProgress = 0
		return try {
			var per = 0
			val zipList = zips.map { ZipFile(File(appContext.cacheDir, it)) }
			val size = zipList.sumOf { it.size() }
			zipList.forEach { zip ->
				zip.use {
					it.entries().asSequence().forEach { entry ->
						zip.getInputStream(entry).use { input ->
							val filePath = File(appContext.filesDir, entry.name)
							if (entry.isDirectory)
								filePath.mkdirs()
							else
								filePath.copyInputStreamToFile(input)
							val currentProgress = 100 * ++per / size
							if (currentProgress > previousProgress) {
								previousProgress = currentProgress
								setProgress(workDataOf(PROGRESS to currentProgress))
							}
						}
					}
				}
			}
			Result.success()
		} catch (e: IOException) {
			val isNotEnoughSpace = e.localizedMessage!!.contains("ENOSPC")
			val out = Data.Builder()
				.putBoolean("restart", !isNotEnoughSpace)
				.build()
			Result.failure(out)
		} finally {
			zips.forEach { File(appContext.cacheDir, it).delete() }
		}
	}

	companion object {
		const val PROGRESS = "progress"
		const val NOTIFICATION_ID = 1
		const val CHANNEL_ID = "MultiCraft channel"
		const val CHANNEL_NAME = "com.multicraft.game"
		const val CHANNEL_DESC = "Notifications from MultiCraft"
		const val EXTRA_KEY_IN_FILE = "com.multicraft.game.file"
	}
}
