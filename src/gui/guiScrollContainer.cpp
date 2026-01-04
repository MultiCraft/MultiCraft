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

#include "guiScrollContainer.h"
#include "porting.h"

#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
GUIScrollContainer::GUIScrollContainer(gui::IGUIEnvironment *env,
		gui::IGUIElement *parent, s32 id, const core::rect<s32> &rectangle,
		const std::string &orientation, f32 scrollfactor) :
		gui::IGUIElement(gui::EGUIET_CUSTOM_SCROLLCONTAINER, env, parent, id,
				rectangle),
		m_scrollbar(nullptr), m_scrollfactor(scrollfactor)
#else
GUIScrollContainer::GUIScrollContainer(gui::IGUIEnvironment *env,
		gui::IGUIElement *parent, s32 id, const core::rect<s32> &rectangle,
		const std::string &orientation, f32 scrollfactor) :
		gui::IGUIElement(gui::EGUIET_ELEMENT, env, parent, id, rectangle),
		m_scrollbar(nullptr), m_scrollfactor(scrollfactor)
#endif
{
	if (orientation == "vertical")
		m_orientation = VERTICAL;
	else if (orientation == "horizontal")
		m_orientation = HORIZONTAL;
	else
		m_orientation = UNDEFINED;
}

bool GUIScrollContainer::OnEvent(const SEvent &event)
{
	if (event.EventType == EET_MOUSE_INPUT_EVENT &&
			event.MouseInput.Event == EMIE_MOUSE_WHEEL &&
			!event.MouseInput.isLeftPressed() && m_scrollbar) {
		Environment->setFocus(m_scrollbar);
		bool retval = m_scrollbar->OnEvent(event);

		// a hacky fix for updating the hovering and co.
		IGUIElement *hovered_elem = getElementFromPoint(core::position2d<s32>(
				event.MouseInput.X, event.MouseInput.Y));
		SEvent mov_event = event;
		mov_event.MouseInput.Event = EMIE_MOUSE_MOVED;
		Environment->postEventFromUser(mov_event);
		if (hovered_elem)
			hovered_elem->OnEvent(mov_event);

		return retval;
	}

#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
#ifdef HAVE_TOUCHSCREENGUI
	if (event.EventType == EET_MOUSE_INPUT_EVENT && m_scrollbar) {
		const int mouse_pos = m_orientation == HORIZONTAL ? event.MouseInput.X
								  : event.MouseInput.Y;
		if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN) {
			if (isPointInside(core::position2d<s32>(
					    event.MouseInput.X, event.MouseInput.Y))) {
				m_swipe_start_px = mouse_pos -
						   m_scrollbar->getPos() * m_scrollfactor;
				m_velocity = 0;
				m_is_coasting = false;
				m_last_pos = m_scrollbar->getPos();
			}
		} else if (event.MouseInput.Event == EMIE_LMOUSE_LEFT_UP) {
			m_swipe_start_px = -1;
			if (m_swipe_started) {
				m_swipe_started = false;
				calculateCoastingVelocity();
				return true;
			}
		} else if (event.MouseInput.Event == EMIE_MOUSE_MOVED) {
			double screen_dpi = RenderingEngine::getDisplayDensity() * 96;

			if (!m_swipe_started && m_orientation != UNDEFINED &&
					m_swipe_start_px != -1 &&
					std::abs(m_swipe_start_px - mouse_pos +
							m_scrollbar->getPos() *
									m_scrollfactor) >
							0.1 * screen_dpi) {
				m_swipe_started = true;
				Environment->setFocus(this);
				m_sample_count = 0;
				m_sample_index = 0;
			}

			if (m_swipe_started) {
				s32 old_pos = m_scrollbar->getPos();
				m_swipe_pos = (float)(mouse_pos - m_swipe_start_px) /
					      m_scrollfactor;

				u64 current_time = porting::getTimeMs();

				m_velocity_samples[m_sample_index].position = m_swipe_pos;
				m_velocity_samples[m_sample_index].timestamp =
						current_time;
				m_sample_index = (m_sample_index + 1) %
						 MAX_VELOCITY_SAMPLES;
				if (m_sample_count < MAX_VELOCITY_SAMPLES) {
					m_sample_count++;
				}

				m_last_time = current_time;
				m_scrollbar->setPos((int)m_swipe_pos);

				if (old_pos != m_scrollbar->getPos()) {
					SEvent e;
					e.EventType = EET_GUI_EVENT;
					e.GUIEvent.Caller = m_scrollbar;
					e.GUIEvent.Element = nullptr;
					e.GUIEvent.EventType = EGET_SCROLL_BAR_CHANGED;
					OnEvent(e);
				}

				return true;
			}
		}
	}
