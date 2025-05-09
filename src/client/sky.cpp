/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2020 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

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

#include "sky.h"
#include "ITexture.h"
#include "IVideoDriver.h"
#include "ISceneManager.h"
#include "ICameraSceneNode.h"
#include "S3DVertex.h"
#include "client/tile.h"
#include "noise.h" // easeCurve
#include "profiler.h"
#include "util/numeric.h"
#include <cmath>
#include "client/renderingengine.h"
#include "settings.h"
#include "camera.h" // CameraModes
#include "config.h"
#include "filesys.h"
#include "porting.h"

using namespace irr::core;

// Menu sky are created later
class Sky;
Sky *g_menusky = NULL;

static video::SMaterial baseMaterial()
{
	video::SMaterial mat;
	mat.Lighting = false;
#if IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR > 8
	mat.ZBuffer = video::ECFN_DISABLED;
#else
	mat.ZBuffer = video::ECFN_NEVER;
#endif
	mat.setFlag(video::EMF_ZWRITE_ENABLE, false); // ZWriteEnable is bool on early 1.9 but E_ZWRITE on later 1.9
	mat.AntiAliasing = 0;
	mat.TextureLayer[0].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
	mat.TextureLayer[0].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
	mat.BackfaceCulling = false;
	return mat;
};


Sky::Sky(s32 id, ITextureSource *tsrc, IShaderSource *ssrc) :
		Sky(id, tsrc, ssrc, RenderingEngine::get_scene_manager())
{}

Sky::Sky(s32 id, ITextureSource *tsrc, IShaderSource *ssrc, scene::ISceneManager *smgr) :
		scene::ISceneNode(smgr->getRootSceneNode(), smgr, id)
{
	setAutomaticCulling(scene::EAC_OFF);
	m_box.MaxEdge.set(0, 0, 0);
	m_box.MinEdge.set(0, 0, 0);

	m_enable_shaders = g_settings->getBool("enable_shaders");

	// Create materials

	m_materials[0] = baseMaterial();
	m_materials[0].MaterialType = ssrc->getShaderInfo(ssrc->getShader("stars_shader", TILE_MATERIAL_ALPHA)).material;
	m_materials[0].Lighting = true;
	m_materials[0].ColorMaterial = video::ECM_NONE;

	m_materials[1] = baseMaterial();
	//m_materials[1].MaterialType = video::EMT_TRANSPARENT_VERTEX_ALPHA;
	m_materials[1].MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;

	if (tsrc && tsrc->isKnownSourceImage("sunrisebg.png")) {
		m_materials[2] = baseMaterial();
		m_materials[2].setTexture(0, tsrc->getTextureForMesh("sunrisebg.png"));
		m_materials[2].MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
		//m_materials[2].MaterialType = video::EMT_TRANSPARENT_ADD_COLOR;
	}

	// Ensures that sun and moon textures and tonemaps are correct.
	setSkyDefaults();
	m_sun_texture = tsrc && tsrc->isKnownSourceImage(m_sun_params.texture) ?
		tsrc->getTextureForMesh(m_sun_params.texture) : nullptr;
	m_moon_texture = tsrc && tsrc->isKnownSourceImage(m_moon_params.texture) ?
		tsrc->getTextureForMesh(m_moon_params.texture) : nullptr;
	m_sun_tonemap = tsrc && tsrc->isKnownSourceImage(m_sun_params.tonemap) ?
		tsrc->getTexture(m_sun_params.tonemap) : nullptr;
	m_moon_tonemap = tsrc && tsrc->isKnownSourceImage(m_moon_params.tonemap) ?
		tsrc->getTexture(m_moon_params.tonemap) : nullptr;

	if (m_sun_texture) {
		m_sun_texture->grab();
		m_materials[3] = baseMaterial();
		m_materials[3].setTexture(0, m_sun_texture);
		m_materials[3].MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
		// Disables texture filtering
		m_materials[3].setFlag(video::E_MATERIAL_FLAG::EMF_BILINEAR_FILTER, false);
		m_materials[3].setFlag(video::E_MATERIAL_FLAG::EMF_TRILINEAR_FILTER, false);
		m_materials[3].setFlag(video::E_MATERIAL_FLAG::EMF_ANISOTROPIC_FILTER, false);
		// Use tonemaps if available
		if (m_sun_tonemap) {
			m_sun_tonemap->grab();
			m_materials[3].Lighting = true;
		}
	}
	if (m_moon_texture) {
		m_moon_texture->grab();
		m_materials[4] = baseMaterial();
		m_materials[4].setTexture(0, m_moon_texture);
		m_materials[4].MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
		// Disables texture filtering
		m_materials[4].setFlag(video::E_MATERIAL_FLAG::EMF_BILINEAR_FILTER, false);
		m_materials[4].setFlag(video::E_MATERIAL_FLAG::EMF_TRILINEAR_FILTER, false);
		m_materials[4].setFlag(video::E_MATERIAL_FLAG::EMF_ANISOTROPIC_FILTER, false);
		// Use tonemaps if available
		if (m_moon_tonemap) {
			m_moon_tonemap->grab();
			m_materials[4].Lighting = true;
		}
	}

	for (int i = 5; i < 11; i++) {
		m_materials[i] = baseMaterial();
		m_materials[i].Lighting = true;
		m_materials[i].MaterialType = video::EMT_SOLID;
	}
	m_directional_colored_fog = g_settings->getBool("directional_colored_fog");
	setStarCount(1000, true);
}

Sky::~Sky()
{
	if (m_moon_texture)
		m_moon_texture->drop();
	if (m_moon_tonemap)
		m_moon_tonemap->drop();
	if (m_sun_texture)
		m_sun_texture->drop();
	if (m_sun_tonemap)
		m_sun_tonemap->drop();
}

