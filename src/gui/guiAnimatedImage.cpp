#include "guiAnimatedImage.h"

#include "client/guiscalingfilter.h"
#include "client/tile.h" // ITextureSource
#include "log.h"
#include "porting.h"
#include "util/string.h"
#include <string>
#include <vector>

GUIAnimatedImage::GUIAnimatedImage(gui::IGUIEnvironment *env, gui::IGUIElement *parent,
	s32 id, const core::rect<s32> &rectangle, ISimpleTextureSource *tsrc,
	const bool use_scaling_filter) :
	gui::IGUIElement(gui::EGUIET_ELEMENT, env, parent, id, rectangle),
	m_tsrc(tsrc), m_use_scaling_filter(use_scaling_filter)
{
}

void GUIAnimatedImage::draw()
{
	video::IVideoDriver *driver = Environment->getVideoDriver();

	// Fill in m_texture when not clipped by a scroll container
	if (m_texture == nullptr && m_tsrc != nullptr && !m_texture_name.empty()) {
		m_texture = m_tsrc->getTexture(m_texture_name);
	}

	if (m_texture == nullptr)
		return;

	core::dimension2d<u32> size = m_texture->getOriginalSize();

	if ((u32)m_frame_count > size.Height)
		m_frame_count = size.Height;
	if (m_frame_idx >= m_frame_count)
		m_frame_idx = m_frame_count - 1;

	size.Height /= m_frame_count;

	core::rect<s32> rect(core::position2d<s32>(0, size.Height * m_frame_idx), size);
	core::rect<s32> *cliprect = NoClip ? nullptr : &AbsoluteClippingRect;

	if (m_middle.getArea() == 0) {
		const video::SColor color(255, 255, 255, 255);
		const video::SColor colors[] = {color, color, color, color};

		if (m_use_scaling_filter) {
			draw2DImageFilterScaled(driver, m_texture, AbsoluteRect, rect, cliprect,
				colors, true);
		} else {
			// Temporary fix for issue #12581 in 5.6.0.
			// Use legacy image when not rendering 9-slice image because draw2DImage9Slice
			// uses NNAA filter which causes visual artifacts when image uses alpha blending.
			driver->draw2DImage(m_texture, AbsoluteRect, rect, cliprect, colors, true);
		}
	} else {
		draw2DImage9Slice(driver, m_texture, AbsoluteRect, rect, m_middle, cliprect);
	}

	// Step the animation
	if (m_frame_count > 1 && m_frame_duration > 0) {
		// Determine the delta time to step
		u64 new_global_time = porting::getTimeMs();
		if (m_global_time > 0)
			m_frame_time += new_global_time - m_global_time;

		m_global_time = new_global_time;

		// Advance by the number of elapsed frames, looping if necessary
		m_frame_idx += (u32)(m_frame_time / m_frame_duration);
		m_frame_idx %= m_frame_count;

		// If 1 or more frames have elapsed, reset the frame time counter with
		// the remainder
		m_frame_time %= m_frame_duration;
	}
}
