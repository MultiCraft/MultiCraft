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

#include "fontengine.h"
#include <cmath>
#include "client/renderingengine.h"
#include "config.h"
#include "porting.h"
#include "filesys.h"
#include "gettext.h"

#if USE_FREETYPE
#include "irrlicht_changes/CGUITTFont.h"
#endif

/** maximum size distance for getting a "similar" font size */
#define MAX_FONT_SIZE_OFFSET 10

/** reference to access font engine, has to be initialized by main */
FontEngine* g_fontengine = NULL;

/** callback to be used on change of font size setting */
void FontEngine::fontSettingChanged(const std::string &name, void *userdata)
{
	((FontEngine *)userdata)->m_needs_reload = true;
}

/******************************************************************************/
FontEngine::FontEngine(gui::IGUIEnvironment* env) :
	m_env(env)
{

	for (u32 &i : m_default_size) {
		i = (FontMode) FONT_SIZE_UNSPECIFIED;
	}

	assert(g_settings != NULL); // pre-condition
	assert(m_env != NULL); // pre-condition
	assert(m_env->getSkin() != NULL); // pre-condition

	readSettings();

	if (m_currentMode == FM_Standard) {
		g_settings->registerChangedCallback("font_size", fontSettingChanged, this);
		g_settings->registerChangedCallback("font_bold", fontSettingChanged, this);
		g_settings->registerChangedCallback("font_italic", fontSettingChanged, this);
		g_settings->registerChangedCallback("font_path", fontSettingChanged, this);
		g_settings->registerChangedCallback("font_path_bold", fontSettingChanged, this);
		g_settings->registerChangedCallback("font_path_italic", fontSettingChanged, this);
		g_settings->registerChangedCallback("font_path_bolditalic", fontSettingChanged, this);
		g_settings->registerChangedCallback("font_shadow", fontSettingChanged, this);
		g_settings->registerChangedCallback("font_shadow_alpha", fontSettingChanged, this);
	}
	else if (m_currentMode == FM_Fallback) {
		g_settings->registerChangedCallback("font_size", fontSettingChanged, this); // fallback_font_size
		g_settings->registerChangedCallback("fallback_font_path", fontSettingChanged, this);
		g_settings->registerChangedCallback("fallback_font_shadow", fontSettingChanged, this);
		g_settings->registerChangedCallback("fallback_font_shadow_alpha", fontSettingChanged, this);
	}

	g_settings->registerChangedCallback("mono_font_path", fontSettingChanged, this);
	g_settings->registerChangedCallback("mono_font_size", fontSettingChanged, this);
	g_settings->registerChangedCallback("screen_dpi", fontSettingChanged, this);
	g_settings->registerChangedCallback("gui_scaling", fontSettingChanged, this);
}

/******************************************************************************/
FontEngine::~FontEngine()
{
	cleanCache();
}

/******************************************************************************/
void FontEngine::cleanCache()
{
	for (auto &font_cache_it : m_font_cache) {

		for (auto &font_it : font_cache_it) {
			font_it.second->drop();
			font_it.second = NULL;
		}
		font_cache_it.clear();
	}
}

/******************************************************************************/
irr::gui::IGUIFont *FontEngine::getFont(FontSpec spec)
{
	if (spec.mode == FM_Unspecified) {
		spec.mode = m_currentMode;
	} else if (m_currentMode == FM_Simple) {
		// Freetype disabled -> Force simple mode
		spec.mode = (spec.mode == FM_Mono ||
				spec.mode == FM_SimpleMono) ?
				FM_SimpleMono : FM_Simple;
		// Support for those could be added, but who cares?
		//spec.bold = false;
		//spec.italic = false;
	}

	// Fallback to default size
	if (spec.size == FONT_SIZE_UNSPECIFIED)
		spec.size = m_default_size[spec.mode];

	const auto &cache = m_font_cache[spec.getHash()];
	auto it = cache.find(spec.size);
	if (it != cache.end())
		return it->second;

	// Font does not yet exist
	gui::IGUIFont *font = nullptr;
	if (spec.mode == FM_Simple || spec.mode == FM_SimpleMono)
		font = initSimpleFont(spec);
	else
		font = initFont(spec);

	m_font_cache[spec.getHash()][spec.size] = font;

	return font;
}

/******************************************************************************/
unsigned int FontEngine::getTextHeight(const FontSpec &spec)
{
	irr::gui::IGUIFont *font = getFont(spec);

	// use current skin font as fallback
	if (font == NULL) {
		font = m_env->getSkin()->getFont();
	}
	FATAL_ERROR_IF(font == NULL, "Could not get skin font");

	return font->getDimension(L"Some unimportant example String").Height;
}

/******************************************************************************/
unsigned int FontEngine::getTextWidth(const std::wstring &text, const FontSpec &spec)
{
	irr::gui::IGUIFont *font = getFont(spec);

	// use current skin font as fallback
	if (font == NULL) {
		font = m_env->getSkin()->getFont();
	}
	FATAL_ERROR_IF(font == NULL, "Could not get font");

	return font->getDimension(text.c_str()).Width;
}


