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

local function delete_content_formspec(dialogdata)
	local title = dialogdata.content.title or dialogdata.content.name
	local retval = {
		"real_coordinates[true]",
		"image[6.5,0.8;2.5,2.5;", defaulttexturedir_esc, "attention.png]",

		"style[msg;content_offset=0]",
		"image_button[1,3.5;13.5,0.8;;msg;",
			fgettext("Are you sure you want to delete \"$1\"?", title), ";false;false]",

		btn_style("dlg_delete_content_confirm", "red"),
		"image_button[4.1,5.3;3.5,0.8;;dlg_delete_content_confirm;",
			fgettext("Delete") .. ";true;false]",

		btn_style("dlg_delete_content_cancel"),
		"image_button[7.9,5.3;3.5,0.8;;dlg_delete_content_cancel;",
			fgettext("Cancel"), ";true;false]",
	}

	return table.concat(retval)
end

--------------------------------------------------------------------------------
local function delete_content_buttonhandler(this, fields)
	if fields["dlg_delete_content_confirm"] ~= nil then

		if this.data.content.path ~= nil and
				this.data.content.path ~= "" and
				this.data.content.path ~= core.get_modpath() and
				this.data.content.path ~= core.get_gamepath() and
				this.data.content.path ~= core.get_texturepath() then
			if not core.delete_dir(this.data.content.path) then
				gamedata.errormessage = fgettext("pkgmgr: failed to delete \"$1\"", this.data.content.path)
			end

			if this.data.content.type == "game" then
				pkgmgr.update_gamelist()
			else
				pkgmgr.refresh_globals()
			end
		else
			gamedata.errormessage = fgettext("pkgmgr: invalid path \"$1\"", this.data.content.path)
		end
		this:delete()
		return true
	end

	if fields["dlg_delete_content_cancel"] then
		this:delete()
		return true
	end

	return false
end

--------------------------------------------------------------------------------
function create_delete_content_dlg(content)
	assert(content.name)

	if content.type == "game" then
		for _, world in ipairs(menudata.worldlist:get_raw_list()) do
			if world.gameid == content.id then
				return messagebox("dlg_delete_content",
					fgettext("You cannot delete this game!") .. "\n" ..
					fgettext("You have worlds that use it."))
			end
		end
	end

	local retval = dialog_create("dlg_delete_content",
					delete_content_formspec,
					delete_content_buttonhandler,
					nil, true)
	retval.data.content = content
	return retval
end
