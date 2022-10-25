/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2017 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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

#include "util/numeric.h"
#include "inputhandler.h"
#include "gui/mainmenumanager.h"
#include "hud.h"
#include "settings.h"

#ifdef __IOS__
#include "porting_ios.h"
extern "C" void external_pause_game();
#endif

#if defined(_IRR_COMPILE_WITH_SDL_DEVICE_)
#include <SDL.h>
#endif

void KeyCache::populate_nonchanging()
{
	key[KeyType::ESC] = EscapeKey;
}

void KeyCache::populate()
{
	key[KeyType::FORWARD] = getKeySetting("keymap_forward");
	key[KeyType::BACKWARD] = getKeySetting("keymap_backward");
	key[KeyType::LEFT] = getKeySetting("keymap_left");
	key[KeyType::RIGHT] = getKeySetting("keymap_right");
	key[KeyType::JUMP] = getKeySetting("keymap_jump");
	key[KeyType::SPECIAL1] = getKeySetting("keymap_special1");
	key[KeyType::SNEAK] = getKeySetting("keymap_sneak");
	key[KeyType::DIG] = getKeySetting("keymap_dig");
	key[KeyType::PLACE] = getKeySetting("keymap_place");

	key[KeyType::AUTOFORWARD] = getKeySetting("keymap_autoforward");

	key[KeyType::DROP] = getKeySetting("keymap_drop");
	key[KeyType::INVENTORY] = getKeySetting("keymap_inventory");
	key[KeyType::CHAT] = getKeySetting("keymap_chat");
	key[KeyType::CMD] = getKeySetting("keymap_cmd");
	key[KeyType::CMD_LOCAL] = getKeySetting("keymap_cmd_local");
	key[KeyType::CONSOLE] = getKeySetting("keymap_console");
	key[KeyType::MINIMAP] = getKeySetting("keymap_minimap");
	key[KeyType::FREEMOVE] = getKeySetting("keymap_freemove");
	key[KeyType::PITCHMOVE] = getKeySetting("keymap_pitchmove");
	key[KeyType::FASTMOVE] = getKeySetting("keymap_fastmove");
	key[KeyType::NOCLIP] = getKeySetting("keymap_noclip");
	key[KeyType::HOTBAR_PREV] = getKeySetting("keymap_hotbar_previous");
	key[KeyType::HOTBAR_NEXT] = getKeySetting("keymap_hotbar_next");
	key[KeyType::MUTE] = getKeySetting("keymap_mute");
	key[KeyType::INC_VOLUME] = getKeySetting("keymap_increase_volume");
	key[KeyType::DEC_VOLUME] = getKeySetting("keymap_decrease_volume");
	key[KeyType::CINEMATIC] = getKeySetting("keymap_cinematic");
	key[KeyType::SCREENSHOT] = getKeySetting("keymap_screenshot");
	key[KeyType::TOGGLE_HUD] = getKeySetting("keymap_toggle_hud");
	key[KeyType::TOGGLE_CHAT] = getKeySetting("keymap_toggle_chat");
	key[KeyType::TOGGLE_FOG] = getKeySetting("keymap_toggle_fog");
	key[KeyType::TOGGLE_UPDATE_CAMERA] = getKeySetting("keymap_toggle_update_camera");
	key[KeyType::TOGGLE_DEBUG] = getKeySetting("keymap_toggle_debug");
	key[KeyType::TOGGLE_PROFILER] = getKeySetting("keymap_toggle_profiler");
	key[KeyType::CAMERA_MODE] = getKeySetting("keymap_camera_mode");
	key[KeyType::INCREASE_VIEWING_RANGE] =
			getKeySetting("keymap_increase_viewing_range_min");
	key[KeyType::DECREASE_VIEWING_RANGE] =
			getKeySetting("keymap_decrease_viewing_range_min");
	key[KeyType::RANGESELECT] = getKeySetting("keymap_rangeselect");
	key[KeyType::ZOOM] = getKeySetting("keymap_zoom");

	key[KeyType::QUICKTUNE_NEXT] = getKeySetting("keymap_quicktune_next");
	key[KeyType::QUICKTUNE_PREV] = getKeySetting("keymap_quicktune_prev");
	key[KeyType::QUICKTUNE_INC] = getKeySetting("keymap_quicktune_inc");
	key[KeyType::QUICKTUNE_DEC] = getKeySetting("keymap_quicktune_dec");

	for (int i = 0; i < HUD_HOTBAR_ITEMCOUNT_MAX; i++) {
		std::string slot_key_name = "keymap_slot" + std::to_string(i + 1);
		key[KeyType::SLOT_1 + i] = getKeySetting(slot_key_name.c_str());
	}

	if (handler) {
		// First clear all keys, then re-add the ones we listen for
		handler->dontListenForKeys();
		for (const KeyPress &k : key) {
			handler->listenForKey(k);
		}
		handler->listenForKey(EscapeKey);
		handler->listenForKey(CancelKey);
	}
}

