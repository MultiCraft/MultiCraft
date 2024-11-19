/*
Copyright (C) 2014 sapier
Copyright (C) 2018 srifqi, Muhammad Rifqi Priyo Susanto
		<muhammadrifqipriyosusanto@gmail.com>
Copyright (C) 2014-2022 Maksim Gamarnik [MoNTE48] Maksym48@pm.me

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

#include "touchscreengui.h"
#include "irrlichttypes.h"
#include "client/keycode.h"
#include "settings.h"
#include "gettext.h"
#include "porting.h"
#include "client/guiscalingfilter.h"
#include "client/renderingengine.h"
#include <IGUIFont.h>
#include <ISceneCollisionManager.h>

using namespace irr::core;

#define MIN_DIG_TIME_MS 500
#define MIN_PLACE_TIME_MS 50
#define OVERFLOW_MENU_COLS 3
#define OVERFLOW_MENU_ROWS 2

const button_data buttons_data[] = {
	{ "jump_btn.png", N_("Jump"), "jump" },
	{ "drop_btn.png", N_("Drop"), "drop" },
	{ "down_btn.png", N_("Sneak"), "sneak" },
	{ "aux_btn.png", N_("Special"), "special1" },
	{ "inventory_btn.png", N_("Inventory"), "inventory" },
	{ "escape_btn.png", N_("Exit"), "escape" },
	{ "minimap_btn.png", N_("Toggle minimap"), "minimap" },
	{ "rangeview_btn.png", N_("Range select"), "rangeselect" },
	{ "camera_btn.png", N_("Change camera"), "camera_mode" },
	{ "chat_btn.png", N_("Chat"), "chat" },
	{ "tab_btn.png", N_("Tab"), "tabb" },
	{ "overflow_btn.png", N_("Overflow menu"), "overflow" },
	{ "chat_show_btn.png", N_("Toggle chat log"), "toggle_chat" },
	{ "overflow_btn.png", N_("Toggle Nametags"), "toggle_nametags" },
	{ "fast_btn.png", N_("Toggle fly"), "freemove" },
	{ "fast_btn.png", N_("Toggle fast"), "fastmove" },
	{ "noclip_btn.png", N_("Toggle noclip"), "noclip" },
	{ "joystick_off.png", "", "" },
	{ "joystick_bg.png", "", "" },
	{ "joystick_center.png", "", "" },
};

static const touch_gui_button_id overflow_buttons_id[] {
	flymove_id, fastmove_id, noclip_id,
	range_id, toggle_chat_id, toggle_nametags_id
};

TouchScreenGUI *g_touchscreengui = nullptr;
bool TouchScreenGUI::m_active = true;

TouchScreenGUI::TouchScreenGUI(IrrlichtDevice *device):
		m_device(device), m_guienv(device->getGUIEnvironment())
{
	m_touchscreen_threshold = g_settings->getU16("touchscreen_threshold");
	m_touch_sensitivity = rangelim(g_settings->getFloat("touch_sensitivity"), 0.1, 1.0);
	m_screensize = m_device->getVideoDriver()->getScreenSize();
	m_button_size = std::min(m_screensize.Y / 4.5f,
			RenderingEngine::getDisplayDensity() *
			g_settings->getFloat("hud_scaling") * 65.0f);

	std::string keyname_dig = g_settings->get("keymap_dig");
	m_keycode_dig = keyname_to_keycode(keyname_dig.c_str());
	std::string keyname_place = g_settings->get("keymap_place");
	m_keycode_place = keyname_to_keycode(keyname_place.c_str());
}

TouchScreenGUI::~TouchScreenGUI()
{
	if (!m_buttons_initialized)
		return;

	for (auto button : m_buttons) {
		if (button->guibutton) {
			button->guibutton->drop();
			button->guibutton = nullptr;
		}

		if (button->text) {
			button->text->drop();
			button->text = nullptr;
		}

		delete button;
	}

	if (m_overflow_bg)
		m_overflow_bg->drop();

	if (m_joystick.button_off) {
		m_joystick.button_off->drop();
		m_joystick.button_off = nullptr;
	}

	if (m_joystick.button_bg) {
		m_joystick.button_bg->drop();
		m_joystick.button_bg = nullptr;
	}

	if (m_joystick.button_center) {
		m_joystick.button_center->drop();
		m_joystick.button_center = nullptr;
	}
}

void TouchScreenGUI::init(ISimpleTextureSource *tsrc, bool simple_singleplayer_mode)
{
	assert(tsrc);
	m_texturesource = tsrc;

	initJoystickButton();

	initButton(jump_id, getButtonRect(jump_id));
	initButton(drop_id, getButtonRect(drop_id));
	initButton(crunch_id, getButtonRect(crunch_id));
	initButton(inventory_id, getButtonRect(inventory_id));
	initButton(special1_id, getButtonRect(special1_id));
	initButton(escape_id, getButtonRect(escape_id));
	initButton(minimap_id, getButtonRect(minimap_id));
	initButton(range_id, getButtonRect(range_id));
	initButton(camera_id, getButtonRect(camera_id));

	if (simple_singleplayer_mode) {
		initButton(chat_id, getButtonRect(chat_id), false, "chat_btn.png");
	} else {
		initButton(chat_id, getButtonRect(chat_id), false, "chat_mp_btn.png");
		initButton(tab_id, getButtonRect(tab_id));
	}

	initButton(overflow_id, getButtonRect(overflow_id));

	m_overflow_bg = m_guienv->addStaticText(L"", recti());
	m_overflow_bg->setBackgroundColor(video::SColor(140, 0, 0, 0));
	m_overflow_bg->setVisible(m_overflow_open);
	m_overflow_bg->grab();

	for (auto id : overflow_buttons_id) {
		initButton(id, recti(), true);
	}

	rebuildOverflowMenu();

	m_buttons_initialized = true;
}

void TouchScreenGUI::loadButtonTexture(IGUIButton *btn, const char *path,
		const rect<s32> &button_rect)
{
	video::ITexture *texture = guiScalingImageButton(m_device->getVideoDriver(),
			m_texturesource->getTexture(path, nullptr), button_rect.getWidth(),
			button_rect.getHeight());

	if (texture) {
		btn->setUseAlphaChannel(true);
		if (g_settings->getBool("gui_scaling_filter")) {
			rect<s32> txr_rect = rect<s32>(v2s32(0, 0), button_rect.getSize());
			btn->setImage(texture, txr_rect);
			btn->setPressedImage(texture, txr_rect);
			btn->setScaleImage(false);
		} else {
			btn->setImage(texture);
			btn->setPressedImage(texture);
			btn->setScaleImage(true);
		}
		btn->setDrawBorder(false);
		btn->setText(L"");
	}
}

void TouchScreenGUI::initButton(touch_gui_button_id id, const rect<s32> &button_rect,
		bool overflow_menu, const char *texture)
{
	button_info *btn = new button_info();
	btn->overflow_menu = overflow_menu;
	btn->id = id;

	btn->guibutton = m_guienv->addButton(button_rect, nullptr);
	btn->guibutton->grab();
	btn->guibutton->setVisible(m_visible && !overflow_menu);
	const char *image = strcmp(texture, "") == 0 ? buttons_data[id].image : texture;
	loadButtonTexture(btn->guibutton, image, button_rect);

	const wchar_t *str = wgettext(buttons_data[id].title);
	btn->text = m_guienv->addStaticText(str, recti());
	btn->text->setTextAlignment(EGUIA_CENTER, EGUIA_UPPERLEFT);
	btn->text->setVisible(m_overflow_open);
	btn->text->grab();
	delete[] str;

	m_buttons.push_back(btn);
}

void TouchScreenGUI::initJoystickButton()
{
	const rect<s32> &button_off_rect = getButtonRect(joystick_off_id);
	m_joystick.button_off = m_guienv->addButton(button_off_rect, nullptr);
	m_joystick.button_off->setVisible(m_visible);
	m_joystick.button_off->grab();
	loadButtonTexture(m_joystick.button_off,
			buttons_data[joystick_off_id].image, button_off_rect);

	const rect<s32> &button_bg_rect = getButtonRect(joystick_bg_id);
	m_joystick.button_bg = m_guienv->addButton(button_bg_rect, nullptr);
	m_joystick.button_bg->setVisible(false);
	m_joystick.button_bg->grab();
	loadButtonTexture(m_joystick.button_bg,
			buttons_data[joystick_bg_id].image, button_bg_rect);

	const rect<s32> &button_center_rect = getButtonRect(joystick_center_id);
	m_joystick.button_center = m_guienv->addButton(button_center_rect, nullptr);
	m_joystick.button_center->setVisible(false);
	m_joystick.button_center->grab();
	loadButtonTexture(m_joystick.button_center,
			buttons_data[joystick_center_id].image, button_center_rect);
}

rect<s32> TouchScreenGUI::getButtonRect(touch_gui_button_id id)
{
	switch (id) {
	case joystick_off_id:
		return rect<s32>(m_button_size / 2,
				m_screensize.Y - m_button_size * 4.5,
				m_button_size * 4.5,
				m_screensize.Y - m_button_size / 2);
	case joystick_bg_id:
		return rect<s32>(m_button_size / 2,
				m_screensize.Y - m_button_size * 4.5,
				m_button_size * 4.5,
				m_screensize.Y - m_button_size / 2);
	case joystick_center_id:
		return rect<s32>(0, 0, m_button_size * 1.5, m_button_size * 1.5);
	case jump_id:
		return rect<s32>(m_screensize.X - m_button_size * 3.37,
				m_screensize.Y - m_button_size * 2.75,
				m_screensize.X - m_button_size * 1.87,
				m_screensize.Y - m_button_size * 1.25);
	case drop_id:
		return rect<s32>(m_screensize.X - m_button_size,
				m_screensize.Y / 2 - m_button_size * 1.5,
				m_screensize.X,
				m_screensize.Y / 2 - m_button_size / 2);
	case crunch_id:
		return rect<s32>(m_screensize.X - m_button_size * 3.38,
				m_screensize.Y - m_button_size * 0.75,
				m_screensize.X - m_button_size * 1.7,
				m_screensize.Y);
	case inventory_id:
		return rect<s32>(m_screensize.X - m_button_size * 1.7,
				m_screensize.Y - m_button_size * 1.5,
				m_screensize.X,
				m_screensize.Y);
	case special1_id:
		return rect<s32>(m_screensize.X - m_button_size * 1.8,
				m_screensize.Y - m_button_size * 4,
				m_screensize.X - m_button_size * 0.3,
				m_screensize.Y - m_button_size * 2.5);
	case escape_id:
		return rect<s32>(m_screensize.X / 2 - m_button_size * 2,
				0,
				m_screensize.X / 2 - m_button_size,
				m_button_size);
	case minimap_id:
		return rect<s32>(m_screensize.X / 2 - m_button_size,
				0,
				m_screensize.X / 2,
				m_button_size);
	case range_id:
		return rect<s32>(m_screensize.X / 2,
				0,
				m_screensize.X / 2 + m_button_size,
				m_button_size);
	case camera_id:
		return rect<s32>(m_screensize.X / 2 + m_button_size,
				0,
				m_screensize.X / 2 + m_button_size * 2,
				m_button_size);
	case chat_id:
		return rect<s32>(m_screensize.X - m_button_size * 1.25,
				0,
				m_screensize.X,
				m_button_size);
	case tab_id:
		return rect<s32>(m_screensize.X - m_button_size * 1.25,
				m_button_size,
				m_screensize.X,
				m_button_size * 2);
	case overflow_id:
		return rect<s32>(m_screensize.X - m_button_size * 1.25,
				m_button_size * 2,
				m_screensize.X,
				m_button_size * 3);
	default:
		return rect<s32>(0, 0, 0, 0);
	}
}

void TouchScreenGUI::updateButtons()
{
	v2u32 screensize = m_device->getVideoDriver()->getScreenSize();

	if (screensize == m_screensize)
		return;

	m_screensize = screensize;
	m_button_size = std::min(m_screensize.Y / 4.5f,
			RenderingEngine::getDisplayDensity() *
			g_settings->getFloat("hud_scaling") * 65.0f);

	for (auto button : m_buttons) {
		if (button->overflow_menu)
			continue;

		if (button->guibutton) {
			rect<s32> rect = getButtonRect(button->id);
			button->guibutton->setRelativePosition(rect);
		}
	}

	if (m_joystick.button_off) {
		rect<s32> rect = getButtonRect(joystick_off_id);
		m_joystick.button_off->setRelativePosition(rect);
	}

	if (m_joystick.button_bg) {
		rect<s32> rect = getButtonRect(joystick_bg_id);
		m_joystick.button_bg->setRelativePosition(rect);
	}

	if (m_joystick.button_center) {
		rect<s32> rect = getButtonRect(joystick_center_id);
		m_joystick.button_center->setRelativePosition(rect);
	}

	rebuildOverflowMenu();
}

void TouchScreenGUI::rebuildOverflowMenu()
{
	recti rect(v2s32(0, 0), dimension2du(m_screensize));
	m_overflow_bg->setRelativePosition(rect);

	assert((s32)ARRLEN(overflow_buttons_id) <= OVERFLOW_MENU_COLS * OVERFLOW_MENU_ROWS);
	v2s32 size(m_button_size, m_button_size);
	v2s32 spacing(m_screensize.X / (OVERFLOW_MENU_COLS + 1),
			m_screensize.Y / (OVERFLOW_MENU_ROWS + 1));
	v2s32 pos(spacing);

	for (auto button : m_buttons) {
		if (!button->overflow_menu)
			continue;

		recti button_rect(pos - size / 2, dimension2du(size.X, size.Y));
		if (button_rect.LowerRightCorner.X > (s32)m_screensize.X) {
			pos.X = spacing.X;
			pos.Y += spacing.Y;
			button_rect = recti(pos - size / 2, dimension2du(size.X, size.Y));
		}

		button->guibutton->setRelativePosition(button_rect);

		const wchar_t *str = wgettext(buttons_data[button->id].title);
		IGUIFont *font = button->text->getActiveFont();
		dimension2du dim = font->getDimension(str);
		dim = dimension2du(dim.Width * 1.25f, dim.Height * 1.25f);
		recti text_rect = recti(pos.X - dim.Width / 2, pos.Y + size.Y / 2,
				pos.X + dim.Width / 2, pos.Y + size.Y / 2 + dim.Height);
		button->text->setRelativePosition(text_rect);
		delete[] str;

		pos.X += spacing.X;
	}
}

bool TouchScreenGUI::preprocessEvent(const SEvent &event)
{
	if (!m_buttons_initialized)
		return false;

	if (!m_visible)
		return false;

	if (event.EventType != EET_TOUCH_INPUT_EVENT)
		return false;

	bool result = false;
	s32 id = (unsigned int)event.TouchInput.ID;
	s32 x = event.TouchInput.X;
	s32 y = event.TouchInput.Y;

	if (event.TouchInput.Event == ETIE_PRESSED_DOWN) {
		m_events[id] = false;
		bool overflow_btn_pressed = false;

		for (auto button : m_buttons) {
			if (m_overflow_open != button->overflow_menu)
				continue;

			if (button->guibutton->isPointInside(core::position2d<s32>(x, y))) {
				m_events[id] = true;
				button->pressed = true;
				button->event_id = id;

				if (button->id == overflow_id)
					overflow_btn_pressed = true;
			}
		}

		if (!m_overflow_open) {
			if (m_joystick.button_off->isPointInside(core::position2d<s32>(x, y))) {
				m_events[id] = true;
				m_joystick.button_off->setVisible(false);
				m_joystick.button_bg->setVisible(true);
				m_joystick.button_center->setVisible(true);
				m_joystick.pressed = true;
				m_joystick.event_id = id;

				moveJoystick(x, y);
			}

			if (!m_events[id]) {
				for (auto &hud_button : m_hud_buttons) {
					if (hud_button.button_rect.isPointInside(v2s32(x, y))) {
						m_events[id] = true;
						hud_button.pressed = true;
					}
				}
			}

			if (!m_events[id]) {
				m_events[id] = true;
				m_camera.downtime = porting::getTimeMs();
				m_camera.x = x;
				m_camera.y = y;
				m_camera.event_id = id;

				updateCamera(x, y);
			}
		}

		if (overflow_btn_pressed || (m_overflow_open && !m_events[id]))
			toggleOverflowMenu();

		result = true;

	} else if (event.TouchInput.Event == ETIE_LEFT_UP) {
		m_events.erase(id);

		for (auto button : m_buttons) {
			if (m_overflow_open != button->overflow_menu)
				continue;

			if (button->event_id == id)
				button->reset();
		}

		if (m_joystick.event_id == id)
			m_joystick.reset(m_visible && !m_overflow_open);

		if (m_camera.event_id == id) {
			bool place = false;
			if (!m_camera.has_really_moved && !m_camera.dig) {
				u64 delta = porting::getDeltaMs(m_camera.downtime, porting::getTimeMs());
				place = (delta >= MIN_PLACE_TIME_MS);
			}
			m_camera.reset();
			m_camera.place = place;
		}

		result = true;

	} else if (event.TouchInput.Event == ETIE_MOVED) {
		if (m_events[id]) {
			if (m_joystick.event_id == id) {
				if (moveJoystick(x, y))
					result = true;
			} else if (m_camera.event_id == id) {
				updateCamera(x, y);
			} else {
				bool overflow_btn_pressed = false;

				for (auto button : m_buttons) {
					if (m_overflow_open != button->overflow_menu)
						continue;

					if (button->guibutton->isPointInside(core::position2d<s32>(x, y))) {
						if (button->id == overflow_id)
							overflow_btn_pressed = true;

						button->pressed = true;
						button->event_id = id;
						result = true;
					} else if (button->event_id == id) {
						button->reset();
						result = true;
					}
				}

				if (overflow_btn_pressed)
					toggleOverflowMenu();
			}
		}

	} else if (event.TouchInput.Event == ETIE_COUNT) {
		result = true;
	}

	return result;
}

bool TouchScreenGUI::moveJoystick(s32 x, s32 y)
{
	s32 dx = x - m_button_size * 5 / 2;
	s32 dy = y - m_screensize.Y + m_button_size * 5 / 2;
	double distance = sqrt(dx * dx + dy * dy);

	if (distance > m_button_size * 1.5) {
		s32 ndx = m_button_size * dx / distance * 1.5f - m_button_size / 2.0f * 1.5f;
		s32 ndy = m_button_size * dy / distance * 1.5f - m_button_size / 2.0f * 1.5f;
		m_joystick.button_center->setRelativePosition(v2s32(
				m_button_size * 5 / 2 + ndx,
				m_screensize.Y - m_button_size * 5 / 2 + ndy));
	} else {
		m_joystick.button_center->setRelativePosition(v2s32(
				x - m_button_size / 2.0f * 1.5f,
				y - m_button_size / 2.0f * 1.5f));
	}

	// angle in degrees
	double angle = acos(dx / distance) * 180 / M_PI;
	if (dy < 0)
		angle *= -1;
	// rotate to make comparing easier
	angle = fmod(angle + 180 + 22.5, 360);

	bool old_move_sideward = m_joystick.move_sideward;
	bool old_move_forward = m_joystick.move_forward;
	m_joystick.move_sideward = 0;
	m_joystick.move_forward = 0;

	if (distance <= m_touchscreen_threshold) {
		// do nothing
	} else if (angle < 45)
		m_joystick.move_sideward = -32768;
	else if (angle < 90) {
		m_joystick.move_forward = -32768;
		m_joystick.move_sideward = -32768;
	} else if (angle < 135)
		m_joystick.move_forward = -32768;
	else if (angle < 180) {
		m_joystick.move_forward = -32768;
		m_joystick.move_sideward = 32767;
	} else if (angle < 225)
		m_joystick.move_sideward = 32767;
	else if (angle < 270) {
		m_joystick.move_forward = 32767;
		m_joystick.move_sideward = 32767;
	} else if (angle < 315)
		m_joystick.move_forward = 32767;
	else if (angle <= 360) {
		m_joystick.move_forward = 32767;
		m_joystick.move_sideward = -32768;
	}

	if (old_move_sideward != m_joystick.move_sideward ||
			old_move_forward != m_joystick.move_forward)
		return true;

	return false;
}

void TouchScreenGUI::updateCamera(s32 x, s32 y)
{
	double distance = sqrt(
			(m_camera.x - x) * (m_camera.x - x) +
			(m_camera.y - y) * (m_camera.y - y));

	if (!m_camera.dig && ((distance > m_touchscreen_threshold) ||
			m_camera.has_really_moved)) {
		m_camera.has_really_moved = true;

		m_camera.yaw_change -= (x - m_camera.x) * m_touch_sensitivity;
		m_camera.pitch += (y - m_camera.y) * m_touch_sensitivity;
		m_camera.pitch = MYMIN(MYMAX(m_camera.pitch, -180), 180);

		m_camera.x = x;
		m_camera.y = y;
	}

	m_camera.shootline = m_device->getSceneManager()
			->getSceneCollisionManager()
			->getRayFromScreenCoordinates(v2s32(x, y));
}

bool TouchScreenGUI::isButtonPressed(irr::EKEY_CODE keycode)
{
	for (auto button : m_buttons) {
		if (!button->pressed)
			continue;

		std::string button_name = buttons_data[button->id].name;
		std::string keyname = g_settings->get("keymap_" + button_name);
		irr::EKEY_CODE button_keycode = keyname_to_keycode(keyname.c_str());

		if (button_keycode == keycode)
			return true;
	}

	for (auto &hud_button : m_hud_buttons) {
		if (!hud_button.pressed)
			continue;

		irr::EKEY_CODE button_keycode = (irr::EKEY_CODE) (KEY_KEY_1 + hud_button.id);

		if (button_keycode == keycode) {
			hud_button.pressed = false;
			return true;
		}
	}

	if (m_joystick.move_sideward) {
		std::string button_name = m_joystick.move_sideward < 0 ? "left" : "right";
		std::string keyname = g_settings->get("keymap_" + button_name);
		irr::EKEY_CODE button_keycode = keyname_to_keycode(keyname.c_str());

		if (button_keycode == keycode)
			return true;
	}

	if (m_joystick.move_forward) {
		std::string button_name = m_joystick.move_forward < 0 ? "forward" : "backward";
		std::string keyname = g_settings->get("keymap_" + button_name);
		irr::EKEY_CODE button_keycode = keyname_to_keycode(keyname.c_str());

		if (button_keycode == keycode)
			return true;
	}

	if (m_camera.dig) {
		if (m_keycode_dig == keycode)
			return true;
	}

	if (m_camera.place) {
		if (m_keycode_place == keycode) {
			m_camera.place = false;
			return true;
		}
	}

	return false;
}

bool TouchScreenGUI::immediateRelease(irr::EKEY_CODE keycode)
{
	if (m_keycode_place == keycode)
		return true;

	for (auto &hud_button : m_hud_buttons) {
		irr::EKEY_CODE button_keycode = (irr::EKEY_CODE) (KEY_KEY_1 + hud_button.id);

		if (button_keycode == keycode)
			return true;
	}

	return false;
}

void TouchScreenGUI::step(float dtime)
{
	updateButtons();

	if (m_camera.event_id != -1 && (!m_camera.has_really_moved)) {
		u64 delta = porting::getDeltaMs(m_camera.downtime, porting::getTimeMs());
		if (delta > MIN_DIG_TIME_MS) {
			m_camera.dig = true;
			wakeUpInputhandler();
		}
	}
}

void TouchScreenGUI::registerHudItem(s32 index, const rect<s32> &button_rect)
{
	for (auto &hud_button : m_hud_buttons) {
		if (hud_button.id == index) {
			hud_button.button_rect = button_rect;
			return;
		}
	}

	hud_button_info hud_button = {index, button_rect};
	m_hud_buttons.push_back(hud_button);
}

void TouchScreenGUI::resetHud()
{
	m_hud_buttons.clear();
}

void TouchScreenGUI::setVisible(bool visible)
{
	m_visible = visible;

	if (!m_buttons_initialized)
		return;

	for (auto button : m_buttons) {
		bool is_visible = m_overflow_open == button->overflow_menu;
		if (button->guibutton)
			button->guibutton->setVisible(m_visible && is_visible);
		if (button->text)
			button->text->setVisible(m_visible && is_visible);
	}

	if (m_joystick.button_off)
		m_joystick.button_off->setVisible(m_visible && !m_overflow_open);
	if (m_joystick.button_bg)
		m_joystick.button_bg->setVisible(false);
	if (m_joystick.button_center)
		m_joystick.button_center->setVisible(false);

	if (m_overflow_bg)
		m_overflow_bg->setVisible(m_visible && m_overflow_open);

	if (!visible)
		reset();
}

void TouchScreenGUI::toggleOverflowMenu()
{
	reset();
	m_overflow_open = !m_overflow_open;
	setVisible(m_visible);
}

void TouchScreenGUI::hide()
{
	if (!m_visible)
		return;

	setVisible(false);
}

void TouchScreenGUI::show()
{
	if (m_visible)
		return;

	setVisible(true);
}

void TouchScreenGUI::reset()
{
	m_events.clear();

	for (auto button : m_buttons) {
		button->reset();
	}

	m_joystick.reset(m_visible && !m_overflow_open);
	m_camera.reset();
}

void TouchScreenGUI::wakeUpInputhandler()
{
	SEvent event;
	memset(&event, 0, sizeof(SEvent));
	event.EventType = EET_TOUCH_INPUT_EVENT;
	event.TouchInput.Event = ETIE_COUNT;

	m_device->postEventFromUser(event);
}
