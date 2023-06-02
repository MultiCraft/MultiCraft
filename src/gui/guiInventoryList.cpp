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

#include "guiInventoryList.h"
#include "guiFormSpecMenu.h"
#include "client/guiscalingfilter.h"
#include "client/hud.h"
#include "client/client.h"

GUIInventoryList::GUIInventoryList(gui::IGUIEnvironment *env,
	gui::IGUIElement *parent,
	s32 id,
	const core::rect<s32> &rectangle,
	InventoryManager *invmgr,
	const InventoryLocation &inventoryloc,
	const std::string &listname,
	const v2s32 &geom,
	const s32 start_item_i,
	const v2s32 &slot_size,
	const v2f32 &slot_spacing,
	video::ITexture* slotbg_n_texture,
	video::ITexture* slotbg_h_texture,
	video::ITexture* slotbg_p_texture,
	const core::rect<s32> &slotbg_middle,
	GUIFormSpecMenu *fs_menu,
	const Options &options,
	gui::IGUIFont *font) :
	gui::IGUIElement(gui::EGUIET_ELEMENT, env, parent, id, rectangle),
	m_invmgr(invmgr),
	m_inventoryloc(inventoryloc),
	m_listname(listname),
	m_geom(geom),
	m_start_item_i(start_item_i),
	m_slot_size(slot_size),
	m_slot_spacing(slot_spacing),
	m_slotbg_n_texture(slotbg_n_texture),
	m_slotbg_h_texture(slotbg_h_texture),
	m_slotbg_p_texture(slotbg_p_texture),
	m_slotbg_middle(slotbg_middle),
	m_fs_menu(fs_menu),
	m_options(options),
	m_font(font),
	m_hovered_i(-1),
	m_pressed(false),
	m_already_warned(false)
{
}

void GUIInventoryList::draw()
{
	if (!IsVisible)
		return;

	Inventory *inv = m_invmgr->getInventory(m_inventoryloc);
	if (!inv) {
		if (!m_already_warned) {
			warningstream << "GUIInventoryList::draw(): "
					<< "The inventory location "
					<< "\"" << m_inventoryloc.dump() << "\" doesn't exist"
					<< std::endl;
			m_already_warned = true;
		}
		return;
	}
	InventoryList *ilist = inv->getList(m_listname);
	if (!ilist) {
		if (!m_already_warned) {
			warningstream << "GUIInventoryList::draw(): "
					<< "The inventory list \"" << m_listname << "\" @ \""
					<< m_inventoryloc.dump() << "\" doesn't exist"
					<< std::endl;
			m_already_warned = true;
		}
		return;
	}
	m_already_warned = false;

	video::IVideoDriver *driver = Environment->getVideoDriver();
	Client *client = m_fs_menu->getClient();
	const ItemSpec *selected_item = m_fs_menu->getSelectedItem();

	core::rect<s32> imgrect(0, 0, m_slot_size.X, m_slot_size.Y);
	v2s32 base_pos = AbsoluteRect.UpperLeftCorner;

	const s32 list_size = (s32)ilist->getSize();

	for (s32 i = 0; i < m_geom.X * m_geom.Y; i++) {
		s32 item_i = i + m_start_item_i;
		if (item_i >= list_size)
			break;

		v2s32 p((i % m_geom.X) * m_slot_spacing.X,
				(i / m_geom.X) * m_slot_spacing.Y);
		core::rect<s32> rect = imgrect + base_pos + p;
		ItemStack item = ilist->getItem(item_i);

		bool selected = selected_item
			&& m_invmgr->getInventory(selected_item->inventoryloc) == inv
			&& selected_item->listname == m_listname
			&& selected_item->i == item_i;
		bool hovering = m_hovered_i == item_i;
		ItemRotationKind rotation_kind = selected ? IT_ROT_SELECTED :
			(hovering ? IT_ROT_HOVERED : IT_ROT_NONE);

		// layer 0
		if (m_slotbg_n_texture) {
			video::ITexture *texture = m_slotbg_n_texture;
			if (hovering) {
				if (m_pressed && m_slotbg_p_texture)
					texture = m_slotbg_p_texture;
				else if (m_slotbg_h_texture)
					texture = m_slotbg_h_texture;
			}

			core::rect<s32> srcrect(core::position2d<s32>(0, 0),
									core::dimension2di(texture->getOriginalSize()));
			if (m_slotbg_middle.getArea() == 0) {
				const video::SColor color(255, 255, 255, 255);
				const video::SColor colors[] = {color, color, color, color};
				draw2DImageFilterScaled(driver, texture, rect, srcrect,
					&AbsoluteClippingRect, colors, true);
			} else {
				draw2DImage9Slice(driver, texture, rect, srcrect, m_slotbg_middle,
					&AbsoluteClippingRect);
			}
		} else {
			if (hovering) {
				driver->draw2DRectangle(m_options.slotbg_h, rect, &AbsoluteClippingRect);
			} else {
				driver->draw2DRectangle(m_options.slotbg_n, rect, &AbsoluteClippingRect);
			}

			// Draw inv slot borders
			if (m_options.slotborder) {
				s32 x1 = rect.UpperLeftCorner.X;
				s32 y1 = rect.UpperLeftCorner.Y;
				s32 x2 = rect.LowerRightCorner.X;
				s32 y2 = rect.LowerRightCorner.Y;
				s32 border = 1;
				core::rect<s32> clipping_rect = Parent ? Parent->getAbsoluteClippingRect()
						: core::rect<s32>();
				core::rect<s32> *clipping_rect_ptr = Parent ? &clipping_rect : nullptr;
				driver->draw2DRectangle(m_options.slotbordercolor,
					core::rect<s32>(v2s32(x1 - border, y1 - border),
									v2s32(x2 + border, y1)), clipping_rect_ptr);
				driver->draw2DRectangle(m_options.slotbordercolor,
					core::rect<s32>(v2s32(x1 - border, y2),
									v2s32(x2 + border, y2 + border)), clipping_rect_ptr);
				driver->draw2DRectangle(m_options.slotbordercolor,
					core::rect<s32>(v2s32(x1 - border, y1),
									v2s32(x1, y2)), clipping_rect_ptr);
				driver->draw2DRectangle(m_options.slotbordercolor,
					core::rect<s32>(v2s32(x2, y1),
									v2s32(x2 + border, y2)), clipping_rect_ptr);
			}
		}

		// layer 1
		if (selected)
			item.takeItem(m_fs_menu->getSelectedAmount());

		if (!item.empty()) {
			// Draw item stack
			drawItemStack(driver, m_font, item, rect, &AbsoluteClippingRect,
					client, rotation_kind);
			// Add hovering tooltip
			if (hovering && !selected_item) {
				std::string tooltip = item.getDescription(client->idef());
				if (m_fs_menu->doTooltipAppendItemname())
					tooltip += "\n[" + item.name + "]";
				m_fs_menu->addHoveredItemTooltip(tooltip);
			}
		}
	}

	IGUIElement::draw();
}

