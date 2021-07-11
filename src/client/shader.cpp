/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2013 Kahrl <kahrl@gmx.net>

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

#include <fstream>
#include <iterator>
#include <unordered_map>
#include "shader.h"
#include "irrlichttypes_extrabloated.h"
#include "irr_ptr.h"
#include "debug.h"
#include "filesys.h"
#include "util/container.h"
#include "util/thread.h"
#include "settings.h"
#include <ICameraSceneNode.h>
#include <IGPUProgrammingServices.h>
#include <IMaterialRenderer.h>
#include <IMaterialRendererServices.h>
#include <IShaderConstantSetCallBack.h>
#include "client/renderingengine.h"
#include "EShaderTypes.h"
#include "log.h"
#include "gamedef.h"
#include "client/tile.h"
#include "light.h"
#include "config.h"
#include "util/string.h"

/*
	A cache from shader name to shader path
*/
MutexedMap<std::string, std::string> g_shadername_to_path_cache;

/*
	Gets the path to a shader by first checking if the file
	  name_of_shader/filename
	exists in shader_path and if not, using the data path.

	If not found, returns "".

	Utilizes a thread-safe cache.
*/
std::string getShaderPath(const std::string &name_of_shader,
		const std::string &filename)
{
	std::string combined = name_of_shader + DIR_DELIM + filename;
	std::string fullpath;
	/*
		Check from cache
	*/
	bool incache = g_shadername_to_path_cache.get(combined, &fullpath);
	if(incache)
		return fullpath;

	/*
		Check from shader_path
	*/
	std::string shader_path = g_settings->get("shader_path");
	if (!shader_path.empty()) {
		std::string testpath = shader_path + DIR_DELIM + combined;
		if(fs::PathExists(testpath))
			fullpath = testpath;
	}

	/*
		Check from default data directory
	*/
	if (fullpath.empty()) {
		std::string rel_path = std::string("client") + DIR_DELIM
				+ "shaders" + DIR_DELIM
				+ name_of_shader + DIR_DELIM
				+ filename;
		std::string testpath = porting::path_share + DIR_DELIM + rel_path;
		if(fs::PathExists(testpath))
			fullpath = testpath;
	}

	// Add to cache (also an empty result is cached)
	g_shadername_to_path_cache.set(combined, fullpath);

	// Finally return it
	return fullpath;
}

/*
	SourceShaderCache: A cache used for storing source shaders.
*/

class SourceShaderCache
{
public:
	void insert(const std::string &name_of_shader, const std::string &filename,
		const std::string &program, bool prefer_local)
	{
		std::string combined = name_of_shader + DIR_DELIM + filename;
		// Try to use local shader instead if asked to
		if(prefer_local){
			std::string path = getShaderPath(name_of_shader, filename);
			if(!path.empty()){
				std::string p = readFile(path);
				if (!p.empty()) {
					m_programs[combined] = p;
					return;
				}
			}
		}
		m_programs[combined] = program;
	}

	std::string get(const std::string &name_of_shader,
		const std::string &filename)
	{
		std::string combined = name_of_shader + DIR_DELIM + filename;
		StringMap::iterator n = m_programs.find(combined);
		if (n != m_programs.end())
			return n->second;
		return "";
	}

	// Primarily fetches from cache, secondarily tries to read from filesystem
	std::string getOrLoad(const std::string &name_of_shader,
		const std::string &filename)
	{
		std::string combined = name_of_shader + DIR_DELIM + filename;
		StringMap::iterator n = m_programs.find(combined);
		if (n != m_programs.end())
			return n->second;
		std::string path = getShaderPath(name_of_shader, filename);
		if (path.empty()) {
			infostream << "SourceShaderCache::getOrLoad(): No path found for \""
				<< combined << "\"" << std::endl;
			return "";
		}
		infostream << "SourceShaderCache::getOrLoad(): Loading path \""
			<< path << "\"" << std::endl;
		std::string p = readFile(path);
		if (!p.empty()) {
			m_programs[combined] = p;
			return p;
		}
		return "";
	}
private:
	StringMap m_programs;

	std::string readFile(const std::string &path)
	{
		std::ifstream is(path.c_str(), std::ios::binary);
		if(!is.is_open())
			return "";
		std::ostringstream tmp_os;
		tmp_os << is.rdbuf();
		return tmp_os.str();
	}
};


/*
	ShaderCallback: Sets constants that can be used in shaders
*/

class ShaderCallback : public video::IShaderConstantSetCallBack
{
	std::vector<std::unique_ptr<IShaderConstantSetter>> m_setters;

public:
	template <typename Factories>
	ShaderCallback(const Factories &factories)
	{
		for (auto &&factory : factories)
			m_setters.push_back(std::unique_ptr<IShaderConstantSetter>(factory->create()));
	}