#endif
#endif

	return IGUIElement::OnEvent(event);
}

void GUIScrollContainer::calculateCoastingVelocity()
{
	if (m_sample_count < 1)
		return;

	u32 last_sample_index = (m_sample_index - 1 + MAX_VELOCITY_SAMPLES) %
				MAX_VELOCITY_SAMPLES;
	VelocitySample last_sample = m_velocity_samples[last_sample_index];

	VelocitySample first_sample = last_sample;
	for (u32 i = 0; i < m_sample_count; i++) {
		VelocitySample &sample = m_velocity_samples[i];

		if (m_last_time - sample.timestamp > 100)
			continue;

		if (sample.timestamp < first_sample.timestamp)
			first_sample = sample;
	}

	u64 dt = last_sample.timestamp - first_sample.timestamp;
	if (dt > 0) {
		double screen_dpi = RenderingEngine::getDisplayDensity() * 96;
		float distance = (last_sample.position - first_sample.position) *
				 m_scrollfactor;
		float distance_in = distance * 1000 / screen_dpi;

		m_velocity = distance_in / dt;
		m_velocity = std::max(std::min(m_velocity, 40.0f), -40.0f);
		m_is_coasting = true;
	}
}

void GUIScrollContainer::updateScrollCoasting()
{
#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
#ifdef HAVE_TOUCHSCREENGUI
	if (!m_is_coasting || m_velocity == 0)
		return;

	u64 current_time = porting::getTimeMs();
	u64 dt = current_time - m_last_time;
	m_last_time = current_time;

	if (dt == 0)
		return;

	const float SLOWING_FACTOR = 0.95f;
	const float VELOCITY_FACTOR = 1.0f;

	// We want SLOWING_FACTOR to be added every frame assuming 60 FPS
	m_velocity *= std::pow(SLOWING_FACTOR, dt / (1000.0f / 60.0f));
	if (std::abs(m_velocity) < 0.5f) {
		m_is_coasting = false;
		return;
	}

	double screen_dpi = RenderingEngine::getDisplayDensity() * 96;
	float pixel_velocity = m_velocity * screen_dpi / m_scrollfactor / 1000;

	m_swipe_pos += pixel_velocity * VELOCITY_FACTOR * dt;

	if (m_swipe_pos <= m_scrollbar->getMin()) {
		m_swipe_pos = m_scrollbar->getMin();
		m_velocity = 0;
		m_is_coasting = false;
	} else if (m_swipe_pos >= m_scrollbar->getMax()) {
		m_swipe_pos = m_scrollbar->getMax();
		m_velocity = 0;
		m_is_coasting = false;
	}

	s32 old_pos = m_scrollbar->getPos();
	m_scrollbar->setPos((int)m_swipe_pos);
	
	if (old_pos != m_scrollbar->getPos()) {
		SEvent e;
		e.EventType = EET_GUI_EVENT;
		e.GUIEvent.Caller = m_scrollbar;
		e.GUIEvent.Element = nullptr;
		e.GUIEvent.EventType = EGET_SCROLL_BAR_CHANGED;
		OnEvent(e);
	}
#endif
#endif
}

void GUIScrollContainer::draw()
{
	updateScrollCoasting();

	if (isVisible()) {
		core::list<IGUIElement *>::Iterator it = Children.begin();
		for (; it != Children.end(); ++it)
			if ((*it)->isNotClipped() ||
					AbsoluteClippingRect.isRectCollided(
							(*it)->getAbsolutePosition()))
				(*it)->draw();
	}
}

void GUIScrollContainer::updateScrolling()
{
	s32 pos = m_scrollbar->getPos();
	core::rect<s32> rect = getRelativePosition();

	if (m_orientation == VERTICAL)
		rect.UpperLeftCorner.Y = pos * m_scrollfactor;
	else if (m_orientation == HORIZONTAL)
		rect.UpperLeftCorner.X = pos * m_scrollfactor;

	setRelativePosition(rect);
}
