/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "guiChatConsole.h"
#include "chat.h"
#include "client/client.h"
#include "debug.h"
#include "gettime.h"
#include "client/keycode.h"
#include "settings.h"
#include "porting.h"
#include "client/tile.h"
#include "client/fontengine.h"
#include "log.h"
#include "gettext.h"
#include "bidi.h"
#include <algorithm>
#include <string>
#ifdef HAVE_TOUCHSCREENGUI
#include "touchscreengui_mc.h"
#endif

#if USE_FREETYPE
	#include "irrlicht_changes/CGUITTFont.h"
#endif

#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
#include <SDL3/SDL.h>
#endif

inline u32 clamp_u8(s32 value)
{
	return (u32) MYMIN(MYMAX(value, 0), 255);
}

GUIChatConsole* GUIChatConsole::m_chat_console = nullptr;

GUIChatConsole::GUIChatConsole(
		gui::IGUIEnvironment* env,
		gui::IGUIElement* parent,
		s32 id,
		ChatBackend* backend,
		Client* client,
		IMenuManager* menumgr
):
	IGUIElement(gui::EGUIET_ELEMENT, env, parent, id,
			core::rect<s32>(0,0,100,100)),
	m_chat_backend(backend),
	m_client(client),
	m_menumgr(menumgr),
	m_animate_time_old(porting::getTimeMs())
{
	m_chat_console = this;

	// load background settings
	s32 console_alpha = g_settings->getS32("console_alpha");
	m_background_color.setAlpha(clamp_u8(console_alpha));

	// load the background texture depending on settings
	ITextureSource *tsrc = client->getTextureSource();
	if (tsrc->isKnownSourceImage("background_chat.jpg")) {
		m_background = tsrc->getTexture("background_chat.jpg");
		m_background_color.setRed(255);
		m_background_color.setGreen(255);
		m_background_color.setBlue(255);
	} else {
		v3f console_color = g_settings->getV3F("console_color");
		m_background_color.setRed(clamp_u8(myround(console_color.X)));
		m_background_color.setGreen(clamp_u8(myround(console_color.Y)));
		m_background_color.setBlue(clamp_u8(myround(console_color.Z)));
	}

	u16 chat_font_size = g_settings->getU16("chat_font_size");
	m_font = g_fontengine->getFont(chat_font_size != 0 ?
		chat_font_size : FONT_SIZE_UNSPECIFIED, FM_Mono);

	if (!m_font) {
		errorstream << "GUIChatConsole: Unable to load mono font" << std::endl;
	} else {
		core::dimension2d<u32> dim = m_font->getDimension(L"M");
		m_fontsize = v2u32(dim.Width, dim.Height);
		m_font->grab();
	}
	m_fontsize.X = MYMAX(m_fontsize.X, 1);
	m_fontsize.Y = MYMAX(m_fontsize.Y, 1);

#if defined(__ANDROID__) || defined(__IOS__)
	m_round_screen_offset = g_settings->getU16("round_screen");
#endif

	createVScrollBar();

	// set default cursor options
	setCursor(true, true, 2.0, 0.1);
}

GUIChatConsole::~GUIChatConsole()
{
	m_chat_console = nullptr;

	removeChild(m_vscrollbar);
	delete m_vscrollbar;

	RenderingEngine::stopTextInput();

	if (m_font)
		m_font->drop();
}

void GUIChatConsole::openConsole(f32 scale)
{
	assert(scale > 0.0f && scale <= 1.0f);

	m_open = true;
	m_desired_height_fraction = scale;

	if (g_settings->getU32("fps_max") < 60) {
		m_desired_height_fraction *= m_screensize.Y;
		m_height = m_desired_height_fraction;
	}

	m_desired_height = scale * m_screensize.Y;
	reformatConsole();
	updateVScrollBar(false, true);
	m_animate_time_old = porting::getTimeMs();
	IGUIElement::setVisible(true);
	m_vscrollbar->setVisible(true);
	Environment->setFocus(this);
	m_menumgr->createdMenu(this);

	RenderingEngine::startTextInput();
}

bool GUIChatConsole::isOpen() const
{
	return m_open;
}

bool GUIChatConsole::isOpenInhibited() const
{
	return m_open_inhibited > 0;
}

void GUIChatConsole::closeConsole()
{
	m_open = false;
	Environment->removeFocus(this);
	m_menumgr->deletingMenu(this);

	if (g_settings->getU32("fps_max") < 60) {
		m_height = 0;
		recalculateConsolePosition();
	}

	RenderingEngine::stopTextInput();

#ifdef HAVE_TOUCHSCREENGUI
	if (g_touchscreengui && g_touchscreengui->isActive())
		g_touchscreengui->show();
#endif
}

void GUIChatConsole::replaceAndAddToHistory(const std::wstring &line)
{
	ChatPrompt& prompt = m_chat_backend->getPrompt();
	prompt.addToHistory(prompt.getLine());
	prompt.replace(line);
}


void GUIChatConsole::setCursor(
	bool visible, bool blinking, f32 blink_speed, f32 relative_height)
{
	if (visible)
	{
		if (blinking)
		{
			// leave m_cursor_blink unchanged
			m_cursor_blink_speed = blink_speed;
		}
		else
		{
			m_cursor_blink = 0x8000;  // on
			m_cursor_blink_speed = 0.0;
		}
	}
	else
	{
		m_cursor_blink = 0;  // off
		m_cursor_blink_speed = 0.0;
	}
	m_cursor_height = relative_height;
}

void GUIChatConsole::draw()
{
	if(!IsVisible)
		return;

	updateVScrollBar();

	video::IVideoDriver* driver = Environment->getVideoDriver();

	// Check screen size
	v2u32 screensize = driver->getScreenSize();
	if (screensize != m_screensize)
	{
		// screen size has changed
		// scale current console height to new window size
		if (m_screensize.Y != 0)
			m_height = m_height * screensize.Y / m_screensize.Y;
		m_screensize = screensize;
		m_desired_height = m_desired_height_fraction * m_screensize.Y;
		reformatConsole();
		updateVScrollBar(true, false);
	}

	// Animation
	u64 now = porting::getTimeMs();
	animate(now - m_animate_time_old);
	m_animate_time_old = now;

	// Draw console elements if visible
	if (m_height > 0)
	{
		drawBackground();
		drawText();
		drawPrompt();
	}

	gui::IGUIElement::draw();
}

void GUIChatConsole::reformatConsole()
{
	s32 cols = (m_screensize.X - 2 * m_round_screen_offset - m_scrollbar_width) / m_fontsize.X - 2; // make room for a margin (looks better)
	s32 rows = m_desired_height / m_fontsize.Y - 1; // make room for the input prompt
	if (cols <= 0 || rows <= 0)
		cols = rows = 0;
	recalculateConsolePosition();
	m_chat_backend->reformat(cols, rows);

	m_mark_begin.reset();
	m_mark_end.reset();
	m_cursor_press_pos.reset();
	m_history_marking = false;
	m_prompt_marking = false;
	m_long_press = false;
}

void GUIChatConsole::recalculateConsolePosition()
{
	core::rect<s32> rect(0, 0, m_screensize.X, m_height);
	DesiredRect = rect;
	recalculateAbsolutePosition(true);

	u32 scrollbar_x = m_screensize.X - m_round_screen_offset;
	irr::core::rect<s32> scrollbarrect(scrollbar_x - m_scrollbar_width, 0, scrollbar_x, m_height);
	m_vscrollbar->setRelativePosition(scrollbarrect);
}