/** get line height for a specific font (including empty room between lines) */
unsigned int FontEngine::getLineHeight(const FontSpec &spec)
{
	irr::gui::IGUIFont *font = getFont(spec);

	// use current skin font as fallback
	if (font == NULL) {
		font = m_env->getSkin()->getFont();
	}
	FATAL_ERROR_IF(font == NULL, "Could not get font");

	return font->getDimension(L"Some unimportant example String").Height
			+ font->getKerningHeight();
}

/******************************************************************************/
unsigned int FontEngine::getDefaultFontSize()
{
	return m_default_size[m_currentMode];
}

unsigned int FontEngine::getFontSize(FontMode mode)
{
	if (m_currentMode == FM_Simple) {
		if (mode == FM_Mono || mode == FM_SimpleMono)
			return m_default_size[FM_SimpleMono];
		else
			return m_default_size[FM_Simple];
	}

	if (mode == FM_Unspecified)
		return m_default_size[FM_Standard];

	return m_default_size[mode];
}

/******************************************************************************/
void FontEngine::readSettings()
{
	if (USE_FREETYPE && g_settings->getBool("freetype")) {
		m_default_size[FM_Standard] = g_settings->getU16("font_size");
		m_default_size[FM_Fallback] = g_settings->getU16("font_size"); // fallback_font_size
		m_default_size[FM_Mono]     = g_settings->getU16("mono_font_size");

		/*~ DO NOT TRANSLATE THIS LITERALLY!
		This is a special string. Put either "no" or "yes"
		into the translation field (literally).
		Choose "yes" if the language requires use of the fallback
		font, "no" otherwise.
		The fallback font is (normally) required for languages with
		non-Latin script, like Chinese.
		When in doubt, test your translation. */
		//m_currentMode = is_yes(gettext("needs_fallback_font")) ?
		//		FM_Fallback : FM_Standard;

		// Never use FM_Fallback mode because the fallback font is loaded anyway
		m_currentMode = FM_Standard;

		m_default_bold = g_settings->getBool("font_bold");
		m_default_italic = g_settings->getBool("font_italic");

	} else {
		m_currentMode = FM_Simple;
	}

	m_default_size[FM_Simple]       = g_settings->getU16("font_size");
	m_default_size[FM_SimpleMono]   = g_settings->getU16("mono_font_size");

	cleanCache();
	updateFontCache();
	updateSkin();
}

/******************************************************************************/
void FontEngine::handleReload()
{
	if (!m_needs_reload)
		return;

	m_needs_reload = false;
	readSettings();
}

/******************************************************************************/
void FontEngine::updateSkin()
{
	gui::IGUIFont *font = getFont();

	if (font)
		m_env->getSkin()->setFont(font);
	else
		errorstream << "FontEngine: Default font file: " <<
				"\n\t\"" << g_settings->get("font_path") << "\"" <<
				"\n\trequired for current screen configuration was not found" <<
				" or was invalid file format." <<
				"\n\tUsing irrlicht default font." << std::endl;

	// If we did fail to create a font our own make irrlicht find a default one
	font = m_env->getSkin()->getFont();
	FATAL_ERROR_IF(font == NULL, "Could not create/get font");

	u32 text_height = font->getDimension(L"Hello, world!").Height;
	infostream << "FontEngine: measured text_height=" << text_height << std::endl;
}

/******************************************************************************/
void FontEngine::updateFontCache()
{
	/* the only font to be initialized is default one,
	 * all others are re-initialized on demand */
	getFont(FONT_SIZE_UNSPECIFIED, FM_Unspecified);
}

