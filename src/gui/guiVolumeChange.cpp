/*
Part of Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2013 Ciaran Gultnieks <ciaran@ciarang.com>
Copyright (C) 2013 RealBadAngel, Maciej Kasatkin <mk@realbadangel.pl>

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "guiVolumeChange.h"
#include "debug.h"
#include "guiBackgroundImage.h"
#include "guiButton.h"
#include "guiScrollBar.h"
#include "serialization.h"
#include <string>
#include <IGUICheckBox.h>
#include <IGUIButton.h>
#include <IGUIStaticText.h>
#include <IGUIFont.h>
#include "settings.h"
#include "StyleSpec.h"
#include "porting.h"

#include "gettext.h"
#include "client/renderingengine.h"
#include "client/sound.h"

const int ID_backgroundImage = 262;
const int ID_soundText = 263;
const int ID_soundExitButton = 264;
const int ID_soundSlider = 265;
const int ID_soundMuteButton = 266;

GUIVolumeChange::GUIVolumeChange(gui::IGUIEnvironment* env,
		gui::IGUIElement* parent, s32 id,
		IMenuManager *menumgr, ISimpleTextureSource *tsrc,
		ISoundManager *sound_manager
):
	GUIModalMenu(env, parent, id, menumgr),
	m_tsrc(tsrc), m_sound_manager(sound_manager)
{
	v3f formspec_bgcolor = g_settings->getV3F("formspec_fullscreen_bg_color");
	m_fullscreen_bgcolor = video::SColor(
		(u8) MYMIN(MYMAX(g_settings->getS32("formspec_fullscreen_bg_opacity"), 0), 255),
		MYMIN(MYMAX(myround(formspec_bgcolor.X), 0), 255),
		MYMIN(MYMAX(myround(formspec_bgcolor.Y), 0), 255),
		MYMIN(MYMAX(myround(formspec_bgcolor.Z), 0), 255)
	);
}

GUIVolumeChange::~GUIVolumeChange()
{
	removeChildren();
}

void GUIVolumeChange::removeChildren()
{
	if (gui::IGUIElement *e = getElementFromId(ID_backgroundImage))
		e->remove();

	if (gui::IGUIElement *e = getElementFromId(ID_soundText))
		e->remove();

	if (gui::IGUIElement *e = getElementFromId(ID_soundExitButton))
		e->remove();

	if (gui::IGUIElement *e = getElementFromId(ID_soundSlider))
		e->remove();

	if (gui::IGUIElement *e = getElementFromId(ID_soundMuteButton))
		e->remove();
}

void GUIVolumeChange::regenerateGui(v2u32 screensize)
{
	/*
		Remove stuff
	*/
	removeChildren();
	/*
		Calculate new sizes and positions
	*/
	float s = MYMIN(screensize.X / 380.f, screensize.Y / 180.f);
#if HAVE_TOUCHSCREENGUI
#ifdef __IOS__
	const char *model = MultiCraft::getDeviceModel();
	if (!isDevice11Inch(model) && !isDevice12and9Inch(model))
		s *= 0.5f;
	else
		s *= 0.4f;
#else
	s *= RenderingEngine::isTablet() ? 0.4f : 0.5f;
#endif
#else
	s *= 0.35f;