void Sky::OnRegisterSceneNode()
{
	if (IsVisible)
		SceneManager->registerNodeForRendering(this, scene::ESNRP_SKY_BOX);

	scene::ISceneNode::OnRegisterSceneNode();
}

void Sky::render()
{
	video::IVideoDriver *driver = SceneManager->getVideoDriver();
	scene::ICameraSceneNode *camera = SceneManager->getActiveCamera();

	if (!camera || !driver)
		return;

	ScopeProfiler sp(g_profiler, "Sky::render()", SPT_AVG);

	// Draw perspective skybox

	core::matrix4 translate(AbsoluteTransformation);
	translate.setTranslation(camera->getAbsolutePosition());

	// Draw the sky box between the near and far clip plane
	const f32 viewDistance = (camera->getNearValue() + camera->getFarValue()) * 0.5f;
	core::matrix4 scale;
	scale.setScale(core::vector3df(viewDistance, viewDistance, viewDistance));

	driver->setTransform(video::ETS_WORLD, translate * scale);

	if (m_sunlight_seen) {
		float sunsize = 0.07;
		video::SColorf suncolor_f(1, 1, 0, 1);
		//suncolor_f.r = 1;
		//suncolor_f.g = MYMAX(0.3, MYMIN(1.0, 0.7 + m_time_brightness * 0.5));
		//suncolor_f.b = MYMAX(0.0, m_brightness * 0.95);
		video::SColorf suncolor2_f(1, 1, 1, 1);
		// The values below were probably meant to be suncolor2_f instead of a
		// reassignment of suncolor_f. However, the resulting colour was chosen
		// and is our long-running classic colour. So preserve, but comment-out
		// the unnecessary first assignments above.
		suncolor_f.r = 1;
		suncolor_f.g = MYMAX(0.3, MYMIN(1.0, 0.85 + m_time_brightness * 0.5));
		suncolor_f.b = MYMAX(0.0, m_brightness);

		float moonsize = 0.04;
		video::SColorf mooncolor_f(0.50, 0.57, 0.65, 1);
		video::SColorf mooncolor2_f(0.85, 0.875, 0.9, 1);

		float nightlength = 0.415;
		float wn = nightlength / 2;
		float wicked_time_of_day = 0;
		if (m_time_of_day > wn && m_time_of_day < 1.0 - wn)
			wicked_time_of_day = (m_time_of_day - wn) / (1.0 - wn * 2) * 0.5 + 0.25;
		else if (m_time_of_day < 0.5)
			wicked_time_of_day = m_time_of_day / wn * 0.25;
		else
			wicked_time_of_day = 1.0 - ((1.0 - m_time_of_day) / wn * 0.25);
		/*std::cerr<<"time_of_day="<<m_time_of_day<<" -> "
				<<"wicked_time_of_day="<<wicked_time_of_day<<std::endl;*/

		video::SColor suncolor = suncolor_f.toSColor();
		video::SColor suncolor2 = suncolor2_f.toSColor();
		video::SColor mooncolor = mooncolor_f.toSColor();
		video::SColor mooncolor2 = mooncolor2_f.toSColor();

		// Calculate offset normalized to the X dimension of a 512x1 px tonemap
		float offset = (1.0 - fabs(sin((m_time_of_day - 0.5) * irr::core::PI))) * 511;

		if (m_sun_tonemap) {
			u8 * texels = (u8 *)m_sun_tonemap->lock();
			video::SColor* texel = (video::SColor *)(texels + (u32)offset * 4);
			video::SColor texel_color (255, texel->getRed(),
				texel->getGreen(), texel->getBlue());
			m_sun_tonemap->unlock();
			m_materials[3].EmissiveColor = texel_color;
		}

		if (m_moon_tonemap) {
			u8 * texels = (u8 *)m_moon_tonemap->lock();
			video::SColor* texel = (video::SColor *)(texels + (u32)offset * 4);
			video::SColor texel_color (255, texel->getRed(),
				texel->getGreen(), texel->getBlue());
			m_moon_tonemap->unlock();
			m_materials[4].EmissiveColor = texel_color;
		}

		const f32 t = 1.0f;
		const f32 o = 0.0f;
		static const u16 indices[6] = {0, 1, 2, 0, 2, 3};
		video::S3DVertex vertices[4];

		driver->setMaterial(m_materials[1]);

		video::SColor cloudyfogcolor = m_bgcolor;

		// Abort rendering if we're in the clouds.
		// Stops rendering a pure white hole in the bottom of the skybox.
		if (m_in_clouds)
			return;

		// Draw the six sided skybox,
		if (m_sky_params.textures.size() == 6) {
			for (u32 j = 5; j < 11; j++) {
				video::SColor c(255, 255, 255, 255);
				driver->setMaterial(m_materials[j]);
				// Use 1.05 rather than 1.0 to avoid colliding with the
				// sun, moon and stars, as this is a background skybox.
				vertices[0] = video::S3DVertex(-1.05, -1.05, -1.05, 0, 0, 1, c, t, t);
				vertices[1] = video::S3DVertex( 1.05, -1.05, -1.05, 0, 0, 1, c, o, t);
				vertices[2] = video::S3DVertex( 1.05,  1.05, -1.05, 0, 0, 1, c, o, o);
				vertices[3] = video::S3DVertex(-1.05,  1.05, -1.05, 0, 0, 1, c, t, o);
				for (video::S3DVertex &vertex : vertices) {
					if (j == 5) { // Top texture
						vertex.Pos.rotateYZBy(90);
						vertex.Pos.rotateXZBy(90);
					} else if (j == 6) { // Bottom texture
						vertex.Pos.rotateYZBy(-90);
						vertex.Pos.rotateXZBy(90);
					} else if (j == 7) { // Left texture
						vertex.Pos.rotateXZBy(90);
					} else if (j == 8) { // Right texture
						vertex.Pos.rotateXZBy(-90);
					} else if (j == 9) { // Front texture, do nothing
						// Irrlicht doesn't like it when vertexes are left
						// alone and not rotated for some reason.
						vertex.Pos.rotateXZBy(0);
					} else {// Back texture
						vertex.Pos.rotateXZBy(180);
					}
				}
				driver->drawIndexedTriangleList(&vertices[0], 4, indices, 2);
			}
		}

		// Draw far cloudy fog thing blended with skycolor
		/*if (m_visible) {
			driver->setMaterial(m_materials[1]);
			for (u32 j = 0; j < 4; j++) {
				vertices[0] = video::S3DVertex(-1, -0.02, -1, 0, 0, 1, m_bgcolor, t, t);
				vertices[1] = video::S3DVertex( 1, -0.02, -1, 0, 0, 1, m_bgcolor, o, t);
				vertices[2] = video::S3DVertex( 1, 0.45, -1, 0, 0, 1, m_skycolor, o, o);
				vertices[3] = video::S3DVertex(-1, 0.45, -1, 0, 0, 1, m_skycolor, t, o);
				for (video::S3DVertex &vertex : vertices) {
					if (j == 0)
						// Don't switch
						{}
					else if (j == 1)
						// Switch from -Z (south) to +X (east)
						vertex.Pos.rotateXZBy(90);
					else if (j == 2)
						// Switch from -Z (south) to -X (west)
						vertex.Pos.rotateXZBy(-90);
					else
						// Switch from -Z (south) to +Z (north)
						vertex.Pos.rotateXZBy(-180);
				}
				driver->drawIndexedTriangleList(&vertices[0], 4, indices, 2);
			}
		}*/

		// Draw stars before sun and moon to be behind them
		if (m_star_params.visible)
			draw_stars(driver, wicked_time_of_day);

		// Draw sunrise/sunset horizon glow texture
		// (textures/base/pack/sunrisebg.png)
		if (m_sun_params.sunrise_visible && !m_use_custom_sky_body_pos) {
			driver->setMaterial(m_materials[2]);
			float mid1 = 0.25;
			float mid = wicked_time_of_day < 0.5 ? mid1 : (1.0 - mid1);
			float a_ = 1.0f - std::fabs(wicked_time_of_day - mid) * 35.0f;
			float a = easeCurve(MYMAX(0, MYMIN(1, a_)));
			//std::cerr<<"a_="<<a_<<" a="<<a<<std::endl;
			video::SColor c(255, 255, 255, 255);
			float y = -(1.0 - a) * 0.22;
			vertices[0] = video::S3DVertex(-1, -0.05 + y, -1, 0, 0, 1, c, t, t);
			vertices[1] = video::S3DVertex( 1, -0.05 + y, -1, 0, 0, 1, c, o, t);
			vertices[2] = video::S3DVertex( 1,   0.2 + y, -1, 0, 0, 1, c, o, o);
			vertices[3] = video::S3DVertex(-1,   0.2 + y, -1, 0, 0, 1, c, t, o);
			for (video::S3DVertex &vertex : vertices) {
				if (wicked_time_of_day < 0.5)
					// Switch from -Z (south) to +X (east)
					vertex.Pos.rotateXZBy(90);
				else
					// Switch from -Z (south) to -X (west)
					vertex.Pos.rotateXZBy(-90);
			}
			driver->drawIndexedTriangleList(&vertices[0], 4, indices, 2);
		}

		// Draw sun
		if (m_sun_params.visible)
			draw_sun(driver, sunsize, suncolor, suncolor2, wicked_time_of_day);

		// Draw moon
		if (m_moon_params.visible)
			draw_moon(driver, moonsize, mooncolor, mooncolor2, wicked_time_of_day);

		// Draw far cloudy fog thing below all horizons in front of sun, moon
		// and stars.
		if (m_visible) {
			driver->setMaterial(m_materials[1]);

			for (u32 j = 0; j < 4; j++) {
				video::SColor c = cloudyfogcolor;
				vertices[0] = video::S3DVertex(-1, -1.0,  -1, 0, 0, 1, c, t, t);
				vertices[1] = video::S3DVertex( 1, -1.0,  -1, 0, 0, 1, c, o, t);
				vertices[2] = video::S3DVertex( 1, -0.02, -1, 0, 0, 1, c, o, o);
				vertices[3] = video::S3DVertex(-1, -0.02, -1, 0, 0, 1, c, t, o);
				for (video::S3DVertex &vertex : vertices) {
					if (j == 0)
						// Don't switch
						{}
					else if (j == 1)
						// Switch from -Z (south) to +X (east)
						vertex.Pos.rotateXZBy(90);
					else if (j == 2)
						// Switch from -Z (south) to -X (west)
						vertex.Pos.rotateXZBy(-90);
					else
						// Switch from -Z (south) to +Z (north)
						vertex.Pos.rotateXZBy(-180);
				}
				driver->drawIndexedTriangleList(&vertices[0], 4, indices, 2);
			}

			// Draw bottom far cloudy fog thing in front of sun, moon and stars
			video::SColor c = cloudyfogcolor;
			vertices[0] = video::S3DVertex(-1, -1.0, -1, 0, 1, 0, c, t, t);
			vertices[1] = video::S3DVertex( 1, -1.0, -1, 0, 1, 0, c, o, t);
			vertices[2] = video::S3DVertex( 1, -1.0, 1, 0, 1, 0, c, o, o);
			vertices[3] = video::S3DVertex(-1, -1.0, 1, 0, 1, 0, c, t, o);
			driver->drawIndexedTriangleList(&vertices[0], 4, indices, 2);
		}
	}
}

