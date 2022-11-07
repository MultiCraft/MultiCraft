--Minetest
--Copyright (C) 2014 sapier
--
--This program is free software; you can redistribute it and/or modify
--it under the terms of the GNU Lesser General Public License as published by
--the Free Software Foundation; either version 3.0 of the License, or
--(at your option) any later version.
--
--this program is distributed in the hope that it will be useful,
--but WITHOUT ANY WARRANTY; without even the implied warranty of
--MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--GNU Lesser General Public License for more details.
--
--You should have received a copy of the GNU Lesser General Public License along
--with this program; if not, write to the Free Software Foundation, Inc.,
--51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

local function dialog_event_handler(self,event)
	if self.user_eventhandler == nil or
		self.user_eventhandler(event) == false then

		--close dialog on esc
		if event == "MenuQuit" then
			self:delete()
			return true
		end
	end
end

local dialog_metatable = {
	eventhandler = dialog_event_handler,
	get_formspec = function(self)
				if not self.hidden then return self.formspec(self.data) end
			end,
	handle_buttons = function(self,fields)
				if not self.hidden then return self.buttonhandler(self,fields) end
			end,
	handle_events  = function(self,event)
				if not self.hidden then return self.eventhandler(self,event) end
			end,
	hide = function(self) self.hidden = true end,
	show = function(self) self.hidden = false end,
	delete = function(self)
			if self.parent ~= nil then
				self.parent:show()
			end
			ui.delete(self)
		end,
	set_parent = function(self,parent) self.parent = parent end
}
dialog_metatable.__index = dialog_metatable

local bg = defaulttexturedir_esc .. "bg_common.png"
function dialog_create(name, get_formspec, buttonhandler, eventhandler, add_background)
	local self = {}

	self.name = name
	self.type = "toplevel"
	self.hidden = true
	self.data = {}

	if add_background then
		function self.formspec(data)
			return ([[
				size[12,5.4]
				bgcolor[#0000]
				background9[0,0;0,0;%s;true;40]
				%s
			]]):format(bg, get_formspec(data))
		end
	else
		self.formspec = get_formspec
	end

	self.buttonhandler = buttonhandler
	self.user_eventhandler  = eventhandler

	setmetatable(self,dialog_metatable)

	ui.add(self)
	return self
end

function messagebox(name, message)
	return dialog_create(name,
			function()
				return ([[
					set_focus[ok;true]
					textarea[1,1;10,4;;;%s]
					%s
					button[5,4.5;2,0.8;ok;%s]
				]]):format(message, btn_style("ok"), fgettext("OK"))
			end,
			function(this, fields)
				if fields.ok then
					this:delete()
					return true
				end
			end,
			nil, true)
end
