/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2018 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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

#include "gameui.h"
#include <irrlicht_changes/static_text.h>
#include <gettext.h>
#include "gui/mainmenumanager.h"
#include "gui/guiChatConsole.h"
#include "util/pointedthing.h"
#include "client.h"
#include "clientmap.h"
#include "fontengine.h"
#include "nodedef.h"
#include "profiler.h"
#include "renderingengine.h"
#include "version.h"

inline static const char *yawToDirectionString(int yaw)
{
	static const char *direction[4] =
		{"North +Z", "West -X", "South -Z", "East +X"};

	yaw = wrapDegrees_0_360(yaw);
	yaw = (yaw + 45) % 360 / 90;

	return direction[yaw];
}

GameUI::GameUI()
{
	if (guienv && guienv->getSkin())
		m_statustext_initial_color = guienv->getSkin()->getColor(gui::EGDC_BUTTON_TEXT);
	else
		m_statustext_initial_color = video::SColor(255, 0, 0, 0);

}
void GameUI::init(Client *client)
{
	// First line of debug text
	m_guitext = gui::StaticText::add(guienv, utf8_to_wide(PROJECT_NAME_C).c_str(),
		core::rect<s32>(0, 0, 0, 0), false, false, guiroot);

	// Second line of debug text
	m_guitext2 = gui::StaticText::add(guienv, L"", core::rect<s32>(0, 0, 0, 0), false,
		false, guiroot);

	// Chat text
	m_guitext_chat = gui::StaticText::add(guienv, L"", core::rect<s32>(0, 0, 0, 0),
		//false, false); // Disable word wrap as of now
		false, true, guiroot);
	u16 chat_font_size = g_settings->getU16("chat_font_size");
	if (chat_font_size != 0) {
		m_guitext_chat->setOverrideFont(g_fontengine->getFont(
			chat_font_size, FM_Unspecified));
	}

	// At the middle of the screen
	// Object infos are shown in this
	u32 chat_font_height = m_guitext_chat->getActiveFont()->getDimension(L"Ay").Height;
	v2u32 screensize = RenderingEngine::get_instance()->getWindowSize();
	s32 text_height = g_fontengine->getTextHeight() * 6;
	s32 top_y = (screensize.Y - text_height) / 2;
	s32 horiz_offset = 100 + client->getRoundScreen();

	m_guitext_info = gui::StaticText::add(guienv, L"",
		core::rect<s32>(horiz_offset, top_y,
			horiz_offset + 400, top_y + text_height),
			false, true, guiroot);

	// Status text (displays info when showing and hiding GUI stuff, etc.)
	m_guitext_status = gui::StaticText::add(guienv, L"<Status>",
		core::rect<s32>(0, 0, 0, 0), false, false, guiroot);
	m_guitext_status->setVisible(false);

	// Profiler text (size is updated when text is updated)
	m_guitext_profiler = gui::StaticText::add(guienv, L"<Profiler>",
		core::rect<s32>(0, 0, 0, 0), false, false, guiroot);
	m_guitext_profiler->setOverrideFont(g_fontengine->getFont(
		g_fontengine->getDefaultFontSize() * 0.9f, FM_Mono));
	m_guitext_profiler->setVisible(false);
}