	void addSetter(IShaderConstantSetter *setter)
	{
		m_setters.emplace_back(setter);
	}

	virtual void OnSetConstants(video::IMaterialRendererServices *services, s32 userData) override
	{
		video::IVideoDriver *driver = services->getVideoDriver();
		sanity_check(driver != NULL);

		for (auto &&setter : m_setters)
			setter->onSetConstants(services);
	}

	virtual void OnSetMaterial(const video::SMaterial& material) override
	{
		for (auto &&setter : m_setters)
			setter->onSetMaterial(material);
	}
};


/*
	MainShaderConstantSetter: Set basic constants required for almost everything
*/

class MainShaderConstantSetter : public IShaderConstantSetter
{
	CachedVertexShaderSetting<float, 16> m_world_view_proj;
	CachedVertexShaderSetting<float, 16> m_world;

	// Shadow-related
	CachedPixelShaderSetting<float, 16> m_shadow_view_proj;
	CachedPixelShaderSetting<float, 3> m_light_direction;
	CachedPixelShaderSetting<float> m_texture_res;
	CachedPixelShaderSetting<float> m_shadow_strength;
	CachedPixelShaderSetting<float> m_time_of_day;
	CachedPixelShaderSetting<float> m_shadowfar;
	CachedPixelShaderSetting<s32> m_shadow_texture;

#if ENABLE_GLES
	// Modelview matrix
	CachedVertexShaderSetting<float, 16> m_world_view;
	// Texture matrix
	CachedVertexShaderSetting<float, 16> m_texture;
#endif
	core::matrix4 m_world_in, m_view_in, m_proj_in;
	core::matrix4 m_mat_world_view, m_mat_world_view_proj;

	CachedPixelShaderSetting<float, 3> m_node_color;
	video::SColor m_material_color = 0xFFFFFFFF;

public:
	MainShaderConstantSetter() :
		  m_world_view_proj("mWorldViewProj")
		, m_world("mWorld")
#if ENABLE_GLES
		, m_world_view("mWorldView")
		, m_texture("mTexture")
#endif
		, m_node_color("nodeColor")
		, m_shadow_view_proj("m_ShadowViewProj")
		, m_light_direction("v_LightDirection")
		, m_texture_res("f_textureresolution")
		, m_shadow_strength("f_shadow_strength")
		, m_time_of_day("f_timeofday")
		, m_shadowfar("f_shadowfar")
		, m_shadow_texture("ShadowMapSampler")
	{}
	~MainShaderConstantSetter() = default;

	virtual void onSetMaterial(const video::SMaterial &material) override
	{
		m_material_color = material.DiffuseColor;
	}

	virtual void onSetConstants(video::IMaterialRendererServices *services) override
	{
		video::IVideoDriver *driver = services->getVideoDriver();
		sanity_check(driver);

		float node_color[3] = {
			m_material_color.getRed() / 255.0f,
			m_material_color.getGreen() / 255.0f,
			m_material_color.getBlue() / 255.0f };
		m_node_color.set(node_color, services);

		const core::matrix4 &world = driver->getTransform(video::ETS_WORLD);
		const core::matrix4 &view = driver->getTransform(video::ETS_VIEW);
		const core::matrix4 &proj = driver->getTransform(video::ETS_PROJECTION);

		// View and projection hold for the whole frame, and blocks baked
		// against one camera offset share a world matrix, so the products
		// survive across most of the draw calls.
		if (world != m_world_in || view != m_view_in || proj != m_proj_in) {
			m_world_in = world;
			m_view_in = view;
			m_proj_in = proj;
			m_mat_world_view = view * world;
			m_mat_world_view_proj = proj * m_mat_world_view;
		}

		m_world.set(*reinterpret_cast<float(*)[16]>(m_world_in.pointer()), services);
		m_world_view_proj.set(*reinterpret_cast<float(*)[16]>(m_mat_world_view_proj.pointer()), services);

#if ENABLE_GLES
		// What OpenGL loads into gl_TextureMatrix: Irrlicht keeps the 2D move in the third column
		const core::matrix4 &t = driver->getTransform(video::ETS_TEXTURE_0);
		const float texture[16] = {t[0], t[1], 0.f, 0.f, t[4], t[5], 0.f, 0.f,
				0.f, 0.f, 1.f, 0.f, t[8], t[9], 0.f, 1.f};
		m_world_view.set(*reinterpret_cast<float(*)[16]>(m_mat_world_view.pointer()), services);
		m_texture.set(texture, services);
#endif

		// Set uniforms for Shadow shader
		if (ShadowRenderer *shadow = RenderingEngine::get_shadow_renderer()) {
			const auto &light = shadow->getDirectionalLight();

			core::matrix4 shadowViewProj = light.getProjectionMatrix();
			shadowViewProj *= light.getViewMatrix();
			m_shadow_view_proj.set(shadowViewProj.pointer(), services);

			float v_LightDirection[3];
			light.getDirection().getAs3Values(v_LightDirection);
			m_light_direction.set(v_LightDirection, services);

			float TextureResolution = light.getMapResolution();
			m_texture_res.set(&TextureResolution, services);

			float ShadowStrength = shadow->getShadowStrength();
			m_shadow_strength.set(&ShadowStrength, services);

			float timeOfDay = shadow->getTimeOfDay();
			m_time_of_day.set(&timeOfDay, services);

			float shadowFar = shadow->getMaxShadowFar();
			m_shadowfar.set(&shadowFar, services);

			// I dont like using this hardcoded value. maybe something like
			// MAX_TEXTURE - 1 or somthing like that??
			s32 TextureLayerID = 3;
			m_shadow_texture.set(&TextureLayerID, services);
		}
	}
};