void MyEventReceiver::handleControllerMouseMovement(int x, int y)
{
#if defined(_IRR_COMPILE_WITH_SDL_DEVICE_)
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

		device->postEventFromUser(translated_event);
		device->getCursorControl()->setPosition(mouse_pos.X, mouse_pos.Y);
	}

	m_mouse_time = current_time;
#endif
}

void MyEventReceiver::handleControllerTriggerLeft(s16 value)
{
#if defined(_IRR_COMPILE_WITH_SDL_DEVICE_)
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
		device->postEventFromUser(translated_event);
	}
	else if (value > deadzone && m_trigger_left_value <= deadzone)
	{
		translated_event.KeyInput.PressedDown = true;
		device->postEventFromUser(translated_event);
	}

	m_trigger_left_value = value;
#endif
}

void MyEventReceiver::handleControllerTriggerRight(s16 value)
{
#if defined(_IRR_COMPILE_WITH_SDL_DEVICE_)
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
		device->postEventFromUser(translated_event);
	}
	else if (value > deadzone && m_trigger_right_value <= deadzone)
	{
		translated_event.KeyInput.PressedDown = true;
		device->postEventFromUser(translated_event);
	}

	m_trigger_right_value = value;
#endif
}

void MyEventReceiver::handleControllerMouseClickLeft(bool pressed)
{
#if defined(_IRR_COMPILE_WITH_SDL_DEVICE_)
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

	device->postEventFromUser(translated_event);
#endif
}

void MyEventReceiver::handleControllerMouseClickRight(bool pressed)
{
#if defined(_IRR_COMPILE_WITH_SDL_DEVICE_)
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

	device->postEventFromUser(translated_event);
#endif
}

void MyEventReceiver::handleControllerButton(const SEvent &event)
{
#if defined(_IRR_COMPILE_WITH_SDL_DEVICE_)
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

		IrrlichtDevice* device = RenderingEngine::get_raw_device();
		device->postEventFromUser(translated_event);
	}
#endif
}

void MyEventReceiver::handleControllerButtonInMenu(const SEvent &event)
{
#if defined(_IRR_COMPILE_WITH_SDL_DEVICE_)
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

		IrrlichtDevice* device = RenderingEngine::get_raw_device();
		device->postEventFromUser(translated_event);
	}
#endif
}

