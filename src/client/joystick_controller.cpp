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

#include "joystick_controller.h"
#include "irrlichttypes_extrabloated.h"
#include "keys.h"
#include "keycode.h"
#include "gui/mainmenumanager.h"
#include "settings.h"
#include "gettime.h"
#include "porting.h"
#include "renderingengine.h"
#include "util/string.h"

bool JoystickButtonCmb::isTriggered(const irr::SEvent::SJoystickEvent &ev) const
{
	u32 buttons = ev.ButtonStates;

	buttons &= filter_mask;
	return buttons == compare_mask;
}

bool JoystickAxisCmb::isTriggered(const irr::SEvent::SJoystickEvent &ev) const
{
	s16 ax_val = ev.Axis[axis_to_compare];

	return (ax_val * direction < -thresh);
}

// spares many characters
#define JLO_B_PB(A, B, C)    jlo.button_keys.emplace_back(A, B, C)
#define JLO_A_PB(A, B, C, D) jlo.axis_keys.emplace_back(A, B, C, D)

JoystickLayout create_default_layout()
{
	JoystickLayout jlo;

	jlo.axes_deadzone = g_settings->getU16("joystick_deadzone");

	const JoystickAxisLayout axes[JA_COUNT] = {
		{0, 1}, // JA_SIDEWARD_MOVE
		{1, 1}, // JA_FORWARD_MOVE
		{3, 1}, // JA_FRUSTUM_HORIZONTAL
		{4, 1}, // JA_FRUSTUM_VERTICAL
	};
	memcpy(jlo.axes, axes, sizeof(jlo.axes));

	u32 sb = 1 << 7; // START button mask
	u32 fb = 1 << 3; // FOUR button mask
	u32 bm = sb | fb; // Mask for Both Modifiers

	// The back button means "ESC".
	JLO_B_PB(KeyType::ESC,        1 << 6,      1 << 6);

	// The start button counts as modifier as well as use key.
	// JLO_B_PB(KeyType::USE,        sb,          sb));

	// Accessible without start modifier button pressed
	// regardless whether four is pressed or not
	JLO_B_PB(KeyType::SNEAK,      sb | 1 << 2, 1 << 2);

	// Accessible without four modifier button pressed
	// regardless whether start is pressed or not
	JLO_B_PB(KeyType::DIG,        fb | 1 << 4, 1 << 4);
	JLO_B_PB(KeyType::PLACE,      fb | 1 << 5, 1 << 5);

	// Accessible without any modifier pressed
	JLO_B_PB(KeyType::JUMP,       bm | 1 << 0, 1 << 0);
	JLO_B_PB(KeyType::SPECIAL1,   bm | 1 << 1, 1 << 1);

	// Accessible with start button not pressed, but four pressed
	// TODO find usage for button 0
	JLO_B_PB(KeyType::DROP,        bm | 1 << 1, fb | 1 << 1);
	JLO_B_PB(KeyType::HOTBAR_PREV, bm | 1 << 4, fb | 1 << 4);
	JLO_B_PB(KeyType::HOTBAR_NEXT, bm | 1 << 5, fb | 1 << 5);

	// Accessible with start button and four pressed
	// TODO find usage for buttons 0, 1 and 4, 5

	// Now about the buttons simulated by the axes

	// Movement buttons, important for vessels
	JLO_A_PB(KeyType::FORWARD,  1,  1, jlo.axes_deadzone);
	JLO_A_PB(KeyType::BACKWARD, 1, -1, jlo.axes_deadzone);
	JLO_A_PB(KeyType::LEFT,     0,  1, jlo.axes_deadzone);
	JLO_A_PB(KeyType::RIGHT,    0, -1, jlo.axes_deadzone);

	// Scroll buttons
	JLO_A_PB(KeyType::HOTBAR_PREV, 2, -1, jlo.axes_deadzone);
	JLO_A_PB(KeyType::HOTBAR_NEXT, 5, -1, jlo.axes_deadzone);

	return jlo;
}

