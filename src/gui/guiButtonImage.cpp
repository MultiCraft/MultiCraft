/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "guiButtonImage.h"

#include "client/guiscalingfilter.h"
#include "debug.h"
#include "IGUIEnvironment.h"
#include "IGUIImage.h"
#include "IVideoDriver.h"
#include "StyleSpec.h"

using namespace irr;
using namespace gui;

GUIButtonImage::GUIButtonImage(gui::IGUIEnvironment *environment,
		gui::IGUIElement *parent, s32 id, core::rect<s32> rectangle,
		ISimpleTextureSource *tsrc, bool noclip)
	: GUIButton(environment, parent, id, rectangle, tsrc, noclip)
{
	GUIButton::setScaleImage(true);
	m_image = make_irr<GUIAnimatedImage>(environment, this, id, rectangle);
	sendToBack(m_image.get());
}

void GUIButtonImage::setForegroundImage(irr_ptr<video::ITexture> image,
		const core::rect<s32> &middle)
{
	if (image == m_foreground_image)
		return;

	m_foreground_image = std::move(image);
	m_image->setTexture(m_foreground_image.get());
	m_image->setMiddleRect(middle);
}

//! Set element properties from a StyleSpec
void GUIButtonImage::setFromStyle(const StyleSpec &style)
{
	GUIButton::setFromStyle(style);

	video::IVideoDriver *driver = Environment->getVideoDriver();

	if (style.isNotDefault(StyleSpec::FGIMG)) {
		video::ITexture *texture = style.getTexture(StyleSpec::FGIMG,
				getTextureSource());

		setForegroundImage(::grab(guiScalingImageButton(driver, texture,
				AbsoluteRect.getWidth(), AbsoluteRect.getHeight())),
				style.getRect(StyleSpec::FGIMG_MIDDLE, m_image->getMiddleRect()));
	} else {
		setForegroundImage();
	}
}

GUIButtonImage *GUIButtonImage::addButton(IGUIEnvironment *environment,
		const core::rect<s32> &rectangle, ISimpleTextureSource *tsrc,
		IGUIElement *parent, s32 id, const wchar_t *text,
		const wchar_t *tooltiptext)
{
	auto button = make_irr<GUIButtonImage>(environment,
			parent ? parent : environment->getRootGUIElement(), id, rectangle, tsrc);

	if (text)
		button->setText(text);

	if (tooltiptext)
		button->setToolTipText(tooltiptext);

	return button.get();
}
