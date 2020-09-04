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

import android.annotation.SuppressLint;

import androidx.appcompat.app.AppCompatActivity;

import com.multicraft.game.R;
import com.multicraft.game.callbacks.CallBackListener;
import com.tedpark.tedpermission.rx2.TedRx2Permission;

import static android.Manifest.permission.ACCESS_FINE_LOCATION;
import static android.Manifest.permission.WRITE_EXTERNAL_STORAGE;

public class PermissionHelper {
	private final AppCompatActivity activity;
	private CallBackListener listener;
	private PreferencesHelper pf;

	public PermissionHelper(AppCompatActivity activity) {
		this.activity = activity;
	}

	public void setListener(CallBackListener listener) {
		this.listener = listener;
	}

	public void askPermissions() {
		pf = PreferencesHelper.getInstance(activity);
		askStoragePermissions();
	}

	// permission block
	@SuppressLint("CheckResult")
	private void askStoragePermissions() {
		TedRx2Permission.with(activity)
				.setPermissions(WRITE_EXTERNAL_STORAGE)
				.request()
				.subscribe(tedPermissionResult -> {
					if (tedPermissionResult.isGranted()) {
						if (pf.getLaunchTimes() % 3 == 1)
							askLocationPermissions();
						else listener.onEvent(true);
					} else {
						if (TedRx2Permission.canRequestPermission(activity, WRITE_EXTERNAL_STORAGE))
							askStorageRationalePermissions();
						else askStorageWhenDoNotShow();
					}
				});
	}

	// storage permissions block
	@SuppressLint("CheckResult")
	private void askStorageRationalePermissions() {
		TedRx2Permission.with(activity)
				.setRationaleMessage(R.string.explain)
				.setDeniedMessage(R.string.denied)
				.setDeniedCloseButtonText(R.string.close_game)
				.setGotoSettingButtonText(R.string.settings)
				.setPermissions(WRITE_EXTERNAL_STORAGE)
				.request()
				.subscribe(tedPermissionResult -> {
					if (tedPermissionResult.isGranted()) {
						if (pf.getLaunchTimes() % 3 == 1)
							askLocationPermissions();
						else listener.onEvent(true);
					} else {
						listener.onEvent(false);
					}
				});
	}

	@SuppressLint("CheckResult")
	private void askStorageWhenDoNotShow() {
		TedRx2Permission.with(activity)
				.setDeniedMessage(R.string.denied)
				.setDeniedCloseButtonText(R.string.close_game)
				.setGotoSettingButtonText(R.string.settings)
				.setPermissions(WRITE_EXTERNAL_STORAGE)
				.request()
				.subscribe(tedPermissionResult -> {
					if (tedPermissionResult.isGranted()) {
						if (pf.getLaunchTimes() % 3 == 1)
							askLocationPermissions();
						else listener.onEvent(true);
					} else {
						listener.onEvent(false);
					}
				});
	}

	// location permissions block
	@SuppressLint("CheckResult")
	private void askLocationPermissions() {
		TedRx2Permission.with(activity)
				.setPermissions(ACCESS_FINE_LOCATION)
				.request()
				.subscribe(tedPermissionResult -> {
					if (tedPermissionResult.isGranted()) {
						listener.onEvent(true);
					} else {
						if (TedRx2Permission.canRequestPermission(activity, ACCESS_FINE_LOCATION))
							askLocationRationalePermissions();
						else listener.onEvent(true);
					}
				});
	}

	@SuppressLint("CheckResult")
	private void askLocationRationalePermissions() {
		TedRx2Permission.with(activity)
				.setRationaleMessage(R.string.location)
				.setPermissions(ACCESS_FINE_LOCATION)
				.request()
				.subscribe(tedPermissionResult -> listener.onEvent(true));
	}
}
