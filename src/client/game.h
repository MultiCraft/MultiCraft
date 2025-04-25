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

#pragma once

#include "irrlichttypes.h"
#include "client.h"
#include "client/mapblock_mesh.h"
#include "daynightratio.h"
#include "gui/guiEngine.h"
#include "minimap.h"
#include "porting.h"
#include "shader.h"
#include "sky.h"
#include <string>

class InputHandler;
class ChatBackend;  /* to avoid having to include chat.h */
struct SubgameSpec;
struct GameStartData;

struct Jitter {
	f32 max, min, avg, counter, max_sample, min_sample, max_fraction;
};

struct RunStats {
	u32 drawtime;

	Jitter dtime_jitter, busy_time_jitter;
};

struct CameraOrientation {
	f32 camera_yaw;    // "right/left"
	f32 camera_pitch;  // "up/down"
};

// before 1.8 there isn't a "integer interface", only float
#if (IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR < 8)
typedef f32 SamplerLayer_t;
#else
typedef s32 SamplerLayer_t;
#endif

class GameGlobalShaderConstantSetter : public IShaderConstantSetter
{
	Sky *m_sky;
	bool *m_force_fog_off;
	f32 *m_fog_range;
	bool m_fog_enabled;
	CachedPixelShaderSetting<float, 4> m_sky_bg_color;
	CachedPixelShaderSetting<float> m_fog_distance;
	CachedVertexShaderSetting<float> m_animation_timer_vertex;
	CachedPixelShaderSetting<float> m_animation_timer_pixel;
	CachedPixelShaderSetting<float, 3> m_day_light;
	CachedPixelShaderSetting<float, 4> m_star_color;
	CachedPixelShaderSetting<float, 3> m_eye_position_pixel;
	CachedVertexShaderSetting<float, 3> m_eye_position_vertex;
	CachedPixelShaderSetting<float, 3> m_minimap_yaw;
	CachedPixelShaderSetting<float, 3> m_camera_offset_pixel;
	CachedPixelShaderSetting<float, 3> m_camera_offset_vertex;
	CachedPixelShaderSetting<SamplerLayer_t> m_base_texture;
	CachedPixelShaderSetting<SamplerLayer_t> m_normal_texture;
	Client *m_client;

public:
	void onSettingsChange(const std::string &name)
	{
		if (name == "enable_fog")
			m_fog_enabled = g_settings->getBool("enable_fog");
	}

	static void settingsCallback(const std::string &name, void *userdata)
	{
		reinterpret_cast<GameGlobalShaderConstantSetter*>(userdata)->onSettingsChange(name);
	}

	void setSky(Sky *sky) { m_sky = sky; }

	GameGlobalShaderConstantSetter(Sky *sky, bool *force_fog_off,
			f32 *fog_range, Client *client) :
		m_sky(sky),
		m_force_fog_off(force_fog_off),
		m_fog_range(fog_range),
		m_sky_bg_color("skyBgColor"),
		m_fog_distance("fogDistance"),
		m_animation_timer_vertex("animationTimer"),
		m_animation_timer_pixel("animationTimer"),
		m_day_light("dayLight"),
		m_star_color("starColor"),
		m_eye_position_pixel("eyePosition"),
		m_eye_position_vertex("eyePosition"),
		m_minimap_yaw("yawVec"),
		m_camera_offset_pixel("cameraOffset"),
		m_camera_offset_vertex("cameraOffset"),
		m_base_texture("baseTexture"),
		m_normal_texture("normalTexture"),
		m_client(client)
	{
		g_settings->registerChangedCallback("enable_fog", settingsCallback, this);
		m_fog_enabled = g_settings->getBool("enable_fog");
	}

	~GameGlobalShaderConstantSetter()
	{
		g_settings->deregisterChangedCallback("enable_fog", settingsCallback, this);
	}

