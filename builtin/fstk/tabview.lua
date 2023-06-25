--Minetest
--Copyright (C) 2014 sapier
--
--This program is free software; you can redistribute it and/or modify
--it under the terms of the GNU Lesser General Public License as published by
--the Free Software Foundation; either version 3.0 of the License, or
--(at your option) any later version.
--
--This program is distributed in the hope that it will be useful,
--but WITHOUT ANY WARRANTY; without even the implied warranty of
--MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--GNU Lesser General Public License for more details.
--
--You should have received a copy of the GNU Lesser General Public License along
--with this program; if not, write to the Free Software Foundation, Inc.,
--51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.


--------------------------------------------------------------------------------
-- A tabview implementation                                                   --
-- Usage:                                                                     --
-- tabview.create: returns initialized tabview raw element                    --
-- element.add(tab): add a tab declaration                                    --
-- element.handle_buttons()                                                   --
-- element.handle_events()                                                    --
-- element.getFormspec() returns formspec of tabview                          --
--------------------------------------------------------------------------------

local fmt = string.format
local tconcat = table.concat

--------------------------------------------------------------------------------
local function add_tab(self,tab)
	assert(tab.size == nil or (type(tab.size) == table and
			tab.size.x ~= nil and tab.size.y ~= nil))
	assert(tab.cbf_formspec ~= nil and type(tab.cbf_formspec) == "function")
	assert(tab.cbf_button_handler == nil or
			type(tab.cbf_button_handler) == "function")
	assert(tab.cbf_events == nil or type(tab.cbf_events) == "function")

	local newtab = {
		name = tab.name,
		caption = tab.caption,
		button_handler = tab.cbf_button_handler,
		event_handler = tab.cbf_events,
		get_formspec = tab.cbf_formspec,
		tabsize = tab.tabsize,
		on_change = tab.on_change,
		tabdata = {},
	}

	-- Hidden tabs have a negative index
	local i
	if tab.hidden then
		newtab.tabdata.hidden = true
		i = -1
		while self.tablist[i] do
			i = i - 1
		end
	else
		i = #self.tablist + 1
	end
	self.tablist[i] = newtab

	if self.last_tab_index == i then
		self.current_tab = tab.name
		if tab.on_activate ~= nil then
			tab.on_activate(nil,tab.name)
		end
	end
end