void GUIChatConsole::animate(u32 msec)
{
	// animate the console height
	s32 goal = m_open ? m_desired_height : 0;

	// Set invisible if close animation finished (reset by openConsole)
	// This function (animate()) is never called once its visibility becomes false so do not
	//		actually set visible to false before the inhibited period is over
	if (!m_open && m_height == 0 && m_open_inhibited == 0) {
		m_vscrollbar->setVisible(false);
		IGUIElement::setVisible(false);
	}

	if (m_height != goal)
	{
		s32 max_change = msec * m_screensize.Y * (m_height_speed / 1000.0);
		if (max_change == 0)
			max_change = 1;

		if (m_height < goal)
		{
			// increase height
			if (m_height + max_change < goal)
				m_height += max_change;
			else
				m_height = goal;
		}
		else
		{
			// decrease height
			if (m_height > goal + max_change)
				m_height -= max_change;
			else
				m_height = goal;
		}

		recalculateConsolePosition();
	}

	// blink the cursor
	if (m_cursor_blink_speed != 0.0)
	{
		u32 blink_increase = 0x10000 * msec * (m_cursor_blink_speed / 1000.0);
		if (blink_increase == 0)
			blink_increase = 1;
		m_cursor_blink = ((m_cursor_blink + blink_increase) & 0xffff);
	}

	// decrease open inhibit counter
	if (m_open_inhibited > msec)
		m_open_inhibited -= msec;
	else
		m_open_inhibited = 0;
}

void GUIChatConsole::drawBackground()
{
	video::IVideoDriver* driver = Environment->getVideoDriver();
	if (m_background != NULL)
	{
		core::rect<s32> sourcerect(0, -m_height, m_screensize.X, 0);
		driver->draw2DImage(
			m_background,
			v2s32(0, 0),
			sourcerect,
			&AbsoluteClippingRect,
			m_background_color,
			false);
	}
	else
	{
		driver->draw2DRectangle(
			m_background_color,
			core::rect<s32>(0, 0, m_screensize.X, m_height),
			&AbsoluteClippingRect);
	}
}

void GUIChatConsole::drawText()
{
	if (m_font == NULL)
		return;

	ChatBuffer& buf = m_chat_backend->getConsoleBuffer();
	for (u32 row = 0; row < buf.getRows(); ++row)
	{
		const ChatFormattedLine& line = buf.getFormattedLine(row);
		if (line.fragments.empty())
			continue;

		s32 line_height = m_fontsize.Y;
		s32 y = row * line_height + m_height - m_desired_height;
		if (y + line_height < 0)
			continue;

		s32 scroll_pos = buf.getScrollPos();
		ChatSelection real_mark_begin = m_mark_end > m_mark_begin ? m_mark_begin : m_mark_end;
		ChatSelection real_mark_end = m_mark_end > m_mark_begin ? m_mark_end : m_mark_begin;

		if (real_mark_begin != real_mark_end &&
				real_mark_begin.selection_type == ChatSelection::SELECTION_HISTORY &&
				real_mark_end.selection_type == ChatSelection::SELECTION_HISTORY &&
				(s32)row + scroll_pos >= real_mark_begin.row + real_mark_begin.scroll &&
				(s32)row + scroll_pos <= real_mark_end.row + real_mark_end.scroll) {

			s32 pos_x = (line.fragments[0].column + 1) * m_fontsize.X + m_round_screen_offset;

			for (u32 i = 0; i < line.fragments.size(); i++) {
				const ChatFormattedFragment &fragment = line.fragments[i];
				core::stringw frag_text = fragment.text.getString().c_str();
				s32 logical_start = 0;
				s32 logical_end = frag_text.size();

				if ((s32)row + scroll_pos == real_mark_begin.row + real_mark_begin.scroll) {
					if (real_mark_begin.fragment == i) {
						logical_start = real_mark_begin.character;
					} else if (real_mark_begin.fragment > i) {
						pos_x += m_font->getDimension(frag_text.c_str()).Width;
						continue;
					}
				}

				if ((s32)row + scroll_pos == real_mark_end.row + real_mark_end.scroll) {
					if (real_mark_end.fragment == i) {
						logical_end = real_mark_end.character;
					} else if (real_mark_end.fragment < i) {
						break;
					}
				}

				TextBidiData text_bidi = applyBidiReordering(frag_text);
				std::vector<core::SelectionBidiRange> visual_ranges =
						text_bidi.getSelectionRanges(logical_start, logical_end);

				for (const auto& range : visual_ranges) {
					if (!range.Selected)
						continue;

					s32 x_begin = pos_x;
					s32 x_end = pos_x;

					if (range.Start > 0) {
						core::stringw text = text_bidi.TextBidi.subString(0, range.Start);
						x_begin += m_font->getDimension(text.c_str()).Width;
					}

					core::stringw text = text_bidi.TextBidi.subString(0, range.End);
					x_end += m_font->getDimension(text.c_str()).Width;

					core::rect<s32> destrect(x_begin, y, x_end, y + m_fontsize.Y);
					video::IVideoDriver* driver = Environment->getVideoDriver();
					IGUISkin* skin = Environment->getSkin();
					driver->draw2DRectangle(skin->getColor(EGDC_HIGH_LIGHT), destrect, &AbsoluteClippingRect);
				}

				pos_x += m_font->getDimension(frag_text.c_str()).Width;
			}
		}

		s32 x = (line.fragments[0].column + 1) * m_fontsize.X + m_round_screen_offset;
		for (const ChatFormattedFragment &fragment : line.fragments) {
			s32 text_size = m_font->getDimension(fragment.text.c_str()).Width;
			core::rect<s32> destrect(x, y, x + text_size, y + m_fontsize.Y);
			x += text_size;

#if USE_FREETYPE
			if (m_font->getType() == irr::gui::EGFT_CUSTOM) {
				// Draw colored text if FreeType is enabled
				irr::gui::CGUITTFont *tmp = dynamic_cast<irr::gui::CGUITTFont *>(m_font);
				tmp->draw(
					fragment.text,
					destrect,
					false,
					false,
					&AbsoluteClippingRect);
			} else
#endif
			{
				// Otherwise use standard text
				m_font->draw(
					fragment.text.c_str(),
					destrect,
					video::SColor(255, 255, 255, 255),
					false,
					false,
					&AbsoluteClippingRect);
			}
		}
	}
}

core::rect<s32> GUIChatConsole::getPromptFrameRect()
{
	u32 row = m_chat_backend->getConsoleBuffer().getRows();
	s32 line_height = m_fontsize.Y;
	s32 y = row * line_height + m_height - m_desired_height;

	core::rect<s32> destrect(
			m_fontsize.X + m_round_screen_offset, y,
			m_screensize.X - m_scrollbar_width - m_round_screen_offset - m_fontsize.X,
			y + m_fontsize.Y * 1.5f);

	core::rect<s32> frame_rect = destrect;

	return frame_rect;
}

core::rect<s32> GUIChatConsole::getPromptTextRect()
{
	core::rect<s32> text_rect;

	if (!m_font)
		return text_rect;

	ChatPrompt& prompt = m_chat_backend->getPrompt();
	std::wstring text = prompt.getLine();
	std::replace_if(text.begin(), text.end(),
			[](wchar_t c) { return (c == L'\n' || c == L'\r'); }, L' ');
	core::dimension2du d = m_font->getDimension(text.c_str());
	d.Height += m_font->getKerningHeight();

	text_rect.UpperLeftCorner.X = 0;
	text_rect.LowerRightCorner.X = d.Width;
	text_rect.UpperLeftCorner.X -= m_hscroll_pos;
	text_rect.LowerRightCorner.X -= m_hscroll_pos;
	text_rect.LowerRightCorner.Y = text_rect.UpperLeftCorner.Y + d.Height;

	core::rect<s32> frame_rect = getPromptFrameRect();
	text_rect += frame_rect.UpperLeftCorner;

	return text_rect;
}