class MainShaderConstantSetterFactory : public IShaderConstantSetterFactory
{
public:
	virtual IShaderConstantSetter* create()
		{ return new MainShaderConstantSetter(); }
};


/*
	SettingsShaderConstantSetter: Sets the settings the header bakes in, for programs built ahead of
	time
*/

class SettingsShaderConstantSetter : public IShaderConstantSetter
{
	CachedPixelShaderSetting<float> m_fog_shading;
	CachedPixelShaderSetting<float, 4> m_light_curve;
	CachedPixelShaderSetting<float, 4> m_light_boost;
	CachedVertexShaderSetting<float, 3> m_water_wave;
	float m_fog_shading_value;
	float m_light_curve_value[4];
	float m_light_boost_value[4];
	float m_water_wave_value[3];

public:
	SettingsShaderConstantSetter(float fog_shading, const float light_curve[4],
			const float light_boost[4], const float water_wave[3]) :
		  m_fog_shading("fogShadingParameter")
		, m_light_curve("lightCurve")
		, m_light_boost("lightBoost")
		, m_water_wave("waterWave")
		, m_fog_shading_value(fog_shading)
	{
		std::copy(light_curve, light_curve + 4, m_light_curve_value);
		std::copy(light_boost, light_boost + 4, m_light_boost_value);
		std::copy(water_wave, water_wave + 3, m_water_wave_value);
	}

	virtual void onSetConstants(video::IMaterialRendererServices *services) override
	{
		m_fog_shading.set(&m_fog_shading_value, services);
		m_light_curve.set(m_light_curve_value, services);
		m_light_boost.set(m_light_boost_value, services);
		m_water_wave.set(m_water_wave_value, services);
	}
};


/*
	AlphaRefShaderConstantSetter: Sets the alpha reference of the material, for programs that
	discard by it
*/

class AlphaRefShaderConstantSetter : public IShaderConstantSetter
{
	CachedPixelShaderSetting<float> m_alpha_ref;
	float m_alpha_ref_value = 0.0f;

public:
	AlphaRefShaderConstantSetter() :
		  m_alpha_ref("alphaRef")
	{}

	virtual void onSetMaterial(const video::SMaterial &material) override
	{
		m_alpha_ref_value = material.MaterialTypeParam;
	}

	virtual void onSetConstants(video::IMaterialRendererServices *services) override
	{
		m_alpha_ref.set(&m_alpha_ref_value, services);
	}
};

// The number a GLSL compiler reads where the header spells the value out
static float asSpelled(float value)
{
	std::stringstream spelled;
	spelled << std::showpoint << value;
	spelled >> value;
	return value;
}


/*
	ShaderSource
*/

class ShaderSource : public IWritableShaderSource
{
public:
	ShaderSource();

	/*
		- If shader material specified by name is found from cache,
		  return the cached id.
		- Otherwise generate the shader material, add to cache and return id.

		The id 0 points to a null shader. Its material is EMT_SOLID.
	*/
	u32 getShaderIdDirect(const std::string &name,
		MaterialType material_type, NodeDrawType drawtype) override;

	/*
		If shader specified by the name pointed by the id doesn't
		exist, create it, then return id.

		Can be called from any thread. If called from some other thread
		and not found in cache, the call is queued to the main thread
		for processing.
	*/

	u32 getShader(const std::string &name,
		MaterialType material_type, NodeDrawType drawtype) override;

	ShaderInfo getShaderInfo(u32 id) override;
	video::E_MATERIAL_TYPE getMaterialWithoutPointLights(video::E_MATERIAL_TYPE material) override;

