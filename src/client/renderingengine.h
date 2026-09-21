/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2017 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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

#include <vector>
#include <memory>
#include <string>
#include "irrlichttypes_extrabloated.h"
#include "debug.h"
#include "client/render/core.h"
// include the shadow mapper classes too
#include "client/shadows/dynamicshadowsrender.h"

struct VideoDriverInfo {
	std::string name;
	std::string friendly_name;
};

class ITextureSource;
class Camera;
class Client;
class LocalPlayer;
class Hud;
class Minimap;
class GUIButton;

class RenderingCore;

class RenderingEngine
{
public:
	RenderingEngine(IEventReceiver *eventReceiver);
	~RenderingEngine();

	void setResizable(bool resize);

	video::IVideoDriver *getVideoDriver() { return driver; }

	static const VideoDriverInfo &getVideoDriverInfo(irr::video::E_DRIVER_TYPE type);
	static float getDisplayDensity();
	static float getScreenScale();
	static float getHudScaling();
	static v2u32 getDisplaySize();
	static int getWindowSafeArea();
#ifdef HAVE_TOUCHSCREENGUI
	static bool isTablet();
#endif
	static bool isHighDpi();
	static void startTextInput();
	static void stopTextInput();

	bool setupTopLevelWindow(const std::string &name);
	void setupTopLevelXorgWindow(const std::string &name);
	bool setWindowIcon();
	bool setXorgWindowIconFromPath(const std::string &icon_file);
	static bool print_video_modes();
	void cleanupMeshCache();

	void removeMesh(const scene::IMesh* mesh);

	static v2u32 getWindowSize()
	{
		sanity_check(s_singleton);
		return s_singleton->_getWindowSize();
	}

	// The last error Irrlicht logged, for the message of a device that did not start
	static void noteEngineError(const std::string &text) { s_engine_error = text; }

	io::IFileSystem *get_filesystem()
	{
		return m_device->getFileSystem();
	}

	static video::IVideoDriver *get_video_driver()
	{
		sanity_check(s_singleton && s_singleton->m_device);
		return s_singleton->m_device->getVideoDriver();
	}

	scene::ISceneManager *get_scene_manager()
	{
		return m_device->getSceneManager();
	}

	static irr::IrrlichtDevice *get_raw_device()
	{
		sanity_check(s_singleton && s_singleton->m_device);
		return s_singleton->m_device;
	}

	u32 get_timer_time()
	{
		return m_device->getTimer()->getTime();
	}

	gui::IGUIEnvironment *get_gui_env()
	{
		return m_device->getGUIEnvironment();
	}

	void draw_load_screen(const std::wstring &text,
			gui::IGUIEnvironment *guienv, ITextureSource *tsrc,
			float dtime = 0, int percent = 0);

	void draw_load_cleanup();

	void set_load_screen_cancel(bool *flag) { m_load_cancel = flag; }

	inline static void cancel_load_screen()
	{
		if (s_singleton && s_singleton->m_load_cancel)
			*s_singleton->m_load_cancel = true;
	}

	//! Feeds a tap to the cancel button, which the GUI cannot reach on its own
	static bool handle_load_screen_touch(const SEvent &event);

	void draw_menu_scene(gui::IGUIEnvironment *guienv, ITextureSource *tsrc, float dtime);
	void draw_scene(video::SColor skycolor, bool show_hud,
			bool show_minimap, bool draw_wield_tool, bool draw_crosshair, bool draw_nametags);

	void initialize(Client *client, Hud *hud);
	void finalize();

	void clear_irrlicht_texture_cache();

	bool run();

	// FIXME: this is still global when it shouldn't be
	static ShadowRenderer *get_shadow_renderer()
	{
		//if (s_singleton && s_singleton->core)
		//	return s_singleton->core->get_shadow_renderer();
		return nullptr;
	}
	static std::vector<irr::video::E_DRIVER_TYPE> getSupportedVideoDrivers();

	void setLoadScreenBackground(const bool clouds, const std::string texture,
			const video::SColor sky_color)
	{
		m_load_bg_clouds = clouds;
		m_load_bg_texture = texture;
		m_sky_color = sky_color;
	}

private:
	v2u32 _getWindowSize() const;

	void _draw_load_bg(gui::IGUIEnvironment *guienv,
			ITextureSource *tsrc, float dtime = 0);

	std::unique_ptr<RenderingCore> core;
	irr::IrrlichtDevice *m_device = nullptr;
	irr::video::IVideoDriver *driver;
	static RenderingEngine *s_singleton;
	static std::string s_engine_error;

	gui::IGUIStaticText *m_load_text = nullptr;
	gui::IGUIElement *m_load_root = nullptr;
	GUIButton *m_load_button = nullptr;
	bool *m_load_cancel = nullptr;

	bool m_load_bg_clouds = false;
	std::string m_load_bg_texture = "";
	video::SColor m_sky_color;
	u64 m_last_time = 0;
	float m_load_screen_dtime = 0;
	bool m_load_screen_drawn = false;
	int m_percent = 0;
	float m_percent_shown = 0.0f;
};