void GUIChatConsole::calculatePromptScrollPos()
{
	if (!m_font)
		return;

	irr::u32 cursor_width = m_font->getDimension(L"_").Width;
	ChatPrompt& prompt = m_chat_backend->getPrompt();
	std::wstring text = prompt.getLine();
	std::replace_if(text.begin(), text.end(),
			[](wchar_t c) { return (c == L'\n' || c == L'\r'); }, L' ');
	core::stringw txt_line = text.c_str();
	s32 logical_cpos = prompt.getCursorPos();

	core::TextBidiData text_bidi = applyBidiReordering(txt_line);
	s32 rtl_cursor_pos = text_bidi.visualCursorPos(logical_cpos);

	if (text_bidi.CharIsRtl.size() > 0 && text_bidi.CharIsRtl[0] && rtl_cursor_pos > 0)
		rtl_cursor_pos--;

	s32 cstart = m_font->getDimension(text_bidi.TextBidi.subString(0, rtl_cursor_pos).c_str()).Width;
	s32 cend = cstart + cursor_width;
	s32 txt_width = m_font->getDimension(text_bidi.TextBidi.c_str()).Width;

	core::rect<s32> frame_rect = getPromptFrameRect();

	if (txt_width < frame_rect.getWidth()) {
		m_hscroll_pos = 0;
	}

	core::rect<s32> text_rect = getPromptTextRect();

	if (text_rect.UpperLeftCorner.X + cstart < frame_rect.UpperLeftCorner.X) {
		m_hscroll_pos -= frame_rect.UpperLeftCorner.X - (text_rect.UpperLeftCorner.X + cstart);
	} else if (text_rect.UpperLeftCorner.X + cend > frame_rect.LowerRightCorner.X)	{
		m_hscroll_pos += (text_rect.UpperLeftCorner.X + cend) - frame_rect.LowerRightCorner.X;
	}
}

void GUIChatConsole::drawPrompt()
{
	if (!m_font)
		return;

	core::rect<s32> frame_rect = getPromptFrameRect();

	core::rect<s32> local_clip_rect = frame_rect;
	local_clip_rect.clipAgainst(AbsoluteClippingRect);

	ChatPrompt& prompt = m_chat_backend->getPrompt();
	std::wstring text = prompt.getLine();
	std::replace_if(text.begin(), text.end(),
			[](wchar_t c) { return (c == L'\n' || c == L'\r'); }, L' ');

	core::stringw s, s2;

	const core::stringw txt_line = text.c_str();

	if (text.size()) {
		core::rect<s32> text_rect = getPromptTextRect();

		// clipping test - don't draw anything outside the visible area
		core::rect<s32> c = local_clip_rect;
		c.clipAgainst(text_rect);

		if (c.isValid()) {

			core::TextBidiData text_bidi = applyBidiReordering(txt_line);
			core::stringw txt_line_bidi = text_bidi.TextBidi;

			ChatSelection real_mark_begin = m_mark_end > m_mark_begin ? m_mark_begin : m_mark_end;
			ChatSelection real_mark_end = m_mark_end > m_mark_begin ? m_mark_end : m_mark_begin;

			// draw mark and marked text
			if (real_mark_begin != real_mark_end &&
					real_mark_begin.selection_type == ChatSelection::SELECTION_PROMPT &&
					real_mark_end.selection_type == ChatSelection::SELECTION_PROMPT) {

				s32 logical_start = real_mark_begin.scroll + real_mark_begin.character;
				s32 logical_end = real_mark_end.scroll + real_mark_end.character;

				std::vector<SelectionBidiRange> visual_ranges =
						text_bidi.getSelectionRanges(logical_start, logical_end);

				IGUISkin* skin = Environment->getSkin();

				for (const auto& range : visual_ranges) {
					if (!range.Selected)
						continue;

					s32 visual_start = range.Start;
					s32 visual_end = range.End;

					s = txt_line_bidi.subString(0, visual_start);
					s32 mbegin = m_font->getDimension(s.c_str()).Width;

					// deal with kerning
					const wchar_t* this_letter = visual_start < (s32)txt_line_bidi.size() ? &(txt_line_bidi[visual_start]) : 0;
					const wchar_t* previous_letter = visual_start > 0 ? &(txt_line_bidi[visual_start - 1]) : 0;
					mbegin += m_font->getKerningWidth(this_letter, previous_letter);

					s2 = txt_line_bidi.subString(0, visual_end);
					s32 mend = m_font->getDimension(s2.c_str()).Width;

					core::rect<s32> mark_rect = text_rect;
					mark_rect.UpperLeftCorner.X += mbegin;
					mark_rect.LowerRightCorner.X = mark_rect.UpperLeftCorner.X + mend - mbegin;

					// draw mark
					skin->draw2DRectangle(this, skin->getColor(EGDC_HIGH_LIGHT), mark_rect, &local_clip_rect);
				}
			}

			m_font->draw(txt_line_bidi, text_rect,
				video::SColor(255, 255, 255, 255),
				false, true, &local_clip_rect, false);
		}
	}

	// draw cursor
	if ((m_cursor_blink & 0x8000) != 0) {
		core::TextBidiData text_bidi = applyBidiReordering(txt_line);
		s32 rtl_cursor_pos = text_bidi.visualCursorPos(prompt.getCursorPos());

		if (text_bidi.CharIsRtl.size() > 0 && text_bidi.CharIsRtl[0] && rtl_cursor_pos > 0)
			rtl_cursor_pos--;

		s = text_bidi.TextBidi.subString(0, rtl_cursor_pos);

		s32 charcursorpos = m_font->getDimension(s.c_str()).Width +
			m_font->getKerningWidth(L"_", rtl_cursor_pos > 0 ? &(text_bidi.TextBidi[rtl_cursor_pos-1]) : 0);

		core::rect<s32> text_rect = getPromptTextRect();
		text_rect.UpperLeftCorner.X += charcursorpos;

		m_font->draw(L"_", text_rect,
			video::SColor(255, 255, 255, 255),
			false, true, &local_clip_rect, false);
	}
}


ChatSelection GUIChatConsole::getCursorPos(s32 x, s32 y)
{
	ChatSelection selection;

	if (m_font == NULL)
		return selection;

	ChatBuffer& buf = m_chat_backend->getConsoleBuffer();
	selection.scroll = buf.getScrollPos();
	selection.selection_type = ChatSelection::SELECTION_HISTORY;

	s32 line_height = m_fontsize.Y;
	s32 y_min = m_height - m_desired_height;
	s32 y_max = buf.getRows() * line_height + y_min;

	if (y <= y_min) {
		selection.row = 0;
	} else if (y >= y_max) {
		selection.row = buf.getRows() - 1;
	} else {
		for (u32 row = 0; row < buf.getRows(); row++) {
			s32 y1 = row * line_height + m_height - m_desired_height;
			s32 y2 = y1 + line_height;

			if (y1 + line_height < 0)
				return selection;

			if (y >= y1 && y <= y2) {
				selection.row = row;
				break;
			}
		}
	}

	ChatFormattedLine line = buf.getFormattedLine(selection.row);
	selection.line_index = line.line_index;
	int current_row = selection.row;

	while (!line.first) {
		current_row--;
		line = buf.getFormattedLine(current_row);
		selection.line++;
	}

	line = buf.getFormattedLine(selection.row);

	if (line.fragments.empty())
		return selection;

	const ChatFormattedFragment &fragment_first = line.fragments[0];
//	const ChatFormattedFragment &fragment_last = line.fragments[line.fragments.size() - 1];
	s32 x_min = (fragment_first.column + 1) * m_fontsize.X + m_round_screen_offset;
	s32 x_max = x_min;
	for (const ChatFormattedFragment &fragment : line.fragments) {
		x_max += m_font->getDimension(fragment.text.c_str()).Width;
	}

	s32 fragment_x = x_min;
	for (unsigned int i = 0; i < line.fragments.size(); i++) {
		const ChatFormattedFragment &fragment = line.fragments[i];
		s32 current_fragment_x = fragment_x;
		s32 text_size = m_font->getDimension(fragment.text.c_str()).Width;
		fragment_x += text_size;

		if (x < current_fragment_x)
			continue;

		if (i < line.fragments.size() - 1) {
			if (x >= fragment_x)
				continue;
		}
		
		core::stringw frag_text = fragment.text.getString().c_str();
		TextBidiData text_bidi = applyBidiReordering(frag_text);
		
		s32 visual_index = m_font->getCharacterFromPos(text_bidi.TextBidi.c_str(), x - current_fragment_x);
		
		s32 logical_index;
		if (visual_index == -1) {
			logical_index = frag_text.size();
		} else {
			logical_index = text_bidi.logicalCursorPos(visual_index);
			if (logical_index < 0)
				logical_index = 0;
			if (logical_index > (s32)frag_text.size())
				logical_index = frag_text.size();
		}
		
		selection.fragment = i;
		selection.character = logical_index;
		return selection;
	}

	return selection;
}

