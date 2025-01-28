#pragma once

#include "irrlichttypes_extrabloated.h"
#include <algorithm>
#include <string>

class ISimpleTextureSource;

class GUIAnimatedImage : public gui::IGUIElement {
public:
	GUIAnimatedImage(gui::IGUIEnvironment *env, gui::IGUIElement *parent,
		s32 id, const core::rect<s32> &rectangle, ISimpleTextureSource *tsrc = nullptr,
		const bool use_scaling_filter = false);

	virtual void draw() override;

	void setTexture(video::ITexture *texture) { m_texture = texture; };
	video::ITexture *getTexture() const { return m_texture; };

	void setTextureName(const std::string &texture_name) {
		if (texture_name != m_texture_name) {
			m_texture_name = texture_name;
			m_texture = nullptr;
		}
	};
	const std::string &getTextureName() const { return m_texture_name; };

	void setMiddleRect(const core::rect<s32> &middle) { m_middle = middle; };
	core::rect<s32> getMiddleRect() const { return m_middle; };

	void setFrameDuration(u64 duration) { m_frame_duration = duration; };
	u64 getFrameDuration() const { return m_frame_duration; };

	void setFrameCount(s32 count) { m_frame_count = std::max(count, 1); };
	s32 getFrameCount() const { return m_frame_count; };

	void setFrameIndex(s32 frame) { m_frame_idx = std::max(frame, 0); };
	s32 getFrameIndex() const { return m_frame_idx; };

private:
	ISimpleTextureSource *m_tsrc;
	video::ITexture *m_texture = nullptr;
	std::string m_texture_name = "";
	bool m_use_scaling_filter;

	u64 m_global_time = 0;
	s32 m_frame_idx = 0;
	s32 m_frame_count = 1;
	u64 m_frame_duration = 0;
	u64 m_frame_time = 0;

	core::rect<s32> m_middle;
};