JoystickLayout create_xbox_layout()
{
	JoystickLayout jlo;

	jlo.axes_deadzone = 7000;

	const JoystickAxisLayout axes[JA_COUNT] = {
		{0, 1}, // JA_SIDEWARD_MOVE
		{1, 1}, // JA_FORWARD_MOVE
		{2, 1}, // JA_FRUSTUM_HORIZONTAL
		{3, 1}, // JA_FRUSTUM_VERTICAL
	};
	memcpy(jlo.axes, axes, sizeof(jlo.axes));

	// The back button means "ESC".
	JLO_B_PB(KeyType::ESC,        1 << 8,  1 << 8); // back
	JLO_B_PB(KeyType::ESC,        1 << 9,  1 << 9); // start

	// 4 Buttons
	JLO_B_PB(KeyType::JUMP,        1 << 0,  1 << 0); // A/green
	JLO_B_PB(KeyType::ESC,         1 << 1,  1 << 1); // B/red
	JLO_B_PB(KeyType::SPECIAL1,    1 << 2,  1 << 2); // X/blue
	JLO_B_PB(KeyType::INVENTORY,   1 << 3,  1 << 3); // Y/yellow

	// Analog Sticks
	JLO_B_PB(KeyType::SPECIAL1,    1 << 11, 1 << 11); // left
	JLO_B_PB(KeyType::SNEAK,       1 << 12, 1 << 12); // right

	// Triggers
	JLO_B_PB(KeyType::DIG,         1 << 6,  1 << 6); // lt
	JLO_B_PB(KeyType::PLACE,       1 << 7,  1 << 7); // rt
	JLO_B_PB(KeyType::HOTBAR_PREV, 1 << 4,  1 << 4); // lb
	JLO_B_PB(KeyType::HOTBAR_NEXT, 1 << 5,  1 << 5); // rb

	// D-PAD
	JLO_B_PB(KeyType::ZOOM,        1 << 15, 1 << 15); // up
	JLO_B_PB(KeyType::DROP,        1 << 13, 1 << 13); // left
	JLO_B_PB(KeyType::SCREENSHOT,  1 << 14, 1 << 14); // right
	JLO_B_PB(KeyType::FREEMOVE,    1 << 16, 1 << 16); // down

	// Movement buttons, important for vessels
	JLO_A_PB(KeyType::FORWARD,  1,  1, jlo.axes_deadzone);
	JLO_A_PB(KeyType::BACKWARD, 1, -1, jlo.axes_deadzone);
	JLO_A_PB(KeyType::LEFT,     0,  1, jlo.axes_deadzone);
	JLO_A_PB(KeyType::RIGHT,    0, -1, jlo.axes_deadzone);

	return jlo;
}

JoystickController::JoystickController() :
		doubling_dtime(g_settings->getFloat("repeat_joystick_button_time"))
{
	for (float &i : m_past_pressed_time) {
		i = 0;
	}
	clear();
}

void JoystickController::onJoystickConnect(const std::vector<irr::SJoystickInfo> &joystick_infos)
{
	s32         id     = g_settings->getS32("joystick_id");
	std::string layout = g_settings->get("joystick_type");

	if (id < 0 || (u16)id >= joystick_infos.size()) {
		// TODO: auto detection
		id = 0;
	}

	if (id >= 0 && (u16)id < joystick_infos.size()) {
		if (layout.empty() || layout == "auto")
			setLayoutFromControllerName(joystick_infos[id].Name.c_str());
		else
			setLayoutFromControllerName(layout);
	}

	m_joystick_id = id;
}

void JoystickController::setLayoutFromControllerName(const std::string &name)
{
	if (lowercase(name).find("xbox") != std::string::npos) {
		m_layout = create_xbox_layout();
	} else {
		m_layout = create_default_layout();
	}
}

