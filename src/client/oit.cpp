#include "oit.h"
#include "client/client.h"
#include "client/shader.h"
#include "client/tile.h"
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

using namespace video;

static constexpr ECOLOR_FORMAT solid_format = ECF_A8R8G8B8;
static constexpr ECOLOR_FORMAT depth_format = ECF_D16;
static constexpr ECOLOR_FORMAT layer_formats[] = {
	ECF_A16B16G16R16F,
	ECF_R16F,
};
static constexpr auto layer_count = sizeof(layer_formats) / sizeof(layer_formats[0]);

static const S3DVertex vertices[4] = {
	S3DVertex(1.0, -1.0, 0.0, 0.0, 0.0, -1.0, SColor(255, 0, 255, 255), 1.0, 0.0),
	S3DVertex(-1.0, -1.0, 0.0, 0.0, 0.0, -1.0, SColor(255, 255, 0, 255), 0.0, 0.0),
	S3DVertex(-1.0, 1.0, 0.0, 0.0, 0.0, -1.0, SColor(255, 255, 255, 0), 0.0, 1.0),
	S3DVertex(1.0, 1.0, 0.0, 0.0, 0.0, -1.0, SColor(255, 255, 255, 255), 1.0, 1.0),
};
static const u16 indices[6] = {0, 1, 2, 2, 3, 0};

void RenderingOIT::OnPreRender(core::array<scene::ISceneNode *> &lightList) {
	v2u32 ss = driver->getScreenSize();
	if (screensize != ss) {
		clearTextures();
		screensize = ss;
		initTextures();
	}
}

void RenderingOIT::OnPostRender() {
	driver->setRenderTargetEx(nullptr, 0);
}

void RenderingOIT::OnRenderPassPreRender(scene::E_SCENE_NODE_RENDER_PASS renderPass) {
	switch (renderPass) {
		case scene::ESNRP_SOLID:
			driver->setRenderTargetEx(rt_solid, ECBF_COLOR | ECBF_DEPTH, SColor(0, 0, 0, 0));
			break;
		case scene::ESNRP_TRANSPARENT:
			driver->setRenderTargetEx(rt_transparent, ECBF_COLOR, SColor(255, 0, 0, 0));
			break;
		case scene::ESNRP_TRANSPARENT_EFFECT:
			driver->setRenderTargetEx(rt_solid, 0);
			break;
		default:;
	}
}

void RenderingOIT::OnRenderPassPostRender(scene::E_SCENE_NODE_RENDER_PASS renderPass) {
	switch (renderPass) {
		case scene::ESNRP_SOLID:
			driver->setRenderTargetEx(nullptr, 0);
			driver->draw2DImage(solid, {0, 0}, true);
			break;
		case scene::ESNRP_TRANSPARENT:
			driver->setRenderTargetEx(nullptr, 0);
			driver->setMaterial(mat);
			driver->drawVertexPrimitiveList(&vertices, 4, &indices, 2);
			break;
		case scene::ESNRP_TRANSPARENT_EFFECT:
			driver->setRenderTargetEx(nullptr, 0);
			driver->draw2DImage(solid, {0, 0}, true);
			break;
		default:;
	}
}

RenderingOIT::RenderingOIT(IrrlichtDevice *_device, Client *_client)
	: device(_device)
	, driver(device->getVideoDriver())
	, color(layer_count)
{
	rt_solid = driver->addRenderTarget();
	rt_transparent = driver->addRenderTarget();
	screensize = {-1u, -1u};
	initMaterial(_client->getShaderSource());
	device->getSceneManager()->setLightManager(this);
}

RenderingOIT::~RenderingOIT()
{
	device->getSceneManager()->setLightManager(nullptr);
	clearTextures();
	driver->removeRenderTarget(rt_solid);
	driver->removeRenderTarget(rt_transparent);
}

void RenderingOIT::initMaterial(IShaderSource *s)
{
	mat.UseMipMaps = false;
	mat.ZBuffer = false;
	mat.setFlag(EMF_ZWRITE_ENABLE, false); // ZWriteEnable is bool on early 1.9 but E_ZWRITE on later 1.9
	u32 shader = s->getShader("oit_merge", TILE_MATERIAL_ALPHA);
	mat.MaterialType = s->getShaderInfo(shader).material;
	mat.BlendOperation = EBO_ADD;
	mat.MaterialTypeParam = pack_textureBlendFunc(EBF_ONE, EBF_ONE_MINUS_SRC_ALPHA);
	for (int k = 0; k < layer_count; ++k) {
		mat.TextureLayer[k].AnisotropicFilter = false;
		mat.TextureLayer[k].BilinearFilter = false;
		mat.TextureLayer[k].TrilinearFilter = false;
		mat.TextureLayer[k].TextureWrapU = ETC_CLAMP_TO_EDGE;
		mat.TextureLayer[k].TextureWrapV = ETC_CLAMP_TO_EDGE;
	}
}

void RenderingOIT::initTextures()
{
	solid = driver->addRenderTargetTexture(screensize, "oit_solid", solid_format);
	depth = driver->addRenderTargetTexture(screensize, "oit_depth", depth_format);
	for (int k = 0; k < layer_count; k++) {
		char name[32];
		snprintf(name, sizeof(name), "oit_layer_%d", k);
		color[k] = driver->addRenderTargetTexture(screensize, name, layer_formats[k]);
		mat.TextureLayer[k].Texture = color[k];
	}
	color.set_used(layer_count);
	rt_solid->setTexture(solid, depth);
	rt_transparent->setTexture(color, depth);
}

void RenderingOIT::clearTextures()
{
	rt_solid->setTexture(nullptr, nullptr);
	rt_transparent->setTexture(nullptr, nullptr);
	for (int k = 0; k < layer_count; k++)
		driver->removeTexture(color[k]);
	color.set_used(0);
	driver->removeTexture(solid);
	driver->removeTexture(depth);
}