ChatSelection GUIChatConsole::getPromptCursorPos(s32 x, s32 y)
{
	ChatSelection selection;

	if (m_font == NULL)
		return selection;

	ChatPrompt& prompt = m_chat_backend->getPrompt();
	std::wstring text = prompt.getLine();
	std::replace_if(text.begin(), text.end(),
			[](wchar_t c) { return (c == L'\n' || c == L'\r'); }, L' ');
	core::stringw txt_line = text.c_str();

	if (txt_line.empty())
		return selection;

	x += 3;

	core::rect<s32> text_rect = getPromptTextRect();
	if (y < text_rect.UpperLeftCorner.Y)
		y = text_rect.UpperLeftCorner.Y;
	if (y > text_rect.LowerRightCorner.Y)
		y = text_rect.LowerRightCorner.Y;

	if (x < text_rect.UpperLeftCorner.X)
		x = text_rect.UpperLeftCorner.X;

	core::TextBidiData text_bidi = applyBidiReordering(txt_line);
	s32 visual_pos = m_font->getCharacterFromPos(text_bidi.TextBidi.c_str(),
			x - text_rect.UpperLeftCorner.X);

	s32 logical_pos = text_bidi.logicalCursorPos(visual_pos);
	if (logical_pos < 0)
		logical_pos = 0;
	if (logical_pos > (s32)txt_line.size())
		logical_pos = txt_line.size();

	selection.selection_type = ChatSelection::SELECTION_PROMPT;
	selection.character = logical_pos;

	return selection;
}

ChatSelection GUIChatConsole::getCurrentPromptCursorPos()
{
	ChatSelection selection;

	if (m_font == NULL)
		return selection;

	ChatPrompt& prompt = m_chat_backend->getPrompt();

	selection.selection_type = ChatSelection::SELECTION_PROMPT;
	selection.scroll = 0;
	selection.character = prompt.getCursorPos();

	return selection;
}

irr::core::stringc GUIChatConsole::getSelectedText()
{
	if (m_font == NULL)
		return "";

	if (m_mark_begin == m_mark_end)
		return "";

	bool add_to_string = false;
	irr::core::stringw text = "";

	ChatSelection real_mark_begin = m_mark_end > m_mark_begin ? m_mark_begin : m_mark_end;
	ChatSelection real_mark_end = m_mark_end > m_mark_begin ? m_mark_end : m_mark_begin;

	ChatBuffer& buf = m_chat_backend->getConsoleBuffer();

	const ChatLine& first_line = buf.getLine(0);
	int first_line_index = first_line.line_index;
	int mark_begin_row_buf = real_mark_begin.line_index - first_line_index;
	int mark_end_row_buf = real_mark_end.line_index - first_line_index;

	if (mark_begin_row_buf < 0 || mark_end_row_buf < 0)
		return "";

	for (int row = mark_begin_row_buf; row < mark_end_row_buf + 1; row++) {
		const ChatLine& line = buf.getLine(row);

		std::vector<ChatFormattedLine> formatted_lines;
		buf.formatChatLine(line, buf.getColsCount(), formatted_lines);

		for (unsigned int i = 0; i < formatted_lines.size(); i++) {
			const ChatFormattedLine &line = formatted_lines[i];

			for (unsigned int j = 0; j < line.fragments.size(); j++) {
				const ChatFormattedFragment &fragment = line.fragments[j];

				for (unsigned int k = 0; k <= fragment.text.size(); k++) {
					if (!add_to_string &&
							row == mark_begin_row_buf &&
							i == real_mark_begin.line &&
							j == real_mark_begin.fragment &&
							k == real_mark_begin.character) {
						add_to_string = true;
					}

					if (add_to_string) {
						if (row == mark_end_row_buf &&
								i == real_mark_end.line &&
								j == real_mark_end.fragment &&
								k == real_mark_end.character) {

							irr::core::stringc text_c;
							text_c = wide_to_utf8(text.c_str()).c_str();
							return text_c;
						}

						if (k < fragment.text.size())
							text += fragment.text.c_str()[k];
					}
				}
			}

			if (row < mark_end_row_buf) {
				text += L"\n";
			}
		}
	}

	irr::core::stringc text_c;
	text_c = wide_to_utf8(text.c_str()).c_str();
	return text_c;
}

irr::core::stringc GUIChatConsole::getPromptSelectedText()
{
	if (m_font == NULL)
		return "";

	if (m_mark_begin == m_mark_end)
		return "";

	ChatPrompt& prompt = m_chat_backend->getPrompt();

	ChatSelection real_mark_begin = m_mark_end > m_mark_begin ? m_mark_begin : m_mark_end;
	ChatSelection real_mark_end = m_mark_end > m_mark_begin ? m_mark_end : m_mark_begin;

	std::wstring prompt_text = prompt.getLine();

	if (real_mark_end.scroll + real_mark_end.character > prompt_text.size())
		return "";

	irr::core::stringw text = prompt_text.c_str();
	int begin = real_mark_begin.scroll + real_mark_begin.character;
	int length = real_mark_end.scroll + real_mark_end.character - begin;
	text = text.subString(begin, length);

	irr::core::stringc text_c;
	text_c = wide_to_utf8(text.c_str()).c_str();
	return text_c;
}

void GUIChatConsole::movePromptCursor(s32 x, s32 y)
{
	ChatSelection selection = getPromptCursorPos(x, y);

	int cursor_pos = selection.scroll + selection.character;

	ChatPrompt& prompt = m_chat_backend->getPrompt();
	prompt.setCursorPos(cursor_pos);
}

void GUIChatConsole::deletePromptSelection()
{
	if (m_mark_begin.selection_type != ChatSelection::SELECTION_PROMPT ||
			m_mark_end.selection_type != ChatSelection::SELECTION_PROMPT ||
			m_mark_begin == m_mark_end)
		return;

	ChatPrompt& prompt = m_chat_backend->getPrompt();
	int scroll_pos = prompt.getViewPosition();

	ChatSelection real_mark_begin = m_mark_end > m_mark_begin ? m_mark_begin : m_mark_end;
	ChatSelection real_mark_end = m_mark_end > m_mark_begin ? m_mark_end : m_mark_begin;

	int pos_begin = real_mark_begin.scroll + real_mark_begin.character;
	int pos_end = real_mark_end.scroll + real_mark_end.character;

	std::wstring prompt_text = prompt.getLine();
	std::wstring new_text;
	new_text = prompt_text.substr(0, pos_begin);
	new_text += prompt_text.substr(pos_end, prompt_text.size() - pos_end);

	prompt.replace(new_text);

	int cursor_pos = real_mark_begin.scroll + real_mark_begin.character;
	prompt.setCursorPos(cursor_pos);
	prompt.setViewPosition(scroll_pos);

	m_mark_begin.reset();
	m_mark_end.reset();
}