	// Processes queued shader requests from other threads.
	// Shall be called from the main thread.
	void processQueue() override;

	// Insert a shader program into the cache without touching the
	// filesystem. Shall be called from the main thread.
	void insertSourceShader(const std::string &name_of_shader,
		const std::string &filename, const std::string &program) override;

	// Rebuild shaders from the current set of source shaders
	// Shall be called from the main thread.
	void rebuildShaders() override;

	void addShaderConstantSetterFactory(IShaderConstantSetterFactory *setter) override
	{
		m_setter_factories.push_back(std::unique_ptr<IShaderConstantSetterFactory>(setter));
	}

private:

	// The id of the thread that is allowed to use irrlicht directly
	std::thread::id m_main_thread;

	// Cache of source shaders
	// This should be only accessed from the main thread
	SourceShaderCache m_sourcecache;

	// A shader id is index in this array.
	// The first position contains a dummy shader.
	std::vector<ShaderInfo> m_shaderinfo_cache;
	// The former container is behind this mutex
	std::mutex m_shaderinfo_cache_mutex;

	// Queued shader fetches (to be processed by the main thread)
	RequestQueue<std::string, u32, u8, u8> m_get_shader_queue;

	// Global constant setter factories
	std::vector<std::unique_ptr<IShaderConstantSetterFactory>> m_setter_factories;

	// The node programs with point lights and the same programs without, main thread only
	std::unordered_map<video::E_MATERIAL_TYPE, video::E_MATERIAL_TYPE> m_without_point_lights;

	// Generate shader given the shader name.
	ShaderInfo generateShader(const std::string &name,
			MaterialType material_type, NodeDrawType drawtype, bool point_lights = true);
};

IWritableShaderSource *createShaderSource()
{
	return new ShaderSource();
}

ShaderSource::ShaderSource()
{
	m_main_thread = std::this_thread::get_id();

	// Add a dummy ShaderInfo as the first index, named ""
	m_shaderinfo_cache.emplace_back();

	// Add main global constant setter
	addShaderConstantSetterFactory(new MainShaderConstantSetterFactory());
}

u32 ShaderSource::getShader(const std::string &name,
		MaterialType material_type, NodeDrawType drawtype)
{
	/*
		Get shader
	*/

	if (std::this_thread::get_id() == m_main_thread) {
		return getShaderIdDirect(name, material_type, drawtype);
	}

	/*errorstream<<"getShader(): Queued: name=\""<<name<<"\""<<std::endl;*/

	// We're gonna ask the result to be put into here

	static ResultQueue<std::string, u32, u8, u8> result_queue;

	// Throw a request in
	m_get_shader_queue.add(name, 0, 0, &result_queue);

	/* infostream<<"Waiting for shader from main thread, name=\""
			<<name<<"\""<<std::endl;*/

	while(true) {
		GetResult<std::string, u32, u8, u8>
			result = result_queue.pop_frontNoEx();

		if (result.key == name) {
			return result.item;
		}

		errorstream << "Got shader with invalid name: " << result.key << std::endl;
	}

	infostream << "getShader(): Failed" << std::endl;

	return 0;
}

/*
	This method generates all the shaders
*/
u32 ShaderSource::getShaderIdDirect(const std::string &name,
		MaterialType material_type, NodeDrawType drawtype)
{
	//infostream<<"getShaderIdDirect(): name=\""<<name<<"\""<<std::endl;

	// Empty name means shader 0
	if (name.empty()) {
		infostream<<"getShaderIdDirect(): name is empty"<<std::endl;
		return 0;
	}

	// Only the dynamic shadows read DRAW_TYPE, without them one program serves every drawtype
	if (!g_settings->getBool("enable_dynamic_shadows"))
		drawtype = NDT_NORMAL;

	// Check if already have such instance
	for(u32 i=0; i<m_shaderinfo_cache.size(); i++){
		ShaderInfo *info = &m_shaderinfo_cache[i];
		if(info->name == name && info->material_type == material_type &&
			info->drawtype == drawtype)
			return i;
	}

	/*
		Calling only allowed from main thread
	*/
	if (std::this_thread::get_id() != m_main_thread) {
		errorstream<<"ShaderSource::getShaderIdDirect() "
				"called not from main thread"<<std::endl;
		return 0;
	}

	ShaderInfo info = generateShader(name, material_type, drawtype);

	/*
		Add shader to caches (add dummy shaders too)
	*/

	MutexAutoLock lock(m_shaderinfo_cache_mutex);

	u32 id = m_shaderinfo_cache.size();
	m_shaderinfo_cache.push_back(info);

	infostream<<"getShaderIdDirect(): "
			<<"Returning id="<<id<<" for name \""<<name<<"\""<<std::endl;

	return id;
}


