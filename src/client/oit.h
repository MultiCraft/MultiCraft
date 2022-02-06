#pragma once
#include "irrlichttypes_extrabloated.h"
#include <ILightManager.h>

class Client;
class Hud;
class IShaderSource;

class RenderingOIT: public scene::ILightManager
{
protected:
	IrrlichtDevice *device = nullptr;
	video::IVideoDriver *driver = nullptr;
	core::array<video::ITexture *> color;
	video::ITexture *solid = nullptr;
	video::ITexture *depth = nullptr;
	video::IRenderTarget *rt_solid = nullptr;
	video::IRenderTarget *rt_transparent = nullptr;
	video::SMaterial mat;
	core::dimension2du screensize = {0, 0};

	void initMaterial(IShaderSource *s);
	void initTextures();
	void clearTextures();

public:
	void OnPreRender(core::array<scene::ISceneNode*> & lightList) override;
	void OnPostRender(void) override;

	void OnRenderPassPreRender(scene::E_SCENE_NODE_RENDER_PASS renderPass) override;
	void OnRenderPassPostRender(scene::E_SCENE_NODE_RENDER_PASS renderPass) override;

	void OnNodePreRender(scene::ISceneNode* node) override {}
	void OnNodePostRender(scene::ISceneNode* node) override {}

	RenderingOIT(IrrlichtDevice *_device, Client *_client);
	~RenderingOIT() override;
};