void Sky::update(float time_of_day, float time_brightness,
	float direct_brightness, bool sunlight_seen,
	CameraMode cam_mode, float yaw, float pitch)
{
	// Stabilize initial brightness and color values by flooding updates
	if (m_first_update) {
		/*dstream<<"First update with time_of_day="<<time_of_day
				<<" time_brightness="<<time_brightness
				<<" direct_brightness="<<direct_brightness
				<<" sunlight_seen="<<sunlight_seen<<std::endl;*/
		m_first_update = false;
		for (u32 i = 0; i < 100; i++) {
			update(time_of_day, time_brightness, direct_brightness,
					sunlight_seen, cam_mode, yaw, pitch);
		}
		return;
	}

	m_time_of_day = time_of_day;
	m_time_brightness = time_brightness;
	m_sunlight_seen = sunlight_seen;
	m_in_clouds = false;

	bool is_dawn = (time_brightness >= 0.20 && time_brightness < 0.35);

	/*
	Development colours

	video::SColorf bgcolor_bright_normal_f(170. / 255, 200. / 255, 230. / 255, 1.0);
	video::SColorf bgcolor_bright_dawn_f(0.666, 200. / 255 * 0.7, 230. / 255 * 0.5, 1.0);
	video::SColorf bgcolor_bright_dawn_f(0.666, 0.549, 0.220, 1.0);
	video::SColorf bgcolor_bright_dawn_f(0.666 * 1.2, 0.549 * 1.0, 0.220 * 1.0, 1.0);
	video::SColorf bgcolor_bright_dawn_f(0.666 * 1.2, 0.549 * 1.0, 0.220 * 1.2, 1.0);

	video::SColorf cloudcolor_bright_dawn_f(1.0, 0.591, 0.4);
	video::SColorf cloudcolor_bright_dawn_f(1.0, 0.65, 0.44);
	video::SColorf cloudcolor_bright_dawn_f(1.0, 0.7, 0.5);
	*/

	video::SColorf bgcolor_bright_normal_f = m_sky_params.sky_color.day_horizon;
	video::SColorf bgcolor_bright_indoor_f = m_sky_params.sky_color.indoors;
	video::SColorf bgcolor_bright_dawn_f = m_sky_params.sky_color.dawn_horizon;
	video::SColorf bgcolor_bright_night_f = m_sky_params.sky_color.night_horizon;

	video::SColorf skycolor_bright_normal_f = m_sky_params.sky_color.day_sky;
	video::SColorf skycolor_bright_dawn_f = m_sky_params.sky_color.dawn_sky;
	video::SColorf skycolor_bright_night_f = m_sky_params.sky_color.night_sky;

	video::SColorf cloudcolor_bright_normal_f = m_cloudcolor_day_f;
	video::SColorf cloudcolor_bright_dawn_f = m_cloudcolor_dawn_f;

	float cloud_color_change_fraction = 0.95;
	if (sunlight_seen) {
		if (std::fabs(time_brightness - m_brightness) < 0.2f) {
			m_brightness = m_brightness * 0.95 + time_brightness * 0.05;
		} else {
			m_brightness = m_brightness * 0.80 + time_brightness * 0.20;
			cloud_color_change_fraction = 0.0;
		}
	} else {
		if (direct_brightness < m_brightness)
			m_brightness = m_brightness * 0.95 + direct_brightness * 0.05;
		else
			m_brightness = m_brightness * 0.98 + direct_brightness * 0.02;
	}

	m_clouds_visible = true;
	float color_change_fraction = 0.98f;
	if (sunlight_seen) {
		if (is_dawn) { // Dawn
			m_bgcolor_bright_f = m_bgcolor_bright_f.getInterpolated(
				bgcolor_bright_dawn_f, color_change_fraction);
			m_skycolor_bright_f = m_skycolor_bright_f.getInterpolated(
				skycolor_bright_dawn_f, color_change_fraction);
			m_cloudcolor_bright_f = m_cloudcolor_bright_f.getInterpolated(
				cloudcolor_bright_dawn_f, color_change_fraction);
		} else {
			if (time_brightness < 0.13f) { // Night
				m_bgcolor_bright_f = m_bgcolor_bright_f.getInterpolated(
					bgcolor_bright_night_f, color_change_fraction);
				m_skycolor_bright_f = m_skycolor_bright_f.getInterpolated(
					skycolor_bright_night_f, color_change_fraction);
			} else { // Day
				m_bgcolor_bright_f = m_bgcolor_bright_f.getInterpolated(
					bgcolor_bright_normal_f, color_change_fraction);
				m_skycolor_bright_f = m_skycolor_bright_f.getInterpolated(
					skycolor_bright_normal_f, color_change_fraction);
			}

			m_cloudcolor_bright_f = m_cloudcolor_bright_f.getInterpolated(
				cloudcolor_bright_normal_f, color_change_fraction);
		}
	} else {
		m_bgcolor_bright_f = m_bgcolor_bright_f.getInterpolated(
			bgcolor_bright_indoor_f, color_change_fraction);
		m_skycolor_bright_f = m_skycolor_bright_f.getInterpolated(
			bgcolor_bright_indoor_f, color_change_fraction);
		m_cloudcolor_bright_f = m_cloudcolor_bright_f.getInterpolated(
			cloudcolor_bright_normal_f, color_change_fraction);
		m_clouds_visible = false;
	}

	video::SColor bgcolor_bright = m_bgcolor_bright_f.toSColor();
	m_bgcolor = video::SColor(
		255,
		bgcolor_bright.getRed() * m_brightness,
		bgcolor_bright.getGreen() * m_brightness,
		bgcolor_bright.getBlue() * m_brightness
	);

	video::SColor skycolor_bright = m_skycolor_bright_f.toSColor();
	m_skycolor = video::SColor(
		255,
		skycolor_bright.getRed() * m_brightness,
		skycolor_bright.getGreen() * m_brightness,
		skycolor_bright.getBlue() * m_brightness
	);

	// Horizon coloring based on sun and moon direction during sunset and sunrise
	video::SColor pointcolor = video::SColor(m_bgcolor.getAlpha(), 255, 255, 255);
	if (m_directional_colored_fog) {
		if (m_horizon_blend() != 0) {
			// Calculate hemisphere value from yaw, (inverted in third person front view)
			s8 dir_factor = 1;
			if (cam_mode > CAMERA_MODE_THIRD)
				dir_factor = -1;
			f32 pointcolor_blend = wrapDegrees_0_360(yaw * dir_factor + 90);
			if (pointcolor_blend > 180)
				pointcolor_blend = 360 - pointcolor_blend;
			pointcolor_blend /= 180;
			// Bound view angle to determine where transition starts and ends
			pointcolor_blend = rangelim(1 - pointcolor_blend * 1.375, 0, 1 / 1.375) *
				1.375;
			// Combine the colors when looking up or down, otherwise turning looks weird
			pointcolor_blend += (0.5 - pointcolor_blend) *
				(1 - MYMIN((90 - std::fabs(pitch)) / 90 * 1.5, 1));
			// Invert direction to match where the sun and moon are rising
			if (m_time_of_day > 0.5)
				pointcolor_blend = 1 - pointcolor_blend;
			// Horizon colors of sun and moon
			f32 pointcolor_light = rangelim(m_time_brightness * 3, 0.2, 1);

			video::SColorf pointcolor_sun_f(1, 1, 1, 1);
			// Use tonemap only if default sun/moon tinting is used
			// which keeps previous behaviour.
			if (m_sun_tonemap && m_default_tint) {
				pointcolor_sun_f.r = pointcolor_light *
					(float)m_materials[3].EmissiveColor.getRed() / 255;
				pointcolor_sun_f.b = pointcolor_light *
					(float)m_materials[3].EmissiveColor.getBlue() / 255;
				pointcolor_sun_f.g = pointcolor_light *
					(float)m_materials[3].EmissiveColor.getGreen() / 255;
			} else if (!m_default_tint) {
				pointcolor_sun_f = m_sky_params.fog_sun_tint;
			} else {
				pointcolor_sun_f.r = pointcolor_light * 1;
				pointcolor_sun_f.b = pointcolor_light *
					(0.25 + (rangelim(m_time_brightness, 0.25, 0.75) - 0.25) * 2 * 0.75);
				pointcolor_sun_f.g = pointcolor_light * (pointcolor_sun_f.b * 0.375 +
					(rangelim(m_time_brightness, 0.05, 0.15) - 0.05) * 10 * 0.625);
			}

			video::SColorf pointcolor_moon_f;
			if (m_default_tint) {
				pointcolor_moon_f = video::SColorf(
					0.5 * pointcolor_light,
					0.6 * pointcolor_light,
					0.8 * pointcolor_light,
					1
				);
			} else {
				pointcolor_moon_f = video::SColorf(
					(m_sky_params.fog_moon_tint.getRed() / 255) * pointcolor_light,
					(m_sky_params.fog_moon_tint.getGreen() / 255) * pointcolor_light,
					(m_sky_params.fog_moon_tint.getBlue() / 255) * pointcolor_light,
					1
				);
			}
			if (m_moon_tonemap && m_default_tint) {
				pointcolor_moon_f.r = pointcolor_light *
					(float)m_materials[4].EmissiveColor.getRed() / 255;
				pointcolor_moon_f.b = pointcolor_light *
					(float)m_materials[4].EmissiveColor.getBlue() / 255;
				pointcolor_moon_f.g = pointcolor_light *
					(float)m_materials[4].EmissiveColor.getGreen() / 255;
			}

			video::SColor pointcolor_sun = pointcolor_sun_f.toSColor();
			video::SColor pointcolor_moon = pointcolor_moon_f.toSColor();
			// Calculate the blend color
			pointcolor = m_mix_scolor(pointcolor_moon, pointcolor_sun, pointcolor_blend);
		}
		m_bgcolor = m_mix_scolor(m_bgcolor, pointcolor, m_horizon_blend() * 0.5);
		m_skycolor = m_mix_scolor(m_skycolor, pointcolor, m_horizon_blend() * 0.25);
	}

	float cloud_direct_brightness = 0.0f;
	if (sunlight_seen) {
		if (!m_directional_colored_fog) {
			cloud_direct_brightness = time_brightness;
			// Boost cloud brightness relative to sky, at dawn, dusk and at night
			if (time_brightness < 0.7f)
				cloud_direct_brightness *= 1.3f;
		} else {
			cloud_direct_brightness = std::fmin(m_horizon_blend() * 0.15f +
				m_time_brightness, 1.0f);
			// Set the same minimum cloud brightness at night
			if (time_brightness < 0.5f)
				cloud_direct_brightness = std::fmax(cloud_direct_brightness,
					time_brightness * 1.3f);
		}
	} else {
		cloud_direct_brightness = direct_brightness;
	}

	m_cloud_brightness = m_cloud_brightness * cloud_color_change_fraction +
		cloud_direct_brightness * (1.0 - cloud_color_change_fraction);
	m_cloudcolor_f = video::SColorf(
		m_cloudcolor_bright_f.r * m_cloud_brightness,
		m_cloudcolor_bright_f.g * m_cloud_brightness,
		m_cloudcolor_bright_f.b * m_cloud_brightness,
		1.0
	);
	if (m_directional_colored_fog) {
		m_cloudcolor_f = m_mix_scolorf(m_cloudcolor_f,
			video::SColorf(pointcolor), m_horizon_blend() * 0.25);
	}
}