ShaderInfo ShaderSource::getShaderInfo(u32 id)
{
	MutexAutoLock lock(m_shaderinfo_cache_mutex);

	if(id >= m_shaderinfo_cache.size())
		return ShaderInfo();

	return m_shaderinfo_cache[id];
}

void ShaderSource::processQueue()
{


}

void ShaderSource::insertSourceShader(const std::string &name_of_shader,
		const std::string &filename, const std::string &program)
{
	/*infostream<<"ShaderSource::insertSourceShader(): "
			"name_of_shader=\""<<name_of_shader<<"\", "
			"filename=\""<<filename<<"\""<<std::endl;*/

	sanity_check(std::this_thread::get_id() == m_main_thread);

	m_sourcecache.insert(name_of_shader, filename, program, true);
}

void ShaderSource::rebuildShaders()
{
	// Programs built ahead take the settings as uniforms, so the first build already stands
	const video::E_DRIVER_TYPE type = RenderingEngine::get_video_driver()->getDriverType();
	if (type == video::EDT_METAL)
		return;

	MutexAutoLock lock(m_shaderinfo_cache_mutex);

	/*// Oh well... just clear everything, they'll load sometime.
	m_shaderinfo_cache.clear();
	m_name_to_id.clear();*/

	/*
		FIXME: Old shader materials can't be deleted in Irrlicht,
		or can they?
		(This would be nice to do in the destructor too)
	*/

	// Recreate shaders
	m_without_point_lights.clear();
	for (ShaderInfo &i : m_shaderinfo_cache) {
		ShaderInfo *info = &i;
		if (!info->name.empty()) {
			*info = generateShader(info->name, info->material_type, info->drawtype);
		}
	}
}


//! Names the branch a driver without a shader compiler builds as a program of its own
static std::string shaderVariant(const std::string &name, MaterialType material_type,
		video::E_MATERIAL_TYPE base_material, bool point_lights)
{
	// Every other shader is one program
	if (name != "nodes_shader" && name != "object_shader")
		return "";

	std::string variant;

	if (name == "nodes_shader") {
		if (g_settings->getBool("enable_waving_leaves") &&
				material_type == TILE_MATERIAL_WAVING_LEAVES)
			variant = ":leaves";
		else if (g_settings->getBool("enable_waving_plants") &&
				material_type == TILE_MATERIAL_WAVING_PLANTS)
			variant = ":plants";
		else if (g_settings->getBool("enable_waving_water") &&
				(material_type == TILE_MATERIAL_WAVING_LIQUID_TRANSPARENT ||
				material_type == TILE_MATERIAL_WAVING_LIQUID_OPAQUE ||
				material_type == TILE_MATERIAL_WAVING_LIQUID_BASIC))
			variant = ":liquid";
	} else if (material_type == TILE_MATERIAL_PLAIN ||
			material_type == TILE_MATERIAL_PLAIN_ALPHA) {
		variant = ":plain";
	}

	// Which test throws pixels away comes from the base material
	if (base_material == video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF)
		variant += ":ref";
	else if (base_material != video::EMT_SOLID)
		variant += ":blend";

	if (g_settings->getBool("tone_mapping"))
		variant += ":tonemap";

	if (point_lights)
		variant += ":lights";

	return variant;
}

// One row per line, its fields split on '|'
static std::vector<std::vector<std::string>> readTable(const std::string &path)
{
	std::vector<std::vector<std::string>> rows;
	std::string text;
	if (fs::ReadFile(path, text))
		for (const std::string &line : str_split(text, '\n'))
			rows.push_back(str_split(line, '|'));
	return rows;
}

void registerPrecompiledShaders(video::IVideoDriver *driver)
{
	if (!driver || driver->getDriverType() != video::EDT_METAL)
		return;

	const std::string dir = porting::path_share + DIR_DELIM + "client" + DIR_DELIM +
			"shaders" + DIR_DELIM + "Metal" + DIR_DELIM;

	// Metal takes the MSL of both stages of each program
	std::unordered_map<std::string, std::string> sources;
	for (const std::vector<std::string> &p : readTable(dir + "programs.txt")) {
		if (p.size() != 3)
			continue;
		std::string vertex = "vs_" + p[1], fragment = "fs_" + p[2];
		for (std::string *stage : {&vertex, &fragment}) {
			std::string &source = sources[*stage];
			if (source.empty())
				fs::ReadFile(dir + *stage + ".metal", source);
			*stage = source;
		}
		if (vertex.empty() || fragment.empty() ||
				!driver->addPrecompiledShader(p[0].c_str(), vertex.c_str(),
				(u32)vertex.size() + 1, fragment.c_str(), (u32)fragment.size() + 1))
			errorstream << "Could not register the Metal program " << p[0] << std::endl;
	}
}