--------------------------------------------------------------------------------
local function add_side_button(self, btn)
	if btn.pos then
		table.insert(self.side_buttons, btn.pos, btn)
	else
		self.side_buttons[#self.side_buttons + 1] = btn
	end
end

--------------------------------------------------------------------------------
local tpath = defaulttexturedir_esc .. "gui" .. DIR_DELIM_esc
local function add_side_menu(self, fs, selected_tab, x, fs_h, darken)
	if #self.side_buttons == 0 then return end

	local side_menu_h = #self.side_buttons * 1.41 + 0.14
	local bg_y = fs_h / 2 - side_menu_h / 2
	fs[#fs + 1] = fmt("image[%s,%s;1.15,%s;%sside_menu.png;30]", x, bg_y, side_menu_h, tpath)

	for i, btn in ipairs(self.side_buttons) do
		local y = bg_y - 1.31 + 1.41 * i
		if i > 1 then
			fs[#fs + 1] = fmt("image[%s,%s;0.9,0.06;%sside_menu_divider.png]", x + 0.1, y - 0.06, tpath)
		end

		local btn_name = self.name .. "_side_" .. i
		fs[#fs + 1] = fmt("style[%s;bgimg=", btn_name)

		local tab_name = btn.tab_name or btn.tab_name_selected
		local texture_prefix = btn.texture_prefix or tab_name
		if tab_name and tab_name == selected_tab then
			fs[#fs + 1] = btn.texture_selected or tpath .. texture_prefix .. "_menu_selected.png"
		else
			fs[#fs + 1] = btn.texture or tpath .. texture_prefix .. "_menu.png"
			fs[#fs + 1] = ";bgimg_hovered="
			fs[#fs + 1] = btn.texture_hover or tpath .. texture_prefix .. "_menu_hover.png"
		end
		fs[#fs + 1] = "]"

		fs[#fs + 1] = fmt("image_button[%s,%s;1,1.35;;%s;;true;false]", x + 0.09, y, btn_name)
		fs[#fs + 1] = fmt("tooltip[%s;%s]", btn_name, btn.tooltip)
	end

	if darken then
		fs[#fs + 1] = fmt("style[cancel;bgimg=%sside_menu_darken.png;bgimg_middle=30]", tpath)
		fs[#fs + 1] = fmt("image_button[%s,%s;1.15,%s;;cancel;;true;false]", x, bg_y, side_menu_h)

		-- Reset "cancel" button styling
		fs[#fs + 1] = "style[cancel;bgimg=;bgimg_middle=]"
	end
end

--------------------------------------------------------------------------------
local function add_button_header(self, fs, fs_w)
	local visible_tabs = {}
	for _, tab in ipairs(self.tablist) do
		if not tab.hidden and tab.caption ~= "" then
			visible_tabs[#visible_tabs + 1] = tab
		end
	end

	local x = 1.5
	local w = (fs_w - 0.53) / #visible_tabs
	for i = 1, #visible_tabs do
		local caption = visible_tabs[i].caption
		local side = i == 1 and "left" or (i < #visible_tabs and "middle" or "right")
		local btn_name = self.name .. "_" .. i
		if i == math.abs(self.last_tab_index) then
			side = side .. "_pressed"
		end

		fs[#fs + 1] = btn_style(btn_name, side)
		fs[#fs + 1] = fmt("style[%s;content_offset=0;font_size=+1]", btn_name)
		fs[#fs + 1] = fmt("image_button[%s,0.25;%s,0.9;;%s;%s;true;false]",
			x, w + 0.03, btn_name, caption)
		x = x + w
	end
end

--------------------------------------------------------------------------------
local function get_formspec(self, popup_w, popup_h, popup_fs)
	if (self.hidden or (self.parent and self.parent.hidden)) and not popup_fs then
		return ""
	end

	local fs = {}

	local current_tab = self.tablist[self.last_tab_index]
	local content, prepend_override = current_tab.get_formspec(self,
		current_tab.name, current_tab.tabdata, current_tab.size)
	local real_coords = current_tab.formspec_version and current_tab.formspec_version > 1
	local w, h
	if prepend_override then
		fs[#fs + 1] = prepend_override
	elseif not self.parent then
		fs[#fs + 1] = "formspec_version[4]"

		local s = current_tab.tabsize or self
		w, h = s.width, s.height
		if not real_coords then
			w = w * 1.25 + 0.6
			h = h * 1.15 + 0.9
		end
		fs[#fs + 1] = fmt("size[%s,%s]", w + 2.5, h + 1.15)

		-- Background
		fs[#fs + 1] = "bgcolor[;neither]"
		fs[#fs + 1] = "listcolors[#000;#000;#000;#dff6f5;#302c2e]"
		fs[#fs + 1] = fmt("background9[1.25,0;%s,%s;%sbg_common.png;false;32]",
			w, h + 1.15, defaulttexturedir)

		add_button_header(self, fs, w)

		-- This container isn't ideal for real_coordinates formspecs but
		-- keeps compatibility with existing code
		fs[#fs + 1] = "container[1,1]"
		fs[#fs + 1] = "real_coordinates[false]"
	end

	fs[#fs + 1] = content

	if not prepend_override and not self.parent then
		-- Make sure that real_coordinates is enabled
		fs[#fs + 1] = "real_coordinates[true]"
		fs[#fs + 1] = "container_end[]"

		-- Darken background
		if popup_fs then
			-- This styling gets reset in add_side_menu
			fs[#fs + 1] = fmt("style[cancel;bgimg=%sbg_darken.png;bgimg_middle=32]", tpath)
			fs[#fs + 1] = fmt("image_button[1.25,0;%s,%s;;cancel;;false;false]",
				w, h + 1.15)
		end

		-- Add the side menu after the darkened background
		add_side_menu(self, fs, current_tab.name, w + 1.175, h + 1.15, popup_fs)

		-- Draw the popup
		if popup_fs then
			fs[#fs + 1] = fmt("container[%s,%s]", (w + 2.5 - popup_w) / 2,
				(h + 1.15 - popup_h) / 2)
			fs[#fs + 1] = fmt("style[popup_bg;bgimg=%sbg_common.png;bgimg_middle=32]",
				defaulttexturedir)
			fs[#fs + 1] = fmt("image_button[0,0;%s,%s;;popup_bg;;false;false]",
				popup_w, popup_h)
			fs[#fs + 1] = popup_fs
			fs[#fs + 1] = "container_end[]"
		end

		-- Disable real_coordinates again in case there are other UI elements
		-- (like the game switcher in buttonbar.lua)
		fs[#fs + 1] = "real_coordinates[false]"
	end

	return tconcat(fs)
end

local set_tab_by_name
--------------------------------------------------------------------------------
local function handle_buttons(self,fields)

	if self.hidden then
		return false
	end

	if self:handle_tab_buttons(fields) then
		return true
	end

	if self.glb_btn_handler ~= nil and
		self.glb_btn_handler(self,fields) then
		return true
	end

	if self.tablist[self.last_tab_index].button_handler ~= nil then
		return
			self.tablist[self.last_tab_index].button_handler(
					self,
					fields,
					self.tablist[self.last_tab_index].name,
					self.tablist[self.last_tab_index].tabdata
					)
	end

	return false
end

--------------------------------------------------------------------------------
local function handle_events(self,event)

	if self.hidden then
		return false
	end

	if self.glb_evt_handler ~= nil and
		self.glb_evt_handler(self,event) then
		return true
	end

	if self.tablist[self.last_tab_index].evt_handler ~= nil then
		return
			self.tablist[self.last_tab_index].evt_handler(
					self,
					event,
					self.tablist[self.last_tab_index].name,
					self.tablist[self.last_tab_index].tabdata
					)
	end

	return false
end

--------------------------------------------------------------------------------
local function switch_to_tab(self, index)
	--first call on_change for tab to leave
	if self.tablist[self.last_tab_index].on_change ~= nil then
		self.tablist[self.last_tab_index].on_change("LEAVE",
				self.current_tab, self.tablist[index].name, self)
	end

	--update tabview data
	self.last_tab_index = index
	local old_tab = self.current_tab
	self.current_tab = self.tablist[index].name

	if (self.autosave_tab) then
		core.settings:set(self.name .. "_LAST",self.current_tab)
	end

	-- call for tab to enter
	if self.tablist[index].on_change ~= nil then
		self.tablist[index].on_change("ENTER",
				old_tab,self.current_tab,self)
	end
end

--------------------------------------------------------------------------------
local function handle_tab_buttons(self, fields)
	--save tab selection to config file
	--[[if fields[self.name] then
		local index = tonumber(fields[self.name])
		switch_to_tab(self, index)
		return true
	end]]

	local name_prefix = self.name .. "_"
	local name_prefix_len = #name_prefix
	for field in pairs(fields) do
		if field:sub(1, name_prefix_len) == name_prefix then
			if field:sub(name_prefix_len + 1, name_prefix_len + 5) == "side_" then
				local btn = self.side_buttons[tonumber(field:sub(name_prefix_len + 6))]
				if btn.tab_name then
					set_tab_by_name(self, btn.tab_name)
				else
					btn.on_click(self)
				end
			else
				local index = tonumber(field:sub(name_prefix_len + 1))
				if math.abs(self.last_tab_index) == index then return false end
				switch_to_tab(self, index)
			end
			return true
		end
	end

	return false
end

--------------------------------------------------------------------------------
-- Declared as a local variable above handle_buttons
function set_tab_by_name(self, name)
	-- This uses pairs so that hidden tabs (with a negative index) are searched
	-- as well
	for i, tab in pairs(self.tablist) do
		if tab.name == name then
			switch_to_tab(self, i)

			if name ~= "local" then
				mm_texture.set_dirt_bg()
			end

			return true
		end
	end

	return false
end

--------------------------------------------------------------------------------
local function hide_tabview(self)
	self.hidden=true

	--call on_change as we're not gonna show self tab any longer
	if self.tablist[self.last_tab_index].on_change ~= nil then
		self.tablist[self.last_tab_index].on_change("LEAVE",
				self.current_tab, nil)
	end
end

--------------------------------------------------------------------------------
local function show_tabview(self)
	self.hidden=false

	-- call for tab to enter
	if self.tablist[self.last_tab_index].on_change ~= nil then
		self.tablist[self.last_tab_index].on_change("ENTER",
				nil,self.current_tab,self)
	end
end

local tabview_metatable = {
	add                       = add_tab,
	handle_buttons            = handle_buttons,
	handle_events             = handle_events,
	get_formspec              = get_formspec,
	show                      = show_tabview,
	hide                      = hide_tabview,
	delete                    = function(self) ui.delete(self) end,
	set_parent                = function(self,parent) self.parent = parent end,
	set_autosave_tab          =
			function(self,value) self.autosave_tab = value end,
	set_tab                   = set_tab_by_name,
	set_global_button_handler =
			function(self,handler) self.glb_btn_handler = handler end,
	set_global_event_handler =
			function(self,handler) self.glb_evt_handler = handler end,
	set_fixed_size =
			function(self,state) self.fixed_size = state end,
	add_side_button = add_side_button,
	handle_tab_buttons = handle_tab_buttons
}

tabview_metatable.__index = tabview_metatable

--------------------------------------------------------------------------------
function tabview_create(name, size, tabheaderpos)
	local self = {}

	self.name     = name
	self.type     = "toplevel"
	self.width    = size.x
	self.height   = size.y
	self.header_x = tabheaderpos.x
	self.header_y = tabheaderpos.y

	setmetatable(self, tabview_metatable)

	self.fixed_size     = true
	self.hidden         = true
	self.current_tab    = nil
	self.last_tab_index = 1
	self.tablist        = {}
	self.side_buttons   = {}

	self.autosave_tab   = false

	ui.add(self)
	return self
end