bool JoystickController::handleEvent(const irr::SEvent::SJoystickEvent &ev)
{
	if (ev.Joystick != m_joystick_id)
		return false;

	m_internal_time = porting::getTimeMs() / 1000.f;

	std::bitset<KeyType::INTERNAL_ENUM_COUNT> keys_pressed;

	// First generate a list of keys pressed

	for (const auto &button_key : m_layout.button_keys) {
		if (button_key.isTriggered(ev)) {
			keys_pressed.set(button_key.key);
		}
	}

	for (const auto &axis_key : m_layout.axis_keys) {
		if (axis_key.isTriggered(ev)) {
			keys_pressed.set(axis_key.key);
		}
	}

	// Then update the values

	for (size_t i = 0; i < KeyType::INTERNAL_ENUM_COUNT; i++) {
		if (keys_pressed[i]) {
			if (!m_past_keys_pressed[i] &&
					m_past_pressed_time[i] < m_internal_time - doubling_dtime) {
				m_past_keys_pressed[i] = true;
				m_past_pressed_time[i] = m_internal_time;
			}
		} else if (m_keys_down[i]) {
			m_keys_released[i] = true;
		}

		if (keys_pressed[i] && !(m_keys_down[i]))
			m_keys_pressed[i] = true;

		m_keys_down[i] = keys_pressed[i];
	}

	for (size_t i = 0; i < JA_COUNT; i++) {
		const JoystickAxisLayout &ax_la = m_layout.axes[i];
		m_axes_vals[i] = ax_la.invert * ev.Axis[ax_la.axis_id];
	}

	return true;
}

void JoystickController::clear()
{
	m_keys_pressed.reset();
	m_keys_down.reset();
	m_past_keys_pressed.reset();
	m_keys_released.reset();
	memset(m_axes_vals, 0, sizeof(m_axes_vals));
}

s16 JoystickController::getAxisWithoutDead(JoystickAxis axis)
{
	s16 v = m_axes_vals[axis];
	if (abs(v) < m_layout.axes_deadzone)
		return 0;
	return v;
}

#if defined(_IRR_COMPILE_WITH_SDL_DEVICE_)
bool SDLGameController::m_active = false;

void SDLGameController::handleMouseMovement(int x, int y)
{
	IrrlichtDevice* device = RenderingEngine::get_raw_device();

	bool changed = false;

	u32 current_time = device->getTimer()->getRealTime();
	v2s32 mouse_pos = device->getCursorControl()->getPosition();
	int deadzone = g_settings->getU16("joystick_deadzone");

	if (x > deadzone || x < -deadzone)
	{
		s32 dt = current_time - m_mouse_time;

		mouse_pos.X += (x * dt / 30000);
		if (mouse_pos.X < 0)
			mouse_pos.X = 0;
		if (mouse_pos.X > device->getVideoDriver()->getScreenSize().Width)
			mouse_pos.X = device->getVideoDriver()->getScreenSize().Width;

		changed = true;
		m_active = true;
	}

	if (y > deadzone || y < -deadzone)
	{
		s32 dt = current_time - m_mouse_time;

		mouse_pos.Y += (y * dt / 30000);
		if (mouse_pos.Y < 0)
			mouse_pos.Y = 0;
		if (mouse_pos.Y > device->getVideoDriver()->getScreenSize().Height)
			mouse_pos.Y = device->getVideoDriver()->getScreenSize().Height;

		changed = true;
		m_active = true;
	}

	if (changed)
	{
		SEvent translated_event;
		translated_event.EventType = irr::EET_MOUSE_INPUT_EVENT;
		translated_event.MouseInput.Event = irr::EMIE_MOUSE_MOVED;
		translated_event.MouseInput.X = mouse_pos.X;
		translated_event.MouseInput.Y = mouse_pos.Y;
		translated_event.MouseInput.Control = false;
		translated_event.MouseInput.Shift = false;
		translated_event.MouseInput.ButtonStates = m_button_states;

		sendEvent(translated_event);
		device->getCursorControl()->setPosition(mouse_pos.X, mouse_pos.Y);
	}

	m_mouse_time = current_time;
}

