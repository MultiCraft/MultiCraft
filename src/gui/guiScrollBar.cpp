/*
Copyright (C) 2002-2013 Nikolaus Gebhardt
This file is part of the "Irrlicht Engine".
For conditions of distribution and use, see copyright notice in irrlicht.h

Modified 2019.05.01 by stujones11, Stuart Jones <stujones111@gmail.com>

This is a heavily modified copy of the Irrlicht CGUIScrollBar class
which includes automatic scaling of the thumb slider and hiding of
the arrow buttons where there is insufficient space.
*/

#include "guiScrollBar.h"
#include <IGUIButton.h>
#include <IGUIImage.h>

GUIScrollBar::GUIScrollBar(IGUIEnvironment *environment, IGUIElement *parent, s32 id,
		core::rect<s32> rectangle, bool horizontal, bool auto_scale) :
		IGUIElement(EGUIET_ELEMENT, environment, parent, id, rectangle),
		up_button(nullptr), down_button(nullptr), bg_image(nullptr),
		slider_image(nullptr), slider_top_image(nullptr),
		slider_bottom_image(nullptr), is_dragging(false), is_horizontal(horizontal),
		is_auto_scaling(auto_scale), dragged_by_slider(false),
		tray_clicked(false), scroll_pos(0), draw_center(0), thumb_size(0),
		min_pos(0), max_pos(100), small_step(10), large_step(50),
		drag_offset(0), page_size(100), border_size(0)
{
	refreshControls();
	setNotClipped(false);
	setTabStop(true);
	setTabOrder(-1);
	setPos(0);
}

