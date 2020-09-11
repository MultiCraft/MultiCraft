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

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.ActivityNotFoundException;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.IntentSender;
import android.graphics.BlendMode;
import android.graphics.BlendModeColorFilter;
import android.graphics.Color;
import android.graphics.Point;
import android.graphics.PorterDuff;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.LayerDrawable;
import android.os.Bundle;
import android.os.Environment;
import android.text.Html;
import android.view.Display;
import android.view.View;
import android.view.WindowManager;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

import com.bugsnag.android.Bugsnag;
import com.google.android.play.core.appupdate.AppUpdateInfo;
import com.google.android.play.core.appupdate.AppUpdateManager;
import com.google.android.play.core.appupdate.AppUpdateManagerFactory;
import com.google.android.play.core.install.InstallStateUpdatedListener;
import com.google.android.play.core.install.model.AppUpdateType;
import com.google.android.play.core.install.model.InstallStatus;
import com.google.android.play.core.install.model.UpdateAvailability;
import com.google.android.play.core.tasks.Task;
import com.multicraft.game.callbacks.CallBackListener;
import com.multicraft.game.helpers.PermissionHelper;
import com.multicraft.game.helpers.PreferencesHelper;
import com.multicraft.game.helpers.Utilities;
import com.multicraft.game.helpers.VersionManagerHelper;

import org.apache.commons.io.FileUtils;
import org.apache.commons.io.FilenameUtils;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.concurrent.TimeUnit;

import io.reactivex.Completable;
import io.reactivex.Observable;
import io.reactivex.android.schedulers.AndroidSchedulers;
import io.reactivex.disposables.Disposable;
import io.reactivex.schedulers.Schedulers;

import static android.provider.Settings.ACTION_WIFI_SETTINGS;
import static android.provider.Settings.ACTION_WIRELESS_SETTINGS;
import static com.multicraft.game.UnzipService.ACTION_FAILURE;
import static com.multicraft.game.UnzipService.UNZIP_FAILURE;
import static com.multicraft.game.UnzipService.UNZIP_SUCCESS;
import static com.multicraft.game.helpers.ApiLevelHelper.isAndroidQ;
import static com.multicraft.game.helpers.ApiLevelHelper.isJellyBeanMR1;
import static com.multicraft.game.helpers.ApiLevelHelper.isLollipop;
import static com.multicraft.game.helpers.ApiLevelHelper.isOreo;
import static com.multicraft.game.helpers.PreferencesHelper.TAG_BUILD_NUMBER;
import static com.multicraft.game.helpers.PreferencesHelper.TAG_LAUNCH_TIMES;
import static com.multicraft.game.helpers.Utilities.addShortcut;
import static com.multicraft.game.helpers.Utilities.deleteFiles;
import static com.multicraft.game.helpers.Utilities.getIcon;
import static com.multicraft.game.helpers.Utilities.isArm64;
import static com.multicraft.game.helpers.Utilities.makeFullScreen;

public class MainActivity extends AppCompatActivity implements CallBackListener {
	public final static Map<String, String> zipLocations = new HashMap<>();
	private final static String UPDATE_LINK = "http://updates.multicraft.world/Android.json";
	private final static int REQUEST_CONNECTION = 104;
	private final static int REQUEST_UPDATE = 102;
	private static String FILES, WORLDS, GAMES, CACHE;
	private final String versionName = BuildConfig.VERSION_NAME;
	private String unzipLocation, appData;
	private int height, width;
	private ProgressBar mProgressBar, mProgressBarIndeterminate;
	private TextView mLoading;
	private VersionManagerHelper versionManagerHelper = null;
	private PreferencesHelper pf;
	private AppUpdateManager appUpdateManager;
	final InstallStateUpdatedListener listener = state -> {
		if (state.installStatus() == InstallStatus.DOWNLOADING) {
			if (mProgressBar != null) {
				int progress = (int) (state.bytesDownloaded() * 100 / state.totalBytesToDownload());
				showProgress(R.string.downloading, R.string.downloadingp, progress);
			}
		} else if (state.installStatus() == InstallStatus.DOWNLOADED) {
			appUpdateManager.completeUpdate();
		}
	};
	private Disposable connectionSub, versionManagerSub, cleanSub, copySub;
	private final BroadcastReceiver myReceiver = new BroadcastReceiver() {
		@Override
		public void onReceive(Context context, Intent intent) {
			int progress = 0;
			if (intent != null)
				progress = intent.getIntExtra(UnzipService.ACTION_PROGRESS, 0);
			if (progress >= 0) {
				if (mProgressBar != null) {
					showProgress(R.string.loading, R.string.loadingp, progress);
				}
			} else if (progress == UNZIP_FAILURE) {
				Toast.makeText(MainActivity.this, intent.getStringExtra(ACTION_FAILURE), Toast.LENGTH_LONG).show();
				showRestartDialog("");
			} else if (progress == UNZIP_SUCCESS) {
				deleteFiles(Arrays.asList(FILES, WORLDS, GAMES, CACHE));
				runGame();
			}
		}
	};
	private Task<AppUpdateInfo> appUpdateInfoTask;

