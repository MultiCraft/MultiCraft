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

import com.bugsnag.android.Bugsnag;
import com.multicraft.game.BuildConfig;
import com.multicraft.game.JsonSettings;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONTokener;
import org.xml.sax.XMLReader;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.List;
import java.util.Locale;

import static com.multicraft.game.helpers.Utilities.getStoreUrl;

public class VersionManagerHelper {
	private static final String JSON_VERSION_CODE = "version_code";
	private static final String JSON_VERSION_CODE_BAD = "version_code_bad";
	private static final String JSON_PACKAGE = "package";
	private static final String JSON_CONTENT_RU = "content_ru";
	private static final String JSON_CONTENT_EN = "content_en";
	private static final String JSON_ADS_DELAY = "ads_delay";
	private static final String JSON_ADS_REPEAT = "ads_repeat";
	private static final String JSON_ADS_ENABLE = "ads_enable";
	private final CustomTagHandler customTagHandler;
	private final String PREF_REMINDER_TIME = "w.reminder.time";
	private final AppCompatActivity activity;
	private final int versionCode = BuildConfig.VERSION_CODE;
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

	private List<Integer> convertToList(JSONArray badVersions) throws JSONException {
		List<Integer> badVersionList = new ArrayList<>();
		if (badVersions != null) {
			for (int i = 0; i < badVersions.length(); i++)
				badVersionList.add(badVersions.getInt(i));
		}
		return badVersionList;
	}

	private JsonSettings parseJson(String result) throws JSONException {
		JsonSettings jsonSettings = new JsonSettings();
		PreferencesHelper pf = PreferencesHelper.getInstance(activity);
		if (!result.startsWith("{")) // for response who append with unknown char
			result = result.substring(1);
		String mResult = result;
		// json format from server:
		JSONObject json = new JSONObject(new JSONTokener(mResult));
		jsonSettings.setVersionCode(json.getInt(JSON_VERSION_CODE));
		jsonSettings.setBadVersionCodes(convertToList(json.getJSONArray(JSON_VERSION_CODE_BAD)));
		String lang = Locale.getDefault().getLanguage();
		String content = lang.equals("ru") ? JSON_CONTENT_RU : JSON_CONTENT_EN;
		jsonSettings.setContent(json.getString(content));
		setMessage(jsonSettings.getContent());
		jsonSettings.setPackageName(json.getString(JSON_PACKAGE));
		setUpdateUrl("market://details?id=" + jsonSettings.getPackageName());
		jsonSettings.setAdsDelay(json.getInt(JSON_ADS_DELAY));
		jsonSettings.setAdsRepeat(json.getInt(JSON_ADS_REPEAT));
		jsonSettings.setAdsEnabled(json.getBoolean(JSON_ADS_ENABLE));
		pf.saveAdsSettings(jsonSettings);
		return jsonSettings;
	}

	public boolean isShow(String result) {
		if (result.equals("{}")) return false;
		JsonSettings jsonSettings;
		try {
			jsonSettings = parseJson(result);
		} catch (JSONException e) {
			return false;
		}
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
				Bugsnag.notify(e);
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
