// Copyright (C) 2002-2012 Nikolaus Gebhardt
// Modified by Mustapha T.
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "guiEditBoxWithScrollbar.h"

#include "IGUISkin.h"
#include "IGUIEnvironment.h"
#include "IGUIFont.h"
#include "IVideoDriver.h"
#include "rect.h"
#include "porting.h"
#include "Keycodes.h"
#include "bidi.h"

/*
todo:
optional scrollbars [done]
ctrl+left/right to select word
double click/ctrl click: word select + drag to select whole words, triple click to select line
optional? dragging selected text
numerical
*/

//! constructor
GUIEditBoxWithScrollBar::GUIEditBoxWithScrollBar(const wchar_t* text, bool border,
	IGUIEnvironment* environment, IGUIElement* parent, s32 id,
	const core::rect<s32>& rectangle, bool writable, bool has_vscrollbar,
	ISoundManager *sound_manager)
	: GUIEditBox(environment, parent, id, rectangle, border, writable),
	m_background(true), m_bg_color_used(false), m_sound_manager(sound_manager)
{
#ifdef _DEBUG
	setDebugName("GUIEditBoxWithScrollBar");
#endif


	Text = text;

	if (Environment)
		m_operator = Environment->getOSOperator();

	if (m_operator)
		m_operator->grab();

	// this element can be tabbed to
	setTabStop(true);
	setTabOrder(-1);

	if (has_vscrollbar) {
		createVScrollBar();
	}

	calculateFrameRect();
	breakText();

	calculateScrollPos();
	setWritable(writable);
}

//! Sets whether to draw the background
void GUIEditBoxWithScrollBar::setDrawBackground(bool draw)
{
	m_background = draw;
}


void GUIEditBoxWithScrollBar::updateAbsolutePosition()
{
	core::rect<s32> old_absolute_rect(AbsoluteRect);
	IGUIElement::updateAbsolutePosition();
	if (old_absolute_rect != AbsoluteRect) {
		calculateFrameRect();
		breakText();
		calculateScrollPos();
	}
}