void MyEventReceiver::translateGameControllerEvent(const SEvent &event)
{
#if defined(_IRR_COMPILE_WITH_SDL_DEVICE_)
	if (event.EventType == irr::EET_SDL_CONTROLLER_BUTTON_EVENT)
	{
		if (isMenuActive())
		{
			if (event.SDLControllerButtonEvent.Button == SDL_CONTROLLER_BUTTON_LEFTSTICK ||
				event.SDLControllerButtonEvent.Button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER)
			{
				handleControllerMouseClickLeft(event.SDLControllerButtonEvent.Pressed);
			}
			else if (event.SDLControllerButtonEvent.Button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER)
			{
				handleControllerMouseClickRight(event.SDLControllerButtonEvent.Pressed);
			}
			else
			{
				handleControllerButtonInMenu(event);
			}
		}
		else
		{
			if (event.SDLControllerButtonEvent.Button == SDL_CONTROLLER_BUTTON_RIGHTSTICK ||
				event.SDLControllerButtonEvent.Button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER)
			{
				handleControllerMouseClickLeft(event.SDLControllerButtonEvent.Pressed);
			}
			else if (event.SDLControllerButtonEvent.Button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER)
			{
				handleControllerMouseClickRight(event.SDLControllerButtonEvent.Pressed);
			}
			else
			{
				handleControllerButton(event);
			}
		}
	}
	else if (event.EventType == irr::EET_SDL_CONTROLLER_AXIS_EVENT)
	{
		const s16* value = event.SDLControllerAxisEvent.Value;

		if (isMenuActive())
		{
			handleControllerMouseMovement(value[SDL_CONTROLLER_AXIS_LEFTX], value[SDL_CONTROLLER_AXIS_LEFTY]);
		}
		else
		{
			handleControllerMouseMovement(value[SDL_CONTROLLER_AXIS_RIGHTX], value[SDL_CONTROLLER_AXIS_RIGHTY]);
			handleControllerTriggerLeft(value[SDL_CONTROLLER_AXIS_TRIGGERLEFT]);
			handleControllerTriggerRight(value[SDL_CONTROLLER_AXIS_TRIGGERRIGHT]);
		}
	}
#endif
}