void GameUI::update(const RunStats &stats, Client *client, MapDrawControl *draw_control,
	const CameraOrientation &cam, const PointedThing &pointed_old,
	const GUIChatConsole *chat_console, float dtime)
{
	v2u32 screensize = RenderingEngine::get_instance()->getWindowSize();
	LocalPlayer *player = client->getEnv().getLocalPlayer();
	v3f player_position = player->getPosition();

	std::ostringstream os(std::ios_base::binary);
	// Minimal debug text must only contain info that can't give a gameplay advantage
	if (m_flags.show_minimal_debug) {
		static float drawtime_avg = 0;
		drawtime_avg = drawtime_avg * 0.95 + stats.drawtime * 0.05;
		u16 fps = 1.0 / stats.dtime_jitter.avg;

		os << std::fixed
			<< PROJECT_NAME_C " " << g_version_hash
			<< " | FPS: " << fps
			<< std::setprecision(0)
			<< " | drawtime: " << drawtime_avg << "ms"
			<< std::setprecision(1)
			<< " | dtime jitter: "
			<< (stats.dtime_jitter.max_fraction * 100.0) << "%"
			<< std::setprecision(1)
			<< " | view range: "
			<< (draw_control->range_all ? "All" : itos(draw_control->wanted_range))
			<< std::setprecision(2)
			<< " | RTT: " << (client->getRTT() * 1000.0f) << "ms";
	} else if (m_flags.show_minimap) {
		os << std::setprecision(1) << std::fixed
			<< "X: " << (player_position.X / BS)
			<< ", Y: " << (player_position.Y / BS)
			<< ", Z: " << (player_position.Z / BS);
	}
	m_guitext->setText(utf8_to_wide(os.str()).c_str());

	m_guitext->setRelativePosition(core::rect<s32>(
		5 + client->getRoundScreen(), 5,
		screensize.X, 5 + g_fontengine->getTextHeight()));

	// Finally set the guitext visible depending on the flag
	m_guitext->setVisible(m_flags.show_hud && (m_flags.show_minimal_debug || m_flags.show_minimap));

	// Basic debug text also shows info that might give a gameplay advantage
	const bool show_pos = m_flags.show_basic_debug || (m_flags.show_minimal_debug && m_flags.show_minimap);
	if (show_pos) {
		std::ostringstream os(std::ios_base::binary);
		os << std::setprecision(1) << std::fixed
			<< "X: " << (player_position.X / BS)
			<< ", Y: " << (player_position.Y / BS)
			<< ", Z: " << (player_position.Z / BS);

		if (m_flags.show_basic_debug) {
			os << " | yaw: " << (wrapDegrees_0_360(cam.camera_yaw)) << "° "
				<< yawToDirectionString(cam.camera_yaw)
				<< " | pitch: " << (-wrapDegrees_180(cam.camera_pitch)) << "°"
				<< " | seed: " << ((u64)client->getMapSeed());

			if (pointed_old.type == POINTEDTHING_NODE) {
				ClientMap &map = client->getEnv().getClientMap();
				const NodeDefManager *nodedef = client->getNodeDefManager();
				MapNode n = map.getNode(pointed_old.node_undersurface);

				if (n.getContent() != CONTENT_IGNORE && nodedef->get(n).name != "unknown") {
					os << ", pointed: " << nodedef->get(n).name
						<< ", param2: " << (u64) n.getParam2();
				}
			}
		}

		setStaticText(m_guitext2, utf8_to_wide(os.str()).c_str());

		m_guitext2->setRelativePosition(core::rect<s32>(5,
			5 + g_fontengine->getTextHeight(), screensize.X,
			5 + g_fontengine->getTextHeight() * 2
		));
	}

	m_guitext2->setVisible(m_flags.show_hud && show_pos);

	setStaticText(m_guitext_info, m_infotext.c_str());
	m_guitext_info->setVisible(m_flags.show_hud && g_menumgr.menuCount() == 0);

	static const float statustext_time_max = 1.5f;

	if (!m_statustext.empty()) {
		m_statustext_time += dtime;

		if (m_statustext_time >= statustext_time_max) {
			clearStatusText();
			m_statustext_time = 0.0f;
		}
	}

	setStaticText(m_guitext_status, m_statustext.c_str());
	m_guitext_status->setVisible(!m_statustext.empty());

	if (!m_statustext.empty()) {
		s32 status_width  = m_guitext_status->getTextWidth();
		s32 status_height = m_guitext_status->getTextHeight();

		s32 status_y = screensize.Y -
			150 * RenderingEngine::getDisplayDensity() * client->getHudScaling();

		s32 status_x = (screensize.X - status_width) / 2;

		m_guitext_status->setRelativePosition(core::rect<s32>(status_x ,
			status_y - status_height, status_x + status_width, status_y));

		// Fade out
		video::SColor final_color = m_statustext_initial_color;
		final_color.setAlpha(0);
		video::SColor fade_color = m_statustext_initial_color.getInterpolated_quadratic(
			m_statustext_initial_color, final_color, m_statustext_time / statustext_time_max);
		m_guitext_status->setOverrideColor(fade_color);
		m_guitext_status->enableOverrideColor(true);
	}

	// Update chat text
	if (m_chat_text_needs_update) {
		m_chat_text_needs_update = false;
		if ((!m_flags.show_hud || (!m_flags.show_minimal_debug && !m_flags.show_minimap)) &&
				client->getRoundScreen() > 0) {
			// Cache the space count
			if (!m_space_count) {
				// Use spaces to shift the text
				const u32 spwidth = g_fontengine->getFont()->getDimension(L" ").Width;
				// Divide and round up
				m_space_count = (client->getRoundScreen() + spwidth - 1) / spwidth;
			}

			EnrichedString padded_chat_text;
			for (int i = 0; i < m_space_count; i++)
				padded_chat_text.addCharNoColor(L' ');

			padded_chat_text += m_chat_text;
			setStaticText(m_guitext_chat, padded_chat_text);
		} else {
			setStaticText(m_guitext_chat, m_chat_text);
		}
	}

	// Hide chat when console is visible
	m_guitext_chat->setVisible(isChatVisible() && !chat_console->isVisible());
}