//! draws the element and its children
void GUIEditBoxWithScrollBar::draw()
{
	if (!IsVisible)
		return;

	const bool focus = Environment->hasFocus(this);
	const bool scollbar_focus = Environment->hasFocus(m_vscrollbar);

	IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;

	video::SColor default_bg_color;
	video::SColor bg_color;

	default_bg_color = m_writable ? skin->getColor(EGDC_WINDOW) : video::SColor(0);
	bg_color = m_bg_color_used ? m_bg_color : default_bg_color;

	if (IsEnabled && m_writable) {
		bg_color = focus ? skin->getColor(EGDC_FOCUSED_EDITABLE) : skin->getColor(EGDC_EDITABLE);
	}

	if (!m_border && m_background) {
		skin->draw2DRectangle(this, bg_color, AbsoluteRect, &AbsoluteClippingRect);
	}

	// draw the border

	if (m_border) {

		if (m_writable) {
			skin->draw3DSunkenPane(this, bg_color, false, m_background,
				AbsoluteRect, &AbsoluteClippingRect);
		}

		calculateFrameRect();
	}

	core::rect<s32> local_clip_rect = m_frame_rect;
	local_clip_rect.clipAgainst(AbsoluteClippingRect);

	// draw the text

	IGUIFont* font = getActiveFont();

	if (font) {
		if (m_last_break_font != font) {
			breakText();
		}

		const core::stringw *txt_line = &Text;
		s32 start_pos = 0;

		core::stringw s, s2;

		// get mark position
		const bool ml = (!m_passwordbox && (m_word_wrap || m_multiline));
		const s32 hline_start = ml ? getLineFromPos(m_real_mark_begin) : 0;
		const s32 hline_count = ml ? getLineFromPos(m_real_mark_end) - hline_start + 1 : 1;
		const s32 line_count = ml ? m_broken_text.size() : 1;

		// Save the override color information.
		// Then, alter it if the edit box is disabled.
		const bool prevOver = m_override_color_enabled;
		const video::SColor prevColor = m_override_color;

		if (Text.size()) {
			if (!isEnabled() && !m_override_color_enabled) {
				m_override_color_enabled = true;
				m_override_color = skin->getColor(EGDC_GRAY_TEXT);
			}

			for (s32 i = 0; i < line_count; ++i) {
				setTextRect(i);

				// clipping test - don't draw anything outside the visible area
				core::rect<s32> c = local_clip_rect;
				c.clipAgainst(m_current_text_rect);
				if (!c.isValid())
					continue;

				// get current line
				if (m_passwordbox) {
					if (m_broken_text.size() != 1) {
						m_broken_text.clear();
						m_broken_text.emplace_back();
					}
					if (m_broken_text[0].size() != Text.size()){
						m_broken_text[0] = Text;
						for (u32 q = 0; q < Text.size(); ++q)
						{
							m_broken_text[0][q] = m_passwordchar;
						}
					}
					txt_line = &m_broken_text[0];
					start_pos = 0;
				} else {
					txt_line = ml ? &m_broken_text[i] : &Text;
					start_pos = ml ? m_broken_text_positions[i] : 0;
				}

				core::TextBidiData text_bidi = applyBidiReordering(*txt_line);
				core::stringw txt_line_bidi = text_bidi.TextBidi;

				// draw mark and marked text
				if ((focus || scollbar_focus) && m_mark_begin != m_mark_end && i >= hline_start && i < hline_start + hline_count) {
					s32 logical_start_in_line = 0;
					s32 logical_end_in_line = txt_line_bidi.size();
					
					if (i == hline_start) {
						logical_start_in_line = m_real_mark_begin - start_pos;
					}
				
					if (i == hline_start + hline_count - 1) {
						logical_end_in_line = m_real_mark_end - start_pos;
					}
					
					logical_start_in_line = core::max_(logical_start_in_line, 0);
					logical_end_in_line = core::min_(logical_end_in_line, (s32)txt_line_bidi.size());
					
					std::vector<core::SelectionBidiRange> visual_ranges = 
							text_bidi.getSelectionRanges(logical_start_in_line, logical_end_in_line);
					
					for (const auto& range : visual_ranges) {
						if (!range.Selected)
							continue;

						s32 visual_start = range.Start;
						s32 visual_end = range.End;
						
						s = txt_line_bidi.subString(0, visual_start);
						s32 mbegin = font->getDimension(s.c_str(), false).Width;
						
						// deal with kerning
						const wchar_t* this_letter = visual_start < (s32)txt_line_bidi.size() ? &(txt_line_bidi[visual_start]) : 0;
						const wchar_t* previous_letter = visual_start > 0 ? &(txt_line_bidi[visual_start - 1]) : 0;
						mbegin += font->getKerningWidth(this_letter, previous_letter);
						
						s2 = txt_line_bidi.subString(0, visual_end);
						s32 mend = font->getDimension(s2.c_str(), false).Width;
						
						core::rect<s32> mark_rect = m_current_text_rect;
						mark_rect.UpperLeftCorner.X += mbegin;
						mark_rect.LowerRightCorner.X = mark_rect.UpperLeftCorner.X + mend - mbegin;
						
						// draw mark
						skin->draw2DRectangle(this, skin->getColor(EGDC_HIGH_LIGHT), mark_rect, &local_clip_rect);
					}
				}

				// draw normal text
				font->draw(txt_line_bidi, m_current_text_rect,
					m_override_color_enabled ? m_override_color : skin->getColor(EGDC_BUTTON_TEXT),
					false, true, &local_clip_rect, false);
			}

			// Return the override color information to its previous settings.
			m_override_color_enabled = prevOver;
			m_override_color = prevColor;
		}

		// draw cursor
		if (IsEnabled && m_writable) {
			s32 cursor_line = 0;
			s32 charcursorpos = 0;

			if (m_word_wrap || m_multiline) {
				cursor_line = getLineFromPos(m_cursor_pos);
				txt_line = &m_broken_text[cursor_line];
				start_pos = m_broken_text_positions[cursor_line];
			}

			core::TextBidiData text_bidi = applyBidiReordering(*txt_line);
			s32 rtl_cursor_pos = text_bidi.visualCursorPos(m_cursor_pos - start_pos);

			if (text_bidi.CharIsRtl.size() > 0 && text_bidi.CharIsRtl[0] && rtl_cursor_pos > 0)
				rtl_cursor_pos--;

			s = text_bidi.TextBidi.subString(0, rtl_cursor_pos);

			charcursorpos = font->getDimension(s.c_str(), false).Width +
				font->getKerningWidth(L"_", rtl_cursor_pos > 0 ? &(text_bidi.TextBidi[rtl_cursor_pos-1]) : 0);

			if (focus && (porting::getTimeMs() - m_blink_start_time) % 700 < 350) {
				setTextRect(cursor_line);
				m_current_text_rect.UpperLeftCorner.X += charcursorpos;

				font->draw(L"_", m_current_text_rect,
					m_override_color_enabled ? m_override_color : skin->getColor(EGDC_BUTTON_TEXT),
					false, true, &local_clip_rect, false);
			}
		}
	}

	// draw children
	IGUIElement::draw();
}