bool MyEventReceiver::OnEvent(const SEvent &event)
{
	if (event.EventType == irr::EET_SDL_CONTROLLER_BUTTON_EVENT ||
		event.EventType == irr::EET_SDL_CONTROLLER_AXIS_EVENT)
	{
		translateGameControllerEvent(event);
		return true;
	}

#ifdef HAVE_TOUCHSCREENGUI
	if (event.EventType == irr::EET_TOUCH_INPUT_EVENT) {
		TouchScreenGUI::setActive(true);
		if (m_touchscreengui && !isMenuActive())
			m_touchscreengui->show();
	} else if (event.EventType == irr::EET_MOUSE_INPUT_EVENT &&
			event.MouseInput.Event == irr::EMIE_MOUSE_MOVED) {
		TouchScreenGUI::setActive(false);
		if (m_touchscreengui && !isMenuActive())
			m_touchscreengui->hide();
	}
#endif

	/*
		React to nothing here if a menu is active
	*/
	if (isMenuActive()) {
#ifdef HAVE_TOUCHSCREENGUI
		if (m_touchscreengui) {
			m_touchscreengui->hide();
		}
#endif
		return g_menumgr.preprocessEvent(event);
	}

	// Remember whether each key is down or up
	if (event.EventType == irr::EET_KEY_INPUT_EVENT) {
		const KeyPress &keyCode = event.KeyInput;
		if (keysListenedFor[keyCode]) {
			if (event.KeyInput.PressedDown) {
				if (!IsKeyDown(keyCode))
					keyWasPressed.set(keyCode);

				keyIsDown.set(keyCode);
				keyWasDown.set(keyCode);
			} else {
				if (IsKeyDown(keyCode))
					keyWasReleased.set(keyCode);

				keyIsDown.unset(keyCode);
			}

			return true;
		}

#ifdef HAVE_TOUCHSCREENGUI
	} else if (m_touchscreengui && event.EventType == irr::EET_TOUCH_INPUT_EVENT) {
		// In case of touchscreengui, we have to handle different events
		m_touchscreengui->translateEvent(event);
		return true;
#endif

#ifdef __IOS__
	} else if (event.EventType == irr::EET_APPLICATION_EVENT) {
		int AppEvent = event.ApplicationEvent.EventType;
		ioswrap_events(AppEvent);
		if (AppEvent == irr::EAET_WILL_PAUSE)
			external_pause_game();
		return true;
#endif

	} else if (event.EventType == irr::EET_JOYSTICK_INPUT_EVENT) {
		/* TODO add a check like:
		if (event.JoystickEvent != joystick_we_listen_for)
			return false;
		*/
		return joystick->handleEvent(event.JoystickEvent);
	} else if (event.EventType == irr::EET_MOUSE_INPUT_EVENT) {
		// Handle mouse events
		KeyPress key;
		switch (event.MouseInput.Event) {
		case EMIE_LMOUSE_PRESSED_DOWN:
			key = "KEY_LBUTTON";
			keyIsDown.set(key);
			keyWasDown.set(key);
			keyWasPressed.set(key);
			break;
		case EMIE_MMOUSE_PRESSED_DOWN:
			key = "KEY_MBUTTON";
			keyIsDown.set(key);
			keyWasDown.set(key);
			keyWasPressed.set(key);
			break;
		case EMIE_RMOUSE_PRESSED_DOWN:
			key = "KEY_RBUTTON";
			keyIsDown.set(key);
			keyWasDown.set(key);
			keyWasPressed.set(key);
			break;
		case EMIE_LMOUSE_LEFT_UP:
			key = "KEY_LBUTTON";
			keyIsDown.unset(key);
			keyWasReleased.set(key);
			break;
		case EMIE_MMOUSE_LEFT_UP:
			key = "KEY_MBUTTON";
			keyIsDown.unset(key);
			keyWasReleased.set(key);
			break;
		case EMIE_RMOUSE_LEFT_UP:
			key = "KEY_RBUTTON";
			keyIsDown.unset(key);
			keyWasReleased.set(key);
			break;
		case EMIE_MOUSE_WHEEL:
			mouse_wheel += event.MouseInput.Wheel;
			break;
		default: break;
		}
	} else if (event.EventType == irr::EET_LOG_TEXT_EVENT) {
		static const LogLevel irr_loglev_conv[] = {
				LL_VERBOSE, // ELL_DEBUG
				LL_INFO,    // ELL_INFORMATION
				LL_WARNING, // ELL_WARNING
				LL_ERROR,   // ELL_ERROR
				LL_NONE,    // ELL_NONE
		};
		assert(event.LogEvent.Level < ARRLEN(irr_loglev_conv));
#if !defined(__ANDROID__) && !defined(__IOS__)
		g_logger.log(irr_loglev_conv[event.LogEvent.Level],
				std::string("Irrlicht: ") + event.LogEvent.Text);
#endif
		return true;
	}
	/* always return false in order to continue processing events */
	return false;
}

/*
 * RandomInputHandler
 */
s32 RandomInputHandler::Rand(s32 min, s32 max)
{
	return (myrand() % (max - min + 1)) + min;
}

struct RandomInputHandlerSimData {
	std::string key;
	float counter;
	int time_max;
};

void RandomInputHandler::step(float dtime)
{
	static RandomInputHandlerSimData rnd_data[] = {
		{ "keymap_jump", 0.0f, 40 },
		{ "keymap_special1", 0.0f, 40 },
		{ "keymap_forward", 0.0f, 40 },
		{ "keymap_left", 0.0f, 40 },
		{ "keymap_dig", 0.0f, 30 },
		{ "keymap_place", 0.0f, 15 }
	};

	for (auto &i : rnd_data) {
		i.counter -= dtime;
		if (i.counter < 0.0) {
			i.counter = 0.1 * Rand(1, i.time_max);
			keydown.toggle(getKeySetting(i.key.c_str()));
		}
	}
	{
		static float counter1 = 0;
		counter1 -= dtime;
		if (counter1 < 0.0) {
			counter1 = 0.1 * Rand(1, 20);
			mousespeed = v2s32(Rand(-20, 20), Rand(-15, 20));
		}
	}
	mousepos += mousespeed;
}