void Sky::draw_sun(video::IVideoDriver *driver, float sunsize, const video::SColor &suncolor,
	const video::SColor &suncolor2, float wicked_time_of_day)
	/* Draw sun in the sky.
	 * driver: Video driver object used to draw
	 * sunsize: the default size of the sun
	 * suncolor: main sun color
	 * suncolor2: second sun color
	 * wicked_time_of_day: current time of day, to know where should be the sun in the sky
	 */
{
	static const u16 indices[] = {0, 1, 2, 0, 2, 3};
	std::array<video::S3DVertex, 4> vertices;
	if (!m_sun_texture) {
		driver->setMaterial(m_materials[1]);
		const float sunsizes[4] = {
			(sunsize * 1.7f) * m_sun_params.scale,
			(sunsize * 1.2f) * m_sun_params.scale,
			(sunsize) * m_sun_params.scale,
			(sunsize * 0.7f) * m_sun_params.scale
		};
		video::SColor c1 = suncolor;
		video::SColor c2 = suncolor;
		c1.setAlpha(0.05 * 255);
		c2.setAlpha(0.15 * 255);
		const video::SColor colors[4] = {c1, c2, suncolor, suncolor2};
		for (int i = 0; i < 4; i++) {
			draw_sky_body(vertices, -sunsizes[i], sunsizes[i], colors[i]);

			if (m_use_custom_sky_body_pos)
				place_sky_body(vertices, m_custom_sun_horizon_pos, m_custom_sun_day_pos);
			else
				place_sky_body(vertices, 90, wicked_time_of_day * 360 - 90);

			driver->drawIndexedTriangleList(&vertices[0], 4, indices, 2);
		}
	} else {
		driver->setMaterial(m_materials[3]);
		float d = (sunsize * 1.7) * m_sun_params.scale;
		video::SColor c;
		if (m_sun_tonemap)
			c = video::SColor(0, 0, 0, 0);
		else
			c = video::SColor(255, 255, 255, 255);
		draw_sky_body(vertices, -d, d, c);

		if (m_use_custom_sky_body_pos)
			place_sky_body(vertices, m_custom_sun_horizon_pos, m_custom_sun_day_pos);
		else
			place_sky_body(vertices, 90, wicked_time_of_day * 360 - 90);

		driver->drawIndexedTriangleList(&vertices[0], 4, indices, 2);
	}
}


