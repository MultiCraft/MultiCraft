/*
Copyright (C) 2014 sapier
Copyright (C) 2018 srifqi, Muhammad Rifqi Priyo Susanto
		<muhammadrifqipriyosusanto@gmail.com>
Copyright (C) 2014-2024 Maksim Gamarnik [MoNTE48] Maksym48@pm.me
Copyright (C) 2023-2024 Dawid Gan <deveee@gmail.com>

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
#include "filesys.h"
#include "gettext.h"
#include "porting.h"
#include "client/guiscalingfilter.h"
#include "client/renderingengine.h"
#include <IGUIFont.h>
#include <ISceneCollisionManager.h>

using namespace irr::core;

const button_data buttons_data[] = {
	{ "jump_btn.png", N_("Jump"), "jump" },
	{ "drop_btn.png", N_("Drop"), "drop" },
	{ "sneak_btn.png", N_("Sneak"), "sneak" },
	{ "special1_btn.png", N_("Special"), "special1" },
	{ "inventory_btn.png", N_("Inventory"), "inventory" },
	{ "escape_btn.png", N_("Exit"), "escape" },
	{ "minimap_btn.png", N_("Toggle minimap"), "minimap" },
	{ "camera_btn.png", N_("Change camera"), "camera_mode" },
	{ "overflow_btn.png", N_("Overflow menu"), "overflow" },
	{ "chat_btn.png", N_("Chat"), "chat" },
	{ "tab_btn.png", N_("Tab"), "tabb" },
	{ "fly_btn.png", N_("Toggle fly"), "freemove" },
	{ "fast_btn.png", N_("Toggle fast"), "fastmove" },
	{ "noclip_btn.png", N_("Toggle noclip"), "noclip" },
	{ "rangeview_btn.png", N_("Range select"), "rangeselect" },
	{ "chat_hide_btn.png", N_("Toggle chat log"), "toggle_chat" },
	{ "names_hide_btn.png", N_("Toggle nametags"), "toggle_nametags" },
	{ "joystick_off.png", "", "joystick" },
	{ "joystick_bg.png", "", "joystick" },
	{ "joystick_center.png", "", "joystick_center" },
	{ "overflow_btn.png", N_("Open editor"), "editor_open" },
	{ "", N_("Close"), "editor_close" },
	{ "", N_("Restore"), "editor_default" },
	{ "", N_("Move"), "editor_move" },
	{ "", N_("Scale"), "editor_scale" },
};

static const touch_gui_button_id overflow_buttons_id[] {
	flymove_id, fastmove_id, noclip_id,
	range_id, toggle_chat_id, toggle_nametags_id, editor_open_id
};

TouchScreenGUI *g_touchscreengui = nullptr;
bool TouchScreenGUI::m_active = true;

TouchScreenGUI::TouchScreenGUI(IrrlichtDevice *device):
		m_device(device), m_guienv(device->getGUIEnvironment())
{
	m_touchscreen_threshold = g_settings->getU16("touchscreen_threshold");
	m_touch_sensitivity = rangelim(g_settings->getFloat("touch_sensitivity"), 0.1, 1.0);
	m_screensize = m_device->getVideoDriver()->getScreenSize();
	m_button_size = RenderingEngine::getDisplayDensity() *
			g_settings->getFloat("hud_scaling") * 64.0f;

	std::string keyname_dig = g_settings->get("keymap_dig");
	m_keycode_dig = keyname_to_keycode(keyname_dig.c_str());
	std::string keyname_place = g_settings->get("keymap_place");
	m_keycode_place = keyname_to_keycode(keyname_place.c_str());
	m_dig_and_move = g_settings->getBool("dig_and_move");
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

	delete m_settings;
}

void TouchScreenGUI::init(ISimpleTextureSource *tsrc, bool simple_singleplayer_mode)
{
	assert(tsrc);
	m_texturesource = tsrc;

	initSettings();

	initJoystickButton();

	initButton(jump_id, getButtonRect(jump_id));
	initButton(drop_id, getButtonRect(drop_id));
	initButton(sneak_id, getButtonRect(sneak_id));
	initButton(inventory_id, getButtonRect(inventory_id));
	initButton(special1_id, getButtonRect(special1_id));
	initButton(escape_id, getButtonRect(escape_id));
	initButton(minimap_id, getButtonRect(minimap_id));
	initButton(camera_id, getButtonRect(camera_id));

	if (simple_singleplayer_mode) {
		initButton(chat_id, getButtonRect(chat_id), STATE_DEFAULT, "chat_btn.png");
	} else {
		initButton(chat_id, getButtonRect(chat_id), STATE_DEFAULT, "chat_mp_btn.png");
		initButton(tab_id, getButtonRect(tab_id));
	}

	initButton(overflow_id, getButtonRect(overflow_id));

	m_overflow_bg = m_guienv->addStaticText(L"", recti());
	m_overflow_bg->setBackgroundColor(video::SColor(140, 0, 0, 0));
	m_overflow_bg->setVisible(m_current_state == STATE_OVERFLOW);
	m_overflow_bg->grab();

	for (auto id : overflow_buttons_id) {
		initButton(id, recti(), STATE_OVERFLOW);
	}

	initButton(editor_close_id, getButtonRect(editor_close_id), STATE_EDITOR);
	initButton(editor_default_id, getButtonRect(editor_default_id), STATE_EDITOR);
	m_editor.button_move = initButton(editor_move_id,
			getButtonRect(editor_move_id), STATE_EDITOR);
	m_editor.button_scale = initButton(editor_scale_id,
			getButtonRect(editor_scale_id), STATE_EDITOR);

	m_editor.button_move->guibutton->setOverrideColor(video::SColor(255, 255, 0, 0));

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

button_info * TouchScreenGUI::initButton(touch_gui_button_id id, const rect<s32> &button_rect,
		touch_gui_state state, const char *custom_image)
{
	button_info *btn = new button_info();
	btn->state = state;
	btn->id = id;

	const char *image = strcmp(custom_image, "") == 0 ? buttons_data[id].image : custom_image;
	const wchar_t *str = wgettext(buttons_data[id].title);

	btn->guibutton = m_guienv->addButton(button_rect, nullptr);
	btn->guibutton->grab();
	btn->guibutton->setVisible(m_visible && state == STATE_DEFAULT);

	if (strcmp(image, "") != 0)
		loadButtonTexture(btn->guibutton, image, button_rect);
	else
		btn->guibutton->setText(str);

	btn->text = m_guienv->addStaticText(str, recti());
	btn->text->setTextAlignment(EGUIA_CENTER, EGUIA_UPPERLEFT);
	btn->text->setVisible(m_current_state == STATE_OVERFLOW);
	btn->text->grab();
	delete[] str;

	m_buttons.push_back(btn);

	return btn;
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

void TouchScreenGUI::updateButtons()
{
	m_button_size = RenderingEngine::getDisplayDensity() *
			g_settings->getFloat("hud_scaling") * 64.0f;

	for (auto button : m_buttons) {
		if (button->state == STATE_OVERFLOW)
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

	s32 cols = 3;
	s32 rows = (ARRLEN(overflow_buttons_id) + cols - 1) / cols;
	assert((s32)ARRLEN(overflow_buttons_id) <= cols * rows);
	v2s32 btn_size(m_button_size, m_button_size);
	v2s32 edge_offset(m_screensize.X * 0.1f, m_screensize.Y * 0.1f);
	v2s32 spacing((m_screensize.X - edge_offset.X * 2) / cols,
			(m_screensize.Y - edge_offset.Y * 2) / rows);
	v2s32 pos(edge_offset + spacing / 2);

	for (auto button : m_buttons) {
		if (button->state != STATE_OVERFLOW)
			continue;

		const wchar_t *str = wgettext(buttons_data[button->id].title);
		IGUIFont *font = button->text->getActiveFont();
		dimension2du text_size = font->getDimension(str) * 5 / 4;
		delete[] str;

		recti button_rect(pos - btn_size / 2, dimension2du(btn_size.X, btn_size.Y));
		button_rect -= v2s32(0, text_size.Height / 2);
		button->guibutton->setRelativePosition(button_rect);

		recti text_rect = recti(v2s32(pos.X - text_size.Width / 2,
				pos.Y + btn_size.Y / 2 - text_size.Height / 2), text_size);
		button->text->setRelativePosition(text_rect);

		pos.X += spacing.X;

		if (pos.X > (s32)m_screensize.X - edge_offset.X - btn_size.X / 2) {
			pos.X = edge_offset.X + spacing.X / 2;
			pos.Y += spacing.Y;
		}
	}
}

void TouchScreenGUI::initSettings()
{
	m_settings = Settings::getLayer(SL_TOUCHSCREENGUI);
	if (m_settings == nullptr)
		m_settings = Settings::createLayer(SL_TOUCHSCREENGUI);

	m_settings_path = porting::path_user + DIR_DELIM + "touchscreengui.conf";

	m_settings->readConfigFile(m_settings_path.c_str());

	setDefaultValues(jump_id,
			m_screensize.X - m_button_size * 3.375, m_screensize.Y - m_button_size * 2.75,
			m_button_size * 1.5, m_button_size * 1.5);

	setDefaultValues(drop_id,
			m_screensize.X - m_button_size, m_screensize.Y / 2 - m_button_size * 1.5,
			m_button_size, m_button_size);

	setDefaultValues(sneak_id,
			m_screensize.X - m_button_size * 3.375, m_screensize.Y - m_button_size * 1.125,
			m_button_size * 1.6875, m_button_size * 1.125);

	setDefaultValues(special1_id,
			m_screensize.X - m_button_size * 1.75, m_screensize.Y - m_button_size * 4,
			m_button_size * 1.5, m_button_size * 1.5);

	setDefaultValues(inventory_id,
			m_screensize.X - m_button_size * 1.6875, m_screensize.Y - m_button_size * 1.5,
			m_button_size * 1.6875, m_button_size * 1.5);

	setDefaultValues(escape_id,
			m_screensize.X / 2 - m_button_size * 2, 0,
			m_button_size, m_button_size);

	setDefaultValues(minimap_id,
			m_screensize.X / 2 - m_button_size, 0,
			m_button_size, m_button_size);

	setDefaultValues(camera_id,
			m_screensize.X / 2, 0,
			m_button_size, m_button_size);

	setDefaultValues(overflow_id,
			m_screensize.X / 2 + m_button_size, 0,
			m_button_size, m_button_size);

	setDefaultValues(chat_id,
			m_screensize.X - m_button_size * 1.25, 0,
			m_button_size * 1.25, m_button_size);

	setDefaultValues(tab_id,
			m_screensize.X - m_button_size * 1.25, 0,
			m_button_size * 1.25, m_button_size);

	setDefaultValues(joystick_off_id,
			m_button_size / 2, m_screensize.Y - m_button_size * 4.5,
			m_button_size * 4.0, m_button_size * 4.0);

	setDefaultValues(joystick_bg_id,
			m_button_size / 2, m_screensize.Y - m_button_size * 4.5,
			m_button_size * 4.0, m_button_size * 4.0);

	setDefaultValues(joystick_center_id,
			0, 0, m_button_size * 1.5, m_button_size * 1.5);

	setDefaultValues(editor_close_id,
			m_button_size, m_screensize.Y - m_button_size,
			m_button_size * 2, m_button_size);

	setDefaultValues(editor_default_id,
			m_button_size * 3, m_screensize.Y - m_button_size,
			m_button_size * 2, m_button_size);

	setDefaultValues(editor_move_id,
			m_button_size * 6, m_screensize.Y - m_button_size,
			m_button_size * 2, m_button_size);

	setDefaultValues(editor_scale_id,
			m_button_size * 8, m_screensize.Y - m_button_size,
			m_button_size * 2, m_button_size);
}

void TouchScreenGUI::setDefaultValues(touch_gui_button_id id, float x, float y, float w, float h)
{
	std::string name = std::string("tg_") + buttons_data[id].name;
	m_settings->setDefault(name + "_x", std::to_string(x / m_screensize.X));
	m_settings->setDefault(name + "_y", std::to_string(y / m_screensize.Y));
	m_settings->setDefault(name + "_w", std::to_string(w / m_button_size));
	m_settings->setDefault(name + "_h", std::to_string(h / m_button_size));
}

void TouchScreenGUI::setValues(touch_gui_button_id id, float x, float y, float w, float h)
{
	std::string name = std::string("tg_") + buttons_data[id].name;
	m_settings->setFloat(name + "_x", x / m_screensize.X);
	m_settings->setFloat(name + "_y", y / m_screensize.Y);
	m_settings->setFloat(name + "_w", w / m_button_size);
	m_settings->setFloat(name + "_h", h / m_button_size);
}

rect<s32> TouchScreenGUI::getButtonRect(touch_gui_button_id id)
{
	std::string name = std::string("tg_") + buttons_data[id].name;
	position2d<s32> pos = position2d<s32>(
			std::round(m_settings->getFloat(name + "_x") * m_screensize.X),
			std::round(m_settings->getFloat(name + "_y") * m_screensize.Y));
	dimension2d<s32> size = dimension2d<s32>(
			std::round(m_settings->getFloat(name + "_w") * m_button_size),
			std::round(m_settings->getFloat(name + "_h") * m_button_size));

	return rect<s32>(pos, size);
}

void TouchScreenGUI::resetAllValues()
{
	for (auto name : m_settings->getNames()) {
		m_settings->remove(name);
	}

	m_settings->updateConfigFile(m_settings_path.c_str());

	initSettings();
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
		bool reset_all_values = false;
		touch_gui_state new_state = m_current_state;

		for (auto button : m_buttons) {
			if (m_current_state != button->state)
				continue;

			if (button->guibutton->isPointInside(core::position2d<s32>(x, y))) {
				m_events[id] = true;
				button->pressed = true;
				button->event_id = id;

				if (button->id == editor_open_id) {
					new_state = STATE_EDITOR;
				} else if (button->id == editor_close_id) {
					new_state = STATE_DEFAULT;
				} else if (button->id == editor_default_id) {
					reset_all_values = true;
					new_state = STATE_DEFAULT;
				} else if (button->id == editor_move_id) {
					m_editor.change_size = false;
					m_editor.button_move->guibutton->
							setOverrideColor(video::SColor(255, 255, 0, 0));
					m_editor.button_scale->guibutton->enableOverrideColor(false);
				} else if (button->id == editor_scale_id) {
					m_editor.change_size = true;
					m_editor.button_scale->guibutton->
							setOverrideColor(video::SColor(255, 255, 0, 0));
					m_editor.button_move->guibutton->enableOverrideColor(false);
				} else if (button->id == overflow_id) {
					if (m_current_state == STATE_OVERFLOW)
						new_state = STATE_DEFAULT;
					else
						new_state = STATE_OVERFLOW;
				} else if (button->state == STATE_OVERFLOW) {
					m_overflow_close_schedule = true;
				}
			}
		}

		if (m_current_state == STATE_DEFAULT) {
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

		if (m_current_state == STATE_EDITOR && !m_events[id] && m_editor.event_id == -1) {
			for (auto button : m_buttons) {
				if (button->state != STATE_DEFAULT)
					continue;

				if (button->guibutton->isPointInside(core::position2d<s32>(x, y))) {
					m_events[id] = true;
					m_editor.guibutton = button->guibutton;
					m_editor.button_id = button->id;
					m_editor.event_id = id;
					m_editor.x = x;
					m_editor.y = y;
					m_editor.old_rect = button->guibutton->getRelativePosition();
				}
			}

			if (m_joystick.button_off->isPointInside(core::position2d<s32>(x, y))) {
				m_events[id] = true;
				m_editor.guibutton = m_joystick.button_off;
				m_editor.button_id = joystick_off_id;
				m_editor.event_id = id;
				m_editor.x = x;
				m_editor.y = y;
				m_editor.old_rect = m_joystick.button_off->getRelativePosition();
			}
		}

		if ((m_current_state == STATE_OVERFLOW) && !m_events[id])
			new_state = STATE_DEFAULT;

		if (reset_all_values)
			resetAllValues();

		if (m_current_state != new_state)
			changeCurrentState(new_state);

		result = true;

	} else if (event.TouchInput.Event == ETIE_LEFT_UP) {
		m_events.erase(id);

		for (auto button : m_buttons) {
			if (m_current_state != button->state)
				continue;

			if (button->event_id == id)
				button->reset();
		}

		if (m_joystick.event_id == id)
			m_joystick.reset(m_visible && m_current_state == STATE_DEFAULT);

		if (m_camera.event_id == id) {
			bool place = false;
			if (!m_camera.has_really_moved && !m_camera.dig) {
				u64 delta = porting::getDeltaMs(m_camera.downtime, porting::getTimeMs());
				place = (delta >= MIN_PLACE_TIME_MS);
			}
			m_camera.reset();
			m_camera.place = place;
		}

		if (m_editor.event_id == id) {
			IGUIButton *button = m_editor.guibutton;
			rect<s32> rect = button->getRelativePosition();
			setValues(m_editor.button_id,
					rect.UpperLeftCorner.X, rect.UpperLeftCorner.Y,
					rect.getWidth(), rect.getHeight());
			m_editor.reset();

			m_settings->updateConfigFile(m_settings_path.c_str());
		}

		result = true;

	} else if (event.TouchInput.Event == ETIE_MOVED) {
		if (m_events[id]) {
			if (m_joystick.event_id == id) {
				if (moveJoystick(x, y))
					result = true;
			} else if (m_camera.event_id == id) {
				updateCamera(x, y);
			} else if (m_editor.event_id == id) {
				if (m_editor.change_size) {
					IGUIButton *button = m_editor.guibutton;
					rect<s32> rect = m_editor.old_rect;
					s32 value = std::max(x - m_editor.x, y - m_editor.y);
					rect.LowerRightCorner += v2s32(value, value);
					s32 min_size = m_button_size * 0.75f;
					if (rect.getWidth() >= min_size && rect.getHeight() >= min_size)
						button->setRelativePosition(rect);
				} else {
					IGUIButton *button = m_editor.guibutton;
					rect<s32> rect = button->getRelativePosition();
					rect += v2s32(x - m_editor.x, y - m_editor.y);
					button->setRelativePosition(rect);
					m_editor.x = x;
					m_editor.y = y;
				}
			} else {
				bool overflow_btn_pressed = false;

				for (auto button : m_buttons) {
					if (m_current_state != button->state)
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

				if (overflow_btn_pressed) {
					if (m_current_state == STATE_DEFAULT)
						changeCurrentState(STATE_OVERFLOW);
					else
						changeCurrentState(STATE_DEFAULT);
				}
			}
		}

	} else if (event.TouchInput.Event == ETIE_COUNT) {
		result = true;
	}

	return result;
}

bool TouchScreenGUI::moveJoystick(s32 x, s32 y)
{
	rect<s32> joystick_rect = m_joystick.button_bg->getRelativePosition();
	s32 joystick_pos_x = joystick_rect.UpperLeftCorner.X;
	s32 joystick_pos_y = joystick_rect.UpperLeftCorner.Y;
	s32 joystick_size = joystick_rect.getWidth();
	rect<s32> joystick_center_rect = m_joystick.button_center->getRelativePosition();
	s32 joystick_center_size = joystick_center_rect.getWidth();

	s32 dx = x - joystick_pos_x - joystick_size / 2;
	s32 dy = y - joystick_pos_y - joystick_size / 2;
	double distance = sqrt(dx * dx + dy * dy);
	float max_distance = (float)joystick_size / 2.66f;

	if (distance == 0)
		return false;

	if (distance > max_distance) {
		s32 ndx = max_distance * dx / distance - joystick_center_size / 2.0f;
		s32 ndy = max_distance * dy / distance - joystick_center_size / 2.0f;
		m_joystick.button_center->setRelativePosition(v2s32(
				joystick_pos_x + joystick_size / 2 + ndx,
				joystick_pos_y + joystick_size / 2 + ndy));
	} else {
		m_joystick.button_center->setRelativePosition(v2s32(
				x - joystick_center_size / 2.0f,
				y - joystick_center_size / 2.0f));
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

	if ((m_dig_and_move || !m_camera.dig) &&
			((distance > m_touchscreen_threshold) || m_camera.has_really_moved)) {
		m_camera.has_really_moved = true;

		m_camera.yaw_change -= (x - m_camera.x) * m_touch_sensitivity;
		m_camera.pitch += (y - m_camera.y) * m_touch_sensitivity;
		m_camera.pitch = std::min(std::max((float) m_camera.pitch, -180.0f), 180.0f);

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

		if (!g_settings->exists("keymap_" + button_name))
			continue;

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
	v2u32 screensize = m_device->getVideoDriver()->getScreenSize();

	if (screensize != m_screensize) {
		m_screensize = screensize;
		initSettings();
		updateButtons();
	}

	if (m_current_state == STATE_OVERFLOW && m_overflow_close_schedule)
		changeCurrentState(STATE_DEFAULT);

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
		bool is_visible = (m_current_state == button->state);

		if (m_current_state == STATE_EDITOR && button->state == STATE_DEFAULT)
			is_visible = true;

		if (button->guibutton)
			button->guibutton->setVisible(m_visible && is_visible);
		if (button->text)
			button->text->setVisible(m_visible && is_visible);
	}

	if (m_joystick.button_off)
		m_joystick.button_off->setVisible(m_visible && (m_current_state != STATE_OVERFLOW));
	if (m_joystick.button_bg)
		m_joystick.button_bg->setVisible(false);
	if (m_joystick.button_center)
		m_joystick.button_center->setVisible(false);

	if (m_overflow_bg)
		m_overflow_bg->setVisible(m_visible && (m_current_state == STATE_OVERFLOW));

	if (!visible)
		reset();
}

void TouchScreenGUI::changeCurrentState(touch_gui_state state)
{
	reset();

	if (m_current_state == STATE_EDITOR)
		updateButtons();

	m_current_state = state;
	m_overflow_close_schedule = false;

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

	m_joystick.reset(m_visible && (m_current_state == STATE_DEFAULT));
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