ShaderInfo ShaderSource::generateShader(const std::string &name,
		MaterialType material_type, NodeDrawType drawtype, bool point_lights)
{
	ShaderInfo shaderinfo;
	shaderinfo.name = name;
	shaderinfo.material_type = material_type;
	shaderinfo.drawtype = drawtype;
	switch (material_type) {
	case TILE_MATERIAL_OPAQUE:
	case TILE_MATERIAL_LIQUID_OPAQUE:
	case TILE_MATERIAL_WAVING_LIQUID_OPAQUE:
		shaderinfo.base_material = video::EMT_SOLID;
		break;
	case TILE_MATERIAL_ALPHA:
	case TILE_MATERIAL_PLAIN_ALPHA:
	case TILE_MATERIAL_LIQUID_TRANSPARENT:
	case TILE_MATERIAL_WAVING_LIQUID_TRANSPARENT:
		shaderinfo.base_material = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
		break;
	case TILE_MATERIAL_BASIC:
	case TILE_MATERIAL_PLAIN:
	case TILE_MATERIAL_WAVING_LEAVES:
	case TILE_MATERIAL_WAVING_PLANTS:
	case TILE_MATERIAL_WAVING_LIQUID_BASIC:
		shaderinfo.base_material = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
		break;
	}
	shaderinfo.material = shaderinfo.base_material;

	bool enable_shaders = g_settings->getBool("enable_shaders");
	if (!enable_shaders)
		return shaderinfo;

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	if (!driver->queryFeature(video::EVDF_ARB_GLSL)) {
		// The setting outlives a driver change, so say so once and keep the fixed pipeline
		static bool warned = false;
		if (!warned) {
			warned = true;
			errorstream << "Shaders are enabled but GLSL is not supported by the driver"
				    << std::endl;
		}
		return shaderinfo;
	}
	video::IGPUProgrammingServices *gpu = driver->getGPUProgrammingServices();

	// Create shaders header
	bool use_gles = false;
#if ENABLE_GLES
	use_gles = driver->getDriverType() == video::EDT_OGLES2;
#endif
	std::stringstream shaders_header;
	shaders_header
		<< std::noboolalpha
		<< std::showpoint // for GLSL ES
		;
	std::string vertex_header, fragment_header, geometry_header;
	if (use_gles) {
		shaders_header << R"(
			#version 100
		)";
		vertex_header = R"(
			uniform highp mat4 mWorldView;
			uniform highp mat4 mWorldViewProj;
			uniform highp mat4 mTexture;

			attribute highp vec4 inVertexPosition;
			attribute lowp vec4 inVertexColor;
			attribute highp vec4 inTexCoord0;
			attribute mediump vec3 inVertexNormal;
			attribute mediump vec4 inVertexTangent;
			attribute mediump vec4 inVertexBinormal;
		)";
		fragment_header = R"(
			precision mediump float;
		)";
	} else {
		shaders_header << R"(
			#version 120
			#define lowp
			#define mediump
			#define highp
		)";
		vertex_header = R"(
			#define mWorldView gl_ModelViewMatrix
			#define mWorldViewProj gl_ModelViewProjectionMatrix
			#define mTexture (gl_TextureMatrix[0])

			#define inVertexPosition gl_Vertex
			#define inVertexColor gl_Color
			#define inTexCoord0 gl_MultiTexCoord0
			#define inVertexNormal gl_Normal
			#define inVertexTangent gl_MultiTexCoord1
			#define inVertexBinormal gl_MultiTexCoord2
		)";
	}

	if (use_gles) {
		if (shaderinfo.base_material == video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF)
			shaders_header << "#define USE_ALPHA_TEST_DISCARD 1\n";
		else if (shaderinfo.base_material != video::EMT_SOLID)
			shaders_header << "#define USE_ALPHA_REF_DISCARD 1\n";
	}