s32 GUIEditBoxWithScrollBar::getCursorPos(s32 x, s32 y)
{
	IGUIFont* font = getActiveFont();

	const u32 line_count = (m_word_wrap || m_multiline) ? m_broken_text.size() : 1;

	core::stringw *txt_line = 0;
	s32 start_pos = 0;
	x += 3;

	for (u32 i = 0; i < line_count; ++i) {
		setTextRect(i);
		if (i == 0 && y < m_current_text_rect.UpperLeftCorner.Y)
			y = m_current_text_rect.UpperLeftCorner.Y;
		if (i == line_count - 1 && y > m_current_text_rect.LowerRightCorner.Y)
			y = m_current_text_rect.LowerRightCorner.Y;

		// is it inside this region?
		if (y >= m_current_text_rect.UpperLeftCorner.Y && y <= m_current_text_rect.LowerRightCorner.Y) {
			// we've found the clicked line
			txt_line = (m_word_wrap || m_multiline) ? &m_broken_text[i] : &Text;
			start_pos = (m_word_wrap || m_multiline) ? m_broken_text_positions[i] : 0;
			break;
		}
	}

	if (x < m_current_text_rect.UpperLeftCorner.X)
		x = m_current_text_rect.UpperLeftCorner.X;

	if (!txt_line)
		return 0;

	core::TextBidiData text_bidi = applyBidiReordering(*txt_line);
	s32 visual_pos = font->getCharacterFromPos(text_bidi.TextBidi.c_str(), x - m_current_text_rect.UpperLeftCorner.X);

	s32 logical_pos = text_bidi.logicalCursorPos(visual_pos);
	if (logical_pos < 0)
		logical_pos = 0;
	if (logical_pos > (s32)txt_line->size())
		logical_pos = txt_line->size();

	return logical_pos + start_pos;
}


//! Breaks the single text line.
void GUIEditBoxWithScrollBar::breakText()
{
	if ((!m_word_wrap && !m_multiline))
		return;

	m_broken_text.clear(); // need to reallocate :/
	m_broken_text_positions.clear();

	IGUIFont* font = getActiveFont();
	if (!font)
		return;

	m_last_break_font = font;

	core::stringw line;
	core::stringw word;
	core::stringw whitespace;
	s32 last_line_start = 0;
	s32 size = Text.size();
	s32 length = 0;
	s32 el_width = RelativeRect.getWidth() - m_scrollbar_width - 10;
	wchar_t c;

	for (s32 i = 0; i < size; ++i) {
		c = Text[i];
		bool line_break = false;

		if (c == L'\r') { // Mac or Windows breaks

			line_break = true;
			c = 0;
			if (Text[i + 1] == L'\n') { // Windows breaks
				// TODO: I (Michael) think that we shouldn't change the text given by the user for whatever reason.
				// Instead rework the cursor positioning to be able to handle this (but not in stable release
				// branch as users might already expect this behavior).
				Text.erase(i + 1);
				--size;
				if (m_cursor_pos > i)
					--m_cursor_pos;
			}
		} else if (c == L'\n') { // Unix breaks
			line_break = true;
			c = 0;
		}

		// don't break if we're not a multi-line edit box
		if (!m_multiline)
			line_break = false;

		if (c == L' ' || c == 0 || i == (size - 1)) {
			// here comes the next whitespace, look if
			// we can break the last word to the next line
			// We also break whitespace, otherwise cursor would vanish beside the right border.
			s32 whitelgth = font->getDimension(whitespace.c_str()).Width;
			s32 worldlgth = font->getDimension(word.c_str()).Width;

			if (m_word_wrap && length + worldlgth + whitelgth > el_width && line.size() > 0) {
				// break to next line
				length = worldlgth;
				m_broken_text.push_back(line);
				m_broken_text_positions.push_back(last_line_start);
				last_line_start = i - (s32)word.size();
				line = word;
			} else {
				// add word to line
				line += whitespace;
				line += word;
				length += whitelgth + worldlgth;
			}

			word = L"";
			whitespace = L"";


			if (c)
				whitespace += c;

			// compute line break
			if (line_break) {
				line += whitespace;
				line += word;
				m_broken_text.push_back(line);
				m_broken_text_positions.push_back(last_line_start);
				last_line_start = i + 1;
				line = L"";
				word = L"";
				whitespace = L"";
				length = 0;
			}
		} else {
			// yippee this is a word..
			word += c;
		}
	}

	line += whitespace;
	line += word;
	m_broken_text.push_back(line);
	m_broken_text_positions.push_back(last_line_start);
}

