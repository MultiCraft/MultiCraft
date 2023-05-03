/*
Copyright (C) 2002-2013 Nikolaus Gebhardt
This file is part of the "Irrlicht Engine".
For conditions of distribution and use, see copyright notice in irrlicht.h

Modified 2019.05.01 by stujones11, Stuart Jones <stujones111@gmail.com>

This is a heavily modified copy of the Irrlicht CGUIScrollBar class
which includes automatic scaling of the thumb slider and hiding of
the arrow buttons where there is insufficient space.
*/

#pragma once

#include "guiAnimatedImage.h"
#include "irrlichttypes_extrabloated.h"
#include "StyleSpec.h"
#include <vector>

using namespace irr;
using namespace gui;

class GUIScrollBar : public IGUIElement
{
public:
	GUIScrollBar(IGUIEnvironment *environment, IGUIElement *parent, s32 id,
			core::rect<s32> rectangle, bool horizontal, bool auto_scale);

	enum ArrowVisibility
	{
		HIDE,
		SHOW,
		DEFAULT
	};

	virtual void draw();
	virtual void updateAbsolutePosition();
	virtual bool OnEvent(const SEvent &event);

	s32 getMax() const { return max_pos; }
	s32 getMin() const { return min_pos; }
	s32 getLargeStep() const { return large_step; }
	s32 getSmallStep() const { return small_step; }
	s32 getPos() const;
	s32 getPageSize() const { return page_size; }

	void setMax(const s32 &max);
	void setMin(const s32 &min);
	void setSmallStep(const s32 &step);
	void setLargeStep(const s32 &step);
	void setPos(const s32 &pos);
	void setPageSize(const s32 &size);
	void setArrowsVisible(ArrowVisibility visible);
	void setTextures(const std::vector<video::ITexture *> &textures);
	void setStyle(const StyleSpec &style, ISimpleTextureSource *tsrc);

private:
	void refreshControls();
	s32 getPosFromMousePos(const core::position2di &p) const;
	f32 range() const { return f32(max_pos - min_pos); }
	gui::IGUIImage *addImage(const core::rect<s32> &rect, video::ITexture *texture);

	IGUIButton *up_button;
	IGUIButton *down_button;
	gui::IGUIImage *bg_image;
	gui::IGUIImage *slider_image;
	gui::IGUIImage *slider_top_image;
	gui::IGUIImage *slider_bottom_image;
	ArrowVisibility arrow_visibility = DEFAULT;
	bool is_dragging;
	bool is_horizontal;
	bool is_auto_scaling;
	bool dragged_by_slider;
	bool tray_clicked;
	s32 scroll_pos;
	s32 draw_center;
	s32 thumb_size;
	s32 min_pos;
	s32 max_pos;
	s32 small_step;
	s32 large_step;
	s32 drag_offset;
	s32 page_size;
	s32 border_size;

	std::vector<video::ITexture *> m_textures;
	core::rect<s32> m_texture_middle;

	core::rect<s32> slider_rect;
	video::SColor current_icon_color;
};
