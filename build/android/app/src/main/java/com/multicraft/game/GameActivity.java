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

import android.app.ActivityManager;
import android.app.NativeActivity;
import android.content.Context;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.os.Bundle;
import android.text.InputType;
import android.text.method.LinkMovementMethod;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.WindowManager.LayoutParams;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.TextView;

import androidx.appcompat.app.AlertDialog;

import com.bugsnag.android.Bugsnag;
import com.multicraft.game.helpers.PreferencesHelper;
import com.multicraft.game.helpers.RateMeHelper;
import com.multicraft.game.helpers.Utilities;

import org.json.JSONObject;
import org.json.JSONTokener;

import io.reactivex.Completable;
import io.reactivex.Observable;
import io.reactivex.android.schedulers.AndroidSchedulers;
import io.reactivex.disposables.Disposable;
import io.reactivex.schedulers.Schedulers;

import static android.content.res.Configuration.KEYBOARD_QWERTY;
import static com.multicraft.game.helpers.AdManager.initAd;
import static com.multicraft.game.helpers.AdManager.setAdsCallback;
import static com.multicraft.game.helpers.AdManager.startAd;
import static com.multicraft.game.helpers.AdManager.stopAd;
import static com.multicraft.game.helpers.Constants.versionCode;
import static com.multicraft.game.helpers.PreferencesHelper.IS_ASK_CONSENT;
import static com.multicraft.game.helpers.PreferencesHelper.TAG_EXIT_GAME_COUNT;
import static com.multicraft.game.helpers.PreferencesHelper.TAG_LAST_RATE_VERSION_CODE;
import static com.multicraft.game.helpers.PreferencesHelper.getInstance;
import static com.multicraft.game.helpers.Utilities.getIcon;
import static com.multicraft.game.helpers.Utilities.makeFullScreen;

public class GameActivity extends NativeActivity {
	static {
		try {
			System.loadLibrary("MultiCraft");
		} catch (UnsatisfiedLinkError | OutOfMemoryError e) {
			Bugsnag.notify(e);
			System.exit(0);
		} catch (IllegalArgumentException i) {
			Bugsnag.notify(i);
			System.exit(0);
		} catch (Error | Exception e) {
			Bugsnag.notify(e);
			System.exit(0);
		}
	}

	private int messageReturnCode = -1;
	private String messageReturnValue = "";
	private int height, width;
	private boolean consent, isMultiPlayer;
	private PreferencesHelper pf;
	private Disposable adInitSub, gdprSub;
	private boolean hasKeyboard;

	public static native void pauseGame();