	// helpful utilities
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		setContentView(R.layout.activity_main);
		pf = PreferencesHelper.getInstance(this);
		appUpdateManager = AppUpdateManagerFactory.create(this);
		appUpdateInfoTask = appUpdateManager.getAppUpdateInfo();
		IntentFilter filter = new IntentFilter(UnzipService.ACTION_UPDATE);
		registerReceiver(myReceiver, filter);
		if (!isTaskRoot()) {
			finish();
			return;
		}
		addLaunchTimes();
		PermissionHelper permission = new PermissionHelper(this);
		permission.setListener(this);
		permission.askPermissions();
	}

	@Override
	protected void onResume() {
		super.onResume();
		makeFullScreen(this);
		appUpdateInfoTask.addOnSuccessListener(appUpdateInfo -> {
			if (appUpdateInfo.installStatus() == InstallStatus.DOWNLOADED)
				appUpdateManager.completeUpdate();
		});
	}

	@Override
	public void onBackPressed() {
		// Prevent abrupt interruption when copy game files from assets
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
		if (connectionSub != null) connectionSub.dispose();
		if (versionManagerSub != null) versionManagerSub.dispose();
		if (cleanSub != null) cleanSub.dispose();
		if (copySub != null) copySub.dispose();
		appUpdateManager.unregisterListener(listener);
		unregisterReceiver(myReceiver);
	}

	private void initZipLocations() {
		// sdcard
		File externalDir = getExternalFilesDir(null); // /sdcard/Android/data/*
		unzipLocation = externalDir + File.separator;
		if (externalDir == null) {
			externalDir = Environment.getExternalStorageDirectory();  // /sdcard
			unzipLocation = externalDir + File.separator + "Android/data/com.multicraft.game" + File.separator;
		}
		// data/data
		appData = getFilesDir() + File.separator;
		// cache
		File cacheDir = getCacheDir(); // /data/data/*/cache
		String cachePath = cacheDir + File.separator;
		if (cacheDir == null) {
			cachePath = unzipLocation + "cache" + File.separator;
		}

		FILES = cachePath + "Files.zip";
		WORLDS = cachePath + "worlds.zip";
		GAMES = cachePath + "games.zip";
		CACHE = cachePath + "cache.zip";
		zipLocations.put(FILES, appData);
		zipLocations.put(GAMES, appData);
		zipLocations.put(WORLDS, unzipLocation);
		zipLocations.put(CACHE, unzipLocation);
	}

	private void addLaunchTimes() {
		int i = pf.getLaunchTimes();
		i++;
		pf.saveSettings(TAG_LAUNCH_TIMES, i);
	}

	private void createDataFolder() throws Exception {
		File folder = new File(unzipLocation);
		if (!folder.mkdirs() && !folder.isDirectory())
			throw new Exception(folder + " (unzipLocation) folder was not created");
	}

	// interface
	private void showProgress(int textMessage, int progressMessage, int progress) {
		if (mProgressBar.getVisibility() == View.GONE) {
			updateViews(textMessage, View.VISIBLE, View.GONE, View.VISIBLE);
			mProgressBar.setProgress(0);
		} else if (progress > 0) {
			mLoading.setText(String.format(getResources().getString(progressMessage), progress));
			mProgressBar.setProgress(progress);
			// colorize the progress bar
			Drawable progressDrawable = ((LayerDrawable)
					mProgressBar.getProgressDrawable()).getDrawable(1);
			int color = Color.rgb(255 - progress * 2, progress * 2, 25);
			if (isAndroidQ())
				progressDrawable.setColorFilter(new BlendModeColorFilter(color, BlendMode.SRC_IN));
			else
				progressDrawable.setColorFilter(color, PorterDuff.Mode.SRC_IN);
		}
	}

	// real screen resolution
	private void getDefaultResolution() {
		Display display = getWindowManager().getDefaultDisplay();
		Point size = new Point();
		if (isJellyBeanMR1())
			display.getRealSize(size);
		else
			display.getSize(size);
		height = Math.min(size.x, size.y);
		width = Math.max(size.x, size.y);
	}

	@Override
	public void onWindowFocusChanged(boolean hasFocus) {
		super.onWindowFocusChanged(hasFocus);
		if (hasFocus)
			makeFullScreen(this);
	}

	private void init() {
		mProgressBar = findViewById(R.id.PB1);
		mProgressBarIndeterminate = findViewById(R.id.PB2);
		mLoading = findViewById(R.id.tv_progress);
		if (!pf.isCreateShortcut() && !isOreo())
			addShortcut(this);
		checkAppVersion();
	}

	void showUpdateDialog() {
		updateViews(R.string.loading, View.VISIBLE, View.VISIBLE, View.GONE);
		AlertDialog.Builder builder = new AlertDialog.Builder(this);
		builder.setIcon(getIcon(this))
				.setTitle(R.string.available)
				.setMessage(Html.fromHtml(
						versionManagerHelper.getMessage(), null, versionManagerHelper.getCustomTagHandler()))
				.setPositiveButton(R.string.update, (dialogInterface, i) -> {
					versionManagerHelper.updateNow(versionManagerHelper.getUpdateUrl());
					finish();
				})
				.setNeutralButton(R.string.later, (dialogInterface, i) -> {
					versionManagerHelper.remindMeLater();
					startNative();
				});
		builder.setCancelable(false);
		final AlertDialog dialog = builder.create();
		if (!isFinishing())
			dialog.show();
	}

	public void startUpdate() {
		appUpdateInfoTask.addOnSuccessListener(appUpdateInfo -> {
			if (appUpdateInfo.updateAvailability() == UpdateAvailability.UPDATE_AVAILABLE
					&& appUpdateInfo.isUpdateTypeAllowed(AppUpdateType.FLEXIBLE)) {
				try {
					appUpdateManager.startUpdateFlowForResult(
							appUpdateInfo, AppUpdateType.FLEXIBLE, this, REQUEST_UPDATE);
				} catch (IntentSender.SendIntentException e) {
					Bugsnag.notify(e);
					showUpdateDialog();
				}
			} else {
				showUpdateDialog();
			}
		});
		appUpdateInfoTask.addOnFailureListener(e -> {
			Bugsnag.notify(e);
			showUpdateDialog();
		});
	}

	private void checkUrlVersion() {
		versionManagerHelper = new VersionManagerHelper(this);
		if (versionManagerHelper.isCheckVersion())
			versionManagerSub = Observable.fromCallable(() -> Utilities.getJson(UPDATE_LINK))
					.subscribeOn(Schedulers.io())
					.observeOn(AndroidSchedulers.mainThread())
					.timeout(3000, TimeUnit.MILLISECONDS)
					.subscribe(result -> isShowDialog(versionManagerHelper.isShow(result)),
							throwable -> runOnUiThread(() -> isShowDialog(false)));
		else isShowDialog(false);
	}

	private void runGame() {
		pf.saveSettings(TAG_BUILD_NUMBER, versionName);
		connectionSub = checkConnection();
	}

	private void startNative() {
		getDefaultResolution();
		Intent intent = new Intent(this, GameActivity.class);
		intent.putExtra("height", height);
		intent.putExtra("width", width);
		intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_CLEAR_TASK);
		startActivity(intent);
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);
		if (requestCode == REQUEST_CONNECTION) {
			checkUrlVersion();
		} else if (requestCode == REQUEST_UPDATE) {
			if (resultCode == RESULT_OK)
				appUpdateManager.registerListener(listener);
			else
				startNative();
		}
	}

	private void cleanUpOldFiles(boolean isAll) {
		updateViews(R.string.preparing, View.VISIBLE, View.VISIBLE, View.GONE);
		List<String> filesList;
		if (isAll)
			filesList = Collections.singletonList(unzipLocation);
		else {
			filesList = Arrays.asList(unzipLocation + "cache",
					unzipLocation + "builtin", appData + "builtin",
					unzipLocation + "games", appData + "games",
					unzipLocation + "debug.txt");
		}
		cleanSub = Observable.fromCallable(() -> deleteFiles(filesList))
				.subscribeOn(Schedulers.io())
				.observeOn(AndroidSchedulers.mainThread())
				.subscribe(result -> startCopy(isAll));
	}

	private void checkAppVersion() {
		if (pf.getBuildNumber().equals(versionName)) {
			mProgressBarIndeterminate.setVisibility(View.VISIBLE);
			runGame();
		} else {
			initZipLocations();
			try {
				createDataFolder();
			} catch (Exception e) {
				Bugsnag.notify(e);
				showRestartDialog("");
			}
			cleanUpOldFiles(pf.getBuildNumber().equals("0"));
		}
	}

	public void updateViews(int text, int textVisib, int progressIndetermVisib, int progressVisib) {
		mLoading.setText(text);
		mLoading.setVisibility(textVisib);
		mProgressBarIndeterminate.setVisibility(progressIndetermVisib);
		mProgressBar.setVisibility(progressVisib);
	}

	public void isShowDialog(boolean flag) {
		if (flag) {
			if (isLollipop())
				startUpdate();
			else
				showUpdateDialog();
		} else
			startNative();
	}

	private Disposable checkConnection() {
		return Observable.fromCallable(Utilities::isReachable)
				.subscribeOn(Schedulers.io())
				.observeOn(AndroidSchedulers.mainThread())
				.timeout(4000, TimeUnit.MILLISECONDS)
				.subscribe(result -> {
							if (result) checkUrlVersion();
							else showConnectionDialog();
						},
						throwable -> runOnUiThread(this::showConnectionDialog));
	}

	private void startCopy(boolean isAll) {
		String[] zips;
		if (isAll) {
			if (isArm64())
				zips = new String[]{FILES, WORLDS, GAMES, CACHE};
			else
				zips = new String[]{FILES, WORLDS, GAMES};
		} else {
			if (isArm64())
				zips = new String[]{FILES, GAMES, CACHE};
			else
				zips = new String[]{FILES, GAMES};
		}
		copySub = Completable.fromAction(() -> copyAssets(zips))
				.subscribeOn(Schedulers.io())
				.observeOn(AndroidSchedulers.mainThread())
				.subscribe(() -> startUnzipService(zips));
	}

	private void copyAssets(String[] zips) {
		for (String zipName : zips) {
			String filename = FilenameUtils.getName(zipName);
			try (InputStream in = getAssets().open(filename)) {
				FileUtils.copyInputStreamToFile(in, new File(zipName));
			} catch (IOException e) {
				Bugsnag.notify(e);
				if (Objects.requireNonNull(e.getLocalizedMessage()).contains("ENOSPC"))
					showRestartDialog("ENOSPC");
				else showRestartDialog("UKNWN");
			}
		}
	}

	private void startUnzipService(String[] file) {
		Intent intent = new Intent(this, UnzipService.class);
		intent.putExtra(UnzipService.EXTRA_KEY_IN_FILE, file);
		startService(intent);
	}

	private void showRestartDialog(final String source) {
		String message;
		if ("ENOSPC".equals(source))
			message = getString(R.string.no_space);
		else
			message = getString(R.string.restart);
		AlertDialog.Builder builder = new AlertDialog.Builder(this);
		builder.setMessage(message)
				.setPositiveButton(R.string.ok, (dialogInterface, i) -> restartApp());
		builder.setCancelable(false);
		final AlertDialog dialog = builder.create();
		if (!isFinishing())
			dialog.show();
	}

	private void restartApp() {
		Intent intent = new Intent(getApplicationContext(), MainActivity.class);
		int mPendingIntentId = 1337;
		AlarmManager mgr = (AlarmManager) getApplicationContext().getSystemService(Context.ALARM_SERVICE);
		if (mgr != null)
			mgr.set(AlarmManager.RTC, System.currentTimeMillis(), PendingIntent.getActivity(
					getApplicationContext(), mPendingIntentId, intent, PendingIntent.FLAG_CANCEL_CURRENT));
		System.exit(0);
	}

	@Override
	public void onEvent(boolean isContinue) {
		if (isFinishing()) return;
		if (isContinue) init();
		else finish();
	}

	private void showConnectionDialog() {
		AlertDialog.Builder builder = new AlertDialog.Builder(this);
		builder.setMessage(R.string.conn_message)
				.setPositiveButton(R.string.conn_wifi, (dialogInterface, i) -> startHandledActivity(new Intent(ACTION_WIFI_SETTINGS)))
				.setNegativeButton(R.string.conn_mobile, (dialogInterface, i) -> startHandledActivity(new Intent(ACTION_WIRELESS_SETTINGS)))
				.setNeutralButton(R.string.ignore, (dialogInterface, i) -> startNative());
		builder.setCancelable(false);
		final AlertDialog dialog = builder.create();
		if (!isFinishing())
			dialog.show();
	}

	private void startHandledActivity(Intent intent) {
		try {
			startActivityForResult(intent, REQUEST_CONNECTION);
		} catch (ActivityNotFoundException e) {
			Bugsnag.notify(e);
			startNative();
		}
	}
}
