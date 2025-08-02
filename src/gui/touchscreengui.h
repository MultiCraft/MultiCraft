/*
Copyright (C) 2014 sapier
Copyright (C) 2014-2024 Maksim Gamarnik [MoNTE48] Maksym48@pm.me
Copyright (C) 2023 Dawid Gan <deveee@gmail.com>

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

#include "IGUIStaticText.h"
#include "irrlichttypes.h"
#include <IGUIButton.h>
#include <IGUIEnvironment.h>
#include <IrrlichtDevice.h>

#include <map>
#include <vector>

#include "client/sound.h"
#include "client/tile.h"

using namespace irr;
using namespace irr::core;
using namespace irr::gui;

#define MIN_DIG_TIME_MS 500
#define MIN_PLACE_TIME_MS 50

typedef enum
{
	unknown_id = -1,
	jump_id = 0,
	drop_id,
	sneak_id,
	special1_id,
	inventory_id,
	escape_id,
	minimap_id,
	camera_id,
	overflow_id,
	chat_id,
	tab_id,
	flymove_id,
	fastmove_id,
	noclip_id,
	range_id,
	toggle_chat_id,
	toggle_nametags_id,
	joystick_off_id,
	joystick_bg_id,
	joystick_center_id,
} touch_gui_button_id;

struct button_data
{
	const char *image;
	const char *title;
	const char *name;
	bool has_sound;
};

struct button_info
{
	IGUIButton *guibutton = nullptr;
	IGUIStaticText *text = nullptr;
	touch_gui_button_id id = unknown_id;
	bool overflow_menu = false;
	bool pressed = false;
	s32 event_id = -1;

	void reset()
	{
		pressed = false;
		event_id = -1;
	}
};

struct joystick_info
{
	IGUIButton *button_off = nullptr;
	IGUIButton *button_bg = nullptr;
	IGUIButton *button_center = nullptr;
	s16 move_sideward = 0;
	s16 move_forward = 0;
	bool pressed = false;
	bool released = false;
	s32 event_id = -1;

	void reset(bool visible)
	{
		if (button_off)
			button_off->setVisible(visible);
		if (button_bg)
			button_bg->setVisible(false);
		if (button_center)
			button_center->setVisible(false);
		move_sideward = 0;
		move_forward = 0;
		pressed = false;
		event_id = -1;
	}
};

struct hud_button_info
{
	s32 id = -1;
	rect<s32> button_rect;
	bool pressed = false;
};

struct camera_info
{
	double yaw_change = 0.0;
	double pitch = 0.0;
	line3d<f32> shootline;
	u64 downtime = 0;
	bool has_really_moved = false;
	bool dig = false;
	bool place = false;
	bool place_shootline = false;
	s32 x = 0;
	s32 y = 0;
	s32 event_id = -1;

	void reset()
	{
		downtime = 0;
		has_really_moved = false;
		dig = false;
		place = false;
		place_shootline = false;
		x = 0;
		y = 0;
		event_id = -1;
	}
};

class TouchScreenGUI
{
public:
	TouchScreenGUI(IrrlichtDevice *device);
	~TouchScreenGUI();

	void init(ISimpleTextureSource *tsrc, bool simple_singleplayer_mode,
			ISoundManager *sound_manage);
	bool preprocessEvent(const SEvent &event);
	bool isButtonPressed(irr::EKEY_CODE keycode);
	bool immediateRelease(irr::EKEY_CODE keycode);

	s16 getMoveSideward() { return m_joystick.move_sideward; }
	s16 getMoveForward() { return m_joystick.move_forward; }

	double getYawChange()
	{
		double res = m_camera.yaw_change;
		m_camera.yaw_change = 0;
		return res;
	}

	double getPitchChange()
	{
		double res = m_camera.pitch;
		m_camera.pitch = 0;
		return res;
	}

	line3d<f32> getShootline()
	{
		if (m_camera_additional.event_id != -1 ||
				m_camera_additional.place_shootline) {
			m_camera_additional.place_shootline = false;
			return m_camera_additional.shootline;
		} else {
			return m_camera.shootline;
		}
	}

	void step(float dtime);
	void hide();
	void show();
	void reset();
	void close() { m_close = true; }

	void resetHud();
	void registerHudItem(s32 index, const rect<s32> &button_rect);

	static bool isActive() { return m_active; }
	static void setActive(bool active) { m_active = active; }

private:
	static bool m_active;

	IrrlichtDevice *m_device = nullptr;
	IGUIEnvironment *m_guienv = nullptr;
	ISimpleTextureSource *m_texturesource = nullptr;
	ISoundManager *m_sound_manager = nullptr;

	v2u32 m_screensize;
	s32 m_button_size;
	double m_touchscreen_threshold;
	double m_touch_sensitivity;
	bool m_visible = true;
	bool m_buttons_initialized = false;
	bool m_close = false;
	bool m_dig_and_move = false;
	irr::EKEY_CODE m_keycode_dig;
	irr::EKEY_CODE m_keycode_place;
	bool m_enable_sound = true;
	std::string m_press_sound;

	std::map<size_t, bool> m_events;
	std::vector<hud_button_info> m_hud_buttons;
	std::vector<button_info *> m_buttons;
	joystick_info m_joystick;
	camera_info m_camera;
	camera_info m_camera_additional;

	bool m_overflow_open = false;
	bool m_overflow_close_schedule = false;
	IGUIStaticText *m_overflow_bg = nullptr;
	std::vector<IGUIStaticText *> m_overflow_button_titles;

	void loadButtonTexture(
			IGUIButton *btn, const char *path, const rect<s32> &button_rect);
	void initButton(touch_gui_button_id id, const rect<s32> &button_rect,
			bool overflow_menu = false, const char *texture = "");
	void initJoystickButton();

	rect<s32> getButtonRect(touch_gui_button_id id);
	void updateButtons();

	void rebuildOverflowMenu();
	void toggleOverflowMenu();

	bool moveJoystick(s32 x, s32 y);
	void updateCamera(camera_info &camera, s32 x, s32 y);

	void setVisible(bool visible);

	void wakeUpInputhandler();

	void playSound();
};

extern TouchScreenGUI *g_touchscreengui;