bool GUIScrollBar::OnEvent(const SEvent &event)
{
	if (isEnabled()) {
		switch (event.EventType) {
		case EET_KEY_INPUT_EVENT:
			if (event.KeyInput.PressedDown) {
				const s32 old_pos = scroll_pos;
				bool absorb = true;
				switch (event.KeyInput.Key) {
				case KEY_LEFT:
				case KEY_UP:
					setPos(scroll_pos - small_step);
					break;
				case KEY_RIGHT:
				case KEY_DOWN:
					setPos(scroll_pos + small_step);
					break;
				case KEY_HOME:
					setPos(min_pos);
					break;
				case KEY_PRIOR:
					setPos(scroll_pos - large_step);
					break;
				case KEY_END:
					setPos(max_pos);
					break;
				case KEY_NEXT:
					setPos(scroll_pos + large_step);
					break;
				default:
					absorb = false;
				}
				if (scroll_pos != old_pos) {
					SEvent e;
					e.EventType = EET_GUI_EVENT;
					e.GUIEvent.Caller = this;
					e.GUIEvent.Element = nullptr;
					e.GUIEvent.EventType = EGET_SCROLL_BAR_CHANGED;
					Parent->OnEvent(e);
				}
				if (absorb)
					return true;
			}
			break;
		case EET_GUI_EVENT:
			if (event.GUIEvent.EventType == EGET_BUTTON_CLICKED) {
				if (event.GUIEvent.Caller == up_button)
					setPos(scroll_pos - small_step);
				else if (event.GUIEvent.Caller == down_button)
					setPos(scroll_pos + small_step);

				SEvent e;
				e.EventType = EET_GUI_EVENT;
				e.GUIEvent.Caller = this;
				e.GUIEvent.Element = nullptr;
				e.GUIEvent.EventType = EGET_SCROLL_BAR_CHANGED;
				Parent->OnEvent(e);
				return true;
			} else if (event.GUIEvent.EventType == EGET_ELEMENT_FOCUS_LOST)
				if (event.GUIEvent.Caller == this)
					is_dragging = false;
			break;
		case EET_MOUSE_INPUT_EVENT: {
			const core::position2di p(event.MouseInput.X, event.MouseInput.Y);
			bool is_inside = isPointInside(p);
			switch (event.MouseInput.Event) {
			case EMIE_MOUSE_WHEEL:
				if (Environment->hasFocus(this)) {
					s8 d = event.MouseInput.Wheel < 0 ? -1 : 1;
					s8 h = is_horizontal ? 1 : -1;
					setPos(getPos() + (d * small_step * h));

					SEvent e;
					e.EventType = EET_GUI_EVENT;
					e.GUIEvent.Caller = this;
					e.GUIEvent.Element = nullptr;
					e.GUIEvent.EventType = EGET_SCROLL_BAR_CHANGED;
					Parent->OnEvent(e);
					return true;
				}
				break;
			case EMIE_LMOUSE_PRESSED_DOWN: {
				if (is_inside) {
					is_dragging = true;
					dragged_by_slider = slider_rect.isPointInside(p);
					core::vector2di corner = slider_rect.UpperLeftCorner;
					drag_offset = is_horizontal ? p.X - corner.X : p.Y - corner.Y;
					tray_clicked = !dragged_by_slider;
					if (tray_clicked) {
						const s32 new_pos = getPosFromMousePos(p);
						const s32 old_pos = scroll_pos;
						setPos(new_pos);
						// drag in the middle
						drag_offset = thumb_size / 2;
						// report the scroll event
						if (scroll_pos != old_pos && Parent) {
							SEvent e;
							e.EventType = EET_GUI_EVENT;
							e.GUIEvent.Caller = this;
							e.GUIEvent.Element = nullptr;
							e.GUIEvent.EventType = EGET_SCROLL_BAR_CHANGED;
							Parent->OnEvent(e);
						}
					}
					Environment->setFocus(this);
					return true;
				}
				break;
			}
			case EMIE_LMOUSE_LEFT_UP:
			case EMIE_MOUSE_MOVED: {
				if (!event.MouseInput.isLeftPressed())
					is_dragging = false;

				if (!is_dragging) {
					if (event.MouseInput.Event == EMIE_MOUSE_MOVED)
						break;
					return is_inside;
				}

				if (event.MouseInput.Event == EMIE_LMOUSE_LEFT_UP)
					is_dragging = false;

				// clang-format off
				if (!dragged_by_slider) {
					if (is_inside) {
						dragged_by_slider = slider_rect.isPointInside(p);
						tray_clicked = !dragged_by_slider;
					}
					if (!dragged_by_slider) {
						tray_clicked = false;
						if (event.MouseInput.Event == EMIE_MOUSE_MOVED)
							return is_inside;
					}
				}
				// clang-format on

				const s32 new_pos = getPosFromMousePos(p);
				const s32 old_pos = scroll_pos;

				setPos(new_pos);

				if (scroll_pos != old_pos && Parent) {
					SEvent e;
					e.EventType = EET_GUI_EVENT;
					e.GUIEvent.Caller = this;
					e.GUIEvent.Element = nullptr;
					e.GUIEvent.EventType = EGET_SCROLL_BAR_CHANGED;
					Parent->OnEvent(e);
				}
				return is_inside;
			}
			default:
				break;
			}
		} break;
		default:
			break;
		}
	}
	return IGUIElement::OnEvent(event);
}

gui::IGUIImage* GUIScrollBar::addImage(const core::rect<s32> &rect, video::ITexture *texture)
{
	gui::IGUIImage *e = Environment->addImage(rect, this);
	e->setImage(texture);
	e->setScaleImage(true);
	return e;
}