bool GUIChatConsole::OnEvent(const SEvent& event)
{
	ChatPrompt &prompt = m_chat_backend->getPrompt();

	if(event.EventType == EET_KEY_INPUT_EVENT && event.KeyInput.PressedDown)
	{
		// Key input
		if (KeyPress(event.KeyInput) == getKeySetting("keymap_console")) {
			closeConsole();

			// inhibit open so the_game doesn't reopen immediately
			m_open_inhibited = 50;
			m_close_on_enter = false;
			return true;
		}

		if (event.KeyInput.Key == KEY_ESCAPE || event.KeyInput.Key == KEY_CANCEL) {
			closeConsole();
			m_close_on_enter = false;
			// inhibit open so the_game doesn't reopen immediately
			m_open_inhibited = 1; // so the ESCAPE button doesn't open the "pause menu"
			return true;
		}
		else if(event.KeyInput.Key == KEY_PRIOR)
		{
			ChatBuffer& buf = m_chat_backend->getConsoleBuffer();
			s32 rows = -(s32)buf.getRows();
			m_vscrollbar->setPos(m_vscrollbar->getPos() + rows);
			m_chat_backend->scrollPageUp();

			calculatePromptScrollPos();

			return true;
		}
		else if(event.KeyInput.Key == KEY_NEXT)
		{
			ChatBuffer& buf = m_chat_backend->getConsoleBuffer();
			s32 rows = buf.getRows();
			m_vscrollbar->setPos(m_vscrollbar->getPos() + rows);
			m_chat_backend->scrollPageDown();

			calculatePromptScrollPos();

			return true;
		}
		else if(event.KeyInput.Key == KEY_RETURN)
		{
			prompt.addToHistory(prompt.getLine());
			std::wstring text = prompt.replace(L"");
			m_client->typeChatMessage(text);
			if (m_close_on_enter) {
				closeConsole();
				m_close_on_enter = false;
			} else {
				updateVScrollBar(true, true);
			}

			calculatePromptScrollPos();

			return true;
		}
		else if(event.KeyInput.Key == KEY_UP)
		{
			// Up pressed
			// Move back in history
			prompt.historyPrev();
			calculatePromptScrollPos();
			return true;
		}
		else if(event.KeyInput.Key == KEY_DOWN)
		{
			// Down pressed
			// Move forward in history
			prompt.historyNext();
			calculatePromptScrollPos();
			return true;
		}
		else if(event.KeyInput.Key == KEY_LEFT || event.KeyInput.Key == KEY_RIGHT)
		{
			ChatSelection old_pos = getCurrentPromptCursorPos();

			// Left/right pressed
			// Move/select character/word to the left depending on control and shift keys
			ChatPrompt::CursorOp op =  ChatPrompt::CURSOROP_MOVE;
			ChatPrompt::CursorOpDir dir = event.KeyInput.Key == KEY_LEFT ?
				ChatPrompt::CURSOROP_DIR_LEFT :
				ChatPrompt::CURSOROP_DIR_RIGHT;
			ChatPrompt::CursorOpScope scope = event.KeyInput.Control ?
				ChatPrompt::CURSOROP_SCOPE_WORD :
				ChatPrompt::CURSOROP_SCOPE_CHARACTER;
			prompt.cursorOperation(op, dir, scope);

			if (event.KeyInput.Shift) {
				if (m_mark_begin.selection_type != ChatSelection::SELECTION_PROMPT ||
						m_mark_end.selection_type != ChatSelection::SELECTION_PROMPT) {
					m_mark_begin = old_pos;
				}
				m_mark_end = getCurrentPromptCursorPos();
			} else {
				if (m_mark_begin.selection_type == ChatSelection::SELECTION_PROMPT &&
						m_mark_end.selection_type == ChatSelection::SELECTION_PROMPT) {
					m_mark_begin.reset();
					m_mark_end.reset();
				}
			}

			calculatePromptScrollPos();

			return true;
		}
		else if(event.KeyInput.Key == KEY_HOME)
		{
			ChatSelection old_pos = getCurrentPromptCursorPos();

			// Home pressed
			// move to beginning of line
			prompt.cursorOperation(
				ChatPrompt::CURSOROP_MOVE,
				ChatPrompt::CURSOROP_DIR_LEFT,
				ChatPrompt::CURSOROP_SCOPE_LINE);

			if (event.KeyInput.Shift) {
				if (m_mark_begin.selection_type != ChatSelection::SELECTION_PROMPT ||
						m_mark_end.selection_type != ChatSelection::SELECTION_PROMPT) {
					m_mark_begin = old_pos;
				}
				m_mark_end = getCurrentPromptCursorPos();
			} else {
				if (m_mark_begin.selection_type == ChatSelection::SELECTION_PROMPT &&
						m_mark_end.selection_type == ChatSelection::SELECTION_PROMPT) {
					m_mark_begin.reset();
					m_mark_end.reset();
				}
			}

			calculatePromptScrollPos();

			return true;
		}
		else if(event.KeyInput.Key == KEY_END)
		{
			ChatSelection old_pos = getCurrentPromptCursorPos();

			// End pressed
			// move to end of line
			prompt.cursorOperation(
				ChatPrompt::CURSOROP_MOVE,
				ChatPrompt::CURSOROP_DIR_RIGHT,
				ChatPrompt::CURSOROP_SCOPE_LINE);

			if (event.KeyInput.Shift) {
				if (m_mark_begin.selection_type != ChatSelection::SELECTION_PROMPT ||
						m_mark_end.selection_type != ChatSelection::SELECTION_PROMPT) {
					m_mark_begin = old_pos;
				}
				m_mark_end = getCurrentPromptCursorPos();
			} else {
				if (m_mark_begin.selection_type == ChatSelection::SELECTION_PROMPT &&
						m_mark_end.selection_type == ChatSelection::SELECTION_PROMPT) {
					m_mark_begin.reset();
					m_mark_end.reset();
				}
			}

			calculatePromptScrollPos();

			return true;
		}
		else if(event.KeyInput.Key == KEY_BACK)
		{
			if (m_mark_begin.selection_type == ChatSelection::SELECTION_PROMPT &&
					m_mark_end.selection_type == ChatSelection::SELECTION_PROMPT &&
					m_mark_begin != m_mark_end) {
				deletePromptSelection();
				calculatePromptScrollPos();
				return true;
			}

			// Backspace or Ctrl-Backspace pressed
			// delete character / word to the left
			ChatPrompt::CursorOpScope scope =
				event.KeyInput.Control ?
				ChatPrompt::CURSOROP_SCOPE_WORD :
				ChatPrompt::CURSOROP_SCOPE_CHARACTER;
			prompt.cursorOperation(
				ChatPrompt::CURSOROP_DELETE,
				ChatPrompt::CURSOROP_DIR_LEFT,
				scope);

			calculatePromptScrollPos();

			return true;
		}
		else if(event.KeyInput.Key == KEY_DELETE)
		{
			if (m_mark_begin.selection_type == ChatSelection::SELECTION_PROMPT &&
					m_mark_end.selection_type == ChatSelection::SELECTION_PROMPT &&
					m_mark_begin != m_mark_end) {
				deletePromptSelection();
				calculatePromptScrollPos();
				return true;
			}

			// Delete or Ctrl-Delete pressed
			// delete character / word to the right
			ChatPrompt::CursorOpScope scope =
				event.KeyInput.Control ?
				ChatPrompt::CURSOROP_SCOPE_WORD :
				ChatPrompt::CURSOROP_SCOPE_CHARACTER;
			prompt.cursorOperation(
				ChatPrompt::CURSOROP_DELETE,
				ChatPrompt::CURSOROP_DIR_RIGHT,
				scope);

			calculatePromptScrollPos();

			return true;
		}
		else if(event.KeyInput.Key == KEY_KEY_A && event.KeyInput.Control)
		{
			if (prompt.getLine().size() > 0) {
				ChatPrompt& prompt = m_chat_backend->getPrompt();

				m_mark_begin.reset();
				m_mark_begin.selection_type = ChatSelection::SELECTION_PROMPT;
				m_mark_begin.scroll = 0;
				m_mark_begin.character = 0;

				m_mark_end.reset();
				m_mark_end.selection_type = ChatSelection::SELECTION_PROMPT;
				m_mark_end.scroll = 0;
				m_mark_end.character = prompt.getLine().size();

				prompt.cursorOperation(
					ChatPrompt::CURSOROP_MOVE,
					ChatPrompt::CURSOROP_DIR_RIGHT,
					ChatPrompt::CURSOROP_SCOPE_LINE);
			}

			calculatePromptScrollPos();

			return true;
		}
		else if(event.KeyInput.Key == KEY_KEY_C && event.KeyInput.Control)
		{
			if (m_mark_begin != m_mark_end) {
				if (m_mark_begin.selection_type == ChatSelection::SELECTION_PROMPT &&
						m_mark_end.selection_type == ChatSelection::SELECTION_PROMPT) {
					irr::core::stringc text = getPromptSelectedText();
					Environment->getOSOperator()->copyToClipboard(text.c_str());
					return true;
				} else if (m_mark_begin.selection_type == ChatSelection::SELECTION_HISTORY &&
						m_mark_end.selection_type == ChatSelection::SELECTION_HISTORY) {
					irr::core::stringc text = getSelectedText();
					Environment->getOSOperator()->copyToClipboard(text.c_str());
					return true;
				}
			}

			// Ctrl-C pressed
			// Copy text to clipboard
			if (prompt.getCursorLength() <= 0)
				return true;
			std::wstring wselected = prompt.getSelection();
			std::string selected = wide_to_utf8(wselected);
			Environment->getOSOperator()->copyToClipboard(selected.c_str());
			return true;
		}
		else if(event.KeyInput.Key == KEY_KEY_V && event.KeyInput.Control)
		{
			if (m_mark_begin.selection_type == ChatSelection::SELECTION_PROMPT &&
					m_mark_end.selection_type == ChatSelection::SELECTION_PROMPT &&
					m_mark_begin != m_mark_end) {
				deletePromptSelection();
			}

			// Ctrl-V pressed
			// paste text from clipboard
			if (prompt.getCursorLength() > 0) {
				// Delete selected section of text
				prompt.cursorOperation(
					ChatPrompt::CURSOROP_DELETE,
					ChatPrompt::CURSOROP_DIR_LEFT, // Ignored
					ChatPrompt::CURSOROP_SCOPE_SELECTION);
			}
			calculatePromptScrollPos();
			IOSOperator *os_operator = Environment->getOSOperator();
			const c8 *text = os_operator->getTextFromClipboard();
			if (!text)
				return true;
			prompt.input(utf8_to_wide(text));
			calculatePromptScrollPos();
			return true;
		}
		else if(event.KeyInput.Key == KEY_KEY_X && event.KeyInput.Control)
		{
			if (m_mark_begin.selection_type == ChatSelection::SELECTION_PROMPT &&
					m_mark_end.selection_type == ChatSelection::SELECTION_PROMPT &&
					m_mark_begin != m_mark_end) {
				irr::core::stringc text = getPromptSelectedText();
				Environment->getOSOperator()->copyToClipboard(text.c_str());
				deletePromptSelection();
				calculatePromptScrollPos();
				return true;
			}

			// Ctrl-X pressed
			// Cut text to clipboard
			if (prompt.getCursorLength() <= 0)
				return true;
			std::wstring wselected = prompt.getSelection();
			std::string selected = wide_to_utf8(wselected);
			Environment->getOSOperator()->copyToClipboard(selected.c_str());
			prompt.cursorOperation(
				ChatPrompt::CURSOROP_DELETE,
				ChatPrompt::CURSOROP_DIR_LEFT, // Ignored
				ChatPrompt::CURSOROP_SCOPE_SELECTION);

			calculatePromptScrollPos();

			return true;
		}
		else if(event.KeyInput.Key == KEY_KEY_U && event.KeyInput.Control)
		{
			// Ctrl-U pressed
			// kill line to left end
			prompt.cursorOperation(
				ChatPrompt::CURSOROP_DELETE,
				ChatPrompt::CURSOROP_DIR_LEFT,
				ChatPrompt::CURSOROP_SCOPE_LINE);

			calculatePromptScrollPos();

			return true;
		}
		else if(event.KeyInput.Key == KEY_KEY_K && event.KeyInput.Control)
		{
			// Ctrl-K pressed
			// kill line to right end
			prompt.cursorOperation(
				ChatPrompt::CURSOROP_DELETE,
				ChatPrompt::CURSOROP_DIR_RIGHT,
				ChatPrompt::CURSOROP_SCOPE_LINE);

			calculatePromptScrollPos();

			return true;
		}
		else if(event.KeyInput.Key == KEY_TAB)
		{
			// Tab or Shift-Tab pressed
			// Nick completion
			std::list<std::string> names = m_client->getConnectedPlayerNames();
			bool backwards = event.KeyInput.Shift;
			prompt.nickCompletion(names, backwards);
			return true;
		} else if (!iswcntrl(event.KeyInput.Char) && !event.KeyInput.Control) {
			if (m_mark_begin.selection_type == ChatSelection::SELECTION_PROMPT &&
					m_mark_end.selection_type == ChatSelection::SELECTION_PROMPT &&
					m_mark_begin != m_mark_end) {
				deletePromptSelection();
			}

			#if defined(__linux__) && (IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR < 9)
				wchar_t wc = L'_';
				mbtowc( &wc, (char *) &event.KeyInput.Char, sizeof(event.KeyInput.Char) );
				prompt.input(wc);
			#else
				prompt.input(event.KeyInput.Char);
			#endif

			calculatePromptScrollPos();

			return true;
		}
	}
#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
	else if(event.EventType == EET_SDL_TEXT_EVENT)
	{
		if (event.SDLTextEvent.Type == ESDLET_TEXTINPUT)
		{
			if (m_mark_begin.selection_type == ChatSelection::SELECTION_PROMPT &&
					m_mark_end.selection_type == ChatSelection::SELECTION_PROMPT &&
					m_mark_begin != m_mark_end) {
				deletePromptSelection();
			}

			if (event.SDLTextEvent.Text) {
				std::wstring text = utf8_to_wide(event.SDLTextEvent.Text);

				for (u32 i = 0; i < text.size(); i++)
					prompt.input(text[i]);
			}

		}

		calculatePromptScrollPos();

		return true;
	}
#endif
	else if(event.EventType == EET_MOUSE_INPUT_EVENT)
	{
		if(event.MouseInput.Event == EMIE_MOUSE_WHEEL)
		{
			s32 rows = myround(-3.0 * event.MouseInput.Wheel);
			m_chat_backend->scroll(rows);
			m_vscrollbar->setPos(m_vscrollbar->getPos() + rows);
		} else if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN) {
			if (event.MouseInput.X <= m_screensize.X - m_round_screen_offset &&
					event.MouseInput.X >= m_round_screen_offset) {
				u32 row = m_chat_backend->getConsoleBuffer().getRows();
				s32 prompt_y = row * m_fontsize.Y + m_height - m_desired_height;

				if (event.MouseInput.Y >= prompt_y) {
					m_prompt_marking = true;
					if (event.MouseInput.Shift) {
						if (m_mark_begin.selection_type != ChatSelection::SELECTION_PROMPT ||
								m_mark_end.selection_type != ChatSelection::SELECTION_PROMPT) {
							m_mark_begin = getCurrentPromptCursorPos();
							m_mark_end = getPromptCursorPos(event.MouseInput.X, event.MouseInput.Y);
						}
					} else {
						m_mark_begin = getPromptCursorPos(event.MouseInput.X, event.MouseInput.Y);
						m_mark_end = m_mark_begin;
					}
					movePromptCursor(event.MouseInput.X, event.MouseInput.Y);
				} else {
					m_history_marking = true;
					m_mark_begin = getCursorPos(event.MouseInput.X, event.MouseInput.Y);
					m_mark_end = m_mark_begin;
				}
			}
		} else if (event.MouseInput.Event == EMIE_LMOUSE_LEFT_UP) {
			if (m_prompt_marking) {
				m_mark_end = getPromptCursorPos(event.MouseInput.X, event.MouseInput.Y);
				m_prompt_marking = false;

				if (!event.MouseInput.Shift) {
					if (m_mark_begin == m_mark_end) {
						m_mark_begin.reset();
						m_mark_end.reset();
					}
				}
			} else if (m_history_marking) {
				m_mark_end = getCursorPos(event.MouseInput.X, event.MouseInput.Y);
				m_history_marking = false;

				if (m_mark_begin == m_mark_end) {
					m_mark_begin.reset();
					m_mark_end.reset();
				}
			}
		} else if (event.MouseInput.Event == EMIE_MOUSE_MOVED) {
			if (m_prompt_marking) {
				m_mark_end = getPromptCursorPos(event.MouseInput.X, event.MouseInput.Y);
				movePromptCursor(event.MouseInput.X, event.MouseInput.Y);
			} else if (m_history_marking) {
				m_mark_end = getCursorPos(event.MouseInput.X, event.MouseInput.Y);
			}
		}

		calculatePromptScrollPos();

		return true;
	}