/******************************************************************************/
gui::IGUIFont *FontEngine::initFont(const FontSpec &spec)
{
	assert(spec.mode != FM_Unspecified);
	assert(spec.size != FONT_SIZE_UNSPECIFIED);

	std::string setting_prefix = "";

	switch (spec.mode) {
		case FM_Fallback:
			setting_prefix = "fallback_";
			break;
		case FM_Mono:
		case FM_SimpleMono:
			setting_prefix = "mono_";
			break;
		default:
			break;
	}

	std::string setting_suffix = "";
	bool bold_outline = false;
	bool italic_outline = false;

	std::string bold_path = g_settings->get(setting_prefix + "font_path_bold");
	std::string italic_path = g_settings->get(setting_prefix + "font_path_italic");
	std::string bold_italic_path = g_settings->get(setting_prefix + "font_path_bold_italic");

	if (spec.bold && spec.italic) {
		if (bold_italic_path.empty()) {
			bold_outline = true;
			italic_outline = true;
		} else {
			setting_suffix.append("_bold_italic");
		}
	} else if (spec.bold) {
		if (bold_path.empty())
			bold_outline = true;
		else
			setting_suffix.append("_bold");
	} else if (spec.italic) {
		if (italic_path.empty())
			italic_outline = true;
		else
			setting_suffix.append("_italic");
	}

	u32 size = std::max(std::floor(RenderingEngine::getDisplayDensity() *
			g_settings->getFloat("gui_scaling") * spec.size), 1.0f);

	/*if (size == 0) {
		errorstream << "FontEngine: attempt to use font size 0" << std::endl;
		errorstream << "  display density: " << RenderingEngine::getDisplayDensity() << std::endl;
		abort();
	}*/

	u16 font_shadow       = 0;
	u16 font_shadow_alpha = 0;
	g_settings->getU16NoEx(setting_prefix + "font_shadow", font_shadow);
	g_settings->getU16NoEx(setting_prefix + "font_shadow_alpha",
			font_shadow_alpha);

	std::string wanted_font_path;
	wanted_font_path = g_settings->get(setting_prefix + "font_path" + setting_suffix);
	std::string fallback_font_path = g_settings->get("fallback_font_path");

	std::string fallback_settings[] = {
		wanted_font_path,
		fallback_font_path,
		Settings::getLayer(SL_DEFAULTS)->get(setting_prefix + "font_path")
	};

	std::string emoji_font_path = g_settings->get("emoji_font_path");
	std::string emoji_font_system_paths = g_settings->get("emoji_font_system_paths");

#if USE_FREETYPE
	for (const std::string &font_path : fallback_settings) {
		irr::gui::CGUITTFont *font = gui::CGUITTFont::createTTFont(m_env,
				font_path.c_str(), size, true, true, bold_outline, italic_outline,
				font_shadow, font_shadow_alpha);

		if (font) {
			std::vector<std::string> emoji_paths = split(emoji_font_system_paths, ',');
			bool success = false;

			// Load first available system emoji font
			for (std::string path : emoji_paths) {
				if (!path.empty() && fs::PathExists(path)) {
					success = font->loadAdditionalFont(path.c_str(), true);
					if (success)
						break;
				}
			}

			// Load fallback emoji font if system fonts are not available
			if (!success)
				font->loadAdditionalFont(emoji_font_path.c_str(), true);

			if (font_path != fallback_font_path)
				font->loadAdditionalFont(fallback_font_path.c_str(), false);

			return font;
		}

		errorstream << "FontEngine: Cannot load '" << font_path <<
				"'. Trying to fall back to another path." << std::endl;
	}


	// give up
	std::string msg = "MultiCraft can not continue without a valid font. "
			"Please correct the 'font_path' setting or install the font "
			"file in the proper location";
	errorstream << msg << std::endl;

#if !defined(__ANDROID__) && !defined(__IOS__)
	abort();
#else
	porting::finishGame(msg);
#endif

#else
	errorstream << "FontEngine: Tried to load freetype fonts but MultiCraft was"
			" not compiled with that library." << std::endl;
	abort();
#endif
}

/** initialize a font without freetype */
gui::IGUIFont *FontEngine::initSimpleFont(const FontSpec &spec)
{
	assert(spec.mode == FM_Simple || spec.mode == FM_SimpleMono);
	assert(spec.size != FONT_SIZE_UNSPECIFIED);

	const std::string &font_path = g_settings->get(
			(spec.mode == FM_SimpleMono) ? "mono_font_path" : "font_path");

	size_t pos_dot = font_path.find_last_of('.');
	std::string basename = font_path;
	std::string ending = lowercase(font_path.substr(pos_dot));

	if (ending == ".ttf") {
		errorstream << "FontEngine: Found font \"" << font_path
				<< "\" but freetype is not available." << std::endl;
		return nullptr;
	}

	if (ending == ".xml" || ending == ".png")
		basename = font_path.substr(0, pos_dot);

	u32 size = std::floor(
			RenderingEngine::getDisplayDensity() *
			g_settings->getFloat("gui_scaling") *
			spec.size);

	irr::gui::IGUIFont *font = nullptr;
	std::string font_extensions[] = { ".png", ".xml" };

	// Find nearest matching font scale
	// Does a "zig-zag motion" (positibe/negative), from 0 to MAX_FONT_SIZE_OFFSET
	for (s32 zoffset = 0; zoffset < MAX_FONT_SIZE_OFFSET * 2; zoffset++) {
		std::stringstream path;

		// LSB to sign
		s32 sign = (zoffset & 1) ? -1 : 1;
		s32 offset = zoffset >> 1;

		for (const std::string &ext : font_extensions) {
			path.str(""); // Clear
			path << basename << "_" << (size + offset * sign) << ext;

			if (!fs::PathExists(path.str()))
				continue;

			font = m_env->getFont(path.str().c_str());

			if (font) {
				verbosestream << "FontEngine: found font: " << path.str() << std::endl;
				break;
			}
		}

		if (font)
			break;
	}

	// try name direct
	if (font == NULL) {
		if (fs::PathExists(font_path)) {
			font = m_env->getFont(font_path.c_str());
			if (font)
				verbosestream << "FontEngine: found font: " << font_path << std::endl;
		}
	}

	return font;
}