void GUIScrollBar::draw()
{
	if (!IsVisible)
		return;

	IGUISkin *skin = Environment->getSkin();
	if (!skin)
		return;

	video::SColor icon_color = skin->getColor(
			isEnabled() ? EGDC_WINDOW_SYMBOL : EGDC_GRAY_WINDOW_SYMBOL);
	if (icon_color != current_icon_color)
		refreshControls();

	slider_rect = AbsoluteRect;

	if (m_textures.size() >= 1) {
		s32 w = RelativeRect.getWidth();
		s32 h = RelativeRect.getHeight();
		core::rect<s32> rect{0, w, w, h - w};

		if (is_horizontal)
			rect = {h, 0, w - h, h};

		if (!bg_image)
			bg_image = addImage(rect, m_textures[0]);
		else
			bg_image->setRelativePosition(rect);
	} else {
		skin->draw2DRectangle(this, skin->getColor(EGDC_SCROLLBAR),
			slider_rect, &AbsoluteClippingRect);
	}

	// Always show scrollbar thumb
	//if (core::isnotzero(range())) {
		if (is_horizontal) {
			slider_rect.UpperLeftCorner.X = AbsoluteRect.UpperLeftCorner.X +
							draw_center - thumb_size / 2;
			slider_rect.LowerRightCorner.X =
					slider_rect.UpperLeftCorner.X + thumb_size;
		} else {
			slider_rect.UpperLeftCorner.Y = AbsoluteRect.UpperLeftCorner.Y +
							draw_center - thumb_size / 2;
			slider_rect.LowerRightCorner.Y =
					slider_rect.UpperLeftCorner.Y + thumb_size;
		}

		if (m_textures.size() >= 2) {
			s32 w = slider_rect.getWidth();
			s32 h = slider_rect.getHeight();
			core::rect<s32> rect{0, draw_center - (h / 2), w, draw_center + h - (h / 2)};

			if (is_horizontal)
				rect = {draw_center - (w / 2), 0, draw_center + w - (w / 2), h};

			if (!slider_image)
				slider_image = addImage(rect, m_textures[1]);
			else
				slider_image->setRelativePosition(rect);

			// Add top and bottom images if required
			// TODO: Horizontal scrollbars
			if (m_textures.size() >= 6 && !is_horizontal) {
				core::rect<s32> top_rect = rect;
				top_rect.LowerRightCorner.Y = rect.UpperLeftCorner.Y + w / 2;
				if (!slider_top_image)
					slider_top_image = addImage(top_rect, m_textures[4]);
				else
					slider_top_image->setRelativePosition(top_rect);

				core::rect<s32> bottom_rect = rect;
				bottom_rect.UpperLeftCorner.Y = rect.LowerRightCorner.Y - w / 2;
				if (!slider_bottom_image)
					slider_bottom_image = addImage(bottom_rect, m_textures[5]);
				else
					slider_bottom_image->setRelativePosition(bottom_rect);
			}
		} else {
			skin->draw3DButtonPaneStandard(this, slider_rect, &AbsoluteClippingRect);
		}
	//}

	IGUIElement::draw();
}

void GUIScrollBar::updateAbsolutePosition()
{
	IGUIElement::updateAbsolutePosition();
	refreshControls();
	setPos(scroll_pos);
}

s32 GUIScrollBar::getPosFromMousePos(const core::position2di &pos) const
{
	s32 w, p;
	s32 offset = dragged_by_slider ? drag_offset : thumb_size / 2;

	if (is_horizontal) {
		w = RelativeRect.getWidth() - border_size * 2 - thumb_size;
		p = pos.X - AbsoluteRect.UpperLeftCorner.X - border_size - offset;
	} else {
		w = RelativeRect.getHeight() - border_size * 2 - thumb_size;
		p = pos.Y - AbsoluteRect.UpperLeftCorner.Y - border_size - offset;
	}
	return core::isnotzero(range()) ? s32(f32(p) / f32(w) * range() + 0.5f) + min_pos : 0;
}

void GUIScrollBar::setPos(const s32 &pos)
{
	s32 thumb_area = 0;
	s32 thumb_min = 0;

	if (is_horizontal) {
		thumb_min = RelativeRect.getHeight();
		thumb_area = RelativeRect.getWidth() - border_size * 2;
	} else {
		thumb_min = RelativeRect.getWidth();
		thumb_area = RelativeRect.getHeight() - border_size * 2;
	}

	if (is_auto_scaling)
		thumb_size = s32(thumb_area /
				 (f32(page_size) / f32(thumb_area + border_size * 2)));

	thumb_size = core::s32_clamp(thumb_size, thumb_min, thumb_area);
	scroll_pos = core::s32_clamp(pos, min_pos, max_pos);

	f32 f = core::isnotzero(range()) ? (f32(thumb_area) - f32(thumb_size)) / range()
					 : 1.0f;
	draw_center = s32((f32(scroll_pos - min_pos) * f) + (f32(thumb_size) * 0.5f)) +
		border_size;
}

void GUIScrollBar::setSmallStep(const s32 &step)
{
	small_step = step > 0 ? step : 10;
}

void GUIScrollBar::setLargeStep(const s32 &step)
{
	large_step = step > 0 ? step : 50;
}

