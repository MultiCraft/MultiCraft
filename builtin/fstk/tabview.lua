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
local function get_side_menu(self, selected_tab)
	if #self.side_buttons == 0 then return "" end

	local side_menu_h = #self.side_buttons * 1.2 + 0.2
	local bg_y = 2.35 - side_menu_h / 2
	local fs = {
		"background9[12.13,", bg_y, ";0.9,", side_menu_h, ";", tpath,
			"side_menu.png;false;30]"
	}

	for i, btn in ipairs(self.side_buttons) do
		local y = bg_y - 1.15 + 1.2 * i
		if i > 1 then
			fs[#fs + 1] = "image[12.15," .. y - 0.04 .. ";0.9,0.06;" ..
					tpath .. "side_menu_divider.png]"
		end

		local btn_name = self.name .. "_side_" .. i
		fs[#fs + 1] = "style[" .. btn_name .. ";bgimg="

		local texture_prefix = btn.texture_prefix or btn.tab_name
		if btn.tab_name and btn.tab_name == selected_tab then
			fs[#fs + 1] = btn.texture_selected or tpath .. texture_prefix .. "_menu_selected.png"
		else
			fs[#fs + 1] = btn.texture or tpath .. texture_prefix .. "_menu.png"
			fs[#fs + 1] = ";bgimg_hovered="
			fs[#fs + 1] = btn.texture_hover or tpath .. texture_prefix .. "_menu_hover.png]"
		end
		fs[#fs + 1] = "]"

		fs[#fs + 1] = "image_button[12.1," .. y .. ";1,1.3;;" .. btn_name .. ";;true;false]"
		fs[#fs + 1] = "tooltip[" .. btn_name .. ";" .. btn.tooltip .. "]"
	end

	return table.concat(fs)
end

local function get_formspec(self)
	local formspec = ""

	if not self.hidden and (self.parent == nil or not self.parent.hidden) then
		local name = self.tablist[self.last_tab_index].name

		if self.parent == nil then
			local tsize = self.tablist[self.last_tab_index].tabsize or
					{width=self.width, height=self.height}
			formspec = formspec ..
					string.format("size[%f,%f,%s]",tsize.width + 2,tsize.height + 1,
						dump(self.fixed_size)) ..
					"bgcolor[#0000]" ..
					"listcolors[#000;#000;#000;#dff6f5;#302c2e]" ..
					"container[1,1]" ..
					"background9[-0.2,-1.26;" .. tsize.width + 0.4 .. "," ..
						tsize.height + 1.75 .. ";" .. defaulttexturedir_esc ..
						"bg_common.png;false;40]" ..
						get_side_menu(self, name)
		end

	--	formspec = formspec .. self:tab_header()
		formspec = formspec .. self:button_header() ..
				self.tablist[self.last_tab_index].get_formspec(
					self,
					name,
					self.tablist[self.last_tab_index].tabdata,
					self.tablist[self.last_tab_index].tabsize
					)

		if self.parent == nil then
			formspec = formspec .. "container_end[]"
		end
	end
	return formspec
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
--[[local function tab_header(self)

	local captions = {}
	for i = 1, #self.tablist do
		captions[i] = self.tablist[i].caption
	end

	local toadd = table.concat(captions, ",")
	return string.format("tabheader[%f,%f;%s;%s;%i;true;false]",
			self.header_x, self.header_y, self.name, toadd,
			math.max(self.last_tab_index, 1))
end]]


--------------------------------------------------------------------------------
local function button_header(self)
	local visible_tabs = {}
--	local btn_widths = {}
--	local total_width = 0
	for i, tab in ipairs(self.tablist) do
		if not tab.hidden and tab.caption ~= "" then
			visible_tabs[#visible_tabs + 1] = tab

		--	local w = math.max(utf8.len(core.get_translated_string(tab.caption)), 10)
		--	btn_widths[#visible_tabs] = w
		--	total_width = total_width + w
		end
	end

	local toadd = ""
--	local coords_per_char = 12 / total_width

	-- The formspec is 15.4875 "real" coordinates wide
	-- local x = (12.39 - total_width) / 2 - 0.3
	local x = -0.1
	local w = 12 / #visible_tabs
	for i = 1, #visible_tabs do
		local caption = visible_tabs[i].caption
	--	local w = btn_widths[i] * coords_per_char
		local side = "middle"
		if i == 1 then
			side = "left"
		elseif i == #visible_tabs then
			side = "right"
		end
		local btn_name = self.name .. "_" .. i
		if i == math.abs(self.last_tab_index) then
			side = side .. "_pressed"
		end

		toadd = toadd ..
			btn_style(btn_name, side) ..
			"style[" .. btn_name .. ";content_offset=0]" ..
			"image_button[" .. x .. ",-1.1;" .. w + 0.22 .. ",0.9;;" ..
				btn_name .. ";" .. caption .. ";true;false]"
		x = x + w
	end

	return toadd
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
--	tab_header = tab_header,
	button_header = button_header,
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