#ifdef HAVE_TOUCHSCREENGUI
	else if (event.EventType == EET_TOUCH_INPUT_EVENT) {
		if (event.TouchInput.Event == irr::ETIE_PRESSED_DOWN) {
			if (event.TouchInput.X <= m_screensize.X - m_round_screen_offset &&
					event.TouchInput.X >= m_round_screen_offset) {
				m_history_marking = false;
				m_prompt_marking = false;
				m_long_press = false;

				u32 row = m_chat_backend->getConsoleBuffer().getRows();
				s32 prompt_y = row * m_fontsize.Y + m_height - m_desired_height;

				ChatSelection real_mark_begin = m_mark_end > m_mark_begin ? m_mark_begin : m_mark_end;
				ChatSelection real_mark_end = m_mark_end > m_mark_begin ? m_mark_end : m_mark_begin;

				if (event.TouchInput.Y >= prompt_y) {

					m_cursor_press_pos = getPromptCursorPos(event.TouchInput.X, event.TouchInput.Y);

					if (real_mark_begin.selection_type != ChatSelection::SELECTION_PROMPT ||
							real_mark_end.selection_type != ChatSelection::SELECTION_PROMPT ||
							m_cursor_press_pos < real_mark_begin ||
							m_cursor_press_pos > real_mark_end) {
						m_mark_begin = m_cursor_press_pos;
						m_mark_end = m_cursor_press_pos;
						m_prompt_marking = true;
					}
				} else {
					m_cursor_press_pos = getCursorPos(event.TouchInput.X, event.TouchInput.Y);

					if (real_mark_begin.selection_type != ChatSelection::SELECTION_HISTORY ||
							real_mark_end.selection_type != ChatSelection::SELECTION_HISTORY ||
							m_cursor_press_pos < real_mark_begin ||
							m_cursor_press_pos > real_mark_end) {
						m_mark_begin = m_cursor_press_pos;
						m_mark_end = m_cursor_press_pos;
						m_history_marking = true;
					}
				}
			}

		} else if (event.TouchInput.Event == irr::ETIE_LEFT_UP) {
			if (m_prompt_marking) {
				ChatSelection cursor_pos = getPromptCursorPos(event.TouchInput.X, event.TouchInput.Y);

				if (!m_long_press && m_cursor_press_pos == cursor_pos) {
					m_mark_begin.reset();
					m_mark_end.reset();
				}

				m_prompt_marking = false;
			} else if (m_history_marking) {
				ChatSelection cursor_pos = getCursorPos(event.TouchInput.X, event.TouchInput.Y);

				if (!m_long_press && m_cursor_press_pos == cursor_pos) {
					m_mark_begin.reset();
					m_mark_end.reset();
				}

				m_history_marking = false;
			}

			m_cursor_press_pos.reset();
			m_long_press = false;

		} else if (event.TouchInput.Event == irr::ETIE_MOVED) {
			ChatSelection cursor_pos = getCursorPos(event.TouchInput.X, event.TouchInput.Y);
			ChatSelection prompt_cursor_pos = getPromptCursorPos(event.TouchInput.X, event.TouchInput.Y);

			if (!m_prompt_marking && !m_long_press &&
					m_cursor_press_pos.selection_type == ChatSelection::SELECTION_PROMPT &&
					m_cursor_press_pos != prompt_cursor_pos) {
				m_mark_begin = m_cursor_press_pos;
				m_mark_end = m_cursor_press_pos;
				m_prompt_marking = true;
			} else if (!m_history_marking && !m_long_press &&
					m_cursor_press_pos.selection_type == ChatSelection::SELECTION_HISTORY &&
					m_cursor_press_pos != cursor_pos) {
				m_mark_begin = m_cursor_press_pos;
				m_mark_end = m_cursor_press_pos;
				m_history_marking = true;
			}

			if (m_prompt_marking) {
				m_mark_end = prompt_cursor_pos;
			} else if (m_history_marking) {
				m_mark_end = cursor_pos;
			}

		} else if (event.TouchInput.Event == irr::ETIE_PRESSED_LONG) {
			if (event.TouchInput.X <= m_screensize.X - m_round_screen_offset &&
					event.TouchInput.X >= m_round_screen_offset) {
				if (!m_history_marking && ! m_prompt_marking) {
					m_long_press = true;
					if (m_mark_begin != m_mark_end) {
						irr::core::stringc text;
						if (m_mark_begin.selection_type == ChatSelection::SELECTION_PROMPT &&
								m_mark_end.selection_type == ChatSelection::SELECTION_PROMPT) {
							text = getPromptSelectedText();
						} else if (m_mark_begin.selection_type == ChatSelection::SELECTION_HISTORY &&
								m_mark_end.selection_type == ChatSelection::SELECTION_HISTORY) {
							text = getSelectedText();
						}
						Environment->getOSOperator()->copyToClipboard(text.c_str());
#if defined(__ANDROID__) || defined(__IOS__)
						porting::showToast("Copied to clipboard");
#endif
					}
				}
			}
		}

		return true;
	}