#endif

	DesiredRect = core::rect<s32>(
		screensize.X / 2 - 380 * s / 2,
		screensize.Y / 2 - 180 * s / 2,
		screensize.X / 2 + 380 * s / 2,
		screensize.Y / 2 + 180 * s / 2 // 200
	);
	recalculateAbsolutePosition(false);

	v2s32 size = DesiredRect.getSize();
	int volume = (int)(g_settings->getFloat("sound_volume") * 100);

	/*
		Add stuff
	*/
	{
		const std::string texture = "bg_common.png";
		const core::rect<s32> rect(0, 0, 0, 0);
		const core::rect<s32> middle(40, 40, -40, -40);
		new GUIBackgroundImage(Environment, this, ID_backgroundImage, rect,
				texture, middle, m_tsrc, true);
	}
	{
		core::rect<s32> rect(0, 0, 320 * s, 25 * s);
		rect = rect + v2s32(30 * s, size.Y / 2 - 35 * s); // 55

		const wchar_t *text = wgettext("Sound Volume: ");
		core::stringw volume_text = text;
		delete[] text;

		volume_text += core::stringw(volume) + core::stringw("%");
		Environment->addStaticText(volume_text.c_str(), rect, false,
				true, this, ID_soundText);
	}
	{
		core::rect<s32> rect(0, 0, 100 * s, 35 * s);
		rect = rect + v2s32(size.X / 2 - 50 * s, size.Y / 2 + 35 * s); // 45
		const wchar_t *text = wgettext("Save");
		GUIButton *e = GUIButton::addButton(Environment, rect, m_tsrc, this,
				ID_soundExitButton, text);
		delete[] text;

		e->setStyles(StyleSpec::getButtonStyle());
	}
	{
		core::rect<s32> rect(0, 0, 320 * s, 30 * s);
		rect = rect + v2s32(size.X / 2 - 160 * s, size.Y / 2 - 12 * s); // 30
		GUIScrollBar *e = new GUIScrollBar(Environment, this,
			ID_soundSlider, rect, true, false, m_sound_manager);
		e->setMax(100);
		e->setPos(volume);
		e->setArrowsVisible(GUIScrollBar::ArrowVisibility::SHOW);

		StyleSpec spec;
		spec.set(StyleSpec::SOUND, g_settings->get("btn_press_sound"));
		spec.set(StyleSpec::SCROLLBAR_BGIMG, "gui/scrollbar_horiz_bg.png");
		spec.set(StyleSpec::SCROLLBAR_THUMB_IMG, "gui/scrollbar_slider.png");
		spec.set(StyleSpec::SCROLLBAR_UP_IMG, "gui/scrollbar_minus.png");
		spec.set(StyleSpec::SCROLLBAR_DOWN_IMG, "gui/scrollbar_plus.png");
		e->setStyle(spec, m_tsrc);
	}
	/*{
		core::rect<s32> rect(0, 0, 150 * s, 25 * s);
		rect = rect + v2s32(30 * s, size.Y / 2 + 5 * s);
		const wchar_t *text = wgettext("Muted");
		Environment->addCheckBox(g_settings->getBool("mute_sound"), rect, this,
				ID_soundMuteButton, text);
		delete[] text;
	}*/
}

void GUIVolumeChange::drawMenu()
{
	gui::IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;
	video::IVideoDriver* driver = Environment->getVideoDriver();

	v2u32 screenSize = driver->getScreenSize();
	core::rect<s32> allbg(0, 0, screenSize.X, screenSize.Y);
	driver->draw2DRectangle(m_fullscreen_bgcolor, allbg, &allbg);

	gui::IGUIElement::draw();
}

void GUIVolumeChange::saveSettingsAndQuit()
{
	if (!g_settings_path.empty())
		g_settings->updateConfigFile(g_settings_path.c_str());
	quitMenu();
}

bool GUIVolumeChange::OnEvent(const SEvent& event)
{
	if (event.EventType == EET_KEY_INPUT_EVENT) {
		if (event.KeyInput.Key == KEY_ESCAPE && event.KeyInput.PressedDown) {
			saveSettingsAndQuit();
			return true;
		}

		if (event.KeyInput.Key == KEY_RETURN && event.KeyInput.PressedDown) {
			saveSettingsAndQuit();
			return true;
		}
	} else if (event.EventType == EET_GUI_EVENT) {
		/*if (event.GUIEvent.EventType == gui::EGET_CHECKBOX_CHANGED) {
			gui::IGUIElement *e = getElementFromId(ID_soundMuteButton);
			if (e != NULL && e->getType() == gui::EGUIET_CHECK_BOX) {
				g_settings->setBool("mute_sound", ((gui::IGUICheckBox*)e)->isChecked());
			}

			Environment->setFocus(this);
			return true;
		}*/

		if (event.GUIEvent.EventType == gui::EGET_BUTTON_CLICKED) {
			if (event.GUIEvent.Caller->getID() == ID_soundExitButton) {
				const std::string sound = g_settings->get("btn_press_sound");
				if (!sound.empty())
					m_sound_manager->playSound(sound, false, 1.0f);
				saveSettingsAndQuit();
				return true;
			}
			Environment->setFocus(this);
		}

		if (event.GUIEvent.EventType == gui::EGET_ELEMENT_FOCUS_LOST
				&& isVisible()) {
			if (!canTakeFocus(event.GUIEvent.Element)) {
				infostream << "GUIVolumeChange: Not allowing focus change."
				<< std::endl;
				// Returning true disables focus change
				return true;
			}
		}
		if (event.GUIEvent.EventType == gui::EGET_SCROLL_BAR_CHANGED) {
			if (event.GUIEvent.Caller->getID() == ID_soundSlider) {
				s32 pos = ((GUIScrollBar*)event.GUIEvent.Caller)->getPos();
				g_settings->setFloat("sound_volume", (float) pos / 100);

				// unmute sound when changing the volume
				if (g_settings->getBool("mute_sound"))
					g_settings->setBool("mute_sound", false);

				gui::IGUIElement *e = getElementFromId(ID_soundText);
				const wchar_t *text = wgettext("Sound Volume: ");
				core::stringw volume_text = text;
				delete[] text;

				volume_text += core::stringw(pos) + core::stringw("%");
				e->setText(volume_text.c_str());
				return true;
			}
		}

	}

	return Parent ? Parent->OnEvent(event) : false;
}