void SDLGameController::handleTriggerLeft(s16 value)
{
	IrrlichtDevice* device = RenderingEngine::get_raw_device();

	int deadzone = g_settings->getU16("joystick_deadzone");

	SEvent translated_event;
	translated_event.EventType = irr::EET_KEY_INPUT_EVENT;
	translated_event.KeyInput.Char = 0;
	translated_event.KeyInput.Key = getKeySetting("keymap_hotbar_previous").getKeyCode();
	translated_event.KeyInput.Shift = false;
	translated_event.KeyInput.Control = false;

	if (value <= deadzone && m_trigger_left_value > deadzone)
	{
		translated_event.KeyInput.PressedDown = false;
		sendEvent(translated_event);
	}
	else if (value > deadzone && m_trigger_left_value <= deadzone)
	{
		translated_event.KeyInput.PressedDown = true;
		sendEvent(translated_event);
	}

	m_trigger_left_value = value;
}

void SDLGameController::handleTriggerRight(s16 value)
{
	IrrlichtDevice* device = RenderingEngine::get_raw_device();

	int deadzone = g_settings->getU16("joystick_deadzone");

	SEvent translated_event;
	translated_event.EventType = irr::EET_KEY_INPUT_EVENT;
	translated_event.KeyInput.Char = 0;
	translated_event.KeyInput.Key = getKeySetting("keymap_hotbar_next").getKeyCode();
	translated_event.KeyInput.Shift = false;
	translated_event.KeyInput.Control = false;

	if (value <= deadzone && m_trigger_right_value > deadzone)
	{
		translated_event.KeyInput.PressedDown = false;
		sendEvent(translated_event);
	}
	else if (value > deadzone && m_trigger_right_value <= deadzone)
	{
		translated_event.KeyInput.PressedDown = true;
		sendEvent(translated_event);
	}

	m_trigger_right_value = value;
}

void SDLGameController::handleMouseClickLeft(bool pressed)
{
	IrrlichtDevice* device = RenderingEngine::get_raw_device();

	v2s32 mouse_pos = device->getCursorControl()->getPosition();

	if (pressed)
		m_button_states |= irr::EMBSM_LEFT;
	else
		m_button_states &= ~irr::EMBSM_LEFT;

	SEvent translated_event;
	translated_event.EventType = irr::EET_MOUSE_INPUT_EVENT;
	translated_event.MouseInput.Event = pressed ? irr::EMIE_LMOUSE_PRESSED_DOWN : irr::EMIE_LMOUSE_LEFT_UP;
	translated_event.MouseInput.X = mouse_pos.X;
	translated_event.MouseInput.Y = mouse_pos.Y;
	translated_event.MouseInput.Control = false;
	translated_event.MouseInput.Shift = false;
	translated_event.MouseInput.ButtonStates = m_button_states;

	sendEvent(translated_event);
}

void SDLGameController::handleMouseClickRight(bool pressed)
{
	IrrlichtDevice* device = RenderingEngine::get_raw_device();

	v2s32 mouse_pos = device->getCursorControl()->getPosition();

	if (pressed)
		m_button_states |= irr::EMBSM_RIGHT;
	else
		m_button_states &= ~irr::EMBSM_RIGHT;

	SEvent translated_event;
	translated_event.EventType = irr::EET_MOUSE_INPUT_EVENT;
	translated_event.MouseInput.Event = pressed ? irr::EMIE_RMOUSE_PRESSED_DOWN : irr::EMIE_RMOUSE_LEFT_UP;
	translated_event.MouseInput.X = mouse_pos.X;
	translated_event.MouseInput.Y = mouse_pos.Y;
	translated_event.MouseInput.Control = false;
	translated_event.MouseInput.Shift = false;
	translated_event.MouseInput.ButtonStates = m_button_states;

	sendEvent(translated_event);
}