void Sky::draw_moon(video::IVideoDriver *driver, float moonsize, const video::SColor &mooncolor,
	const video::SColor &mooncolor2, float wicked_time_of_day)
/*
	* Draw moon in the sky.
	* driver: Video driver object used to draw
	* moonsize: the default size of the moon
	* mooncolor: main moon color
	* mooncolor2: second moon color
	* wicked_time_of_day: current time of day, to know where should be the moon in
	* the sky
	*/
{
	static const u16 indices[] = {0, 1, 2, 0, 2, 3};
	std::array<video::S3DVertex, 4> vertices;
	if (!m_moon_texture) {
		driver->setMaterial(m_materials[1]);
		const float moonsizes_1[4] = {
			(-moonsize * 1.9f) * m_moon_params.scale,
			(-moonsize * 1.3f) * m_moon_params.scale,
			(-moonsize) * m_moon_params.scale,
			(-moonsize) * m_moon_params.scale
		};
		const float moonsizes_2[4] = {
			(moonsize * 1.9f) * m_moon_params.scale,
			(moonsize * 1.3f) * m_moon_params.scale,
			(moonsize) *m_moon_params.scale,
			(moonsize * 0.6f) * m_moon_params.scale
		};
		video::SColor c1 = mooncolor;
		video::SColor c2 = mooncolor;
		c1.setAlpha(0.05 * 255);
		c2.setAlpha(0.15 * 255);
		const video::SColor colors[4] = {c1, c2, mooncolor, mooncolor2};
		for (int i = 0; i < 4; i++) {
			draw_sky_body(vertices, moonsizes_1[i], moonsizes_2[i], colors[i]);

			if (m_use_custom_sky_body_pos)
				place_sky_body(vertices, m_custom_moon_horizon_pos, m_custom_moon_day_pos);
			else
				place_sky_body(vertices, -90, wicked_time_of_day * 360 - 90);

			driver->drawIndexedTriangleList(&vertices[0], 4, indices, 2);
		}
	} else {
		driver->setMaterial(m_materials[4]);
		float d = (moonsize * 1.9) * m_moon_params.scale;
		video::SColor c;
		if (m_moon_tonemap)
			c = video::SColor(0, 0, 0, 0);
		else
			c = video::SColor(255, 255, 255, 255);
		draw_sky_body(vertices, -d, d, c);

		if (m_use_custom_sky_body_pos)
			place_sky_body(vertices, m_custom_moon_horizon_pos, m_custom_moon_day_pos);
		else
			place_sky_body(vertices, -90, wicked_time_of_day * 360 - 90);

		driver->drawIndexedTriangleList(&vertices[0], 4, indices, 2);
	}
}