	public static native void keyboardEvent(boolean keyboard);

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		Bundle bundle = getIntent().getExtras();
		Resources resources = getResources();
		height = bundle != null ? bundle.getInt("height", 0) : resources.getDisplayMetrics().heightPixels;
		width = bundle != null ? bundle.getInt("width", 0) : resources.getDisplayMetrics().widthPixels;
		getWindow().addFlags(LayoutParams.FLAG_KEEP_SCREEN_ON);
		hasKeyboard = !(resources.getConfiguration().hardKeyboardHidden == KEYBOARD_QWERTY);
		keyboardEvent(hasKeyboard);
		pf = getInstance(this);
		RateMeHelper.onStart(this);
		askGdpr();
	}

	private void subscribeAds() {
		if (pf.isAdsEnable()) {
			adInitSub = Completable.fromAction(() -> initAd(this, consent))
					.subscribeOn(Schedulers.io())
					.observeOn(AndroidSchedulers.mainThread())
					.subscribe(() -> setAdsCallback(this));
		}
	}

	// GDPR check
	private void askGdpr() {
		if (pf.isAskConsent())
			isGdprSubject();
		else {
			consent = true;
			subscribeAds();
		}
	}

	private void isGdprSubject() {
		gdprSub = Observable.fromCallable(() -> Utilities.getJson("http://adservice.google.com/getconfig/pubvendors"))
				.subscribeOn(Schedulers.io())
				.observeOn(AndroidSchedulers.mainThread())
				.subscribe(result -> {
							JSONObject json = new JSONObject(new JSONTokener(result));
							if (json.getBoolean("is_request_in_eea_or_unknown"))
								showGdprDialog();
							else {
								consent = true;
								subscribeAds();
							}
						},
						throwable -> {
							consent = true;
							subscribeAds();
						});
	}

	private void showGdprDialog() {
		AlertDialog.Builder builder = new AlertDialog.Builder(this);
		builder.setIcon(getIcon(this));
		builder.setTitle(getString(R.string.app_name));
		TextView tv = new TextView(this);
		tv.setText(R.string.gdpr_main_text);
		tv.setTypeface(null);
		tv.setPadding(20, 0, 20, 0);
		tv.setGravity(Gravity.CENTER);
		tv.setMovementMethod(LinkMovementMethod.getInstance());
		builder.setView(tv);
		builder.setPositiveButton(getString(R.string.gdpr_agree), (dialogInterface, i) -> {
			dialogInterface.dismiss();
			pf.saveSettings(IS_ASK_CONSENT, false);
			consent = true;
			subscribeAds();
		});
		builder.setNegativeButton(getString(R.string.gdpr_disagree), (dialogInterface, i) -> {
			dialogInterface.dismiss();
			pf.saveSettings(IS_ASK_CONSENT, false);
			consent = false;
			subscribeAds();
		});
		builder.setCancelable(false);
		final AlertDialog dialog = builder.create();
		if (!isFinishing())
			dialog.show();
	}

	private void checkRateDialog() {
		if (RateMeHelper.shouldShowRateDialog()) {
			pf.saveSettings(TAG_LAST_RATE_VERSION_CODE, versionCode);
			runOnUiThread(() -> RateMeHelper.showRateDialog(this));
		}
	}

	@Override
	public void onWindowFocusChanged(boolean hasFocus) {
		super.onWindowFocusChanged(hasFocus);
		if (hasFocus)
			makeFullScreen(this);
	}

	@Override
	protected void onResume() {
		super.onResume();
		makeFullScreen(this);
	}

	@Override
	public void onBackPressed() {
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
		if (adInitSub != null) adInitSub.dispose();
		if (gdprSub != null) gdprSub.dispose();
	}

	public void showDialog(String acceptButton, String hint, String current, int editType) {
		runOnUiThread(() -> showDialogUI(hint, current, editType));
	}

	@Override
	protected void onPause() {
		super.onPause();
		pauseGame();
	}

	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		super.onConfigurationChanged(newConfig);
		boolean statusKeyboard = !(getResources().getConfiguration().hardKeyboardHidden == KEYBOARD_QWERTY);
		if (hasKeyboard != statusKeyboard) {
			hasKeyboard = statusKeyboard;
			keyboardEvent(hasKeyboard);
		}
	}

	private void showDialogUI(String hint, String current, int editType) {
		final AlertDialog.Builder builder = new AlertDialog.Builder(this);
		EditText editText = new CustomEditText(this);
		builder.setView(editText);
		AlertDialog alertDialog = builder.create();
		editText.requestFocus();
		editText.setHint(hint);
		editText.setText(current);
		final InputMethodManager imm = (InputMethodManager) getSystemService(INPUT_METHOD_SERVICE);
		imm.toggleSoftInput(InputMethodManager.SHOW_FORCED,
				InputMethodManager.HIDE_IMPLICIT_ONLY);
		if (editType == 1)
			editText.setInputType(InputType.TYPE_CLASS_TEXT |
					InputType.TYPE_TEXT_FLAG_MULTI_LINE);
		else if (editType == 3)
			editText.setInputType(InputType.TYPE_CLASS_TEXT |
					InputType.TYPE_TEXT_VARIATION_PASSWORD);
		else
			editText.setInputType(InputType.TYPE_CLASS_TEXT);
		editText.setSelection(editText.getText().length());
		editText.setOnKeyListener((view, KeyCode, event) -> {
			if (KeyCode == KeyEvent.KEYCODE_ENTER) {
				imm.hideSoftInputFromWindow(editText.getWindowToken(), 0);
				messageReturnCode = 0;
				messageReturnValue = editText.getText().toString();
				alertDialog.dismiss();
				return true;
			}
			return false;
		});
		alertDialog.show();
		alertDialog.setOnCancelListener(dialog -> {
			getWindow().setSoftInputMode(LayoutParams.SOFT_INPUT_STATE_ALWAYS_HIDDEN);
			messageReturnValue = current;
			messageReturnCode = -1;
		});
	}

	public int getDialogState() {
		return messageReturnCode;
	}

	public String getDialogValue() {
		messageReturnCode = -1;
		return messageReturnValue;
	}

	public float getDensity() {
		return getResources().getDisplayMetrics().density;
	}

	public int getDisplayHeight() {
		return height;
	}

	public int getDisplayWidth() {
		return width;
	}

	public float getMemoryMax() {
		ActivityManager actManager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
		ActivityManager.MemoryInfo memInfo = new ActivityManager.MemoryInfo();
		float memory = 1.0f;
		if (actManager != null) {
			actManager.getMemoryInfo(memInfo);
			memory = memInfo.totalMem * 1.0f / (1024 * 1024 * 1024);
			memory = Math.round(memory * 100) / 100.0f;
		}
		return memory;
	}

	public void notifyServerConnect(boolean multiplayer) {
		isMultiPlayer = multiplayer;
		if (isMultiPlayer)
			stopAd();
	}

	public void notifyExitGame() {
		pf.saveSettings(TAG_EXIT_GAME_COUNT, pf.getExitGameCount() + 1);
		if (!isFinishing()) {
			if (isMultiPlayer) {
				if (pf.isAdsEnable())
					startAd(this, false, true);
			} else
				checkRateDialog();
		}
	}
}