// TODO: that function does interpret VAlign according to line-index (indexed
// line is placed on top-center-bottom) but HAlign according to line-width
// (pixels) and not by row.
// Intuitively I suppose HAlign handling is better as VScrollPos should handle
// the line-scrolling.
// But please no one change this without also rewriting (and this time
// testing!!!) autoscrolling (I noticed this when fixing the old autoscrolling).
void GUIEditBoxWithScrollBar::setTextRect(s32 line)
{
	if (line < 0)
		return;

	IGUIFont* font = getActiveFont();
	if (!font)
		return;

	core::dimension2du d;

	// get text dimension
	const u32 line_count = (m_word_wrap || m_multiline) ? m_broken_text.size() : 1;
	if (m_word_wrap || m_multiline) {
		d = font->getDimension(m_broken_text[line].c_str());
	} else {
		d = font->getDimension(Text.c_str());
		d.Height = AbsoluteRect.getHeight();
	}
	d.Height += font->getKerningHeight();

	// justification
	switch (m_halign) {
	case EGUIA_CENTER:
		// align to h centre
		m_current_text_rect.UpperLeftCorner.X = (m_frame_rect.getWidth() / 2) - (d.Width / 2);
		m_current_text_rect.LowerRightCorner.X = (m_frame_rect.getWidth() / 2) + (d.Width / 2);
		break;
	case EGUIA_LOWERRIGHT:
		// align to right edge
		m_current_text_rect.UpperLeftCorner.X = m_frame_rect.getWidth() - d.Width;
		m_current_text_rect.LowerRightCorner.X = m_frame_rect.getWidth();
		break;
	default:
		// align to left edge
		m_current_text_rect.UpperLeftCorner.X = 0;
		m_current_text_rect.LowerRightCorner.X = d.Width;

	}

	switch (m_valign) {
	case EGUIA_CENTER:
		// align to v centre
		m_current_text_rect.UpperLeftCorner.Y =
			(m_frame_rect.getHeight() / 2) - (line_count*d.Height) / 2 + d.Height*line;
		break;
	case EGUIA_LOWERRIGHT:
		// align to bottom edge
		m_current_text_rect.UpperLeftCorner.Y =
			m_frame_rect.getHeight() - line_count*d.Height + d.Height*line;
		break;
	default:
		// align to top edge
		m_current_text_rect.UpperLeftCorner.Y = d.Height*line;
		break;
	}

	m_current_text_rect.UpperLeftCorner.X -= m_hscroll_pos;
	m_current_text_rect.LowerRightCorner.X -= m_hscroll_pos;
	m_current_text_rect.UpperLeftCorner.Y -= m_vscroll_pos;
	m_current_text_rect.LowerRightCorner.Y = m_current_text_rect.UpperLeftCorner.Y + d.Height;

	m_current_text_rect += m_frame_rect.UpperLeftCorner;
}