void Sky::draw_stars(video::IVideoDriver * driver, float wicked_time_of_day)
{
	// Tune values so that stars first appear just after the sun
	// disappears over the horizon, and disappear just before the sun
	// appears over the horizon.
	// Also tune so that stars are at full brightness from time 20000
	// to time 4000.

	float tod = wicked_time_of_day < 0.5f ? wicked_time_of_day : (1.0f - wicked_time_of_day);
	float starbrightness = (0.25f - fabsf(tod)) * 20.0f;
	m_star_color = m_star_params.starcolor;
	m_star_color.a *= clamp(starbrightness, 0.0f, 1.0f);
	if (m_star_color.a <= 0.0f) // Stars are only drawn when not fully transparent
		return;
	m_materials[0].DiffuseColor = m_materials[0].EmissiveColor = m_star_color.toSColor();
	auto sky_rotation = core::matrix4().setRotationAxisRadians(2.0f * M_PI * (wicked_time_of_day - 0.25f), v3f(0.0f, 0.0f, 1.0f));
	auto world_matrix = driver->getTransform(video::ETS_WORLD);
	driver->setTransform(video::ETS_WORLD, world_matrix * sky_rotation);
	driver->setMaterial(m_materials[0]);
	driver->drawMeshBuffer(m_stars.get());
	driver->setTransform(video::ETS_WORLD, world_matrix);
}