void SDLGameController::handleButton(const SEvent &event)
{
	irr::EKEY_CODE key = KEY_UNKNOWN;

	switch (event.SDLControllerButtonEvent.Button)
	{
	case SDL_CONTROLLER_BUTTON_A:
		key = getKeySetting("keymap_jump").getKeyCode();
		break;
	case SDL_CONTROLLER_BUTTON_B:
		key = getKeySetting("keymap_sneak").getKeyCode();
		break;
	case SDL_CONTROLLER_BUTTON_X:
		key = getKeySetting("keymap_special1").getKeyCode();
		break;
	case SDL_CONTROLLER_BUTTON_Y:
		key = getKeySetting("keymap_minimap").getKeyCode();
		break;
	case SDL_CONTROLLER_BUTTON_BACK:
		key = getKeySetting("keymap_chat").getKeyCode();
		break;
	case SDL_CONTROLLER_BUTTON_GUIDE:
		break;
	case SDL_CONTROLLER_BUTTON_START:
		key = KEY_ESCAPE;
		break;
	case SDL_CONTROLLER_BUTTON_LEFTSTICK:
		key = getKeySetting("keymap_camera_mode").getKeyCode();
		break;
	case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
		key = KEY_LBUTTON;
		break;
	case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
		key = KEY_LBUTTON;
		break;
	case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
		key = KEY_RBUTTON;
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_UP:
		key = getKeySetting("keymap_rangeselect").getKeyCode();
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
		key = getKeySetting("keymap_drop").getKeyCode();
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
		key = KEY_ESCAPE;
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
		key = getKeySetting("keymap_inventory").getKeyCode();
		break;
	case SDL_CONTROLLER_BUTTON_MISC1:
	case SDL_CONTROLLER_BUTTON_PADDLE1:
	case SDL_CONTROLLER_BUTTON_PADDLE2:
	case SDL_CONTROLLER_BUTTON_PADDLE3:
	case SDL_CONTROLLER_BUTTON_PADDLE4:
	case SDL_CONTROLLER_BUTTON_TOUCHPAD:
		break;
	}

	if (key != KEY_UNKNOWN)
	{
		SEvent translated_event;
		translated_event.EventType = irr::EET_KEY_INPUT_EVENT;
		translated_event.KeyInput.Char = 0;
		translated_event.KeyInput.Key = key;
		translated_event.KeyInput.PressedDown = event.SDLControllerButtonEvent.Pressed;
		translated_event.KeyInput.Shift = false;
		translated_event.KeyInput.Control = false;

		sendEvent(translated_event);
	}
}

void SDLGameController::handleButtonInMenu(const SEvent &event)
{
	irr::EKEY_CODE key = KEY_UNKNOWN;

	// Just used game mapping for escape key for now
	switch (event.SDLControllerButtonEvent.Button)
	{
	case SDL_CONTROLLER_BUTTON_A:
	case SDL_CONTROLLER_BUTTON_B:
	case SDL_CONTROLLER_BUTTON_X:
	case SDL_CONTROLLER_BUTTON_Y:
	case SDL_CONTROLLER_BUTTON_BACK:
	case SDL_CONTROLLER_BUTTON_GUIDE:
		break;
	case SDL_CONTROLLER_BUTTON_START:
		key = KEY_ESCAPE;
		break;
	case SDL_CONTROLLER_BUTTON_LEFTSTICK:
	case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
	case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
	case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
	case SDL_CONTROLLER_BUTTON_DPAD_UP:
	case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
		key = KEY_ESCAPE;
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
	case SDL_CONTROLLER_BUTTON_MISC1:
	case SDL_CONTROLLER_BUTTON_PADDLE1:
	case SDL_CONTROLLER_BUTTON_PADDLE2:
	case SDL_CONTROLLER_BUTTON_PADDLE3:
	case SDL_CONTROLLER_BUTTON_PADDLE4:
	case SDL_CONTROLLER_BUTTON_TOUCHPAD:
		break;
	}

	if (key != KEY_UNKNOWN)
	{
		SEvent translated_event;
		translated_event.EventType = irr::EET_KEY_INPUT_EVENT;
		translated_event.KeyInput.Char = 0;
		translated_event.KeyInput.Key = key;
		translated_event.KeyInput.PressedDown = event.SDLControllerButtonEvent.Pressed;
		translated_event.KeyInput.Shift = false;
		translated_event.KeyInput.Control = false;

		sendEvent(translated_event);
	}
}

