/*
Copyright (C) 2014 sapier
Copyright (C) 2018 srifqi, Muhammad Rifqi Priyo Susanto
		<muhammadrifqipriyosusanto@gmail.com>
Copyright (C) 2014-2025 Maksim Gamarnik [MoNTE48] Maksym48@pm.me
Copyright (C) 2023-2025 Dawid Gan <deveee@gmail.com>

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

#include "touchscreengui_mc.h"
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
	// image, image_pressed, image_float, title, name, has_sound, group
	{ "jump_btn.png", "", "", N_("Jump"), "jump", false, -1 },
	{ "drop_btn.png", "", "drop_btn_float.png", N_("Drop"), "drop", false, -1 },
	{ "sneak_btn.png", "", "sneak_btn_float.png", N_("Sneak"), "sneak", false, -1 },
	{ "special1_btn.png", "", "", N_("Special"), "special1", false, -1 },
	{ "inventory_btn.png", "", "inventory_btn_float.png", N_("Inventory"), "inventory", true, -1 },
	{ "escape_btn.png", "", "escape_btn_float.png", N_("Exit"), "escape", true, 1 },
	{ "minimap_btn.png", "", "minimap_btn_float.png", N_("Toggle minimap"), "minimap", true, 1 },
	{ "camera_btn.png", "", "camera_btn_float.png", N_("Change camera"), "camera_mode", true, 1 },
	{ "overflow_btn.png", "", "overflow_btn_float.png", N_("Overflow menu"), "overflow", true, 1 },
	{ "chat_btn.png", "", "chat_btn_float.png", N_("Chat"), "chat", true, 2 },
	{ "tab_btn.png", "", "tab_btn_float.png", N_("Tab"), "tabb", true, 2 },
	{ "fly_btn.png", "", "", N_("Toggle fly"), "freemove", true, -1 },
	{ "fast_btn.png", "", "", N_("Toggle fast"), "fastmove", true, -1 },
	{ "noclip_btn.png", "", "", N_("Toggle noclip"), "noclip", true, -1 },
	{ "rangeview_btn.png", "", "", N_("Range select"), "rangeselect", true, -1 },
	{ "chat_hide_btn.png", "", "", N_("Toggle chat log"), "toggle_chat", true, -1 },
	{ "names_hide_btn.png", "", "", N_("Toggle nametags"), "toggle_nametags", true, -1 },
	{ "joystick_off.png", "", "", "", "joystick", false, -1 },
	{ "joystick_bg.png", "", "", "", "joystick", false, -1 },
	{ "joystick_center.png", "", "", "", "joystick_center", false, -1 },
	{ "", "", "", N_("Open editor"), "editor_open", true, -1 },
	{ "edit_ui_save.png", "edit_ui_save_pressed.png", "", N_("Save"), "editor_save", true, -1 },
	{ "edit_ui_restore.png", "edit_ui_restore_pressed.png", "", N_("Restore"), "editor_default", true, -1 },
	{ "edit_ui_move.png", "edit_ui_move_pressed.png", "", N_("Move"), "editor_move", true, -1 },
	{ "edit_ui_scale.png", "edit_ui_scale_pressed.png", "", N_("Scale"), "editor_scale", true, -1 },
	{ "edit_ui_undo.png", "edit_ui_undo_pressed.png", "", N_("Undo"), "editor_undo", true, -1 },
	{ "edit_ui_redo.png", "edit_ui_redo_pressed.png", "", N_("Redo"), "editor_redo", true, -1 },
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
	m_button_size = RenderingEngine::getDisplayDensity() *
			g_settings->getFloat("hud_scaling") * 64.0f;

	std::string keyname_dig = g_settings->get("keymap_dig");
	m_keycode_dig = keyname_to_keycode(keyname_dig.c_str());
	std::string keyname_place = g_settings->get("keymap_place");
	m_keycode_place = keyname_to_keycode(keyname_place.c_str());
	m_dig_and_move = g_settings->getBool("dig_and_move");
	m_press_sound = g_settings->get("btn_press_sound");
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

void TouchScreenGUI::init(ISimpleTextureSource *tsrc, bool simple_singleplayer_mode,
		ISoundManager *sound_manager)
{
	assert(tsrc);
	m_texturesource = tsrc;
	m_sound_manager = sound_manager;

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

	m_editor.button_save = initButton(editor_save_id,
			getButtonRect(editor_save_id), STATE_EDITOR);
	m_editor.button_save->guibutton->setIsPushButton();
	m_editor.button_default = initButton(editor_default_id,
			getButtonRect(editor_default_id), STATE_EDITOR);
	m_editor.button_default->guibutton->setIsPushButton();
	m_editor.button_move = m_editor.button_move = initButton(editor_move_id,
			getButtonRect(editor_move_id), STATE_EDITOR);
	m_editor.button_move->guibutton->setIsPushButton();
	m_editor.button_scale = initButton(editor_scale_id,
			getButtonRect(editor_scale_id), STATE_EDITOR);
	m_editor.button_scale->guibutton->setIsPushButton();
	m_editor.button_undo = initButton(editor_undo_id,
			getButtonRect(editor_undo_id), STATE_EDITOR);
	m_editor.button_undo->guibutton->setIsPushButton();
	m_editor.button_redo = initButton(editor_redo_id,
			getButtonRect(editor_redo_id), STATE_EDITOR);
	m_editor.button_redo->guibutton->setIsPushButton();

	updateButtons();

	m_buttons_initialized = true;
}

void TouchScreenGUI::loadButtonTexture(button_info *button, IGUIButton *guibutton,
		std::string image, std::string image_pressed, const rect<s32> &button_rect)
{
	if (image.empty())
		return;

	std::string image_path = std::string("touch") + DIR_DELIM + image;
	video::ITexture *texture = guiScalingImageButton(m_device->getVideoDriver(),
			m_texturesource->getTexture(image_path.c_str(), nullptr),
			button_rect.getWidth(), button_rect.getHeight());
	video::ITexture *texture_pressed = texture;

	if (!image_pressed.empty()) {
		std::string pressed_path = std::string("touch") + DIR_DELIM + image_pressed;
		video::ITexture *tmp = guiScalingImageButton(m_device->getVideoDriver(),
				m_texturesource->getTexture(pressed_path.c_str(), nullptr),
				button_rect.getWidth(), button_rect.getHeight());
		if (tmp)
			texture_pressed = tmp;
	}

	if (texture) {
		if (button) {
			core::dimension2d<u32> texture_size = texture->getOriginalSize();
			button->aspect_ratio = (float)texture_size.Width / texture_size.Height;
		}

		guibutton->setUseAlphaChannel(true);
		if (g_settings->getBool("gui_scaling_filter")) {
			rect<s32> txr_rect = rect<s32>(v2s32(0, 0), button_rect.getSize());
			guibutton->setImage(texture, txr_rect);
			guibutton->setPressedImage(texture_pressed, txr_rect);
			guibutton->setScaleImage(false);
		} else {
			guibutton->setImage(texture);
			guibutton->setPressedImage(texture_pressed);
			guibutton->setScaleImage(true);
		}
		guibutton->setDrawBorder(false);
		guibutton->setText(L"");
	}
}

button_info *TouchScreenGUI::initButton(touch_gui_button_id id,
		const rect<s32> &button_rect, touch_gui_state state,
		std::string custom_image)
{
	std::string image = buttons_data[id].image;
	std::string image_pressed = buttons_data[id].image_pressed;
	std::string image_float = buttons_data[id].image_float;

	button_info *btn = new button_info();
	btn->state = state;
	btn->id = id;
	btn->image = image;

	if (!custom_image.empty()) {
		image = custom_image;
		image_pressed = custom_image;
		btn->image = image;
	}

	if (!image_float.empty() && state == STATE_DEFAULT &&
			button_rect != getDefaultButtonRect(id)) {
		image = image_float;
		image_pressed = image_float;
		btn->floating = true;
	}

	const wchar_t *str = wgettext(buttons_data[id].title);

	btn->guibutton = m_guienv->addButton(button_rect, nullptr);
	btn->guibutton->grab();
	btn->guibutton->setVisible(m_visible && state == STATE_DEFAULT);

	if (!image.empty())
		loadButtonTexture(btn, btn->guibutton, image, image_pressed, button_rect);
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
	loadButtonTexture(nullptr, m_joystick.button_off,
			buttons_data[joystick_off_id].image, "", button_off_rect);

	const rect<s32> &button_bg_rect = getButtonRect(joystick_bg_id);
	m_joystick.button_bg = m_guienv->addButton(button_bg_rect, nullptr);
	m_joystick.button_bg->setVisible(false);
	m_joystick.button_bg->grab();
	loadButtonTexture(nullptr, m_joystick.button_bg,
			buttons_data[joystick_bg_id].image, "", button_bg_rect);

	const rect<s32> &button_center_rect = getButtonRect(joystick_center_id);
	m_joystick.button_center = m_guienv->addButton(button_center_rect, nullptr);
	m_joystick.button_center->setVisible(false);
	m_joystick.button_center->grab();
	loadButtonTexture(nullptr, m_joystick.button_center,
			buttons_data[joystick_center_id].image, "", button_center_rect);
}

void TouchScreenGUI::updateButtons()
{
	m_button_size = RenderingEngine::getDisplayDensity() *
			g_settings->getFloat("hud_scaling") * 64.0f;

	for (auto button : m_buttons) {
		if (button->state == STATE_OVERFLOW)
			continue;

		if (button->guibutton) {
			rect<s32> current_rect = getButtonRect(button->id);
			updateButtonTexture(button, current_rect);

			if (button->aspect_ratio > 0) {
				if (button->floating) {
					s32 height = std::round((float)current_rect.getWidth() / button->aspect_ratio);
					current_rect.LowerRightCorner.Y = current_rect.UpperLeftCorner.Y + height;
				} else {
					rect<s32> default_rect = getDefaultButtonRect(button->id);
					float default_aspect_ratio = (float)default_rect.getWidth() / default_rect.getHeight();
					s32 height = std::round((float)current_rect.getWidth() / default_aspect_ratio);
					current_rect.LowerRightCorner.Y = current_rect.UpperLeftCorner.Y + height;
				}
			}

			button->guibutton->setRelativePosition(current_rect);
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

	updateEditorButtonsState();
	rebuildOverflowMenu();
}

void TouchScreenGUI::updateButtonTexture(button_info *button,
		rect<s32> current_rect, bool should_float)
{
	if (!should_float) {
		rect<s32> default_rect = getDefaultButtonRect(button->id);

		if (current_rect.UpperLeftCorner.X != default_rect.UpperLeftCorner.X ||
				current_rect.UpperLeftCorner.Y != default_rect.UpperLeftCorner.Y ||
				current_rect.getWidth() != default_rect.getWidth())
			should_float = true;
	}

	int group = buttons_data[button->id].group;

	if (!should_float && group > -1) {
		for (auto button : m_buttons) {
			if (buttons_data[button->id].group != group)
				continue;

			rect<s32> default_rect = getDefaultButtonRect(button->id);
			rect<s32> current_rect = getButtonRect(button->id);

			if (current_rect.UpperLeftCorner.X != default_rect.UpperLeftCorner.X ||
					current_rect.UpperLeftCorner.Y != default_rect.UpperLeftCorner.Y ||
					current_rect.getWidth() != default_rect.getWidth()) {
				should_float = true;
				break;
			}
		}
	}

	if (should_float && !button->floating) {
		loadButtonTexture(button, button->guibutton,
				buttons_data[button->id].image_float, "",
				getButtonRect(button->id));
		button->floating = true;

		if (button->aspect_ratio > 0) {
			rect<s32> rect = getButtonRect(button->id);
			s32 height = std::round((float)rect.getWidth() / button->aspect_ratio);
			rect.LowerRightCorner.Y = rect.UpperLeftCorner.Y + height;
			button->guibutton->setRelativePosition(rect);
		}
	} else if (!should_float && button->floating) {
		loadButtonTexture(button, button->guibutton, button->image, "",
				getButtonRect(button->id));
		button->floating = false;

		if (button->aspect_ratio > 0) {
			rect<s32> current_rect = getButtonRect(button->id);
			rect<s32> default_rect = getDefaultButtonRect(button->id);
			float default_aspect_ratio = (float)default_rect.getWidth() / default_rect.getHeight();
			s32 height = std::round((float)current_rect.getWidth() / default_aspect_ratio);
			current_rect.LowerRightCorner.Y = current_rect.UpperLeftCorner.Y + height;
			button->guibutton->setRelativePosition(current_rect);
		}
	}
}

void TouchScreenGUI::updateEditorButtonsState()
{
	if (m_editor.button_move) {
		if (m_editor.change_size) {
			m_editor.button_move->guibutton->enableOverrideColor(false);
			m_editor.button_move->guibutton->setPressed(false);
		} else {
			m_editor.button_move->guibutton->
					setOverrideColor(video::SColor(255, 255, 0, 0));
			m_editor.button_move->guibutton->setPressed(true);
		}
	}

	if (m_editor.button_scale) {
		if (m_editor.change_size) {
			m_editor.button_scale->guibutton->
					setOverrideColor(video::SColor(255, 255, 0, 0));
			m_editor.button_scale->guibutton->setPressed(true);
		} else {
			m_editor.button_scale->guibutton->enableOverrideColor(false);
			m_editor.button_scale->guibutton->setPressed(false);
		}
	}

	if (m_editor.button_undo) {
		if (m_editor.history_current_id > 0) {
			m_editor.button_undo->guibutton->setPressed(true);
			m_editor.button_undo->inactive = false;
		} else {
			m_editor.button_undo->guibutton->setPressed(false);
			m_editor.button_undo->inactive = true;
		}
	}

	if (m_editor.button_redo) {
		if (m_editor.history_current_id > -1 &&
				m_editor.history_current_id < m_editor.history_data.size()) {
			m_editor.button_redo->guibutton->setPressed(true);
			m_editor.button_redo->inactive = false;
		} else {
			m_editor.button_redo->guibutton->setPressed(false);
			m_editor.button_redo->inactive = true;
		}
	}
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
			m_screensize.X - m_button_size * 1.25, m_button_size,
			m_button_size * 1.25, m_button_size);

	setDefaultValues(joystick_off_id,
			m_button_size / 2, m_screensize.Y - m_button_size * 4.5,
			m_button_size * 4.0, m_button_size * 4.0);

	setDefaultValues(joystick_bg_id,
			m_button_size / 2, m_screensize.Y - m_button_size * 4.5,
			m_button_size * 4.0, m_button_size * 4.0);

	setDefaultValues(joystick_center_id,
			0, 0, m_button_size * 1.5, m_button_size * 1.5);

	setDefaultValues(editor_save_id,
			m_screensize.X / 2 - m_button_size * 5.0, m_screensize.Y - m_button_size * 0.75,
			m_button_size * 1.5, m_button_size * 0.75);

	setDefaultValues(editor_default_id,
			m_screensize.X / 2 - m_button_size * 3.5, m_screensize.Y - m_button_size * 0.75,
			m_button_size * 1.5, m_button_size * 0.75);

	setDefaultValues(editor_move_id,
			m_screensize.X / 2 - m_button_size * 1.5, m_screensize.Y - m_button_size * 0.75,
			m_button_size * 1.5, m_button_size * 0.75);

	setDefaultValues(editor_scale_id,
			m_screensize.X / 2, m_screensize.Y - m_button_size * 0.75,
			m_button_size * 1.5, m_button_size * 0.75);

	setDefaultValues(editor_undo_id,
			m_screensize.X / 2 + m_button_size * 2.0, m_screensize.Y - m_button_size * 0.75,
			m_button_size * 1.5, m_button_size * 0.75);

	setDefaultValues(editor_redo_id,
			m_screensize.X / 2 + m_button_size * 3.5, m_screensize.Y - m_button_size * 0.75,
			m_button_size * 1.5, m_button_size * 0.75);
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

rect<s32> TouchScreenGUI::getDefaultButtonRect(touch_gui_button_id id)
{
	Settings *settings = Settings::getLayer(SL_DEFAULTS);
	std::string name = std::string("tg_") + buttons_data[id].name;
	position2d<s32> pos = position2d<s32>(
			std::round(settings->getFloat(name + "_x") * m_screensize.X),
			std::round(settings->getFloat(name + "_y") * m_screensize.Y));
	dimension2d<s32> size = dimension2d<s32>(
			std::round(settings->getFloat(name + "_w") * m_button_size),
			std::round(settings->getFloat(name + "_h") * m_button_size));

	return rect<s32>(pos, size);
}

void TouchScreenGUI::resetAllValues()
{
	std::vector<editor_history_data> history_data;

	for (auto button : m_buttons) {
		if (button->state != STATE_DEFAULT)
			continue;

		rect<s32> default_rect = getDefaultButtonRect(button->id);
		rect<s32> current_rect = button->guibutton->getRelativePosition();

		if (current_rect != default_rect) {
			button->guibutton->setRelativePosition(default_rect);
			setValues(button->id,
					default_rect.UpperLeftCorner.X, default_rect.UpperLeftCorner.Y,
					default_rect.getWidth(), default_rect.getHeight());

			editor_history_data data = {button->guibutton, button->id, current_rect, default_rect};
			history_data.push_back(data);
		}
	}

	rect<s32> default_rect = getDefaultButtonRect(joystick_off_id);
	rect<s32> current_rect = m_joystick.button_off->getRelativePosition();

	if (current_rect != default_rect) {
		m_joystick.button_off->setRelativePosition(default_rect);
		setValues(joystick_off_id,
				default_rect.UpperLeftCorner.X, default_rect.UpperLeftCorner.Y,
				default_rect.getWidth(), default_rect.getHeight());

		editor_history_data data = {m_joystick.button_off, joystick_off_id, current_rect, default_rect};
		history_data.push_back(data);
	}

	if (history_data.size() > 0) {
		if (m_editor.history_current_id > -1)
			m_editor.history_data.resize(m_editor.history_current_id);

		m_editor.history_data.push_back(history_data);
		m_editor.history_current_id = m_editor.history_data.size();

		updateButtons();
	}
}

void TouchScreenGUI::restoreAllValues()
{
	for (auto name : m_settings->getNames()) {
		m_settings->remove(name);
	}

	initSettings();
}

bool TouchScreenGUI::preprocessEvent(const SEvent &event)
{
	if (!m_buttons_initialized || !m_visible || m_close)
		return false;

	if (event.EventType != EET_TOUCH_INPUT_EVENT)
		return false;

	bool result = false;
	s32 id = (unsigned int)event.TouchInput.ID;
	s32 x = event.TouchInput.X;
	s32 y = event.TouchInput.Y;

	if (event.TouchInput.Event == ETIE_PRESSED_DOWN) {
		m_events[id] = false;
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
				} else if (button->id == overflow_id) {
					if (m_current_state == STATE_OVERFLOW)
						new_state = STATE_DEFAULT;
					else
						new_state = STATE_OVERFLOW;
				} else if (button->state == STATE_OVERFLOW) {
					m_overflow_close_schedule = true;
				}

				if (button->state != STATE_EDITOR && buttons_data[button->id].has_sound)
					playSound();

				if (button->state == STATE_EDITOR && button->guibutton->isPushButton() &&
						!button->inactive) {
					button->guibutton->setPressed(true);
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
				if (m_camera.event_id == -1) {
					m_events[id] = true;
					m_camera.downtime = porting::getTimeMs();
					m_camera.x = x;
					m_camera.y = y;
					m_camera.event_id = id;

					updateCamera(m_camera, x, y);
				} else if (m_dig_and_move && m_camera_additional.event_id == -1) {
					m_events[id] = true;
					m_camera_additional.downtime = porting::getTimeMs();
					m_camera_additional.x = x;
					m_camera_additional.y = y;
					m_camera_additional.event_id = id;

					updateCamera(m_camera_additional, x, y);
				}
			}
		}

		if (m_current_state == STATE_EDITOR && !m_events[id] && m_editor.event_id == -1) {
			for (auto button : m_buttons) {
				if (button->state != STATE_DEFAULT)
					continue;

				if (button->guibutton->isPointInside(core::position2d<s32>(x, y))) {
					m_events[id] = true;
					m_editor.button = button;
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
				m_editor.button = nullptr;
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

		if (m_current_state != new_state)
			changeCurrentState(new_state);

		result = true;

	} else if (event.TouchInput.Event == ETIE_LEFT_UP) {
		m_events.erase(id);

		touch_gui_state new_state = m_current_state;
		bool reset_all_values = false;
		bool restore_all_values = false;

		for (auto button : m_buttons) {
			if (m_current_state != button->state)
				continue;

			if (button->event_id == id) {
				if (button->guibutton->isPointInside(core::position2d<s32>(x, y))) {
					bool play_sound = false;

					if (button->id == editor_save_id) {
						play_sound = true;
						m_settings->updateConfigFile(m_settings_path.c_str());
						new_state = STATE_DEFAULT;
					} else if (button->id == editor_default_id) {
						play_sound = true;
						reset_all_values = true;
					} else if (button->id == editor_move_id) {
						if (m_editor.change_size) {
							play_sound = true;
							m_editor.change_size = false;
							updateEditorButtonsState();
						}
					} else if (button->id == editor_scale_id) {
						if (!m_editor.change_size) {
							play_sound = true;
							m_editor.change_size = true;
							updateEditorButtonsState();
						}
					} else if (button->id == editor_undo_id) {
						if (m_editor.history_current_id > 0) {
							play_sound = true;
							m_editor.history_current_id--;

							for (auto data : m_editor.history_data[m_editor.history_current_id]) {
								rect<s32> rect = data.old_rect;
								data.guibutton->setRelativePosition(rect);
								setValues(data.button_id,
										rect.UpperLeftCorner.X, rect.UpperLeftCorner.Y,
										rect.getWidth(), rect.getHeight());
							}

							updateButtons();
						}
					} else if (button->id == editor_redo_id) {
						if (m_editor.history_current_id >= 0 && m_editor.history_current_id < m_editor.history_data.size()) {
							for (auto data : m_editor.history_data[m_editor.history_current_id]) {
								rect<s32> rect = data.new_rect;
								data.guibutton->setRelativePosition(rect);
								setValues(data.button_id,
										rect.UpperLeftCorner.X, rect.UpperLeftCorner.Y,
										rect.getWidth(), rect.getHeight());
							}

							play_sound = true;
							m_editor.history_current_id++;

							updateButtons();
						}
					}

					if (button->state == STATE_EDITOR && play_sound &&
							buttons_data[button->id].has_sound)
						playSound();
				}

				if (button->state == STATE_EDITOR && button->guibutton->isPushButton()) {
					button->guibutton->setPressed(false);
					updateEditorButtonsState();
				}

				button->reset();
			}
		}

		if (m_joystick.event_id == id)
			m_joystick.reset(m_visible && m_current_state == STATE_DEFAULT);

		if (m_camera.event_id == id && m_camera_additional.event_id != -1) {
			m_camera.reset();
			m_camera.event_id = m_camera_additional.event_id;
			m_camera.x = m_camera_additional.x;
			m_camera.y = m_camera_additional.y;
			m_camera.has_really_moved = true;
			m_camera_additional.reset();
			updateCamera(m_camera, m_camera.x, m_camera.y);
		} else if (m_camera.event_id == id) {
			bool place = false;
			if (!m_camera.has_really_moved && !m_camera.dig) {
				u64 delta = porting::getDeltaMs(m_camera.downtime, porting::getTimeMs());
				place = (delta >= MIN_PLACE_TIME_MS);
			}
			m_camera.reset();
			m_camera.place = place;
		} else if (m_dig_and_move && m_camera_additional.event_id == id) {
			bool place = false;
			if (!m_camera_additional.has_really_moved && !m_camera_additional.dig) {
				u64 delta = porting::getDeltaMs(m_camera_additional.downtime, porting::getTimeMs());
				place = (delta >= MIN_PLACE_TIME_MS);
			}
			m_camera_additional.reset();
			m_camera_additional.place = place;
			m_camera_additional.place_shootline = place;
		}

		if (m_editor.event_id == id) {
			IGUIButton *guibutton = m_editor.guibutton;
			rect<s32> rect = guibutton->getRelativePosition();

			if (m_editor.change_size) {
				if (rect.UpperLeftCorner.X < 0)
					rect += v2s32(-rect.UpperLeftCorner.X, 0);
				else if (rect.LowerRightCorner.X > m_screensize.X)
					rect -= v2s32(rect.LowerRightCorner.X - m_screensize.X, 0);

				if (rect.UpperLeftCorner.Y < 0)
					rect += v2s32(0, -rect.UpperLeftCorner.Y);
				else if (rect.LowerRightCorner.Y > m_screensize.Y)
					rect -= v2s32(0, rect.LowerRightCorner.Y - m_screensize.Y);
			}

			setValues(m_editor.button_id,
					rect.UpperLeftCorner.X, rect.UpperLeftCorner.Y,
					rect.getWidth(), rect.getHeight());

			if (m_editor.history_current_id > -1)
				m_editor.history_data.resize(m_editor.history_current_id);

			if (rect != m_editor.old_rect) {
				editor_history_data data = {guibutton, m_editor.button_id, m_editor.old_rect, rect};
				std::vector<editor_history_data> history_data;
				history_data.push_back(data);
				m_editor.history_data.push_back(history_data);
				m_editor.history_current_id = m_editor.history_data.size();
			}
			m_editor.reset();

			updateButtons();
		}

		if (reset_all_values)
			resetAllValues();

		if (restore_all_values)
			restoreAllValues();

		if (m_current_state != new_state)
			changeCurrentState(new_state);

		result = true;

	} else if (event.TouchInput.Event == ETIE_MOVED) {
		if (m_events[id]) {
			if (m_joystick.event_id == id) {
				if (moveJoystick(x, y))
					result = true;
			} else if (m_camera.event_id == id) {
				updateCamera(m_camera, x, y);
			} else if (m_editor.event_id == id) {
				if (m_editor.change_size) {
					IGUIButton *guibutton = m_editor.guibutton;
					rect<s32> rect = m_editor.old_rect;
					s32 value = std::max(x - m_editor.x, y - m_editor.y);
					rect.LowerRightCorner += v2s32(value, value);
					s32 min_size = m_button_size * 0.75f;
					s32 max_size = std::min(m_button_size * 6.0f, m_screensize.Y * 0.75f);

					if (rect.getWidth() >= min_size && rect.getHeight() >= min_size &&
							rect.getWidth() <= max_size && rect.getHeight() <= max_size) {
						if (m_editor.button && !m_editor.button->floating) {
							updateButtonTexture(m_editor.button, rect, true);

							if (buttons_data[m_editor.button_id].group > -1) {
								for (auto button : m_buttons) {
									if (buttons_data[button->id].group == buttons_data[m_editor.button_id].group &&
											!button->floating) {
										updateButtonTexture(button, getButtonRect(button->id), true);
									}
								}
							}
						}

						if (m_editor.button && m_editor.button->floating && m_editor.button->aspect_ratio > 0) {
							s32 height = std::round((float)rect.getWidth() / m_editor.button->aspect_ratio);
							rect.LowerRightCorner.Y = rect.UpperLeftCorner.Y + height;
						}

						guibutton->setRelativePosition(rect);
					}
				} else {
					IGUIButton *guibutton = m_editor.guibutton;
					rect<s32> rect = guibutton->getRelativePosition();
					rect += v2s32(x - m_editor.x, y - m_editor.y);

					if (m_editor.button && !m_editor.button->floating) {
						updateButtonTexture(m_editor.button, rect, true);

						if (buttons_data[m_editor.button_id].group > -1) {
							for (auto button : m_buttons) {
								if (buttons_data[button->id].group == buttons_data[m_editor.button_id].group &&
										!button->floating) {
									updateButtonTexture(button, getButtonRect(button->id), true);
								}
							}
						}

						if (m_editor.button->aspect_ratio > 0) {
							s32 height = std::round((float)rect.getWidth() / m_editor.button->aspect_ratio);
							rect.LowerRightCorner.Y = rect.UpperLeftCorner.Y + height;
						}
					}

					if (rect.UpperLeftCorner.X < 0)
						rect += v2s32(-rect.UpperLeftCorner.X, 0);
					else if (rect.LowerRightCorner.X > m_screensize.X)
						rect -= v2s32(rect.LowerRightCorner.X - m_screensize.X, 0);

					if (rect.UpperLeftCorner.Y < 0)
						rect += v2s32(0, -rect.UpperLeftCorner.Y);
					else if (rect.LowerRightCorner.Y > m_screensize.Y)
						rect -= v2s32(0, rect.LowerRightCorner.Y - m_screensize.Y);

					guibutton->setRelativePosition(rect);
					m_editor.x = x;
					m_editor.y = y;
				}
			} else if (m_dig_and_move && m_camera_additional.event_id == id) {
				updateCamera(m_camera_additional, x, y);
			} else {
				bool overflow_btn_pressed = false;

				for (auto button : m_buttons) {
					if (m_current_state != button->state)
						continue;

					if (button->guibutton->isPointInside(core::position2d<s32>(x, y))) {
						if (button->id == overflow_id)
							overflow_btn_pressed = true;

						if (button->state == STATE_DEFAULT) {
							button->pressed = true;
							button->event_id = id;
							result = true;
						}
					} else if (button->event_id == id) {
						if (button->state == STATE_DEFAULT) {
							button->reset();
							result = true;
						}
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

void TouchScreenGUI::updateCamera(camera_info &camera, s32 x, s32 y)
{
	double distance = sqrt(
			(camera.x - x) * (camera.x - x) +
			(camera.y - y) * (camera.y - y));

	if ((m_dig_and_move || !camera.dig) &&
			((distance > m_touchscreen_threshold) || camera.has_really_moved)) {
		camera.has_really_moved = true;

		camera.yaw_change -= (x - camera.x) * m_touch_sensitivity;
		camera.pitch += (y - camera.y) * m_touch_sensitivity;
		camera.pitch = std::min(std::max((float) camera.pitch, -180.0f), 180.0f);

		camera.x = x;
		camera.y = y;
	}

	camera.shootline = m_device->getSceneManager()
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

	if (m_camera_additional.dig) {
		if (m_keycode_dig == keycode)
			return true;
	}

	if (m_camera_additional.place) {
		if (m_keycode_place == keycode) {
			m_camera_additional.place = false;
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
	if (!m_buttons_initialized || m_close)
		return;

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

	if (m_camera_additional.event_id != -1 && (!m_camera_additional.has_really_moved)) {
		u64 delta = porting::getDeltaMs(m_camera_additional.downtime, porting::getTimeMs());
		if (delta > MIN_DIG_TIME_MS) {
			m_camera_additional.dig = true;
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

	if (!m_buttons_initialized || m_close)
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

void TouchScreenGUI::openEditor()
{
	if (!m_buttons_initialized || m_close)
		return;

	m_editor.reset();
	updateEditorButtonsState();

	changeCurrentState(STATE_EDITOR);
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
	m_camera_additional.reset();
	m_editor.reset();
}

void TouchScreenGUI::wakeUpInputhandler()
{
	SEvent event;
	memset(&event, 0, sizeof(SEvent));
	event.EventType = EET_TOUCH_INPUT_EVENT;
	event.TouchInput.Event = ETIE_COUNT;

	m_device->postEventFromUser(event);
}

void TouchScreenGUI::playSound()
{
	if (m_sound_manager && !m_press_sound.empty())
		m_sound_manager->playSound(m_press_sound, false, 1.0f);
}