void GUIScrollBar::setMax(const s32 &max)
{
	max_pos = max;
	if (min_pos > max_pos)
		min_pos = max_pos;

	bool enable = core::isnotzero(range());
	up_button->setEnabled(enable);
	down_button->setEnabled(enable);
	setPos(scroll_pos);
}

void GUIScrollBar::setMin(const s32 &min)
{
	min_pos = min;
	if (max_pos < min_pos)
		max_pos = min_pos;

	bool enable = core::isnotzero(range());
	up_button->setEnabled(enable);
	down_button->setEnabled(enable);
	setPos(scroll_pos);
}

void GUIScrollBar::setPageSize(const s32 &size)
{
	page_size = size;
	setPos(scroll_pos);
}

void GUIScrollBar::setArrowsVisible(ArrowVisibility visible)
{
	arrow_visibility = visible;
	refreshControls();
}

s32 GUIScrollBar::getPos() const
{
	return scroll_pos;
}

void GUIScrollBar::setTextures(const std::vector<video::ITexture *> &textures)
{
	m_textures = textures;
	refreshControls();
};

void GUIScrollBar::setStyle(const StyleSpec &style, ISimpleTextureSource *tsrc)
{
	if (style.isNotDefault(StyleSpec::SCROLLBAR_BGIMG) &&
			style.isNotDefault(StyleSpec::SCROLLBAR_THUMB_IMG) &&
			style.isNotDefault(StyleSpec::SCROLLBAR_UP_IMG) &&
			style.isNotDefault(StyleSpec::SCROLLBAR_DOWN_IMG)) {
		arrow_visibility = ArrowVisibility::SHOW;
		std::vector<video::ITexture *> textures = {
			style.getTexture(StyleSpec::SCROLLBAR_BGIMG, tsrc),
			style.getTexture(StyleSpec::SCROLLBAR_THUMB_IMG, tsrc),
			style.getTexture(StyleSpec::SCROLLBAR_UP_IMG, tsrc),
			style.getTexture(StyleSpec::SCROLLBAR_DOWN_IMG, tsrc)
		};
		if (style.isNotDefault(StyleSpec::SCROLLBAR_THUMB_TOP_IMG) &&
				style.isNotDefault(StyleSpec::SCROLLBAR_THUMB_BOTTOM_IMG)) {
			textures.push_back(style.getTexture(StyleSpec::SCROLLBAR_THUMB_TOP_IMG, tsrc));
			textures.push_back(style.getTexture(StyleSpec::SCROLLBAR_THUMB_BOTTOM_IMG, tsrc));
		}
		setTextures(textures);
	}
}

