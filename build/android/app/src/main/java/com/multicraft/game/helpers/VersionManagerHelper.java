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

import android.content.Intent;
import android.net.Uri;
import android.text.Editable;
import android.text.Html;

import androidx.appcompat.app.AppCompatActivity;
import androidx.preference.PreferenceManager;

import com.google.gson.Gson;
import com.multicraft.game.JsonSettings;

import org.xml.sax.XMLReader;

import java.util.Calendar;
import java.util.List;
import java.util.Locale;

import static com.multicraft.game.helpers.Constants.versionCode;
import static com.multicraft.game.helpers.PreferencesHelper.ADS_DELAY;
import static com.multicraft.game.helpers.PreferencesHelper.ADS_ENABLE;
import static com.multicraft.game.helpers.PreferencesHelper.ADS_REPEAT;
import static com.multicraft.game.helpers.PreferencesHelper.TAG_RATE_MIN_VERSION_CODE;
import static com.multicraft.game.helpers.PreferencesHelper.TAG_REVIEW_ENABLE;
import static com.multicraft.game.helpers.Utilities.getStoreUrl;

public class VersionManagerHelper {
	private final CustomTagHandler customTagHandler;
	private final String PREF_REMINDER_TIME = "w.reminder.time";
	private final AppCompatActivity activity;
	private String message, updateUrl;

	public VersionManagerHelper(AppCompatActivity act) {
		this.activity = act;
		this.customTagHandler = new CustomTagHandler();
	}

	public boolean isCheckVersion() {
		long currentTimeStamp = Calendar.getInstance().getTimeInMillis();
		long reminderTimeStamp = getReminderTime();
		return currentTimeStamp > reminderTimeStamp;
	}

	private boolean isBadVersion(List<Integer> badVersions) {
		return badVersions.contains(versionCode);
	}

	private JsonSettings parseJson(String result) {
		JsonSettings settings = new Gson().fromJson(result, JsonSettings.class);
		String lang = Locale.getDefault().getLanguage();
		String content = lang.equals("ru") ? settings.getContentRus() : settings.getContentEng();
		setMessage(content);
		setUpdateUrl("market://details?id=" + settings.getPackageName());
		savePrefSettings(settings);
		return settings;
	}

	private void savePrefSettings(JsonSettings settings) {
		PreferencesHelper pf = PreferencesHelper.getInstance(activity);
		pf.saveSettings(TAG_RATE_MIN_VERSION_CODE, settings.getRateMinVersionCode());
		pf.saveSettings(TAG_REVIEW_ENABLE, settings.isReviewEnabled());
		pf.saveSettings(ADS_DELAY, settings.getAdsDelay());
		pf.saveSettings(ADS_REPEAT, settings.getAdsRepeat());
		pf.saveSettings(ADS_ENABLE, settings.isAdsEnabled());
	}

	public boolean isShow(String result) {
		if (result.equals("{}")) return false;
		JsonSettings jsonSettings = parseJson(result);
		return (versionCode < jsonSettings.getVersionCode()) ||
				isBadVersion(jsonSettings.getBadVersionCodes());
	}

	public String getMessage() {
		String defaultMessage = "What's new?";
		return message != null ? message : defaultMessage;
	}

	private void setMessage(String message) {
		this.message = message;
	}

	public String getUpdateUrl() {
		return updateUrl != null ? updateUrl : getStoreUrl();
	}

	private void setUpdateUrl(String updateUrl) {
		this.updateUrl = updateUrl;
	}

	public void updateNow(String url) {
		if (url != null) {
			try {
				Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
				activity.startActivity(intent);
			} catch (Exception e) {
				// nothing
			}
		}
	}

	public void remindMeLater() {
		Calendar c = Calendar.getInstance();
		c.add(Calendar.MINUTE, 1);
		long reminderTimeStamp = c.getTimeInMillis();
		setReminderTime(reminderTimeStamp);
	}

	private long getReminderTime() {
		return PreferenceManager.getDefaultSharedPreferences(activity).getLong(
				PREF_REMINDER_TIME, 0);
	}

	private void setReminderTime(long reminderTimeStamp) {
		PreferenceManager.getDefaultSharedPreferences(activity).edit().putLong(
				PREF_REMINDER_TIME, reminderTimeStamp).apply();
	}

	public CustomTagHandler getCustomTagHandler() {
		return customTagHandler;
	}

	private static class CustomTagHandler implements Html.TagHandler {
		@Override
		public void handleTag(boolean opening, String tag, Editable output,
		                      XMLReader xmlReader) {
			// you may add more tag handler which are not supported by android here
			if ("li".equals(tag)) {
				if (opening)
					output.append(" \u2022 ");
				else
					output.append("\n");
			}
		}
	}
}