// calculate autoscroll
void GUIEditBoxWithScrollBar::calculateScrollPos()
{
	if (!m_autoscroll)
		return;

	IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;
	IGUIFont* font = m_override_font ? m_override_font : skin->getFont();
	if (!font)
		return;

	s32 curs_line = getLineFromPos(m_cursor_pos);
	if (curs_line < 0)
		return;
	setTextRect(curs_line);
	const bool has_broken_text = m_multiline || m_word_wrap;

	// Check horizonal scrolling
	// NOTE: Calculations different to vertical scrolling because setTextRect interprets VAlign relative to line but HAlign not relative to row
	{
		// get cursor position
		IGUIFont* font = getActiveFont();
		if (!font)
			return;

		// get cursor area
		irr::u32 cursor_width = font->getDimension(L"_").Width;
		core::stringw *txt_line = has_broken_text ? &m_broken_text[curs_line] : &Text;
		s32 logical_cpos = has_broken_text ? m_cursor_pos - m_broken_text_positions[curs_line] : m_cursor_pos;

		core::TextBidiData text_bidi = applyBidiReordering(*txt_line);
		s32 rtl_cursor_pos = text_bidi.visualCursorPos(logical_cpos);

		if (text_bidi.CharIsRtl.size() > 0 && text_bidi.CharIsRtl[0] && rtl_cursor_pos > 0)
			rtl_cursor_pos--;

		s32 cstart = font->getDimension(text_bidi.TextBidi.subString(0, rtl_cursor_pos).c_str()).Width;
		s32 cend = cstart + cursor_width;
		s32 txt_width = font->getDimension(text_bidi.TextBidi.c_str()).Width;

		if (txt_width < m_frame_rect.getWidth()) {
			// TODO: Needs a clean left and right gap removal depending on HAlign, similar to vertical scrolling tests for top/bottom.
			// This check just fixes the case where it was most noticable (text smaller than clipping area).

			m_hscroll_pos = 0;
			setTextRect(curs_line);
		}

		if (m_current_text_rect.UpperLeftCorner.X + cstart < m_frame_rect.UpperLeftCorner.X) {
			// cursor to the left of the clipping area
			m_hscroll_pos -= m_frame_rect.UpperLeftCorner.X - (m_current_text_rect.UpperLeftCorner.X + cstart);
			setTextRect(curs_line);

			// TODO: should show more characters to the left when we're scrolling left
			//	and the cursor reaches the border.
		} else if (m_current_text_rect.UpperLeftCorner.X + cend > m_frame_rect.LowerRightCorner.X)	{
			// cursor to the right of the clipping area
			m_hscroll_pos += (m_current_text_rect.UpperLeftCorner.X + cend) - m_frame_rect.LowerRightCorner.X;
			setTextRect(curs_line);
		}
	}

	// calculate vertical scrolling
	if (has_broken_text) {
		irr::u32 line_height = font->getDimension(L"A").Height + font->getKerningHeight();
		// only up to 1 line fits?
		if (line_height >= (irr::u32)m_frame_rect.getHeight()) {
			m_vscroll_pos = 0;
			setTextRect(curs_line);
			s32 unscrolledPos = m_current_text_rect.UpperLeftCorner.Y;
			s32 pivot = m_frame_rect.UpperLeftCorner.Y;
			switch (m_valign) {
			case EGUIA_CENTER:
				pivot += m_frame_rect.getHeight() / 2;
				unscrolledPos += line_height / 2;
				break;
			case EGUIA_LOWERRIGHT:
				pivot += m_frame_rect.getHeight();
				unscrolledPos += line_height;
				break;
			default:
				break;
			}
			m_vscroll_pos = unscrolledPos - pivot;
			setTextRect(curs_line);
		} else {
			// First 2 checks are necessary when people delete lines
			setTextRect(0);
			if (m_current_text_rect.UpperLeftCorner.Y > m_frame_rect.UpperLeftCorner.Y && m_valign != EGUIA_LOWERRIGHT) {
				// first line is leaving a gap on top
				m_vscroll_pos = 0;
			} else if (m_valign != EGUIA_UPPERLEFT) {
				u32 lastLine = m_broken_text_positions.empty() ? 0 : m_broken_text_positions.size() - 1;
				setTextRect(lastLine);
				if (m_current_text_rect.LowerRightCorner.Y < m_frame_rect.LowerRightCorner.Y)
				{
					// last line is leaving a gap on bottom
					m_vscroll_pos -= m_frame_rect.LowerRightCorner.Y - m_current_text_rect.LowerRightCorner.Y;
				}
			}

			setTextRect(curs_line);
			if (m_current_text_rect.UpperLeftCorner.Y < m_frame_rect.UpperLeftCorner.Y) {
				// text above valid area
				m_vscroll_pos -= m_frame_rect.UpperLeftCorner.Y - m_current_text_rect.UpperLeftCorner.Y;
				setTextRect(curs_line);
			} else if (m_current_text_rect.LowerRightCorner.Y > m_frame_rect.LowerRightCorner.Y){
				// text below valid area
				m_vscroll_pos += m_current_text_rect.LowerRightCorner.Y - m_frame_rect.LowerRightCorner.Y;
				setTextRect(curs_line);
			}
		}
	}

	if (m_vscrollbar) {
		m_vscrollbar->setPos(m_vscroll_pos);
	}
}

