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

local function rename_modpack_formspec(dialogdata)
	local retval =
		btn_style("dlg_rename_modpack_confirm", "green") ..
		"button[3,4.8;3,0.5;dlg_rename_modpack_confirm;"..
				fgettext("Accept") .. "]" ..
		btn_style("dlg_rename_modpack_cancel") ..
		"button[6,4.8;3,0.5;dlg_rename_modpack_cancel;"..
				fgettext("Cancel") .. "]"

	local input_y = 2
	if dialogdata.mod.is_name_explicit then
		retval = retval .. "textarea[1,0.2;10,2;;;" ..
				fgettext("This modpack has an explicit name given in its modpack.conf " ..
						"which will override any renaming here.") .. "]"
		input_y = 2.5
	end
	retval = retval ..
		"formspec_version[3]" ..
		"image[2.5," .. input_y - 0.47 .. ";8.55,0.84;" ..
		defaulttexturedir_esc .. "field_bg.png;32]" ..
		"style[te_modpack_name;border=false;bgcolor=transparent]" ..
		"field[2.85," .. input_y .. ";6.9,0.5;te_modpack_name;" ..
		fgettext("Rename Modpack:") .. ";" .. dialogdata.mod.dir_name .. "]"

	return retval
end

--------------------------------------------------------------------------------
local function rename_modpack_buttonhandler(this, fields)
	if fields["dlg_rename_modpack_confirm"] ~= nil then
		local oldpath = this.data.mod.path
		local targetpath = this.data.mod.parent_dir .. DIR_DELIM .. fields["te_modpack_name"]
		os.rename(oldpath, targetpath)
		pkgmgr.refresh_globals()
		pkgmgr.selected_mod = pkgmgr.global_mods:get_current_index(
			pkgmgr.global_mods:raw_index_by_uid(fields["te_modpack_name"]))

		this:delete()
		return true
	end

	if fields["dlg_rename_modpack_cancel"] then
		this:delete()
		return true
	end

	return false
end

--------------------------------------------------------------------------------
function create_rename_modpack_dlg(modpack)

	local retval = dialog_create("dlg_delete_mod",
					rename_modpack_formspec,
					rename_modpack_buttonhandler,
					nil, true)
	retval.data.mod = modpack
	return retval
end
