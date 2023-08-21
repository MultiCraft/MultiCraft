/*
Minetest
Copyright (C) 2018 srifqi, Muhammad Rifqi Priyo Susanto
		<muhammadrifqipriyosusanto@gmail.com>

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

#include "guiConfirmRegistration.h"
#include "client/client.h"
#include "filesys.h"
#include "guiBackgroundImage.h"
#include "guiButton.h"
#include "guiEditBoxWithScrollbar.h"
#include <IGUICheckBox.h>
#include <IGUIButton.h>
#include <IGUIStaticText.h>
#include <IGUIFont.h>
#include "porting.h"

#if USE_FREETYPE && IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR < 9
	#include "intlGUIEditBox.h"
#endif

#ifdef HAVE_TOUCHSCREENGUI
	#include "client/renderingengine.h"
#endif

#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
	#include <SDL.h>
#endif

#include "gettext.h"

// Continuing from guiPasswordChange.cpp
const int ID_confirmPassword = 262;
const int ID_confirm = 263;
const int ID_intotext = 264;
const int ID_cancel = 265;
const int ID_message = 266;
const int ID_background = 267;
const int ID_confirmPasswordBg = 268;

GUIConfirmRegistration::GUIConfirmRegistration(gui::IGUIEnvironment *env,
		gui::IGUIElement *parent, s32 id, IMenuManager *menumgr, Client *client,
		const std::string &playername, const std::string &password,
		bool *aborted, ISimpleTextureSource *tsrc) :
		GUIModalMenu(env, parent, id, menumgr),
		m_client(client), m_playername(playername), m_password(password),
		m_aborted(aborted), m_tsrc(tsrc)
{
#ifdef HAVE_TOUCHSCREENGUI
	m_touchscreen_visible = false;
#endif

#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
	if (porting::hasRealKeyboard())
		SDL_StartTextInput();
#endif
}

GUIConfirmRegistration::~GUIConfirmRegistration()
{
	removeChildren();

#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
	if (porting::hasRealKeyboard() && SDL_IsTextInputActive())
		SDL_StopTextInput();
#endif
}

void GUIConfirmRegistration::removeChildren()
{
	const core::list<gui::IGUIElement *> &children = getChildren();
	core::list<gui::IGUIElement *> children_copy;
	for (gui::IGUIElement *i : children)
		children_copy.push_back(i);
	for (gui::IGUIElement *i : children_copy)
		i->remove();
}

void GUIConfirmRegistration::regenerateGui(v2u32 screensize)
{
	acceptInput();
	removeChildren();

	/*
		Calculate new sizes and positions
	*/
	float s = MYMIN(screensize.X / 600.f, screensize.Y / 360.f);
#if HAVE_TOUCHSCREENGUI
	s *= RenderingEngine::isTablet() ? 0.7f : 0.8f;
#else
	s *= 0.5f;
#endif

	DesiredRect = core::rect<s32>(
		screensize.X / 2 - 600 * s / 2,
		screensize.Y / 2 - 360 * s / 2,
		screensize.X / 2 + 600 * s / 2,
		screensize.Y / 2 + 360 * s / 2
	);
	recalculateAbsolutePosition(false);

	v2s32 size = DesiredRect.getSize();
	v2s32 topleft_client(0, 0);

	const wchar_t *text;

	/*
		Add stuff
	*/

	// Background image
	{
		const std::string texture = "bg_common.png";
		const core::rect<s32> rect(0, 0, 0, 0);
		const core::rect<s32> middle(40, 40, -40, -40);
		new GUIBackgroundImage(Environment, this, ID_background, rect,
				texture, middle, m_tsrc, true);
	}

	s32 ypos = 20 * s;
	{
		core::rect<s32> rect2(0, 0, 540 * s, 180 * s);
		rect2 += topleft_client + v2s32(30 * s, ypos);
		const std::string info_text_template = strgettext(
				"You are about to join this server with the name \"%s\" for the "
				"first time.\n"
				"If you proceed, a new account using your credentials will be "
				"created on this server.\n"
				"Please retype your password and click 'Register and Join' to "
				"confirm account creation, or click 'Cancel' to abort.");
		char info_text_buf[1024];
		porting::mt_snprintf(info_text_buf, sizeof(info_text_buf),
				info_text_template.c_str(), m_playername.c_str());

		wchar_t *info_text_buf_wide = utf8_to_wide_c(info_text_buf);
#if USE_FREETYPE && IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR < 9
		gui::IGUIEditBox *e = new gui::intlGUIEditBox(info_text_buf_wide, true,
				Environment, this, ID_intotext, rect2, false, true);
#else
		gui::IGUIEditBox *e = new GUIEditBoxWithScrollBar(info_text_buf_wide, true,
				Environment, this, ID_intotext, rect2, false, true);
#endif
		delete[] info_text_buf_wide;
		e->drop();
		e->setMultiLine(true);
		e->setWordWrap(true);
		e->setTextAlignment(gui::EGUIA_UPPERLEFT, gui::EGUIA_CENTER);
	}

	ypos += 140 * s;
	{
		core::rect<s32> rect2(0, 0, 500 * s, 40 * s);
		rect2 += topleft_client + v2s32(30 * s, ypos + 45 * s);
		text = wgettext("Passwords do not match!");
		IGUIElement *e = Environment->addStaticText(
				text, rect2, false, true, this, ID_message);
		e->setVisible(false);
		delete[] text;
	}

	ypos += 75 * s;
	{
		core::rect<s32> rect2(0, 0, 540 * s, 40 * s);
		rect2 += topleft_client + v2s32(30 * s, ypos);

		core::rect<s32> bg_middle(10, 10, -10, -10);
		new GUIBackgroundImage(Environment, this, ID_confirmPasswordBg,
				rect2, "field_bg.png", bg_middle, m_tsrc, false);

		rect2.UpperLeftCorner.X += 5 * s;
		rect2.LowerRightCorner.X -= 5 * s;

		gui::IGUIEditBox *e = Environment->addEditBox(m_pass_confirm.c_str(),
				rect2, true, this, ID_confirmPassword);
		e->setDrawBorder(false);
		e->setDrawBackground(false);
		e->setPasswordBox(true);
		Environment->setFocus(e);
	}

	ypos += 60 * s;
	{
		core::rect<s32> rect2(0, 0, 300 * s, 40 * s);
		rect2 = rect2 + v2s32(size.X / 2 - 250 * s, ypos);
		text = wgettext("Register and Join");
		GUIButton *e = GUIButton::addButton(Environment, rect2, m_tsrc, this, ID_confirm, text);
		e->setStyles(StyleSpec::getButtonStyle("", "green"));
		delete[] text;
	}
	{
		core::rect<s32> rect2(0, 0, 140 * s, 40 * s);
		rect2 = rect2 + v2s32(size.X / 2 + 110 * s, ypos);
		text = wgettext("Cancel");
		GUIButton *e = GUIButton::addButton(Environment, rect2, m_tsrc, this, ID_cancel, text);
		e->setStyles(StyleSpec::getButtonStyle());
		delete[] text;
	}
}

