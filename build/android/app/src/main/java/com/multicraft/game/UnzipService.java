/*
MultiCraft
Copyright (C) 2014-2020 MoNTE48, Maksim Gamarnik <MoNTE48@mail.ua>
Copyright (C) 2014-2020 ubulem,  Bektur Mambetov <berkut87@gmail.com>

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

package com.multicraft.game;

import android.app.IntentService;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;

import com.bugsnag.android.Bugsnag;

import net.lingala.zip4j.ZipFile;
import net.lingala.zip4j.io.inputstream.ZipInputStream;
import net.lingala.zip4j.model.LocalFileHeader;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.Objects;

import static com.multicraft.game.MainActivity.zipLocations;
import static com.multicraft.game.helpers.ApiLevelHelper.isOreo;

public class UnzipService extends IntentService {
	public static final String ACTION_UPDATE = "com.multicraft.game.UPDATE";
	public static final String EXTRA_KEY_IN_FILE = "com.multicraft.game.file";
	public static final String ACTION_PROGRESS = "com.multicraft.game.progress";
	public static final String ACTION_FAILURE = "com.multicraft.game.failure";
	public static final int UNZIP_SUCCESS = -1;
	public static final int UNZIP_FAILURE = -2;
	private final int id = 1;
	private NotificationManager mNotifyManager;
	private String failureMessage;
	private boolean isSuccess = true;

	public UnzipService() {
		super("com.multicraft.game.UnzipService");
	}

	private void isDir(String dir, String unzipLocation) {
		File f = new File(unzipLocation + dir);
		if (!f.mkdirs() && !f.isDirectory())
			Bugsnag.leaveBreadcrumb(f + " (destination) folder was not created");
	}

	@Override
	protected void onHandleIntent(Intent intent) {
		createNotification();
		unzip(intent);
	}

	private String getSettings() {
		return getString(R.string.gdpr_main_text);
	}

	private void createNotification() {
		// There are hardcoding only for show it's just strings
		String name = "com.multicraft.game";
		String channelId = "MultiCraft channel"; // The user-visible name of the channel.
		String description = "notifications from MultiCraft"; // The user-visible description of the channel.
		Notification.Builder builder;
		if (mNotifyManager == null)
			mNotifyManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
		if (isOreo()) {
			int importance = NotificationManager.IMPORTANCE_LOW;
			NotificationChannel mChannel = null;
			if (mNotifyManager != null)
				mChannel = mNotifyManager.getNotificationChannel(channelId);
			if (mChannel == null) {
				mChannel = new NotificationChannel(channelId, name, importance);
				mChannel.setDescription(description);
				// Configure the notification channel, NO SOUND
				mChannel.setSound(null, null);
				mChannel.enableLights(false);
				mChannel.enableVibration(false);
				mNotifyManager.createNotificationChannel(mChannel);
			}
			builder = new Notification.Builder(this, channelId);
		} else
			builder = new Notification.Builder(this);
		builder.setContentTitle(getString(R.string.notification_title))
				.setContentText(getString(R.string.notification_description))
				.setSmallIcon(R.drawable.update);
		mNotifyManager.notify(id, builder.build());
	}

	private void unzip(Intent intent) {
		String[] zips = intent.getStringArrayExtra(EXTRA_KEY_IN_FILE);
		int per = 0;
		int size = getSummarySize(Objects.requireNonNull(zips));
		byte[] readBuffer = new byte[8192];
		for (String zip : zips) {
			File zipFile = new File(zip);
			LocalFileHeader localFileHeader;
			int readLen;
			try (FileInputStream fileInputStream = new FileInputStream(zipFile);
			     ZipInputStream zipInputStream = new ZipInputStream(fileInputStream)) {
				while ((localFileHeader = zipInputStream.getNextEntry()) != null) {
					String fileName = localFileHeader.getFileName();
					if (localFileHeader.isDirectory()) {
						++per;
						isDir(fileName, zipLocations.get(zip));
					} else {
						File extractedFile = new File(zipLocations.get(zip) + fileName);
						publishProgress(100 * ++per / size);
						try (OutputStream outputStream = new FileOutputStream(extractedFile)) {
							while ((readLen = zipInputStream.read(readBuffer)) != -1) {
								outputStream.write(readBuffer, 0, readLen);
							}
						}
					}
				}
			} catch (IOException e) {
				Bugsnag.notify(e);
				failureMessage = e.getLocalizedMessage();
				isSuccess = false;
			}
		}
	}

	private void publishProgress(int progress) {
		Intent intentUpdate = new Intent(ACTION_UPDATE);
		intentUpdate.putExtra(ACTION_PROGRESS, progress);
		if (!isSuccess) intentUpdate.putExtra(ACTION_FAILURE, failureMessage);
		sendBroadcast(intentUpdate);
	}

	private int getSummarySize(String[] zips) {
		int size = 0;
		for (String z : zips) {
			try {
				ZipFile zipFile = new ZipFile(z);
				size += zipFile.getFileHeaders().size();
			} catch (IOException e) {
				Bugsnag.notify(e);
			}
		}
		return size;
	}

	@Override
	public void onDestroy() {
		super.onDestroy();
		mNotifyManager.cancel(id);
		publishProgress(isSuccess ? UNZIP_SUCCESS : UNZIP_FAILURE);
	}
}
