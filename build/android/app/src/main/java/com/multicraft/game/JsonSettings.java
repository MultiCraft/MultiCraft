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

import java.util.List;

public class JsonSettings {
	private int versionCode = 0;
	private List<Integer> badVersionCodes;
	private String packageName, content;
	private int adsDelay = -1;
	private int adsRepeat = -1;
	private boolean adsEnabled = true;

	public int getVersionCode() {
		return versionCode;
	}

	public void setVersionCode(int versionCode) {
		this.versionCode = versionCode;
	}

	public List<Integer> getBadVersionCodes() {
		return badVersionCodes;
	}

	public void setBadVersionCodes(List<Integer> badVersionCodes) {
		this.badVersionCodes = badVersionCodes;
	}

	public String getPackageName() {
		return packageName;
	}

	public void setPackageName(String packageName) {
		this.packageName = packageName;
	}

	public String getContent() {
		return content;
	}

	public void setContent(String content) {
		this.content = content;
	}

	public int getAdsDelay() {
		return adsDelay;
	}

	public void setAdsDelay(int adsDelay) {
		this.adsDelay = adsDelay;
	}

	public int getAdsRepeat() {
		return adsRepeat;
	}

	public void setAdsRepeat(int adsRepeat) {
		this.adsRepeat = adsRepeat;
	}

	public boolean isAdsEnabled() {
		return adsEnabled;
	}

	public void setAdsEnabled(boolean adsEnabled) {
		this.adsEnabled = adsEnabled;
	}
}
