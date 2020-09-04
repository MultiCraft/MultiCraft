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

import com.google.gson.annotations.SerializedName;

import java.util.List;

public class JsonSettings {
	@SerializedName(value = "version_code")
	private int versionCode;
	@SerializedName(value = "version_code_bad")
	private List<Integer> badVersionCodes;
	@SerializedName(value = "rate_min_version_code")
	private int rateMinVersionCode;
	@SerializedName(value = "package")
	private String packageName;
	@SerializedName(value = "content_ru")
	private String contentRus;
	@SerializedName(value = "content_en")
	private String contentEng;
	@SerializedName(value = "ads_delay")
	private int adsDelay;
	@SerializedName(value = "ads_repeat")
	private int adsRepeat;
	@SerializedName(value = "ads_enable")
	private boolean adsEnabled;
	@SerializedName(value = "review_enable")
	private boolean reviewEnabled;

	public int getVersionCode() {
		return versionCode;
	}

	public List<Integer> getBadVersionCodes() {
		return badVersionCodes;
	}

	public int getRateMinVersionCode() {
		return rateMinVersionCode;
	}

	public String getPackageName() {
		return packageName;
	}

	public int getAdsDelay() {
		return adsDelay;
	}

	public int getAdsRepeat() {
		return adsRepeat;
	}

	public boolean isAdsEnabled() {
		return adsEnabled;
	}

	public boolean isReviewEnabled() {
		return reviewEnabled;
	}

	public String getContentRus() {
		return contentRus;
	}

	public String getContentEng() {
		return contentEng;
	}
}
