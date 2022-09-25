/*
 Minetest
 Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
 Copyright (C) 2013 Ciaran Gultnieks <ciaran@ciarang.com>
 Copyright (C) 2013 teddydestodes <derkomtur@schattengang.net>

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

#include "guiKeyChangeMenu.h"
#include "debug.h"
#include "guiBackgroundImage.h"
#include "guiButton.h"
#include "serialization.h"
#include <string>
#include <IGUICheckBox.h>
#include <IGUIEditBox.h>
#include <IGUIButton.h>
#include <IGUIStaticText.h>
#include <IGUIFont.h>
#include "filesys.h"
#include "porting.h"
#include "settings.h"
#include "StyleSpec.h"
#include <algorithm>

#include "mainmenumanager.h"  // for g_gamecallback
#include "client/renderingengine.h"

#define KMaxButtonPerColumns 12

extern MainGameCallback *g_gamecallback;

enum
{
	GUI_ID_BACK_BUTTON = 101, GUI_ID_ABORT_BUTTON, GUI_ID_SCROLL_BAR,
	GUI_ID_BACKGROUND_IMG,
	// buttons
	GUI_ID_KEY_FORWARD_BUTTON,
	GUI_ID_KEY_BACKWARD_BUTTON,
	GUI_ID_KEY_LEFT_BUTTON,
	GUI_ID_KEY_RIGHT_BUTTON,
	GUI_ID_KEY_USE_BUTTON,
	GUI_ID_KEY_FLY_BUTTON,
	GUI_ID_KEY_FAST_BUTTON,
	GUI_ID_KEY_JUMP_BUTTON,
	GUI_ID_KEY_NOCLIP_BUTTON,
	GUI_ID_KEY_PITCH_MOVE,
	GUI_ID_KEY_CHAT_BUTTON,
	GUI_ID_KEY_CMD_BUTTON,
	GUI_ID_KEY_CMD_LOCAL_BUTTON,
	GUI_ID_KEY_CONSOLE_BUTTON,
	GUI_ID_KEY_SNEAK_BUTTON,
	GUI_ID_KEY_DROP_BUTTON,
	GUI_ID_KEY_INVENTORY_BUTTON,
	GUI_ID_KEY_HOTBAR_PREV_BUTTON,
	GUI_ID_KEY_HOTBAR_NEXT_BUTTON,
	GUI_ID_KEY_MUTE_BUTTON,
	GUI_ID_KEY_DEC_VOLUME_BUTTON,
	GUI_ID_KEY_INC_VOLUME_BUTTON,
	GUI_ID_KEY_RANGE_BUTTON,
	GUI_ID_KEY_ZOOM_BUTTON,
	GUI_ID_KEY_CAMERA_BUTTON,
	GUI_ID_KEY_MINIMAP_BUTTON,
	GUI_ID_KEY_SCREENSHOT_BUTTON,
	GUI_ID_KEY_CHATLOG_BUTTON,
	GUI_ID_KEY_HUD_BUTTON,
	GUI_ID_KEY_FOG_BUTTON,
	GUI_ID_KEY_DEC_RANGE_BUTTON,
	GUI_ID_KEY_INC_RANGE_BUTTON,
	GUI_ID_KEY_AUTOFWD_BUTTON,
	// other
	GUI_ID_CB_AUX1_DESCENDS,
	GUI_ID_CB_DOUBLETAP_JUMP,
	GUI_ID_CB_AUTOJUMP,
};

GUIKeyChangeMenu::GUIKeyChangeMenu(gui::IGUIEnvironment* env,
		gui::IGUIElement* parent, s32 id, IMenuManager *menumgr,
		ISimpleTextureSource *tsrc, bool main_menu) :
		GUIModalMenu(env, parent, id, menumgr),
		m_tsrc(tsrc),
		m_main_menu(main_menu)
{
	init_keys();

	if (m_main_menu) return;
	v3f formspec_bgcolor = g_settings->getV3F("formspec_fullscreen_bg_color");
	m_fullscreen_bgcolor = video::SColor(
		(u8) MYMIN(MYMAX(g_settings->getS32("formspec_fullscreen_bg_opacity"), 0), 255),
		MYMIN(MYMAX(myround(formspec_bgcolor.X), 0), 255),
		MYMIN(MYMAX(myround(formspec_bgcolor.Y), 0), 255),
		MYMIN(MYMAX(myround(formspec_bgcolor.Z), 0), 255)
	);
}

GUIKeyChangeMenu::~GUIKeyChangeMenu()
{
	removeChildren();

	for (key_setting *ks : key_settings) {
		delete[] ks->button_name;
		delete ks;
	}
	key_settings.clear();
}

void GUIKeyChangeMenu::removeChildren()
{
	const core::list<gui::IGUIElement*> &children = getChildren();
	core::list<gui::IGUIElement*> children_copy;
	for (gui::IGUIElement*i : children) {
		children_copy.push_back(i);
	}

	for (gui::IGUIElement *i : children_copy) {
		i->remove();
	}
	key_used_text = nullptr;
}

void GUIKeyChangeMenu::regenerateGui(v2u32 screensize)
{
	removeChildren();

	float s = MYMIN(screensize.X / 835.f, screensize.Y / 430.f);
#if HAVE_TOUCHSCREENGUI
	s *= g_settings->getBool("device_is_tablet") ? 0.8f : 0.9f;
#else
	s *= 0.75f;
#endif

	// Make sure the GUI will fit on the screen
	// The change keys GUI is 835x430 pixels (with a scaling of 1)
	if (835 * s > screensize.X)
		s = screensize.X / 835.f;
	if (430 * s > screensize.Y)
		s = screensize.Y / 430.f;

	DesiredRect = core::rect<s32>(
		screensize.X / 2 - 835 * s / 2,
		screensize.Y / 2 - 430 * s / 2,
		screensize.X / 2 + 835 * s / 2,
		screensize.Y / 2 + 430 * s / 2
	);
	recalculateAbsolutePosition(false);

	v2s32 size = DesiredRect.getSize();
	v2s32 topleft(0, 0);

	std::string texture_path = "";
	if (m_main_menu)
		texture_path = porting::path_share + DIR_DELIM "textures" DIR_DELIM
			"base" DIR_DELIM "pack" DIR_DELIM;

	// Background image
	{
		const std::string texture = texture_path + "bg_common.png";
		const core::rect<s32> rect(0, 0, 0, 0);
		const core::rect<s32> middle(40, 40, -40, -40);
		new GUIBackgroundImage(Environment, this, GUI_ID_BACKGROUND_IMG, rect,
				texture, middle, m_tsrc, true);
	}

	{
		core::rect<s32> rect(0, 0, 795 * s, 50 * s);
		rect += topleft + v2s32(25 * s, 10 * s);
		//gui::IGUIStaticText *t =
#if !defined(__ANDROID__) && !defined(__IOS__)
		const wchar_t *text = wgettext("Keybindings. (If this menu screws up, remove stuff from multicraft.conf)");
#else
		const wchar_t *text = wgettext("Change Keys");
#endif
		Environment->addStaticText(text,
								   rect, false, true, this, -1);
		delete[] text;
		//t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
	}

	// Build buttons

	v2s32 offset(25 * s, 60 * s);

	for(size_t i = 0; i < key_settings.size(); i++)
	{
		key_setting *k = key_settings.at(i);
		{
			core::rect<s32> rect(0, 0, 150 * s, 20 * s);
			rect += topleft + v2s32(offset.X, offset.Y);
			Environment->addStaticText(k->button_name, rect, false, true, this, -1);
		}

		{
			core::rect<s32> rect(0, 0, 100 * s, 30 * s);
			rect += topleft + v2s32(offset.X + 150 * s, offset.Y - 5 * s);
			const wchar_t *text = wgettext(k->key.name());
			k->button = GUIButton::addButton(Environment, rect, m_tsrc, this, k->id, text);
			delete[] text;
		}
		if ((i + 1) % KMaxButtonPerColumns == 0) {
			offset.X += 260 * s;
			offset.Y = 60 * s;
		} else {
			offset += v2s32(0, 25 * s);
		}
	}

	{
		s32 option_x = offset.X;
		s32 option_y = offset.Y + 5 * s;
		u32 option_w = 300 * s;
		{
			core::rect<s32> rect(0, 0, option_w, 30 * s);
			rect += topleft + v2s32(option_x, option_y);
			const wchar_t *text = wgettext("\"Special\" = climb down");
			Environment->addCheckBox(g_settings->getBool("aux1_descends"), rect, this,
					GUI_ID_CB_AUX1_DESCENDS, text);
			delete[] text;
		}
		offset += v2s32(0, 25 * s);
	}

	{
		s32 option_x = offset.X;
		s32 option_y = offset.Y + 5 * s;
		u32 option_w = 300 * s;
		{
			core::rect<s32> rect(0, 0, option_w, 30 * s);
			rect += topleft + v2s32(option_x, option_y);
			const wchar_t *text = wgettext("Double tap \"jump\" to toggle fly");
			Environment->addCheckBox(g_settings->getBool("doubletap_jump"), rect, this,
					GUI_ID_CB_DOUBLETAP_JUMP, text);
			delete[] text;
		}
		offset += v2s32(0, 25 * s);
	}

	{
		s32 option_x = offset.X;
		s32 option_y = offset.Y + 5 * s;
		u32 option_w = 300 * s;
		{
			core::rect<s32> rect(0, 0, option_w, 30 * s);
			rect += topleft + v2s32(option_x, option_y);
			const wchar_t *text = wgettext("Automatic jumping");
			Environment->addCheckBox(g_settings->getBool("autojump"), rect, this,
					GUI_ID_CB_AUTOJUMP, text);
			delete[] text;
		}
		offset += v2s32(0, 25);
	}

	const std::array<StyleSpec, StyleSpec::NUM_STATES> styles =
			StyleSpec::getButtonStyle(texture_path);

	{
		core::rect<s32> rect(0, 0, 150 * s, 35 * s);
		rect += topleft + v2s32(size.X / 2 - 165 * s, size.Y - 50 * s);
		const wchar_t *text = wgettext("Save");
		GUIButton *e = GUIButton::addButton(Environment, rect, m_tsrc, this, GUI_ID_BACK_BUTTON, text);
		e->setStyles(styles);
		delete[] text;
	}
	{
		core::rect<s32> rect(0, 0, 150 * s, 35 * s);
		rect += topleft + v2s32(size.X / 2 + 15 * s, size.Y - 50 * s);
		const wchar_t *text = wgettext("Cancel");
		GUIButton *e = GUIButton::addButton(Environment, rect, m_tsrc, this, GUI_ID_ABORT_BUTTON, text);
		e->setStyles(styles);
		delete[] text;
	}
}

void GUIKeyChangeMenu::drawMenu()
{
	gui::IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;

	if (!m_main_menu) {
		video::IVideoDriver* driver = Environment->getVideoDriver();
		v2u32 screenSize = driver->getScreenSize();
		core::rect<s32> allbg(0, 0, screenSize.X, screenSize.Y);
		driver->draw2DRectangle(m_fullscreen_bgcolor, allbg, &allbg);
	}

	gui::IGUIElement::draw();
}

bool GUIKeyChangeMenu::acceptInput()
{
	for (key_setting *k : key_settings) {
		std::string default_key;
		Settings::getLayer(SL_DEFAULTS)->getNoEx(k->setting_name, default_key);

		if (k->key.sym() != default_key)
			g_settings->set(k->setting_name, k->key.sym());
		else
			g_settings->remove(k->setting_name);
	}

	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_CB_AUX1_DESCENDS);
		if(e && e->getType() == gui::EGUIET_CHECK_BOX)
			g_settings->setBool("aux1_descends", ((gui::IGUICheckBox*)e)->isChecked());
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_CB_DOUBLETAP_JUMP);
		if(e && e->getType() == gui::EGUIET_CHECK_BOX)
			g_settings->setBool("doubletap_jump", ((gui::IGUICheckBox*)e)->isChecked());
	}
	{
		gui::IGUIElement *e = getElementFromId(GUI_ID_CB_AUTOJUMP);
		if(e && e->getType() == gui::EGUIET_CHECK_BOX)
			g_settings->setBool("autojump", ((gui::IGUICheckBox*)e)->isChecked());
	}

	clearKeyCache();

	g_gamecallback->signalKeyConfigChange();

	if (!g_settings_path.empty())
		g_settings->updateConfigFile(g_settings_path.c_str());

	return true;
}

bool GUIKeyChangeMenu::resetMenu()
{
	if (active_key) {
		const wchar_t *text = wgettext(active_key->key.name());
		active_key->button->setText(text);
		delete[] text;
		active_key = nullptr;
		return false;
	}
	return true;
}
bool GUIKeyChangeMenu::OnEvent(const SEvent& event)
{
	if (event.EventType == EET_KEY_INPUT_EVENT && active_key
			&& event.KeyInput.PressedDown) {

		bool prefer_character = shift_down;
		KeyPress kp(event.KeyInput, prefer_character);

		if (event.KeyInput.Key == irr::KEY_DELETE)
			kp = KeyPress(""); // To erase key settings
		else if (event.KeyInput.Key == irr::KEY_ESCAPE)
			kp = active_key->key; // Cancel

		bool shift_went_down = false;
		if(!shift_down &&
				(event.KeyInput.Key == irr::KEY_SHIFT ||
				event.KeyInput.Key == irr::KEY_LSHIFT ||
				event.KeyInput.Key == irr::KEY_RSHIFT))
			shift_went_down = true;

		// Display Key already in use message
		bool key_in_use = false;
		if (strcmp(kp.sym(), "") != 0) {
			for (key_setting *ks : key_settings) {
				if (ks != active_key && ks->key == kp) {
					key_in_use = true;
					break;
				}
			}
		}

		if (key_in_use && !this->key_used_text) {
			core::rect<s32> rect(0, 0, 600, 40);
			rect += v2s32(0, 0) + v2s32(25, 30);
			const wchar_t *text = wgettext("Key already in use");
			this->key_used_text = Environment->addStaticText(text,
					rect, false, true, this, -1);
			delete[] text;
		} else if (!key_in_use && this->key_used_text) {
			this->key_used_text->remove();
			this->key_used_text = nullptr;
		}

		// But go on
		{
			active_key->key = kp;
			const wchar_t *text = wgettext(kp.name());
			active_key->button->setText(text);
			delete[] text;

			// Allow characters made with shift
			if (shift_went_down){
				shift_down = true;
				return false;
			}

			active_key = nullptr;
			return true;
		}
	} else if (event.EventType == EET_KEY_INPUT_EVENT && !active_key
			&& event.KeyInput.PressedDown
			&& event.KeyInput.Key == irr::KEY_ESCAPE) {
		quitMenu();
		return true;
	} else if (event.EventType == EET_GUI_EVENT) {
		if (event.GUIEvent.EventType == gui::EGET_ELEMENT_FOCUS_LOST
			&& isVisible())
		{
			if (!canTakeFocus(event.GUIEvent.Element))
			{
				infostream << "GUIKeyChangeMenu: Not allowing focus change."
				<< std::endl;
				// Returning true disables focus change
				return true;
			}
		}
		if (event.GUIEvent.EventType == gui::EGET_BUTTON_CLICKED)
		{
			switch (event.GUIEvent.Caller->getID())
			{
				case GUI_ID_BACK_BUTTON: //back
					acceptInput();
					quitMenu();
					return true;
				case GUI_ID_ABORT_BUTTON: //abort
					quitMenu();
					return true;
				default:
					resetMenu();
					for (key_setting *ks : key_settings) {
						if (ks->id == event.GUIEvent.Caller->getID()) {
							active_key = ks;
							break;
						}
					}
					FATAL_ERROR_IF(!active_key, "Key setting not found");

					shift_down = false;
					const wchar_t *text = wgettext("press key");
					active_key->button->setText(text);
					delete[] text;
					break;
			}
			Environment->setFocus(this);
		}
	}
	return Parent ? Parent->OnEvent(event) : false;
}

void GUIKeyChangeMenu::add_key(int id, const wchar_t *button_name, const std::string &setting_name)
{
	key_setting *k = new key_setting;
	k->id = id;

	k->button_name = button_name;
	k->setting_name = setting_name;
	k->key = getKeySetting(k->setting_name.c_str());
	key_settings.push_back(k);
}

void GUIKeyChangeMenu::init_keys()
{
	this->add_key(GUI_ID_KEY_FORWARD_BUTTON,   wgettext("Forward"),          "keymap_forward");
	this->add_key(GUI_ID_KEY_BACKWARD_BUTTON,  wgettext("Backward"),         "keymap_backward");
	this->add_key(GUI_ID_KEY_LEFT_BUTTON,      wgettext("Left"),             "keymap_left");
	this->add_key(GUI_ID_KEY_RIGHT_BUTTON,     wgettext("Right"),            "keymap_right");
	this->add_key(GUI_ID_KEY_USE_BUTTON,       wgettext("Special"),          "keymap_special1");
	this->add_key(GUI_ID_KEY_JUMP_BUTTON,      wgettext("Jump"),             "keymap_jump");
	this->add_key(GUI_ID_KEY_SNEAK_BUTTON,     wgettext("Sneak"),            "keymap_sneak");
	this->add_key(GUI_ID_KEY_DROP_BUTTON,      wgettext("Drop"),             "keymap_drop");
	this->add_key(GUI_ID_KEY_INVENTORY_BUTTON, wgettext("Inventory"),        "keymap_inventory");
	this->add_key(GUI_ID_KEY_HOTBAR_PREV_BUTTON,wgettext("Prev. item"),      "keymap_hotbar_previous");
	this->add_key(GUI_ID_KEY_HOTBAR_NEXT_BUTTON,wgettext("Next item"),       "keymap_hotbar_next");
	this->add_key(GUI_ID_KEY_ZOOM_BUTTON,      wgettext("Zoom"),             "keymap_zoom");
	this->add_key(GUI_ID_KEY_CAMERA_BUTTON,    wgettext("Change camera"),    "keymap_camera_mode");
	this->add_key(GUI_ID_KEY_MINIMAP_BUTTON,   wgettext("Toggle minimap"),   "keymap_minimap");
	this->add_key(GUI_ID_KEY_FLY_BUTTON,       wgettext("Toggle fly"),       "keymap_freemove");
	this->add_key(GUI_ID_KEY_PITCH_MOVE,       wgettext("Toggle pitchmove"), "keymap_pitchmove");
	this->add_key(GUI_ID_KEY_FAST_BUTTON,      wgettext("Toggle fast"),      "keymap_fastmove");
	this->add_key(GUI_ID_KEY_NOCLIP_BUTTON,    wgettext("Toggle noclip"),    "keymap_noclip");
	this->add_key(GUI_ID_KEY_MUTE_BUTTON,      wgettext("Mute"),             "keymap_mute");
	this->add_key(GUI_ID_KEY_DEC_VOLUME_BUTTON,wgettext("Dec. volume"),      "keymap_decrease_volume");
	this->add_key(GUI_ID_KEY_INC_VOLUME_BUTTON,wgettext("Inc. volume"),      "keymap_increase_volume");
	this->add_key(GUI_ID_KEY_AUTOFWD_BUTTON,   wgettext("Autoforward"),      "keymap_autoforward");
	this->add_key(GUI_ID_KEY_CHAT_BUTTON,      wgettext("Chat"),             "keymap_chat");
	this->add_key(GUI_ID_KEY_SCREENSHOT_BUTTON,wgettext("Screenshot"),       "keymap_screenshot");
	this->add_key(GUI_ID_KEY_RANGE_BUTTON,     wgettext("Range select"),     "keymap_rangeselect");
	this->add_key(GUI_ID_KEY_DEC_RANGE_BUTTON, wgettext("Dec. range"),       "keymap_decrease_viewing_range_min");
	this->add_key(GUI_ID_KEY_INC_RANGE_BUTTON, wgettext("Inc. range"),       "keymap_increase_viewing_range_min");
	this->add_key(GUI_ID_KEY_CONSOLE_BUTTON,   wgettext("Console"),          "keymap_console");
	this->add_key(GUI_ID_KEY_CMD_BUTTON,       wgettext("Command"),          "keymap_cmd");
	this->add_key(GUI_ID_KEY_CMD_LOCAL_BUTTON, wgettext("Local command"),    "keymap_cmd_local");
	this->add_key(GUI_ID_KEY_HUD_BUTTON,       wgettext("Toggle HUD"),       "keymap_toggle_hud");
	this->add_key(GUI_ID_KEY_CHATLOG_BUTTON,   wgettext("Toggle chat log"),  "keymap_toggle_chat");
	this->add_key(GUI_ID_KEY_FOG_BUTTON,       wgettext("Toggle fog"),       "keymap_toggle_fog");
}