#endif
	else if (event.EventType == EET_GUI_EVENT) {
		if (event.GUIEvent.EventType == EGET_SCROLL_BAR_CHANGED) {
			updateVScrollBar();
		}
	}

	return Parent ? Parent->OnEvent(event) : false;
}

void GUIChatConsole::setVisible(bool visible)
{
	m_open = visible;
	IGUIElement::setVisible(visible);
	m_vscrollbar->setVisible(visible);
	if (!visible) {
		m_height = 0;
		recalculateConsolePosition();
	}
}

//! create a vertical scroll bar
void GUIChatConsole::createVScrollBar()
{
	IGUISkin *skin = 0;
	if (Environment)
		skin = Environment->getSkin();

	m_scrollbar_width = skin ? skin->getSize(gui::EGDS_SCROLLBAR_SIZE) : 16;
	m_scrollbar_width *= 2;

	u32 scrollbar_x = m_screensize.X - m_round_screen_offset;
	irr::core::rect<s32> scrollbarrect(scrollbar_x - m_scrollbar_width,
			0, scrollbar_x, m_height);
	m_vscrollbar = new GUIScrollBar(Environment, getParent(), -1,
			scrollbarrect, false, true);

	m_vscrollbar->setVisible(false);
	m_vscrollbar->setMax(0);
	m_vscrollbar->setPos(0);
	m_vscrollbar->setPageSize(0);
	m_vscrollbar->setSmallStep(1);
	m_vscrollbar->setLargeStep(1);
	m_vscrollbar->setArrowsVisible(GUIScrollBar::ArrowVisibility::SHOW);

	ITextureSource *tsrc = m_client->getTextureSource();
	m_vscrollbar->setTextures({
		tsrc->getTexture("gui/scrollbar_bg.png"),
		tsrc->getTexture("gui/scrollbar_slider_middle.png"),
		tsrc->getTexture("gui/scrollbar_up.png"),
		tsrc->getTexture("gui/scrollbar_down.png"),
		tsrc->getTexture("gui/scrollbar_slider_top.png"),
		tsrc->getTexture("gui/scrollbar_slider_bottom.png"),
	});

	addChild(m_vscrollbar);
}

