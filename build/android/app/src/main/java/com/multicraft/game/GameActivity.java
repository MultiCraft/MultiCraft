/*
MultiCraft
Copyright (C) 2014-2021 MoNTE48, Maksim Gamarnik <MoNTE48@mail.ua>
Copyright (C) 2014-2021 ubulem,  Bektur Mambetov <berkut87@gmail.com>

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

import static android.content.res.Configuration.HARDKEYBOARDHIDDEN_NO;
import static android.text.InputType.TYPE_CLASS_TEXT;
import static android.text.InputType.TYPE_TEXT_FLAG_MULTI_LINE;
import static android.text.InputType.TYPE_TEXT_VARIATION_PASSWORD;
import static com.multicraft.game.helpers.Utilities.finishApp;
import static com.multicraft.game.helpers.Utilities.getTotalMem;
import static com.multicraft.game.helpers.Utilities.makeFullScreen;

import android.app.NativeActivity;
import android.content.Intent;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.WindowManager.LayoutParams;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;

import androidx.appcompat.app.AlertDialog;

import com.multicraft.game.databinding.InputTextBinding;

public class GameActivity extends NativeActivity {
	public static boolean isMultiPlayer;

	static {
		try {
			System.loadLibrary("MultiCraft");
		} catch (UnsatisfiedLinkError e) {
			System.exit(0);
		}
	}

	private int messageReturnCode = -1;
	private String messageReturnValue = "";
	private boolean hasKeyboard;

	public static native void pauseGame();

	public static native void keyboardEvent(boolean keyboard);

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		getWindow().addFlags(LayoutParams.FLAG_KEEP_SCREEN_ON);
		hasKeyboard = getResources().getConfiguration().hardKeyboardHidden == HARDKEYBOARDHIDDEN_NO;
		keyboardEvent(hasKeyboard);
	}

	@Override
	public void onWindowFocusChanged(boolean hasFocus) {
		super.onWindowFocusChanged(hasFocus);
		if (hasFocus) makeFullScreen(getWindow());
	}

	@Override
	public void onBackPressed() {
		// Ignore the back press so MultiCraft can handle it
	}

	@Override
	protected void onPause() {
		super.onPause();
		pauseGame();
	}

	@Override
	protected void onResume() {
		super.onResume();
		makeFullScreen(getWindow());
	}

	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		super.onConfigurationChanged(newConfig);
		boolean statusKeyboard = getResources().getConfiguration().hardKeyboardHidden == HARDKEYBOARDHIDDEN_NO;
		if (hasKeyboard != statusKeyboard) {
			hasKeyboard = statusKeyboard;
			keyboardEvent(hasKeyboard);
		}
	}

	@SuppressWarnings("unused")
	public void showDialog(String s, String hint, String current, int editType) {
		runOnUiThread(() -> showDialogUI(hint, current, editType));
	}

	private void showDialogUI(String hint, String current, int editType) {
		final AlertDialog.Builder builder = new AlertDialog.Builder(this);
		if (editType == 1) builder.setPositiveButton(R.string.done, null);
		InputTextBinding binding = InputTextBinding.inflate(getLayoutInflater());
		String hintText = (!hint.isEmpty()) ? hint : getResources().getString(
				(editType == 3) ? R.string.input_password : R.string.input_text);
		binding.inputLayout.setHint(hintText);
		builder.setView(binding.getRoot());
		AlertDialog alertDialog = builder.create();
		EditText editText = binding.editText;
		editText.requestFocus();
		editText.setText(current);
		if (editType != 1) editText.setImeOptions(EditorInfo.IME_FLAG_NO_FULLSCREEN);
		final InputMethodManager imm = (InputMethodManager) getSystemService(INPUT_METHOD_SERVICE);
		int inputType = TYPE_CLASS_TEXT;
		if (editType == 1) {
			inputType = inputType | TYPE_TEXT_FLAG_MULTI_LINE;
			editText.setMaxLines(8);
		} else if (editType == 3)
			inputType = inputType | TYPE_TEXT_VARIATION_PASSWORD;
		editText.setInputType(inputType);
		editText.setSelection(editText.getText().length());
		editText.setOnEditorActionListener((view, KeyCode, event) -> {
			if (KeyCode == KeyEvent.KEYCODE_ENTER || KeyCode == KeyEvent.KEYCODE_ENDCALL) {
				imm.hideSoftInputFromWindow(editText.getWindowToken(), 0);
				messageReturnCode = 0;
				messageReturnValue = editText.getText().toString();
				alertDialog.dismiss();
				return true;
			}
			return false;
		});
		// should be above `show()`
		alertDialog.getWindow().setSoftInputMode(LayoutParams.SOFT_INPUT_STATE_VISIBLE);
		alertDialog.show();
		Button button = alertDialog.getButton(AlertDialog.BUTTON_POSITIVE);
		if (button != null) {
			button.setOnClickListener(view -> {
				imm.hideSoftInputFromWindow(editText.getWindowToken(), 0);
				messageReturnCode = 0;
				messageReturnValue = editText.getText().toString();
				alertDialog.dismiss();
			});
		}
		alertDialog.setOnCancelListener(dialog -> {
			getWindow().setSoftInputMode(LayoutParams.SOFT_INPUT_STATE_ALWAYS_HIDDEN);
			messageReturnValue = current;
			messageReturnCode = -1;
		});
	}

	public int getDialogState() {
		return messageReturnCode;
	}

	@SuppressWarnings("unused")
	public String getDialogValue() {
		messageReturnCode = -1;
		return messageReturnValue;
	}

	public float getDensity() {
		return getResources().getDisplayMetrics().density;
	}

	public float getMemoryMax() {
		return getTotalMem(this);
	}

	public void notifyServerConnect(boolean multiplayer) {
		isMultiPlayer = multiplayer;
	}

	public void notifyExitGame() {
	}

	@SuppressWarnings("unused")
	public void openURI(String uri) {
		try {
			Intent browserIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(uri));
			startActivity(browserIntent);
		} catch (Exception ignored) {
		}
	}

	@SuppressWarnings("unused")
	public void finishGame(String exc) {
		finishApp(true, this);
	}

	@SuppressWarnings("unused")
	public void handleError(String exc) {
	}
}