bool GUIInventoryList::OnEvent(const SEvent &event)
{
	if (event.EventType != EET_MOUSE_INPUT_EVENT) {
		if (event.EventType == EET_GUI_EVENT &&
				event.GUIEvent.EventType == EGET_ELEMENT_LEFT) {
			// element is no longer hovered
			m_hovered_i = -1;
		}
		return IGUIElement::OnEvent(event);
	}

	m_hovered_i = getItemIndexAtPos(v2s32(event.MouseInput.X, event.MouseInput.Y));

	if (m_hovered_i != -1) {
		if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN)
			m_pressed = true;
		else if (event.MouseInput.Event == EMIE_LMOUSE_LEFT_UP)
			m_pressed = false;
		return IGUIElement::OnEvent(event);
	}

	m_pressed = false;

	// no item slot at pos of mouse event => allow clicking through
	// find the element that would be hovered if this inventorylist was invisible
	bool was_visible = IsVisible;
	IsVisible = false;
	IGUIElement *hovered =
		Environment->getRootGUIElement()->getElementFromPoint(
			core::position2d<s32>(event.MouseInput.X, event.MouseInput.Y));

	// if the player clicks outside of the formspec window, hovered is not
	// m_fs_menu, but some other weird element (with ID -1). we do however need
	// hovered to be m_fs_menu as item dropping when clicking outside of the
	// formspec window is handled in its OnEvent callback
	if (!hovered || hovered->getID() == -1)
		hovered = m_fs_menu;

	bool ret = hovered->OnEvent(event);

	IsVisible = was_visible;

	return ret;
}

s32 GUIInventoryList::getItemIndexAtPos(v2s32 p) const
{
	// no item if no gui element at pointer
	if (!IsVisible || AbsoluteClippingRect.getArea() <= 0 ||
			!AbsoluteClippingRect.isPointInside(p))
		return -1;

	// there can not be an item if the inventory or the inventorylist does not exist
	Inventory *inv = m_invmgr->getInventory(m_inventoryloc);
	if (!inv)
		return -1;
	InventoryList *ilist = inv->getList(m_listname);
	if (!ilist)
		return -1;

	core::rect<s32> imgrect(0, 0, m_slot_size.X, m_slot_size.Y);
	v2s32 base_pos = AbsoluteRect.UpperLeftCorner;

	// instead of looping through each slot, we look where p would be in the grid
	s32 i = (p.X - base_pos.X) / (s32)m_slot_spacing.X
			+ m_geom.X * ((p.Y - base_pos.Y) / (s32)m_slot_spacing.Y);

	v2s32 p0((i % m_geom.X) * m_slot_spacing.X,
			(i / m_geom.X) * m_slot_spacing.Y);

	core::rect<s32> rect = imgrect + base_pos + p0;

	rect.clipAgainst(AbsoluteClippingRect);

	if (rect.getArea() > 0 && rect.isPointInside(p) &&
			i + m_start_item_i < (s32)ilist->getSize())
		return i + m_start_item_i;

	return -1;
}
