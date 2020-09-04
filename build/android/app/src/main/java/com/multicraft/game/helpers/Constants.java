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

import com.multicraft.game.BuildConfig;

public class Constants {
	public static final int REQUEST_CONNECTION = 104;
	public static final int REQUEST_UPDATE = 102;
	public static final int SESSION_COUNT = 5;
	public static final String NO_SPACE_LEFT = "ENOSPC";
	public static final String FILES = "Files.zip";
	public static final String WORLDS = "worlds.zip";
	public static final String GAMES = "games.zip";
	public static final int versionCode = BuildConfig.VERSION_CODE;
	public static final String versionName = BuildConfig.VERSION_NAME;
	public static final String appPackage = BuildConfig.APPLICATION_ID;
	public static final String UPDATE_LINK = "http://updates.multicraft.world/Android.json";
}