#define PROVIDE(constant) shaders_header << "#define " #constant " " << (int)constant << "\n"

	PROVIDE(NDT_NORMAL);
	PROVIDE(NDT_AIRLIKE);
	PROVIDE(NDT_LIQUID);
	PROVIDE(NDT_FLOWINGLIQUID);
	PROVIDE(NDT_GLASSLIKE);
	PROVIDE(NDT_ALLFACES);
	PROVIDE(NDT_ALLFACES_OPTIONAL);
	PROVIDE(NDT_TORCHLIKE);
	PROVIDE(NDT_SIGNLIKE);
	PROVIDE(NDT_PLANTLIKE);
	PROVIDE(NDT_FENCELIKE);
	PROVIDE(NDT_RAILLIKE);
	PROVIDE(NDT_NODEBOX);
	PROVIDE(NDT_GLASSLIKE_FRAMED);
	PROVIDE(NDT_FIRELIKE);
	PROVIDE(NDT_GLASSLIKE_FRAMED_OPTIONAL);
	PROVIDE(NDT_PLANTLIKE_ROOTED);

	PROVIDE(TILE_MATERIAL_BASIC);
	PROVIDE(TILE_MATERIAL_ALPHA);
	PROVIDE(TILE_MATERIAL_LIQUID_TRANSPARENT);
	PROVIDE(TILE_MATERIAL_LIQUID_OPAQUE);
	PROVIDE(TILE_MATERIAL_WAVING_LEAVES);
	PROVIDE(TILE_MATERIAL_WAVING_PLANTS);
	PROVIDE(TILE_MATERIAL_OPAQUE);
	PROVIDE(TILE_MATERIAL_WAVING_LIQUID_BASIC);
	PROVIDE(TILE_MATERIAL_WAVING_LIQUID_TRANSPARENT);
	PROVIDE(TILE_MATERIAL_WAVING_LIQUID_OPAQUE);
	PROVIDE(TILE_MATERIAL_PLAIN);
	PROVIDE(TILE_MATERIAL_PLAIN_ALPHA);