	void onSetConstants(video::IMaterialRendererServices *services) override
	{
		// Background color
		video::SColor bgcolor = m_sky->getBgColor();
		video::SColorf bgcolorf(bgcolor);
		float bgcolorfa[4] = {
			bgcolorf.r,
			bgcolorf.g,
			bgcolorf.b,
			bgcolorf.a,
		};
		m_sky_bg_color.set(bgcolorfa, services);

		// Fog distance
		float fog_distance = -1.0f; // sentinel for disabled fog

		if (m_fog_enabled && !*m_force_fog_off)
			fog_distance = *m_fog_range;

		m_fog_distance.set(&fog_distance, services);

		u32 daynight_ratio = 0;
		if (m_client)
			daynight_ratio = m_client->getEnv().getDayNightRatio();
		else
			daynight_ratio = time_to_daynight_ratio(GUIEngine::g_timeofday * 24000.0f, true);

		video::SColorf sunlight;
		get_sunlight_color(&sunlight, daynight_ratio);
		float dnc[3] = {
			sunlight.r,
			sunlight.g,
			sunlight.b };
		m_day_light.set(dnc, services);

		video::SColorf star_color = m_sky->getCurrentStarColor();
		float clr[4] = {star_color.r, star_color.g, star_color.b, star_color.a};
		m_star_color.set(clr, services);

		u32 animation_timer = porting::getTimeMs() % 1000000;
		float animation_timer_f = (float)animation_timer / 100000.f;
		m_animation_timer_vertex.set(&animation_timer_f, services);
		m_animation_timer_pixel.set(&animation_timer_f, services);

		if (m_client) {
			float eye_position_array[3];
			v3f epos = m_client->getEnv().getLocalPlayer()->getEyePosition();
#if (IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR < 8)
			eye_position_array[0] = epos.X;
			eye_position_array[1] = epos.Y;
			eye_position_array[2] = epos.Z;
#else
			epos.getAs3Values(eye_position_array);
#endif
			m_eye_position_pixel.set(eye_position_array, services);
			m_eye_position_vertex.set(eye_position_array, services);
		}

		if (m_client && m_client->getMinimap()) {
			float minimap_yaw_array[3];
			v3f minimap_yaw = m_client->getMinimap()->getYawVec();
#if (IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR < 8)
			minimap_yaw_array[0] = minimap_yaw.X;
			minimap_yaw_array[1] = minimap_yaw.Y;
			minimap_yaw_array[2] = minimap_yaw.Z;
#else
			minimap_yaw.getAs3Values(minimap_yaw_array);
#endif
			m_minimap_yaw.set(minimap_yaw_array, services);
		}

		if (m_client) {
			float camera_offset_array[3];
			v3f offset = intToFloat(m_client->getCamera()->getOffset(), BS);
#if (IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR < 8)
			camera_offset_array[0] = offset.X;
			camera_offset_array[1] = offset.Y;
			camera_offset_array[2] = offset.Z;
#else
			offset.getAs3Values(camera_offset_array);
#endif
			m_camera_offset_pixel.set(camera_offset_array, services);
			m_camera_offset_vertex.set(camera_offset_array, services);

			SamplerLayer_t base_tex = 0, normal_tex = 1;
			m_base_texture.set(&base_tex, services);
			m_normal_texture.set(&normal_tex, services);
		}
	}
};

class GameGlobalShaderConstantSetterFactory : public IShaderConstantSetterFactory
{
	Sky *m_sky;
	bool *m_force_fog_off;
	f32 *m_fog_range;
	Client *m_client;
	std::vector<GameGlobalShaderConstantSetter *> created_nosky;
public:
	GameGlobalShaderConstantSetterFactory(bool *force_fog_off,
			f32 *fog_range, Client *client) :
		m_sky(NULL),
		m_force_fog_off(force_fog_off),
		m_fog_range(fog_range),
		m_client(client)
	{}

	void setSky(Sky *sky) {
		m_sky = sky;
		for (GameGlobalShaderConstantSetter *ggscs : created_nosky) {
			ggscs->setSky(m_sky);
		}
		created_nosky.clear();
	}

	virtual IShaderConstantSetter* create()
	{
		auto *scs = new GameGlobalShaderConstantSetter(
				m_sky, m_force_fog_off, m_fog_range, m_client);
		if (!m_sky)
			created_nosky.push_back(scs);
		return scs;
	}
};

void the_game(bool *kill,
		InputHandler *input,
		const GameStartData &start_data,
		std::string &error_message,
		ChatBackend &chat_backend,
		bool *reconnect_requested);
