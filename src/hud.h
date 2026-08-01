/*
Minetest
Copyright (C) 2010-2013 kwolekr, Ryan Kwolek <kwolekr@minetest.net>
Copyright (C) 2017 red-001 <red-001@outlook.ie>

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

#pragma once

#include "irrlichttypes_extrabloated.h"
#include <string>
#include "common/c_types.h"
#include "util/numeric.h"

#define HUD_DIR_LEFT_RIGHT 0
#define HUD_DIR_RIGHT_LEFT 1
#define HUD_DIR_TOP_BOTTOM 2
#define HUD_DIR_BOTTOM_TOP 3

#define HUD_CORNER_UPPER  0
#define HUD_CORNER_LOWER  1
#define HUD_CORNER_CENTER 2

// Note that these visibility flags do not determine if the hud items are
// actually drawn, but rather, whether to draw the item should the rest
// of the game state permit it.
#define HUD_FLAG_HOTBAR_VISIBLE        (1 << 0)
#define HUD_FLAG_HEALTHBAR_VISIBLE     (1 << 1)
#define HUD_FLAG_CROSSHAIR_VISIBLE     (1 << 2)
#define HUD_FLAG_WIELDITEM_VISIBLE     (1 << 3)
#define HUD_FLAG_BREATHBAR_VISIBLE     (1 << 4)
#define HUD_FLAG_MINIMAP_VISIBLE       (1 << 5)
#define HUD_FLAG_MINIMAP_RADAR_VISIBLE (1 << 6)
#define HUD_FLAG_BASIC_DEBUG           (1 << 7)

#define HUD_PARAM_HOTBAR_ITEMCOUNT 1
#define HUD_PARAM_HOTBAR_IMAGE 2
#define HUD_PARAM_HOTBAR_SELECTED_IMAGE 3

#define HUD_HOTBAR_ITEMCOUNT_DEFAULT 8
#if !defined(__ANDROID__) && !defined(__IOS__)
#define HUD_HOTBAR_ITEMCOUNT_MAX     32
#else
#define HUD_HOTBAR_ITEMCOUNT_MAX     9
#endif

#define HOTBAR_IMAGE_SIZE 48

// Unscaled edge of a touchscreen button; every button rect is a multiple of it
#define TOUCH_BUTTON_SIZE 64

// Share of the screen height one hotbar slot is tuned to cover
#define HOTBAR_SCREEN_SHARE 0.054f

// Largest hud_scaling the screen has room for
inline float getAutoHudScaling(v2u32 screen_size, float display_density)
{
#ifdef HAVE_TOUCHSCREENGUI
	// half the widest hotbar a game may ask for, and the joystick, in unscaled pixels
	const float hotbar = HUD_HOTBAR_ITEMCOUNT_MAX * HOTBAR_IMAGE_SIZE * 7 / 12.0f;
	// the joystick reaches 4.8 button sizes in once shifted on a round screen
	const float joystick = TOUCH_BUTTON_SIZE * 4.8f;

	return rangelim(screen_size.X / (2.0f * display_density * (hotbar + joystick)),
			0.5f, 1.0f);
#else
	// the height sets the size, the width has to hold the stat bars flanking the hotbar
	const float wanted = screen_size.Y * HOTBAR_SCREEN_SHARE;
	const float fits = screen_size.X / (HUD_HOTBAR_ITEMCOUNT_DEFAULT * 7.0f / 3.0f);
	return rangelim(MYMIN(wanted, fits) / (HOTBAR_IMAGE_SIZE * display_density),
			0.75f, 2.0f);
#endif
}

enum HudElementType {
	HUD_ELEM_IMAGE     = 0,
	HUD_ELEM_TEXT      = 1,
	HUD_ELEM_STATBAR   = 2,
	HUD_ELEM_INVENTORY = 3,
	HUD_ELEM_WAYPOINT  = 4,
	HUD_ELEM_IMAGE_WAYPOINT = 5,
	HUD_ELEM_COMPASS   = 6,
	HUD_ELEM_MINIMAP   = 7,
	// (some space so as to not conflict with Luanti)
	HUD_ELEM_CSM_BUTTON = 100,
};

enum HudElementStat {
	HUD_STAT_POS = 0,
	HUD_STAT_NAME,
	HUD_STAT_SCALE,
	HUD_STAT_TEXT,
	HUD_STAT_NUMBER,
	HUD_STAT_ITEM,
	HUD_STAT_DIR,
	HUD_STAT_ALIGN,
	HUD_STAT_OFFSET,
	HUD_STAT_WORLD_POS,
	HUD_STAT_SIZE,
	HUD_STAT_Z_INDEX,
	HUD_STAT_TEXT2,
	HUD_STAT_STYLE,
	HUD_STAT_UNHIDEABLE,
	// (some space so as to not conflict with Luanti)
	HUD_STAT_TOUCH_ONLY = 100,
};

enum HudCompassDir {
	HUD_COMPASS_ROTATE = 0,
	HUD_COMPASS_ROTATE_REVERSE,
	HUD_COMPASS_TRANSLATE,
	HUD_COMPASS_TRANSLATE_REVERSE,
};

struct HudElement {
	HudElementType type;
	v2f pos;
	std::string name;
	v2f scale;
	std::string text;
	u32 number;
	u32 item;
	u32 dir;
	v2f align;
	v2f offset;
	v3f world_pos;
	v2s32 size;
	s16 z_index = 0;
	std::string text2;
	bool unhideable = false;
	bool touch_only = false;
};

extern const EnumString es_HudElementType[];
extern const EnumString es_HudElementStat[];
extern const EnumString es_HudBuiltinElement[];

// Minimap stuff

enum MinimapType {
	MINIMAP_TYPE_OFF,
	MINIMAP_TYPE_SURFACE,
	MINIMAP_TYPE_RADAR,
	MINIMAP_TYPE_TEXTURE,
};