void GameUI::initFlags()
{
	m_flags = GameUI::Flags();
	m_flags.show_minimal_debug = g_settings->getBool("show_debug");
}

void GameUI::showMinimap(bool show)
{
	m_chat_text_needs_update = m_chat_text_needs_update || show != m_flags.show_minimap;
	m_flags.show_minimap = show;
}

void GameUI::showTranslatedStatusText(const char *str)
{
	const wchar_t *wmsg = wgettext(str);
	showStatusText(wmsg);
	delete[] wmsg;
}

void GameUI::setChatText(const EnrichedString &chat_text, u32 recent_chat_count)
{
	m_chat_text = chat_text;
	m_chat_text_needs_update = true;
	m_recent_chat_count = recent_chat_count;
}

void GameUI::updateChatSize()
{
	// Update gui element size and position
	s32 chat_y = 5;

	if (m_flags.show_hud) {
		if (m_flags.show_minimal_debug || m_flags.show_minimap)
			chat_y += g_fontengine->getLineHeight();
		if (m_flags.show_basic_debug || (m_flags.show_minimal_debug && m_flags.show_minimap))
			chat_y += g_fontengine->getLineHeight();
	}

	const v2u32 &window_size = RenderingEngine::get_instance()->getWindowSize();

	core::rect<s32> chat_size(10, chat_y,
		window_size.X - 20, 0);
	chat_size.LowerRightCorner.Y = std::min((s32)window_size.Y,
		m_guitext_chat->getTextHeight() + chat_y);

	if (chat_size == m_current_chat_size)
		return;
	m_current_chat_size = chat_size;

	m_guitext_chat->setRelativePosition(chat_size);
}

void GameUI::updateProfiler()
{
	if (m_profiler_current_page != 0) {
		std::ostringstream os(std::ios_base::binary);
		os << "   Profiler page " << (int)m_profiler_current_page <<
				", elapsed: " << g_profiler->getElapsedMs() << " ms)" << std::endl;

		int lines = g_profiler->print(os, m_profiler_current_page, m_profiler_max_page);
		++lines;

		EnrichedString str(utf8_to_wide(os.str()));
		str.setBackground(video::SColor(120, 0, 0, 0));
		setStaticText(m_guitext_profiler, str);

		core::dimension2d<u32> size = m_guitext_profiler->getOverrideFont()->
				getDimension(str.c_str());
		core::position2di upper_left(6, 50);
		core::position2di lower_right = upper_left;
		lower_right.X += size.Width + 10;
		lower_right.Y += size.Height;

		m_guitext_profiler->setRelativePosition(core::rect<s32>(upper_left, lower_right));
	}

	m_guitext_profiler->setVisible(m_profiler_current_page != 0);
}

void GameUI::toggleChat()
{
	m_flags.show_chat = !m_flags.show_chat;
	if (m_flags.show_chat)
		showTranslatedStatusText("Chat shown");
	else
		showTranslatedStatusText("Chat hidden");
}

void GameUI::toggleHud()
{
	m_flags.show_hud = !m_flags.show_hud;
	if (m_flags.show_hud)
		showTranslatedStatusText("HUD shown");
	else
		showTranslatedStatusText("HUD hidden");
	m_chat_text_needs_update = true;
}

void GameUI::toggleProfiler()
{
	m_profiler_current_page = (m_profiler_current_page + 1) % (m_profiler_max_page + 1);

	// FIXME: This updates the profiler with incomplete values
	updateProfiler();

	if (m_profiler_current_page != 0) {
		wchar_t buf[255];
		const wchar_t* str = wgettext("Profiler shown (page %d of %d)");
		swprintf(buf, sizeof(buf) / sizeof(wchar_t), str,
			m_profiler_current_page, m_profiler_max_page);
		delete[] str;
		showStatusText(buf);
	} else {
		showTranslatedStatusText("Profiler hidden");
	}
}


void GameUI::deleteFormspec()
{
	if (m_formspec) {
		m_formspec->drop();
		m_formspec = nullptr;
	}

	m_formname.clear();
}