void SDLGameController::handlePlayerMovement(int x, int y)
{
	int deadzone = g_settings->getU16("joystick_deadzone");

	m_move_sideward = x;
	if (m_move_sideward < deadzone && m_move_sideward > -deadzone)
		m_move_sideward = 0;
	else
		m_active = true;

	m_move_forward = y;
	if (m_move_forward < deadzone && m_move_forward > -deadzone)
		m_move_forward = 0;
	else
		m_active = true;
}

void SDLGameController::handleCameraOrientation(int x, int y)
{
	int deadzone = g_settings->getU16("joystick_deadzone");

	m_camera_yaw = x;
	if (m_camera_yaw < deadzone && m_camera_yaw > -deadzone)
		m_camera_yaw = 0;
	else
		m_active = true;

	m_camera_pitch = y;
	if (m_camera_pitch < deadzone && m_camera_pitch > -deadzone)
		m_camera_pitch = 0;
	else
		m_active = true;
}

void SDLGameController::sendEvent(const SEvent &event)
{
	m_is_fake_event = true;
	IrrlichtDevice* device = RenderingEngine::get_raw_device();
	device->postEventFromUser(event);
	m_is_fake_event = false;
}

void SDLGameController::translateEvent(const SEvent &event)
{
	if (event.EventType == irr::EET_SDL_CONTROLLER_BUTTON_EVENT)
	{
		if (isMenuActive())
		{
			if (event.SDLControllerButtonEvent.Button == SDL_CONTROLLER_BUTTON_LEFTSTICK ||
				event.SDLControllerButtonEvent.Button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER)
			{
				handleMouseClickLeft(event.SDLControllerButtonEvent.Pressed);
			}
			else if (event.SDLControllerButtonEvent.Button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER)
			{
				handleMouseClickRight(event.SDLControllerButtonEvent.Pressed);
			}
			else
			{
				handleButtonInMenu(event);
			}
		}
		else
		{
			if (event.SDLControllerButtonEvent.Button == SDL_CONTROLLER_BUTTON_RIGHTSTICK ||
				event.SDLControllerButtonEvent.Button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER)
			{
				handleMouseClickLeft(event.SDLControllerButtonEvent.Pressed);
			}
			else if (event.SDLControllerButtonEvent.Button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER)
			{
				handleMouseClickRight(event.SDLControllerButtonEvent.Pressed);
			}
			else
			{
				handleButton(event);
			}
		}
	}
	else if (event.EventType == irr::EET_SDL_CONTROLLER_AXIS_EVENT)
	{
		const s16* value = event.SDLControllerAxisEvent.Value;

		if (isMenuActive())
		{
			handleMouseMovement(value[SDL_CONTROLLER_AXIS_LEFTX], value[SDL_CONTROLLER_AXIS_LEFTY]);
		}
		else
		{
			handleTriggerLeft(value[SDL_CONTROLLER_AXIS_TRIGGERLEFT]);
			handleTriggerRight(value[SDL_CONTROLLER_AXIS_TRIGGERRIGHT]);
			handlePlayerMovement(value[SDL_CONTROLLER_AXIS_LEFTX], value[SDL_CONTROLLER_AXIS_LEFTY]);
			handleCameraOrientation(value[SDL_CONTROLLER_AXIS_RIGHTX], value[SDL_CONTROLLER_AXIS_RIGHTY]);
		}
	}
}
#endif
