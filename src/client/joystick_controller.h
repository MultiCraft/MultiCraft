/*
Minetest
Copyright (C) 2016 est31, <MTest31@outlook.com>

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
#include "keys.h"
#include <bitset>
#include <vector>

enum JoystickAxis {
	JA_SIDEWARD_MOVE,
	JA_FORWARD_MOVE,

	JA_FRUSTUM_HORIZONTAL,
	JA_FRUSTUM_VERTICAL,

	// To know the count of enum values
	JA_COUNT,
};

struct JoystickAxisLayout {
	u16 axis_id;
	// -1 if to invert, 1 if to keep it.
	int invert;
};


struct JoystickCombination {

	virtual bool isTriggered(const irr::SEvent::SJoystickEvent &ev) const=0;

	GameKeyType key;
};

struct JoystickButtonCmb : public JoystickCombination {

	JoystickButtonCmb() = default;

	JoystickButtonCmb(GameKeyType key, u32 filter_mask, u32 compare_mask) :
		filter_mask(filter_mask),
		compare_mask(compare_mask)
	{
		this->key = key;
	}

	virtual ~JoystickButtonCmb() = default;

	virtual bool isTriggered(const irr::SEvent::SJoystickEvent &ev) const;

	u32 filter_mask;
	u32 compare_mask;
};

struct JoystickAxisCmb : public JoystickCombination {

	JoystickAxisCmb() = default;

	JoystickAxisCmb(GameKeyType key, u16 axis_to_compare, int direction, s16 thresh) :
		axis_to_compare(axis_to_compare),
		direction(direction),
		thresh(thresh)
	{
		this->key = key;
	}

	virtual ~JoystickAxisCmb() = default;

	bool isTriggered(const irr::SEvent::SJoystickEvent &ev) const override;

	u16 axis_to_compare;

	// if -1, thresh must be smaller than the axis value in order to trigger
	// if  1, thresh must be bigger  than the axis value in order to trigger
	int direction;
	s16 thresh;
};

struct JoystickLayout {
	std::vector<JoystickButtonCmb> button_keys;
	std::vector<JoystickAxisCmb> axis_keys;
	JoystickAxisLayout axes[JA_COUNT];
	s16 axes_deadzone;
};

class JoystickController {

public:
	JoystickController();

	void onJoystickConnect(const std::vector<irr::SJoystickInfo> &joystick_infos);

	bool handleEvent(const irr::SEvent::SJoystickEvent &ev);
	void clear();

	bool wasKeyDown(GameKeyType b)
	{
		bool r = m_past_keys_pressed[b];
		m_past_keys_pressed[b] = false;
		return r;
	}

	bool wasKeyReleased(GameKeyType b)
	{
		return m_keys_released[b];
	}
	void clearWasKeyReleased(GameKeyType b)
	{
		m_keys_released[b] = false;
	}

	bool wasKeyPressed(GameKeyType b)
	{
		return m_keys_pressed[b];
	}
	void clearWasKeyPressed(GameKeyType b)
	{
		m_keys_pressed[b] = false;
	}

	bool isKeyDown(GameKeyType b)
	{
		return m_keys_down[b];
	}

	s16 getAxis(JoystickAxis axis)
	{
		return m_axes_vals[axis];
	}

	s16 getAxisWithoutDead(JoystickAxis axis);

	f32 doubling_dtime;

private:
	void setLayoutFromControllerName(const std::string &name);

	JoystickLayout m_layout;

	s16 m_axes_vals[JA_COUNT];

	u8 m_joystick_id = 0;

	std::bitset<KeyType::INTERNAL_ENUM_COUNT> m_keys_down;
	std::bitset<KeyType::INTERNAL_ENUM_COUNT> m_keys_pressed;

	f32 m_internal_time;

	f32 m_past_pressed_time[KeyType::INTERNAL_ENUM_COUNT];

	std::bitset<KeyType::INTERNAL_ENUM_COUNT> m_past_keys_pressed;
	std::bitset<KeyType::INTERNAL_ENUM_COUNT> m_keys_released;
};

#if defined(_IRR_COMPILE_WITH_SDL_DEVICE_)
class SDLGameController
{
private:
	void handleMouseMovement(int x, int y);
	void handleTriggerLeft(s16 value);
	void handleTriggerRight(s16 value);
	void handleMouseClickLeft(bool pressed);
	void handleMouseClickRight(bool pressed);
	void handleButton(const SEvent &event);
	void handleButtonInMenu(const SEvent &event);
	void handlePlayerMovement(int x, int y);
	void handleCameraOrientation(int x, int y);
	void sendEvent(const SEvent &event);

	int m_button_states = 0;
	u32 m_mouse_time = 0;
	s16 m_trigger_left_value = 0;
	s16 m_trigger_right_value = 0;
	s16 m_move_sideward = 0;
	s16 m_move_forward = 0;
	s16 m_camera_yaw = 0;
	s16 m_camera_pitch = 0;

	static bool m_active;
	static bool m_cursor_visible;
	bool m_is_fake_event = false;

public:
	void translateEvent(const SEvent &event);

	s16 getMoveSideward() { return m_move_sideward; }
	s16 getMoveForward() { return m_move_forward; }
	s16 getCameraYaw() { return m_camera_yaw; }
	s16 getCameraPitch() { return m_camera_pitch; }

	void setActive(bool value) { m_active = value; }
	static bool isActive() { return m_active; }
	void setCursorVisible(bool visible) { m_cursor_visible = visible; }
	static bool isCursorVisible() { return m_cursor_visible; }
	bool isFakeEvent() { return m_is_fake_event; }
};
#endif
