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
#include "gui/guiChatConsole.h"
#include "gui/mainmenumanager.h"
#include "hud.h"
#include "settings.h"

#ifdef __IOS__
#include "porting.h"
extern "C" void external_pause_game();
#endif

#if defined(_IRR_COMPILE_WITH_SDL_DEVICE_)
#include <SDL3/SDL.h>
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
	key[KeyType::TOGGLE_NAMETAGS] = getKeySetting("keymap_toggle_nametags");
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

	key[KeyType::TABB] = getKeySetting("keymap_tabb");

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

bool MyEventReceiver::OnEvent(const SEvent &event)
{
	setLastInputDevice(event);

#if defined(_IRR_COMPILE_WITH_SDL_DEVICE_)
	if (event.EventType == irr::EET_SDL_CONTROLLER_BUTTON_EVENT ||
			event.EventType == irr::EET_SDL_CONTROLLER_AXIS_EVENT) {
		if (g_settings->getBool("enable_joysticks") && sdl_game_controller) {
			sdl_game_controller->translateEvent(event);
			input->setCursorVisible(sdl_game_controller->isCursorVisible());
		}
	} else if ((event.EventType == irr::EET_MOUSE_INPUT_EVENT &&
			event.MouseInput.Event == irr::EMIE_MOUSE_MOVED) ||
			event.EventType == irr::EET_TOUCH_INPUT_EVENT) {
		if (sdl_game_controller && !sdl_game_controller->isFakeEvent() &&
				sdl_game_controller->isActive()) {
			sdl_game_controller->setActive(false);
			input->setCursorVisible(sdl_game_controller->isCursorVisible());
		}
	}
#endif

#ifdef HAVE_TOUCHSCREENGUI
	if (event.EventType == irr::EET_TOUCH_INPUT_EVENT) {
		TouchScreenGUI::setActive(true);
		if (m_touchscreengui && !isMenuActive())
			m_touchscreengui->show();
	} else if ((event.EventType == irr::EET_MOUSE_INPUT_EVENT &&
			event.MouseInput.Event == irr::EMIE_MOUSE_MOVED) ||
			(sdl_game_controller && sdl_game_controller->isActive())) {
		TouchScreenGUI::setActive(false);
		if (m_touchscreengui && m_touchscreengui->getCurrentState() != STATE_EDITOR && !isMenuActive())
			m_touchscreengui->hide();
	}
#endif

#ifdef HAVE_TOUCHSCREENGUI
	// Always send touch events to the touchscreen gui, so it has up-to-date information
	if (m_touchscreengui) {
		bool result = m_touchscreengui->preprocessEvent(event);

		if (result) {
			for (auto key : keysListenedFor) {
				irr::EKEY_CODE key_code = key.getKeyCode();
				bool pressed = m_touchscreengui->isButtonPressed(key_code);
				if (pressed) {
					if (!IsKeyDown(key)) {
						keyWasPressed.set(key);
						keyIsDown.set(key);
						keyWasDown.set(key);
					}
				}
				if (!pressed || m_touchscreengui->immediateRelease(key_code)) {
					if (IsKeyDown(key))
						keyWasReleased.set(key);

					keyIsDown.unset(key);
				}
			}
		}
	}
#endif

#if defined(_IRR_COMPILE_WITH_SDL_DEVICE_)
	if (event.EventType == irr::EET_SDL_CONTROLLER_BUTTON_EVENT)
		return true;
#endif

	GUIChatConsole* chat_console = GUIChatConsole::getChatConsole();
	if (chat_console && chat_console->isOpen()) {
		bool result = chat_console->preprocessEvent(event);
		if (result)
			return true;
	}

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
	} else if (event.EventType == irr::EET_JOYSTICK_INPUT_EVENT) {
		/* TODO add a check like:
		if (event.JoystickEvent != joystick_we_listen_for)
			return false;
		*/
		if (joystick)
			return joystick->handleEvent(event.JoystickEvent);
		else
			return true;
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
#ifndef NDEBUG
		static const LogLevel irr_loglev_conv[] = {
				LL_VERBOSE, // ELL_DEBUG
				LL_INFO,    // ELL_INFORMATION
				LL_WARNING, // ELL_WARNING
				LL_ERROR,   // ELL_ERROR
				LL_NONE,    // ELL_NONE
		};
		assert(event.LogEvent.Level < ARRLEN(irr_loglev_conv));
		g_logger.log(irr_loglev_conv[event.LogEvent.Level],
				std::string("Irrlicht: ") + event.LogEvent.Text);
#endif
		return true;
	}
	/* always return false in order to continue processing events */
	return false;
}

void MyEventReceiver::setLastInputDevice(const SEvent &event)
{
	if (!input)
		return;

	InputDeviceType input_device = IDT_NONE;

	if (g_settings->getBool("enable_joysticks")) {
		if (event.EventType == irr::EET_SDL_CONTROLLER_BUTTON_EVENT) {
			input_device = IDT_GAMEPAD;
		} else if (event.EventType == irr::EET_SDL_CONTROLLER_AXIS_EVENT) {
			const s16* value = event.SDLControllerAxisEvent.Value;
			int deadzone = g_settings->getU16("joystick_deadzone");

			if (value[SDL_GAMEPAD_AXIS_LEFT_TRIGGER] > deadzone ||
					value[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER] > deadzone ||
					std::abs(value[SDL_GAMEPAD_AXIS_LEFTX]) > deadzone ||
					std::abs(value[SDL_GAMEPAD_AXIS_LEFTY]) > deadzone ||
					std::abs(value[SDL_GAMEPAD_AXIS_RIGHTX]) > deadzone ||
					std::abs(value[SDL_GAMEPAD_AXIS_RIGHTY]) > deadzone)
				input_device = IDT_GAMEPAD;
#if defined(_IRR_COMPILE_WITH_SDL_DEVICE_)
		} else if (sdl_game_controller && sdl_game_controller->isFakeEvent()) {
			input_device = IDT_GAMEPAD;
#endif
		}
	}

	if (input_device == IDT_NONE) {
		if (event.EventType == irr::EET_KEY_INPUT_EVENT) {
			input_device = IDT_KEYBOARD;
		} else if (event.EventType == irr::EET_MOUSE_INPUT_EVENT) {
			input_device = IDT_MOUSE;
		} else if (event.EventType == irr::EET_TOUCH_INPUT_EVENT) {
			input_device = IDT_TOUCH;
		}
	}

	if (input_device != IDT_NONE)
		input->last_input_device = input_device;
}

RealInputHandler::RealInputHandler(MyEventReceiver *receiver) : m_receiver(receiver)
{
	m_receiver->joystick = &joystick;
#if defined(_IRR_COMPILE_WITH_SDL_DEVICE_)
	m_receiver->sdl_game_controller = &sdl_game_controller;
	m_receiver->input = this;

#ifdef __IOS__
	SDL_AddEventWatch(SdlEventWatcher, nullptr);
#endif
#endif
}

RealInputHandler::~RealInputHandler()
{
#if defined(_IRR_COMPILE_WITH_SDL_DEVICE_)
#ifdef __IOS__
	SDL_RemoveEventWatch(SdlEventWatcher, nullptr);
#endif
#endif
}

#ifdef __IOS__
bool RealInputHandler::SdlEventWatcher(void *userdata, SDL_Event *event)
{
	if (event->type == SDL_EVENT_WILL_ENTER_BACKGROUND) {
		external_pause_game();
		return true;
	}

	return false;
}
#endif

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
