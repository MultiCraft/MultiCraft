#include "oit.h"
#include "client/client.h"
#include "client/shader.h"
#include "client/tile.h"
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

using namespace video;

static constexpr ECOLOR_FORMAT depth_format = ECF_D24S8;
static constexpr ECOLOR_FORMAT layer_formats[] = {
	ECF_A16B16G16R16F,
	ECF_R16F,
};
static constexpr auto layer_count = sizeof(layer_formats) / sizeof(layer_formats[0]);

void RenderingOIT::OnRenderPassPreRender(scene::E_SCENE_NODE_RENDER_PASS renderPass) {
	if (renderPass == scene::ESNRP_TRANSPARENT)
		preRender();
}

void RenderingOIT::OnRenderPassPostRender(scene::E_SCENE_NODE_RENDER_PASS renderPass) {
	if (renderPass == scene::ESNRP_TRANSPARENT)
		postRender();
}


RenderingOIT::RenderingOIT(IVideoDriver *_driver, Client *_client)
	: driver(_driver)
	, color(layer_count)
{
	rt = driver->addRenderTarget();
	screensize = {-1u, -1u};
	initMaterial(_client->getShaderSource());
}

RenderingOIT::~RenderingOIT()
{
	clearTextures();
	driver->removeRenderTarget(rt);
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
	depth = driver->addRenderTargetTexture(screensize, "oit_depth", depth_format);
	for (int k = 0; k < layer_count; k++) {
		char name[32];
		snprintf(name, sizeof(name), "oit_layer_%d", k);
		color[k] = driver->addRenderTargetTexture(screensize, name, layer_formats[k]);
		mat.TextureLayer[k].Texture = color[k];
	}
	color.set_used(layer_count);
	rt->setTexture(color, depth);
}

void RenderingOIT::clearTextures()
{
	rt->setTexture(nullptr, nullptr);
	for (int k = 0; k < layer_count; k++)
		driver->removeTexture(color[k]);
	color.set_used(0);
	driver->removeTexture(depth);
}

void RenderingOIT::preRender() {
	v2u32 ss = driver->getScreenSize();
	if (screensize != ss) {
		clearTextures();
		screensize = ss;
		initTextures();
	}
	int fb_main, fb_my, fb_read;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fb_main);
	driver->setRenderTargetEx(rt, ECBF_COLOR | ECBF_DEPTH, SColor(255, 0, 0, 0));
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fb_my);
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &fb_read);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fb_main);
	int err0 = glGetError();
	glBlitFramebuffer(0, 0, ss.X, ss.Y, 0, 0, ss.X, ss.Y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	int err1 = glGetError();
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fb_read);
	int err2 = glGetError();
	if (err0 || err1 || err2)
		printf("%d/%d/%d: %d/%d/%d\n", fb_main, fb_my, fb_read, err0, err1, err2);
}

void RenderingOIT::postRender() {
	static const S3DVertex vertices[4] = {
			S3DVertex(1.0, -1.0, 0.0, 0.0, 0.0, -1.0,
					SColor(255, 0, 255, 255), 1.0, 0.0),
			S3DVertex(-1.0, -1.0, 0.0, 0.0, 0.0, -1.0,
					SColor(255, 255, 0, 255), 0.0, 0.0),
			S3DVertex(-1.0, 1.0, 0.0, 0.0, 0.0, -1.0,
					SColor(255, 255, 255, 0), 0.0, 1.0),
			S3DVertex(1.0, 1.0, 0.0, 0.0, 0.0, -1.0,
					SColor(255, 255, 255, 255), 1.0, 1.0),
	};
	static const u16 indices[6] = {0, 1, 2, 2, 3, 0};
	driver->setRenderTargetEx(nullptr, 0);
	driver->setMaterial(mat);
	driver->drawVertexPrimitiveList(&vertices, 4, &indices, 2);
}