void Sky::draw_sky_body(std::array<video::S3DVertex, 4> &vertices, float pos_1, float pos_2, const video::SColor &c)
{
	/*
	* Create an array of vertices with the dimensions specified.
	* pos_1, pos_2: position of the body's vertices
	* c: color of the body
	*/

	const f32 t = 1.0f;
	const f32 o = 0.0f;
	vertices[0] = video::S3DVertex(pos_1, pos_1, -1, 0, 0, 1, c, t, t);
	vertices[1] = video::S3DVertex(pos_2, pos_1, -1, 0, 0, 1, c, o, t);
	vertices[2] = video::S3DVertex(pos_2, pos_2, -1, 0, 0, 1, c, o, o);
	vertices[3] = video::S3DVertex(pos_1, pos_2, -1, 0, 0, 1, c, t, o);
}


void Sky::place_sky_body(
	std::array<video::S3DVertex, 4> &vertices, float horizon_position, float day_position)
	/*
	* Place body in the sky.
	* vertices: The body as a rectangle of 4 vertices
	* horizon_position: turn the body around the Y axis
	* day_position: turn the body around the Z axis, to place it depending of the time of the day
	*/
{
	for (video::S3DVertex &vertex : vertices) {
		// Body is directed to -Z (south) by default
		vertex.Pos.rotateXZBy(horizon_position);
		vertex.Pos.rotateXYBy(day_position);
	}
}

void Sky::setSunTexture(const std::string &sun_texture,
		const std::string &sun_tonemap, ITextureSource *tsrc)
{
	// Ignore matching textures (with modifiers) entirely,
	// but lets at least update the tonemap before hand.
	if (m_sun_tonemap)
		m_sun_tonemap->drop();

	m_sun_params.tonemap = sun_tonemap;
	m_sun_tonemap = tsrc->isKnownSourceImage(m_sun_params.tonemap) ?
		tsrc->getTexture(m_sun_params.tonemap) : nullptr;
	m_materials[3].Lighting = !!m_sun_tonemap;

	if (m_sun_tonemap)
		m_sun_tonemap->grab();

	if (m_sun_params.texture == sun_texture)
		return;

	m_sun_params.texture = sun_texture;

	if (m_sun_texture)
		m_sun_texture->drop();

	if (sun_texture != "") {
		// We want to ensure the texture exists first.
		m_sun_texture = tsrc->getTextureForMesh(m_sun_params.texture);

		if (m_sun_texture) {
			m_sun_texture->grab();
			m_materials[3] = baseMaterial();
			m_materials[3].setTexture(0, m_sun_texture);
			m_materials[3].MaterialType = video::
				EMT_TRANSPARENT_ALPHA_CHANNEL;
			// Disables texture filtering
			m_materials[3].setFlag(
				video::E_MATERIAL_FLAG::EMF_BILINEAR_FILTER, false);
			m_materials[3].setFlag(
				video::E_MATERIAL_FLAG::EMF_TRILINEAR_FILTER, false);
			m_materials[3].setFlag(
				video::E_MATERIAL_FLAG::EMF_ANISOTROPIC_FILTER, false);
		}
	} else {
		m_sun_texture = nullptr;
	}
}

void Sky::setSunriseTexture(const std::string &sunglow_texture,
		ITextureSource* tsrc)
{
	// Ignore matching textures (with modifiers) entirely.
	if (m_sun_params.sunrise == sunglow_texture)
		return;
	m_sun_params.sunrise = sunglow_texture;
	m_materials[2].setTexture(0, tsrc->getTextureForMesh(
		sunglow_texture.empty() ? "sunrisebg.png" : sunglow_texture)
	);
}

void Sky::setMoonTexture(const std::string &moon_texture,
	const std::string &moon_tonemap, ITextureSource *tsrc)
{
	// Ignore matching textures (with modifiers) entirely,
	// but lets at least update the tonemap before hand.
	if (m_moon_tonemap)
		m_moon_tonemap->drop();

	m_moon_params.tonemap = moon_tonemap;
	m_moon_tonemap = tsrc->isKnownSourceImage(m_moon_params.tonemap) ?
		tsrc->getTexture(m_moon_params.tonemap) : nullptr;
	m_materials[4].Lighting = !!m_moon_tonemap;

	if (m_moon_tonemap)
		m_moon_tonemap->grab();

	if (m_moon_params.texture == moon_texture)
		return;

	m_moon_params.texture = moon_texture;

	if (m_moon_texture)
		m_moon_texture->drop();

	if (moon_texture != "") {
		// We want to ensure the texture exists first.
		m_moon_texture = tsrc->getTextureForMesh(m_moon_params.texture);

		if (m_moon_texture) {
			m_moon_texture->grab();
			m_materials[4] = baseMaterial();
			m_materials[4].setTexture(0, m_moon_texture);
			m_materials[4].MaterialType = video::
				EMT_TRANSPARENT_ALPHA_CHANNEL;
			// Disables texture filtering
			m_materials[4].setFlag(
				video::E_MATERIAL_FLAG::EMF_BILINEAR_FILTER, false);
			m_materials[4].setFlag(
				video::E_MATERIAL_FLAG::EMF_TRILINEAR_FILTER, false);
			m_materials[4].setFlag(
				video::E_MATERIAL_FLAG::EMF_ANISOTROPIC_FILTER, false);
		}
	} else {
		m_moon_texture = nullptr;
	}
}

