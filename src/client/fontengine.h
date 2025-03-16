/*
Minetest
Copyright (C) 2010-2014 sapier <sapier at gmx dot net>

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

#pragma once

#include <map>
#include <vector>
#include "util/basic_macros.h"
#include "irrlichttypes.h"
#include <IGUIFont.h>
#include <IGUISkin.h>
#include <IGUIEnvironment.h>
#include "settings.h"

#define FONT_SIZE_UNSPECIFIED 0xFFFFFFFF

enum FontMode : u8 {
	FM_Standard = 0,
	FM_Mono,
	FM_Fallback,
	FM_Simple,
	FM_SimpleMono,
	FM_MaxMode,
	FM_Unspecified
};

struct FontSpec {
	FontSpec(unsigned int font_size, FontMode mode, bool bold, bool italic) :
		size(font_size),
		mode(mode),
		bold(bold),
		italic(italic) {}

	u16 getHash()
	{
		return (mode << 2) | (static_cast<u8>(bold) << 1) | static_cast<u8>(italic);
	}

	unsigned int size;
	FontMode mode;
	bool bold;
	bool italic;
};

class FontEngine
{
public:

	FontEngine(gui::IGUIEnvironment* env);

	~FontEngine();

	// Get best possible font specified by FontSpec
	irr::gui::IGUIFont *getFont(FontSpec spec);

	irr::gui::IGUIFont *getFont(unsigned int font_size=FONT_SIZE_UNSPECIFIED,
			FontMode mode=FM_Unspecified)
	{
		FontSpec spec(font_size, mode, m_default_bold, m_default_italic);
		return getFont(spec);
	}

	/** get text height for a specific font */
	unsigned int getTextHeight(const FontSpec &spec);

	/** get text width if a text for a specific font */
	unsigned int getTextHeight(
			unsigned int font_size=FONT_SIZE_UNSPECIFIED,
			FontMode mode=FM_Unspecified)
	{
		FontSpec spec(font_size, mode, m_default_bold, m_default_italic);
		return getTextHeight(spec);
	}

	unsigned int getTextWidth(const std::wstring &text, const FontSpec &spec);

	/** get text width if a text for a specific font */
	unsigned int getTextWidth(const std::wstring& text,
			unsigned int font_size=FONT_SIZE_UNSPECIFIED,
			FontMode mode=FM_Unspecified)
	{
		FontSpec spec(font_size, mode, m_default_bold, m_default_italic);
		return getTextWidth(text, spec);
	}

	unsigned int getTextWidth(const std::string &text, const FontSpec &spec)
	{
		return getTextWidth(utf8_to_wide(text), spec);
	}

	unsigned int getTextWidth(const std::string& text,
			unsigned int font_size=FONT_SIZE_UNSPECIFIED,
			FontMode mode=FM_Unspecified)
	{
		FontSpec spec(font_size, mode, m_default_bold, m_default_italic);
		return getTextWidth(utf8_to_wide(text), spec);
	}

	/** get line height for a specific font (including empty room between lines) */
	unsigned int getLineHeight(const FontSpec &spec);

	unsigned int getLineHeight(unsigned int font_size=FONT_SIZE_UNSPECIFIED,
			FontMode mode=FM_Unspecified)
	{
		FontSpec spec(font_size, mode, m_default_bold, m_default_italic);
		return getLineHeight(spec);
	}

	/** get default font size */
	unsigned int getDefaultFontSize();

	/** get font size for a specific mode */
	unsigned int getFontSize(FontMode mode);

	/** update internal parameters from settings */
	void readSettings();

	/** reload fonts if settings were changed */
	void handleReload();

private:
	/** update content of font cache in case of a setting change made it invalid */
	void updateFontCache();

	/** initialize a new font */
	gui::IGUIFont *initFont(const FontSpec &spec);

	/** initialize a font without freetype */
	gui::IGUIFont *initSimpleFont(const FontSpec &spec);

	/** update current minetest skin with font changes */
	void updateSkin();

	/** clean cache */
	void cleanCache();

	/** callback to be used on change of font size setting */
	static void fontSettingChanged(const std::string &name, void *userdata);

	/** pointer to irrlicht gui environment */
	gui::IGUIEnvironment* m_env = nullptr;

	/** internal storage for caching fonts of different size */
	std::map<unsigned int, irr::gui::IGUIFont*> m_font_cache[FM_MaxMode << 2];

	/** default font size to use */
	unsigned int m_default_size[FM_MaxMode];

	/** default bold and italic */
	bool m_default_bold = false;
	bool m_default_italic = false;

	/** current font engine mode */
	FontMode m_currentMode = FM_Standard;

	bool m_needs_reload = false;

	DISABLE_CLASS_COPY(FontEngine);
};

/** interface to access main font engine*/
extern FontEngine* g_fontengine;
