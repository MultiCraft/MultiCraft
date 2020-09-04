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

import android.app.Dialog;
import android.app.NativeActivity;
import android.content.Intent;
import android.net.Uri;
import android.view.View;
import android.widget.RatingBar;
import android.widget.Toast;

import com.multicraft.game.R;

import static com.multicraft.game.helpers.ApiLevelHelper.isKitkat;
import static com.multicraft.game.helpers.Constants.SESSION_COUNT;
import static com.multicraft.game.helpers.PreferencesHelper.TAG_OPT_OUT;
import static com.multicraft.game.helpers.Utilities.getStoreUrl;

public class RateMeHelper {
	private static PreferencesHelper pf;

	public static void onStart(NativeActivity activity) {
		pf = PreferencesHelper.getInstance(activity);
	}

	private static boolean isEnoughTimePassed() {
		return (pf.getLastRateVersionCode() == 0) && pf.getLaunchTimes() >= SESSION_COUNT
				&& pf.getExitGameCount() >= SESSION_COUNT;
	}

	private static boolean isWorthToReview() {
		return (pf.getLastRateVersionCode() != 0)
				&& pf.getRateMinVersionCode() > pf.getLastRateVersionCode();
	}

	public static boolean shouldAskForReview() {
		return pf.isReviewEnable() && (isEnoughTimePassed() || isWorthToReview());
	}

	public static boolean shouldShowRateDialog() {
		if (pf.isOptOut())
			return false;
		else {
			return shouldAskForReview();
		}
	}

	public static void showRateDialog(NativeActivity activity) {
		final Dialog dialog = new Dialog(activity, R.style.RateMe);
		dialog.setCancelable(false);
		if (isKitkat())
			dialog.getWindow().getDecorView().setSystemUiVisibility(
					View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
							| View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
							| View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION);
		dialog.setContentView(R.layout.rate_dialog);
		RatingBar ratingBar = dialog.findViewById(R.id.ratingBar);
		ratingBar.setOnRatingBarChangeListener((ratingBar1, rating, fromUser) -> {
			if (rating >= 4) {
				try {
					Toast.makeText(activity, R.string.thank, Toast.LENGTH_LONG).show();
					Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(getStoreUrl()));
					activity.startActivity(intent);
				} catch (Exception e) {
					// nothing
				}
				dialog.dismiss();
				pf.saveSettings(TAG_OPT_OUT, true);
			} else {
				dialog.dismiss();
				Toast.makeText(activity, R.string.sad, Toast.LENGTH_LONG).show();
			}
		});
		if (!activity.isFinishing())
			dialog.show();
	}
}