#undef PROVIDE

	shaders_header << "#define MATERIAL_TYPE " << (int)material_type << "\n";
	shaders_header << "#define DRAW_TYPE " << (int)drawtype << "\n";

	bool enable_waving_water = g_settings->getBool("enable_waving_water");
	shaders_header << "#define ENABLE_WAVING_WATER " << enable_waving_water << "\n";
	float water_wave[3] = {};
	if (enable_waving_water) {
		water_wave[0] = g_settings->getFloat("water_wave_height");
		water_wave[1] = g_settings->getFloat("water_wave_length");
		water_wave[2] = g_settings->getFloat("water_wave_speed");
		shaders_header << "#define WATER_WAVE_HEIGHT " << water_wave[0] << "\n";
		shaders_header << "#define WATER_WAVE_LENGTH " << water_wave[1] << "\n";
		shaders_header << "#define WATER_WAVE_SPEED " << water_wave[2] << "\n";
	}

	const bool smooth_lighting = g_settings->getBool("smooth_lighting");
	shaders_header << "#define POINT_LIGHTS "
		<< (smooth_lighting && point_lights ? MAX_POINT_LIGHTS : 0) << "\n";
	shaders_header << "#define ENABLE_WAVING_LEAVES " << g_settings->getBool("enable_waving_leaves") << "\n";
	shaders_header << "#define ENABLE_WAVING_PLANTS " << g_settings->getBool("enable_waving_plants") << "\n";
	shaders_header << "#define ENABLE_TONE_MAPPING " << g_settings->getBool("tone_mapping") << "\n";

	const float fog_start = core::clamp(g_settings->getFloat("fog_start"), 0.0f, 0.99f);
	shaders_header << "#define FOG_START " << fog_start << "\n";

	// The light source color folds into the curve, but not past a pow()
	const LightingParams &lp = get_lighting_params();
	const bool fold = lp.gamma == 1.0f;
	const float f = fold ? 1.04f : 1.0f;
	const float sigma = std::fmax(lp.sigma, 0.001f);
	// The boost carries LIGHT_SCALE in its last component
	const float light_curve[4] = { lp.a * f, lp.b * f, lp.c * f, 1.0f / lp.gamma };
	const float light_boost[4] = { lp.boost * f, lp.center, -0.5f / (sigma * sigma),
			fold ? 1.0f : 1.04f };
	shaders_header << "#define LIGHT_CURVE vec4(" << light_curve[0] << "," << light_curve[1]
		<< "," << light_curve[2] << "," << light_curve[3] << ")\n";
	shaders_header << "#define LIGHT_BOOST vec3(" << light_boost[0] << "," << light_boost[1]
		<< "," << light_boost[2] << ")\n";
	shaders_header << "#define LIGHT_SCALE " << light_boost[3] << "\n";

	if (g_settings->getBool("enable_dynamic_shadows")) {
		shaders_header << "#define ENABLE_DYNAMIC_SHADOWS 1\n";
		if (g_settings->getBool("shadow_map_color"))
			shaders_header << "#define COLORED_SHADOWS 1\n";

		if (g_settings->getBool("shadow_poisson_filter"))
			shaders_header << "#define POISSON_FILTER 1\n";

		s32 shadow_filter = g_settings->getS32("shadow_filters");
		shaders_header << "#define SHADOW_FILTER " << shadow_filter << "\n";

		float shadow_soft_radius = g_settings->getFloat("shadow_soft_radius");
		if (shadow_soft_radius < 1.0f)
			shadow_soft_radius = 1.0f;
		shaders_header << "#define SOFTSHADOWRADIUS " << shadow_soft_radius << "\n";
	}

	std::string common_header = shaders_header.str();

	std::string vertex_shader = m_sourcecache.getOrLoad(name, "opengl_vertex.glsl");
	std::string fragment_shader = m_sourcecache.getOrLoad(name, "opengl_fragment.glsl");
	std::string geometry_shader = m_sourcecache.getOrLoad(name, "opengl_geometry.glsl");

	vertex_shader = common_header + vertex_header + vertex_shader;
	fragment_shader = common_header + fragment_header + fragment_shader;
	const char *geometry_shader_ptr = nullptr; // optional
	if (!geometry_shader.empty()) {
		geometry_shader = common_header + geometry_header + geometry_shader;
		geometry_shader_ptr = geometry_shader.c_str();
	}

	// Only the nodes read the lights around them
	const bool lights = name == "nodes_shader" && smooth_lighting && point_lights;
	// The name a driver without a shader compiler picks its prebuilt program by, branch included
	vertex_shader = "//SHADER:" + name + shaderVariant(name, material_type,
			shaderinfo.base_material, lights) + "\n" + vertex_shader;

	irr_ptr<ShaderCallback> cb{new ShaderCallback(m_setter_factories)};

	// A program built ahead of time takes the values the header spells out as uniforms
	if (driver->getDriverType() == video::EDT_METAL) {
		float spelled_curve[4], spelled_boost[4], spelled_wave[3];
		for (int i = 0; i < 4; i++) {
			spelled_curve[i] = asSpelled(light_curve[i]);
			spelled_boost[i] = asSpelled(light_boost[i]);
		}
		for (int i = 0; i < 3; i++)
			spelled_wave[i] = asSpelled(water_wave[i]);
		cb->addSetter(new SettingsShaderConstantSetter(
				1.0f / (1.0f - asSpelled(fog_start)), spelled_curve, spelled_boost, spelled_wave));
		// A blending base alpha tests inside the program, against the reference of the material
		if (shaderinfo.base_material == video::EMT_TRANSPARENT_ALPHA_CHANNEL)
			cb->addSetter(new AlphaRefShaderConstantSetter());
	}

	infostream<<"Compiling high level shaders for "<<name<<std::endl;
	s32 shadermat = gpu->addHighLevelShaderMaterial(
		vertex_shader.c_str(), nullptr, video::EVST_VS_1_1,
		fragment_shader.c_str(), nullptr, video::EPST_PS_1_1,
		geometry_shader_ptr, nullptr, video::EGST_GS_4_0, scene::EPT_TRIANGLES, scene::EPT_TRIANGLES, 0,
		cb.get(), shaderinfo.base_material,  1);
	if (shadermat == -1) {
		errorstream<<"generate_shader(): "
				"failed to generate \""<<name<<"\", "
				"addHighLevelShaderMaterial failed."
				<<std::endl;
#ifndef NDEBUG
		dumpShaderProgram(warningstream, "Vertex", vertex_shader);
		dumpShaderProgram(warningstream, "Fragment", fragment_shader);
		dumpShaderProgram(warningstream, "Geometry", geometry_shader);
#endif
		return shaderinfo;
	}

	// Apply the newly created material type
	shaderinfo.material = (video::E_MATERIAL_TYPE) shadermat;

	// Frames with no light source near draw the nodes with the same program without point lights
	if (lights) {
		const ShaderInfo unlit = generateShader(name, material_type, drawtype, false);
		if (unlit.material != unlit.base_material)
			m_without_point_lights[shaderinfo.material] = unlit.material;
	}
	return shaderinfo;
}

video::E_MATERIAL_TYPE ShaderSource::getMaterialWithoutPointLights(video::E_MATERIAL_TYPE material)
{
	const auto it = m_without_point_lights.find(material);
	return it == m_without_point_lights.end() ? material : it->second;
}

void dumpShaderProgram(std::ostream &output_stream,
		const std::string &program_type, const std::string &program)
{
	output_stream << program_type << " shader program:" << std::endl <<
		"----------------------------------" << std::endl;
	size_t pos = 0;
	size_t prev = 0;
	s16 line = 1;
	while ((pos = program.find('\n', prev)) != std::string::npos) {
		output_stream << line++ << ": "<< program.substr(prev, pos - prev) <<
			std::endl;
		prev = pos + 1;
	}
	output_stream << line << ": " << program.substr(prev) << std::endl <<
		"End of " << program_type << " shader program." << std::endl <<
		" " << std::endl;
}
