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

package com.multicraft.game.helpers;

import android.app.Activity;
import android.app.ActivityManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.view.View;

import com.bugsnag.android.Bugsnag;
import com.multicraft.game.BuildConfig;
import com.multicraft.game.MainActivity;
import com.multicraft.game.R;

import org.apache.commons.io.FileUtils;

import java.io.File;
import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.TimeUnit;

import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.Response;

import static com.multicraft.game.helpers.ApiLevelHelper.isKitkat;
import static com.multicraft.game.helpers.ApiLevelHelper.isLollipop;
import static com.multicraft.game.helpers.PreferencesHelper.TAG_SHORTCUT_EXIST;

public class Utilities {
	private static final String appPackage = BuildConfig.APPLICATION_ID;

	private static boolean isInternetAvailable(String url) {
		try {
			HttpURLConnection urlc =
					(HttpURLConnection) new URL(url).openConnection();
			urlc.setRequestProperty("Connection", "close");
			urlc.setConnectTimeout(2000);
			urlc.connect();
			int ResponseCode = urlc.getResponseCode();
			return ResponseCode == HttpURLConnection.HTTP_NO_CONTENT ||
					ResponseCode == HttpURLConnection.HTTP_OK;
		} catch (IOException e) {
			return false;
		}
	}

	public static boolean isReachable() {
		return isInternetAvailable("http://clients3.google.com/generate_204") ||
				isInternetAvailable("http://servers.multicraft.world");
	}

	public static boolean deleteFiles(List<String> files) {
		boolean result = true;
		for (String f : files) {
			File file = new File(f);
			if (file.exists()) {
				result = result && FileUtils.deleteQuietly(file);
			}
		}
		return result;
	}

	public static boolean isArm64() {
		return isLollipop() && (Build.SUPPORTED_64_BIT_ABIS.length > 0);
	}


	public static String getStoreUrl() {
		return "market://details?id=" + appPackage;
	}

	public static void makeFullScreen(Activity activity) {
		if (isKitkat())
			activity.getWindow().getDecorView().setSystemUiVisibility(
					View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
							View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
							View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
	}

	public static Drawable getIcon(Activity activity) {
		try {
			return activity.getPackageManager().getApplicationIcon(activity.getPackageName());
		} catch (PackageManager.NameNotFoundException e) {
			Bugsnag.notify(e);
			return activity.getResources().getDrawable(R.mipmap.ic_launcher);
		}
	}

	public static void addShortcut(Activity activity) {
		ActivityManager activityManager =
				(ActivityManager) activity.getSystemService(Context.ACTIVITY_SERVICE);
		int size = 0;
		if (activityManager != null)
			size = activityManager.getLauncherLargeIconSize();
		Bitmap shortcutIconBitmap = ((BitmapDrawable) getIcon(activity)).getBitmap();
		if (shortcutIconBitmap.getWidth() != size || shortcutIconBitmap.getHeight() != size)
			shortcutIconBitmap = Bitmap.createScaledBitmap(shortcutIconBitmap, size, size, true);
		PreferencesHelper.getInstance(activity).saveSettings(TAG_SHORTCUT_EXIST, true);
		Intent shortcutIntent = new Intent(activity, MainActivity.class);
		shortcutIntent.setAction(Intent.ACTION_MAIN);
		Intent addIntent = new Intent();
		addIntent.putExtra("duplicate", false);
		addIntent.putExtra(Intent.EXTRA_SHORTCUT_INTENT, shortcutIntent);
		addIntent.putExtra(Intent.EXTRA_SHORTCUT_NAME, activity.getResources().getString(R.string.app_name));
		addIntent.putExtra(Intent.EXTRA_SHORTCUT_ICON, shortcutIconBitmap);
		addIntent.setAction("com.android.launcher.action.INSTALL_SHORTCUT");
		activity.getApplicationContext().sendBroadcast(addIntent);
	}

	public static String getJson(String url) {
		OkHttpClient client = new OkHttpClient.Builder()
				.callTimeout(3000, TimeUnit.MILLISECONDS)
				.build();
		Request request = new Request.Builder()
				.url(url)
				.build();
		try {
			Response response = client.newCall(request).execute();
			return Objects.requireNonNull(response.body()).string();
		} catch (IOException | NullPointerException e) {
			// nothing
		}
		return "{}";
	}
}