void GUIConfirmRegistration::drawMenu()
{
	gui::IGUISkin *skin = Environment->getSkin();
	if (!skin)
		return;

	gui::IGUIElement::draw();
#if defined(__ANDROID__) || defined(__IOS__)
	getAndroidUIInput();
#endif
}

void GUIConfirmRegistration::closeMenu(bool goNext)
{
	if (goNext) {
		m_client->confirmRegistration();
	} else {
		*m_aborted = true;
		infostream << "Connect aborted [Escape]" << std::endl;
	}
	quitMenu();
}

void GUIConfirmRegistration::acceptInput()
{
	gui::IGUIElement *e;
	e = getElementFromId(ID_confirmPassword);
	if (e)
		m_pass_confirm = e->getText();
}

bool GUIConfirmRegistration::processInput()
{
	if (utf8_to_wide(m_password) != m_pass_confirm) {
		gui::IGUIElement *e = getElementFromId(ID_message);
		if (e)
			e->setVisible(true);
		return false;
	}
	return true;
}

bool GUIConfirmRegistration::OnEvent(const SEvent &event)
{
	if (event.EventType == EET_KEY_INPUT_EVENT) {
		// clang-format off
		if ((event.KeyInput.Key == KEY_ESCAPE ||
				event.KeyInput.Key == KEY_CANCEL) &&
				event.KeyInput.PressedDown) {
			closeMenu(false);
			return true;
		}
		// clang-format on
		if (event.KeyInput.Key == KEY_RETURN && event.KeyInput.PressedDown) {
			acceptInput();
			if (processInput())
				closeMenu(true);
			return true;
		}
	}

	if (event.EventType != EET_GUI_EVENT)
		return Parent ? Parent->OnEvent(event) : false;

	if (event.GUIEvent.EventType == gui::EGET_ELEMENT_FOCUS_LOST && isVisible()) {
		if (!canTakeFocus(event.GUIEvent.Element)) {
			infostream << "GUIConfirmRegistration: Not allowing focus change."
				<< std::endl;
			// Returning true disables focus change
			return true;
		}
	} else if (event.GUIEvent.EventType == gui::EGET_BUTTON_CLICKED) {
		switch (event.GUIEvent.Caller->getID()) {
		case ID_confirm:
			acceptInput();
			if (processInput())
				closeMenu(true);
			return true;
		case ID_cancel:
			closeMenu(false);
			return true;
		}
	} else if (event.GUIEvent.EventType == gui::EGET_EDITBOX_ENTER) {
		switch (event.GUIEvent.Caller->getID()) {
		case ID_confirmPassword:
			acceptInput();
			if (processInput())
				closeMenu(true);
			return true;
		}
	}

	return false;
}

#if defined(__ANDROID__) || defined(__IOS__)
bool GUIConfirmRegistration::getAndroidUIInput()
{
	if (m_jni_field_name.empty() || m_jni_field_name != "password")
		return false;

	// still waiting
	if (porting::getInputDialogState() == -1)
		return true;

	m_jni_field_name.clear();

	gui::IGUIElement *e = getElementFromId(ID_confirmPassword);

	if (!e || e->getType() != irr::gui::EGUIET_EDIT_BOX)
		return false;

	std::string text = porting::getInputDialogValue();
	e->setText(utf8_to_wide(text).c_str());
	return false;
}
#endif
