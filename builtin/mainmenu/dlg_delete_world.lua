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


local function delete_world_formspec(dialogdata)
	local game_name = dialogdata.delete_game
	local delete_name = dialogdata.delete_name
	if game_name then
		delete_name = delete_name .. " (" .. game_name .. ")"
	end

	local retval =
		"style[ignore;content_offset=0]" ..
		"image_button[4.75,0.5;2.5,2.5;" .. core.formspec_escape(defaulttexturedir ..
			"attention.png") .. ";;;true;false;]" ..
		"style[ignore;content_offset=0]" ..
		"image_button[2,1.75;8,3;" .. core.formspec_escape(defaulttexturedir ..
			"blank.png") .. ";ignore;" .. fgettext("Delete World \"$1\"?", delete_name) ..
			";true;false;]" ..
		btn_style("world_delete_confirm", "red") ..
		"button[3,4.8;3,0.5;world_delete_confirm;" .. fgettext("Delete") .. "]" ..
		btn_style("world_delete_cancel") ..
		"button[6,4.8;3,0.5;world_delete_cancel;" .. fgettext("Cancel") .. "]"
	return retval
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

	return retval
end