void Sky::setStarScale(f32 star_scale)
{
	if (m_star_params.scale != star_scale) {
		m_star_params.scale = star_scale;
		updateStars();
	}
}

void Sky::setStarCount(u16 star_count, bool force_update)
{
	// Allow force updating star count at game init.
	if (m_star_params.count != star_count || force_update) {
		m_star_params.count = star_count;
		m_seed = (u64)myrand() << 32 | myrand();
		updateStars();
	}
}

void Sky::updateStars()
{
	m_stars.reset(new scene::SMeshBuffer());
	// Stupid IrrLicht doesn’t allow non-indexed rendering, and indexed quad
	// rendering is slow due to lack of hardware support. So as indices are
	// 16-bit and there are 4 vertices per star... the limit is 2^16/4 = 0x4000.
	// That should be well enough actually.
	if (m_star_params.count > 0x4000) {
		warningstream << "Requested " << m_star_params.count << " stars but " << 0x4000 << " is the max\n";
		m_star_params.count = 0x4000;
	}
	m_stars->Vertices.reallocate(4 * m_star_params.count);
	m_stars->Indices.reallocate(6 * m_star_params.count);

	video::SColor fallback_color = m_star_params.starcolor; // used on GLES 2 “without shaders”
	PcgRandom rgen(m_seed);
	float d = (0.003 / 2) * m_star_params.scale;
	for (u16 i = 0; i < m_star_params.count; i++) {
		v3f r = v3f(
			rgen.range(-10000, 10000),
			rgen.range(-10000, 10000),
			rgen.range(-10000, 10000)
		);
		core::CMatrix4<f32> a;
		a.buildRotateFromTo(v3f(0, 1, 0), r);
		v3f p = v3f(-d, 1, -d);
		v3f p1 = v3f(d, 1, -d);
		v3f p2 = v3f(d, 1, d);
		v3f p3 = v3f(-d, 1, d);
		a.rotateVect(p);
		a.rotateVect(p1);
		a.rotateVect(p2);
		a.rotateVect(p3);
		m_stars->Vertices.push_back(video::S3DVertex(p, {}, fallback_color, {}));
		m_stars->Vertices.push_back(video::S3DVertex(p1, {}, fallback_color, {}));
		m_stars->Vertices.push_back(video::S3DVertex(p2, {}, fallback_color, {}));
		m_stars->Vertices.push_back(video::S3DVertex(p3, {}, fallback_color, {}));
	}
	for (u16 i = 0; i < m_star_params.count; i++) {
		m_stars->Indices.push_back(i * 4 + 0);
		m_stars->Indices.push_back(i * 4 + 1);
		m_stars->Indices.push_back(i * 4 + 2);
		m_stars->Indices.push_back(i * 4 + 2);
		m_stars->Indices.push_back(i * 4 + 3);
		m_stars->Indices.push_back(i * 4 + 0);
	}
	m_stars->setHardwareMappingHint(scene::EHM_STATIC);
}

void Sky::setSkyColors(const SkyColor &sky_color)
{
	m_sky_params.sky_color = sky_color;
}

void Sky::setHorizonTint(video::SColor sun_tint, video::SColor moon_tint,
	const std::string &use_sun_tint)
{
	// Change sun and moon tinting:
	m_sky_params.fog_sun_tint = sun_tint;
	m_sky_params.fog_moon_tint = moon_tint;
	// Faster than comparing strings every rendering frame
	if (use_sun_tint == "default")
		m_default_tint = true;
	else if (use_sun_tint == "custom")
		m_default_tint = false;
	else
		m_default_tint = true;
}

void Sky::addTextureToSkybox(const std::string &texture, int material_id,
		ITextureSource *tsrc)
{
	// Sanity check for more than six textures.
	if (material_id + 5 >= SKY_MATERIAL_COUNT)
		return;
	// Keep a list of texture names handy.
	m_sky_params.textures.emplace_back(texture);
	video::ITexture *result = tsrc->getTextureForMesh(texture);
	m_materials[material_id+5] = baseMaterial();
	m_materials[material_id+5].setTexture(0, result);
	m_materials[material_id+5].MaterialType = video::EMT_SOLID;
}

// To be called once at game init to setup default values.
void Sky::setSkyDefaults()
{
	SkyboxDefaults sky_defaults;
	m_sky_params.sky_color = sky_defaults.getSkyColorDefaults();
	m_sun_params = sky_defaults.getSunDefaults();
	m_moon_params = sky_defaults.getMoonDefaults();
	m_star_params = sky_defaults.getStarDefaults();
}

void Sky::setCustomSkyBodyPos(float moon_horizon_pos, float moon_day_pos,
		float sun_horizon_pos, float sun_day_pos)
{
	m_use_custom_sky_body_pos = true;
	m_custom_moon_horizon_pos = moon_horizon_pos;
	m_custom_moon_day_pos = moon_day_pos;
	m_custom_sun_horizon_pos = sun_horizon_pos;
	m_custom_sun_day_pos = sun_day_pos;
}
