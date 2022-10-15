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


local b = core.formspec_escape(defaulttexturedir .. "blank.png")
local button_gray = core.formspec_escape(defaulttexturedir .. "blank.png")


local function delete_world_formspec(dialogdata)
	local game_name = dialogdata.delete_game
	local delete_name = dialogdata.delete_name
	if game_name then
		delete_name = delete_name .. " (" .. game_name .. ")"
	end

	local formspec = {
		"real_coordinates[true]",
		"image[6.5,0.72;2.5,2.5;", core.formspec_escape(defaulttexturedir ..
			"attention.png"), "]",

		"style[msg,wait;content_offset=0]",
		"image_button[1,3.1;13.5,0.8;", b, ";msg;",
			fgettext("Delete World \"$1\"?", delete_name), ";false;false]",
		"image_button[1,3.9;13.5,1.4;", b, ";msg;",
			fgettext("This action cannot be undone!"), "\n",
			fgettext("You will permanently lose your World."), ";false;false]",

		btn_style("world_delete_cancel"),
		"image_button[7.9,5.8;3.5,0.8;;world_delete_cancel;",
			fgettext("Cancel"), ";true;false]",
	}

	if dialogdata.cooldown > 0 then
		formspec[#formspec + 1] = btn_style("wait", "gray")
		formspec[#formspec + 1] = "image_button[4.1,5.8;3.5,0.8;;wait;" ..
			fgettext("Wait $1 sec.", dialogdata.cooldown) .. ";true;false]"
	else
		formspec[#formspec + 1] = btn_style("world_delete_confirm", "red")
		formspec[#formspec + 1] = "image_button[4.1,5.8;3.5,0.8;;world_delete_confirm;" ..
			fgettext("Delete") .. ";true;false]"
	end

	return table.concat(formspec)
end

local function delete_world_buttonhandler(this, fields)
	if fields["world_delete_confirm"] then
		if this.data.delete_index > 0 and
				this.data.delete_index <= #menudata.worldlist:get_raw_list() then
			core.delete_world(this.data.delete_index)
			menudata.worldlist:refresh()
		end
		this:delete()
		return true
	end

	if fields["world_delete_cancel"] then
		this:delete()
		return true
	end

	return false
end

-- core.handle_async requires a function defined in Lua
local function sleep_ms(delay)
	return core.sleep_ms(delay)
end

local function start_timer(msgbox)
	core.handle_async(sleep_ms, 1000, function()
		-- If this.hidden isn't true then the dialog must have been closed and
		-- the countdown can be stopped.
		if msgbox.parent and msgbox.parent.hidden then
			msgbox.data.cooldown = msgbox.data.cooldown - 1
			ui.update()
			start_timer(msgbox)
		end
	end)
end


function create_delete_world_dlg(name_to_del, index_to_del, game_to_del)
	assert(name_to_del ~= nil and type(name_to_del) == "string" and name_to_del ~= "")
	assert(index_to_del ~= nil and type(index_to_del) == "number")

	local retval = dialog_create("delete_world",
					delete_world_formspec,
					delete_world_buttonhandler,
					nil, true)
	retval.data.delete_name  = name_to_del
	retval.data.delete_game  = game_to_del
	retval.data.delete_index = index_to_del
	retval.data.cooldown     = 5
	start_timer(retval)

	return retval
end
