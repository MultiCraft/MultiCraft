/*
Minetest
Copyright (C) 2020 DS

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

#include "irrlichttypes_extrabloated.h"
#include "util/string.h"
#include "guiScrollBar.h"

class GUIScrollContainer : public gui::IGUIElement
{
public:
	GUIScrollContainer(gui::IGUIEnvironment *env, gui::IGUIElement *parent, s32 id,
			const core::rect<s32> &rectangle, const std::string &orientation,
			f32 scrollfactor);

	virtual bool OnEvent(const SEvent &event) override;

	virtual void draw() override;

	inline void onScrollEvent(gui::IGUIElement *caller)
	{
		if (caller == m_scrollbar)
			updateScrolling();
	}

	inline void setScrollBar(GUIScrollBar *scrollbar)
	{
		m_scrollbar = scrollbar;
		updateScrolling();
	}

private:
	enum OrientationEnum
	{
		VERTICAL,
		HORIZONTAL,
		UNDEFINED
	};

	GUIScrollBar *m_scrollbar;
	OrientationEnum m_orientation;
	f32 m_scrollfactor;

	bool m_swipe_started = false;
	bool m_is_coasting = false;
	int m_swipe_start_px = -1;
	u64 m_last_time = 0;
	float m_swipe_pos = 0;
	float m_last_pos = 0;
	float m_velocity = 0;

	struct VelocitySample
	{
		float position;
		u64 timestamp;
	};

	static const u32 MAX_VELOCITY_SAMPLES = 10;
	VelocitySample m_velocity_samples[MAX_VELOCITY_SAMPLES];
	u32 m_sample_count = 0;
	u32 m_sample_index = 0;

	void updateScrolling();
	void updateScrollCoasting();
	void calculateCoastingVelocity();
};