void GUIScrollBar::refreshControls()
{
	IGUISkin *skin = Environment->getSkin();
	IGUISpriteBank *sprites = nullptr;
	current_icon_color = video::SColor(255, 255, 255, 255);

	if (skin) {
		sprites = skin->getSpriteBank();
		current_icon_color =
				skin->getColor(isEnabled() ? EGDC_WINDOW_SYMBOL
							   : EGDC_GRAY_WINDOW_SYMBOL);
	}
	if (is_horizontal) {
		s32 h = RelativeRect.getHeight();
		border_size = RelativeRect.getWidth() < h * 4 ? 0 : h;

		if (!up_button) {
			up_button = Environment->addButton(
					core::rect<s32>(0, 0, h, h), this);
			up_button->setSubElement(true);
			up_button->setTabStop(false);
		}

		if (m_textures.size() >= 3) {
			up_button->setImage(m_textures[2]);
			up_button->setScaleImage(true);
			up_button->setDrawBorder(false);
			up_button->setUseAlphaChannel(true);
			up_button->setSpriteBank(nullptr);
		} else if (sprites) {
			up_button->setSpriteBank(sprites);
			up_button->setSprite(EGBS_BUTTON_UP,
					s32(skin->getIcon(EGDI_CURSOR_LEFT)),
					current_icon_color);
			up_button->setSprite(EGBS_BUTTON_DOWN,
					s32(skin->getIcon(EGDI_CURSOR_LEFT)),
					current_icon_color);
		}
		up_button->setRelativePosition(core::rect<s32>(0, 0, h, h));
		up_button->setAlignment(EGUIA_UPPERLEFT, EGUIA_UPPERLEFT, EGUIA_UPPERLEFT,
				EGUIA_LOWERRIGHT);

		if (!down_button) {
			down_button = Environment->addButton(
					core::rect<s32>(RelativeRect.getWidth() - h, 0,
							RelativeRect.getWidth(), h),
					this);
			down_button->setSubElement(true);
			down_button->setTabStop(false);
		}

		if (m_textures.size() >= 4) {
			down_button->setImage(m_textures[3]);
			down_button->setScaleImage(true);
			down_button->setDrawBorder(false);
			down_button->setUseAlphaChannel(true);
			down_button->setSpriteBank(nullptr);
		} else if (sprites) {
			down_button->setSpriteBank(sprites);
			down_button->setSprite(EGBS_BUTTON_UP,
					s32(skin->getIcon(EGDI_CURSOR_RIGHT)),
					current_icon_color);
			down_button->setSprite(EGBS_BUTTON_DOWN,
					s32(skin->getIcon(EGDI_CURSOR_RIGHT)),
					current_icon_color);
		}

		down_button->setRelativePosition(
				core::rect<s32>(RelativeRect.getWidth() - h, 0,
						RelativeRect.getWidth(), h));
		down_button->setAlignment(EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT,
				EGUIA_UPPERLEFT, EGUIA_LOWERRIGHT);
	} else {
		s32 w = RelativeRect.getWidth();
		s32 h = RelativeRect.getHeight();
		border_size = h < w * 4 ? 0 : w;

		if (!up_button) {
			up_button = Environment->addButton(
					core::rect<s32>(0, 0, w, w), this);
			up_button->setSubElement(true);
			up_button->setTabStop(false);
		}

		if (m_textures.size() >= 3) {
			up_button->setImage(m_textures[2]);
			up_button->setScaleImage(true);
			up_button->setDrawBorder(false);
			up_button->setUseAlphaChannel(true);
			up_button->setSpriteBank(nullptr);
		} else if (sprites) {
			up_button->setSpriteBank(sprites);
			up_button->setSprite(EGBS_BUTTON_UP,
					s32(skin->getIcon(EGDI_CURSOR_UP)),
					current_icon_color);
			up_button->setSprite(EGBS_BUTTON_DOWN,
					s32(skin->getIcon(EGDI_CURSOR_UP)),
					current_icon_color);
		}
		up_button->setRelativePosition(core::rect<s32>(0, 0, w, w));
		up_button->setAlignment(EGUIA_UPPERLEFT, EGUIA_LOWERRIGHT,
				EGUIA_UPPERLEFT, EGUIA_UPPERLEFT);

		if (!down_button) {
			down_button = Environment->addButton(
					core::rect<s32>(0, 0, w, w), this);
			down_button->setSubElement(true);
			down_button->setTabStop(false);
		}

		if (m_textures.size() >= 4) {
			down_button->setImage(m_textures[3]);
			down_button->setScaleImage(true);
			down_button->setDrawBorder(false);
			down_button->setUseAlphaChannel(true);
			down_button->setSpriteBank(nullptr);
		} else if (sprites) {
			down_button->setSpriteBank(sprites);
			down_button->setSprite(EGBS_BUTTON_UP,
					s32(skin->getIcon(EGDI_CURSOR_DOWN)),
					current_icon_color);
			down_button->setSprite(EGBS_BUTTON_DOWN,
					s32(skin->getIcon(EGDI_CURSOR_DOWN)),
					current_icon_color);
		}
		down_button->setRelativePosition(
				core::rect<s32>(0, RelativeRect.getHeight() - w, w,
						RelativeRect.getHeight()));
		down_button->setAlignment(EGUIA_UPPERLEFT, EGUIA_LOWERRIGHT,
				EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT);
	}

	bool visible;
	if (arrow_visibility == DEFAULT)
		visible = (border_size != 0);
	else if (arrow_visibility == HIDE) {
		visible = false;
		border_size = 0;
	} else {
		visible = true;
		if (is_horizontal)
			border_size = RelativeRect.getHeight();
		else
			border_size = RelativeRect.getWidth();
	}

	up_button->setVisible(visible);
	down_button->setVisible(visible);
}