void GUIChatConsole::updateVScrollBar(bool force_update, bool move_bottom)
{
	if (!m_vscrollbar)
		return;

	ChatBuffer& buf = m_chat_backend->getConsoleBuffer();

	if (m_bottom_scroll_pos != buf.getBottomScrollPos() || force_update) {
		bool is_bottom = m_vscrollbar->getPos() == m_bottom_scroll_pos;
		m_bottom_scroll_pos = buf.getBottomScrollPos();

		if (buf.getBottomScrollPos() > 0) {
			buf.scrollAbsolute(m_bottom_scroll_pos);
			m_vscrollbar->setMax(m_bottom_scroll_pos);
			if (is_bottom || move_bottom)
				m_vscrollbar->setPos(m_bottom_scroll_pos);
		} else {
			m_vscrollbar->setMax(0);
			m_vscrollbar->setPos(0);
		}
	}

	s32 page_size = (m_bottom_scroll_pos + buf.getRows() + 1) * m_fontsize.Y;
	if (m_vscrollbar->getPageSize() != page_size) {
		m_vscrollbar->setPageSize(page_size);
	}

	if (buf.getDelFormatted() > 0) {
		bool is_bottom = m_vscrollbar->getPos() == m_bottom_scroll_pos;

		if (!is_bottom && ! move_bottom) {
			s32 pos = m_vscrollbar->getPos() - buf.getDelFormatted();

			if (pos >= 0)
				m_vscrollbar->setPos(pos);
		}

		m_mark_begin.scroll -= buf.getDelFormatted();
		m_mark_end.scroll -= buf.getDelFormatted();
		buf.resetDelFormatted();
	}

	if (m_vscrollbar->getPos() != buf.getScrollPos()) {
		if (buf.getScrollPos() >= 0) {
			s32 deltaScrollY = m_vscrollbar->getPos() - buf.getScrollPos();
			m_chat_backend->scroll(deltaScrollY);
		}
	}

	if (IsVisible) {
		if (m_vscrollbar->isVisible() && m_vscrollbar->getMax() == 0)
			m_vscrollbar->setVisible(false);
		else if (!m_vscrollbar->isVisible() && m_vscrollbar->getMax() > 0)
			m_vscrollbar->setVisible(true);
	}
}

void GUIChatConsole::onLinesModified()
{
	if (m_mark_begin.selection_type == ChatSelection::SELECTION_HISTORY &&
			m_mark_end.selection_type == ChatSelection::SELECTION_HISTORY) {

		ChatBuffer& buf = m_chat_backend->getConsoleBuffer();
		const ChatLine& first_line = buf.getLine(0);
		int first_line_index = first_line.line_index;

		if (m_mark_begin.line_index < first_line_index ||
				m_mark_end.line_index < first_line_index) {
			m_mark_begin.reset();
			m_mark_end.reset();
			m_cursor_press_pos.reset();
			m_history_marking = false;
			m_long_press = false;
		}
	}

	updateVScrollBar(true);
}

void GUIChatConsole::onPromptModified()
{
	if (m_mark_begin.selection_type == ChatSelection::SELECTION_PROMPT)
		m_mark_begin.reset();
	if (m_mark_end.selection_type == ChatSelection::SELECTION_PROMPT)
		m_mark_end.reset();
	if (m_cursor_press_pos.selection_type == ChatSelection::SELECTION_PROMPT)
		m_cursor_press_pos.reset();
	if (m_prompt_marking) {
		m_prompt_marking = false;
		m_long_press = false;
	}
}

bool GUIChatConsole::hasFocus()
{
	if (Environment->hasFocus(this))
		return true;

	if (Environment->hasFocus(m_vscrollbar))
		return true;

	const core::list<gui::IGUIElement*> &children = m_vscrollbar->getChildren();

	for (gui::IGUIElement *it : children) {
		if (Environment->hasFocus(it))
			return true;
	}

	return false;
}

bool GUIChatConsole::convertToMouseEvent(
		SEvent &mouse_event, SEvent touch_event) const noexcept
{
#ifdef HAVE_TOUCHSCREENGUI
	mouse_event = {};
	mouse_event.EventType = EET_MOUSE_INPUT_EVENT;
	mouse_event.MouseInput.X = touch_event.TouchInput.X;
	mouse_event.MouseInput.Y = touch_event.TouchInput.Y;
	switch (touch_event.TouchInput.Event) {
	case ETIE_PRESSED_DOWN:
		mouse_event.MouseInput.Event = EMIE_LMOUSE_PRESSED_DOWN;
		mouse_event.MouseInput.ButtonStates = EMBSM_LEFT;
		break;
	case ETIE_MOVED:
		mouse_event.MouseInput.Event = EMIE_MOUSE_MOVED;
		mouse_event.MouseInput.ButtonStates = EMBSM_LEFT;
		break;
	case ETIE_LEFT_UP:
		mouse_event.MouseInput.Event = EMIE_LMOUSE_LEFT_UP;
		mouse_event.MouseInput.ButtonStates = 0;
		break;
	default:
		return false;
	}

	return true;
#else

	return false;
#endif
}

bool GUIChatConsole::preprocessEvent(SEvent event)
{
	updateVScrollBar();

#ifdef HAVE_TOUCHSCREENGUI
	if (event.EventType == irr::EET_TOUCH_INPUT_EVENT) {
		const core::position2di p(event.TouchInput.X, event.TouchInput.Y);

		u32 row = m_chat_backend->getConsoleBuffer().getRows();
		s32 prompt_y = row * m_fontsize.Y + m_height - m_desired_height;

		if (m_vscrollbar->isPointInside(p) || !isPointInside(p)) {
			SEvent mouse_event = {};
			bool success = convertToMouseEvent(mouse_event, event);
			if (success) {
				Environment->postEventFromUser(mouse_event);
			}
		}
#if defined(__ANDROID__) || defined(__IOS__)
		else if (!porting::hasRealKeyboard() &&
				event.TouchInput.Y >= prompt_y &&
				event.TouchInput.Y <= m_height) {
			if (event.TouchInput.Event == ETIE_PRESSED_DOWN &&
					!porting::isInputDialogActive()) {
			//	ChatPrompt& prompt = m_chat_backend->getPrompt();
				porting::showInputDialog("", "", 2, "chat");
			}
		}
#endif
		else {
			OnEvent(event);
		}

		return true;
	}
#endif

	return false;
}