void GUIEditBoxWithScrollBar::calculateFrameRect()
{
	m_frame_rect = AbsoluteRect;


	IGUISkin *skin = 0;
	if (Environment)
		skin = Environment->getSkin();
	if (m_border && skin) {
		m_frame_rect.UpperLeftCorner.X += skin->getSize(EGDS_TEXT_DISTANCE_X) + 1;
		m_frame_rect.UpperLeftCorner.Y += skin->getSize(EGDS_TEXT_DISTANCE_Y) + 1;
		m_frame_rect.LowerRightCorner.X -= skin->getSize(EGDS_TEXT_DISTANCE_X) + 1;
		m_frame_rect.LowerRightCorner.Y -= skin->getSize(EGDS_TEXT_DISTANCE_Y) + 1;
	}

	updateVScrollBar();
}

//! create a vertical scroll bar
void GUIEditBoxWithScrollBar::createVScrollBar()
{
	IGUISkin *skin = 0;
	if (Environment)
		skin = Environment->getSkin();

	m_scrollbar_width = skin ? skin->getSize(gui::EGDS_SCROLLBAR_SIZE) : 16;

	irr::core::rect<s32> scrollbarrect = m_frame_rect;
	scrollbarrect.UpperLeftCorner.X += m_frame_rect.getWidth() - m_scrollbar_width;
	m_vscrollbar = new GUIScrollBar(Environment, getParent(), -1,
			scrollbarrect, false, true, m_sound_manager);

	m_vscrollbar->setVisible(false);
	m_vscrollbar->setSmallStep(1);
	m_vscrollbar->setLargeStep(1);
}



//! Change the background color
void GUIEditBoxWithScrollBar::setBackgroundColor(const video::SColor &bg_color)
{
	m_bg_color = bg_color;
	m_bg_color_used = true;
}

//! Writes attributes of the element.
void GUIEditBoxWithScrollBar::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options = 0) const
{
	out->addBool("Border", m_border);
	out->addBool("Background", m_background);
	// out->addFont("OverrideFont", OverrideFont);

	GUIEditBox::serializeAttributes(out, options);
}


//! Reads attributes of the element
void GUIEditBoxWithScrollBar::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options = 0)
{
	GUIEditBox::deserializeAttributes(in, options);

	setDrawBorder(in->getAttributeAsBool("Border"));
	setDrawBackground(in->getAttributeAsBool("Background"));
}

bool GUIEditBoxWithScrollBar::isDrawBackgroundEnabled() const { return false; }
bool GUIEditBoxWithScrollBar::isDrawBorderEnabled() const { return false; }
void GUIEditBoxWithScrollBar::setCursorChar(const wchar_t cursorChar) { }
wchar_t GUIEditBoxWithScrollBar::getCursorChar() const { return '|'; }
void GUIEditBoxWithScrollBar::setCursorBlinkTime(irr::u32 timeMs) { }
irr::u32 GUIEditBoxWithScrollBar::getCursorBlinkTime() const { return 500; }
