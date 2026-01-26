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

#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
GUIScrollContainer::GUIScrollContainer(gui::IGUIEnvironment *env,
		gui::IGUIElement *parent, s32 id, const core::rect<s32> &rectangle,
		const std::string &orientation, f32 scrollfactor) :
		gui::IGUIElement(gui::EGUIET_CUSTOM_SCROLLCONTAINER, env, parent, id,
				rectangle),
		m_scrollbar(nullptr), m_scroll_swipe(nullptr),
		m_scrollfactor(scrollfactor)
#else
GUIScrollContainer::GUIScrollContainer(gui::IGUIEnvironment *env,
		gui::IGUIElement *parent, s32 id, const core::rect<s32> &rectangle,
		const std::string &orientation, f32 scrollfactor) :
		gui::IGUIElement(gui::EGUIET_ELEMENT, env, parent, id, rectangle),
		m_scrollbar(nullptr), m_scroll_swipe(nullptr),
		m_scrollfactor(scrollfactor)
#endif
{
	if (orientation == "vertical")
		m_orientation = VERTICAL;
	else if (orientation == "horizontal")
		m_orientation = HORIZONTAL;
	else
		m_orientation = UNDEFINED;

#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
#ifdef HAVE_TOUCHSCREENGUI
	m_scroll_swipe = new ScrollSwipe(
			env, this, (ScrollSwipe::OrientationEnum)m_orientation);
	m_scroll_swipe->setScrollFactor(m_scrollfactor);
#endif
#endif
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

	if (m_scroll_swipe) {
		bool retval = m_scroll_swipe->onEvent(event);
		if (retval)
			return true;
	}

	return IGUIElement::OnEvent(event);
}

void GUIScrollContainer::draw()
{
	if (m_scroll_swipe)
		m_scroll_swipe->updateScrollCoasting();

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
