--Minetest
--Copyright (C) 2014 sapier
--Copyright (C) 2018 rubenwardy <rw@rubenwardy.com>
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

local packages_raw
local packages

--------------------------------------------------------------------------------
local function get_formspec(tabview, name, tabdata)
	if pkgmgr.global_mods == nil then
		pkgmgr.refresh_globals()
	end
	if pkgmgr.games == nil then
		pkgmgr.update_gamelist()
	end

	if packages == nil then
		packages_raw = {}
		local i = 0
		for _, game in ipairs(pkgmgr.games) do
			if game.id ~= "default" then
				i = i + 1
				packages_raw[i] = game
			end
		end
		table.insert_all(packages_raw, pkgmgr.get_texture_packs())
		table.insert_all(packages_raw, pkgmgr.global_mods:get_list())

		local function get_data()
			return packages_raw
		end

		local function is_equal(element, uid) --uid match
			return (element.type == "game" and element.id == uid) or
					element.name == uid
		end

		packages = filterlist.create(get_data, pkgmgr.compare_package,
				is_equal, nil, {})
	end

	if tabdata.selected_pkg == nil then
		tabdata.selected_pkg = 1
	end


	local retval =
		"label[-0.05,-0.25;".. fgettext("Installed Packages:") .. "]" ..
		"background9[0,0.23;5.3,4.46;" .. defaulttexturedir_esc .. "worldlist_bg.png;false;40]" ..
		"tablecolumns[color;tree;text]" ..
		"tableoptions[background=#0000;border=false]" ..
		scrollbar_style("pkglist") ..
		"table[0,0.25;5.1,4.3;pkglist;" ..
		pkgmgr.render_packagelist(packages) ..
		";" .. tabdata.selected_pkg .. "]" ..
		btn_style("btn_contentdb") ..
		"image_button[-0.11,4.8;5.5,0.9;;btn_contentdb;" .. fgettext("Browse online content") .. ";true;false]"


	local selected_pkg
	if filterlist.size(packages) >= tabdata.selected_pkg then
		selected_pkg = packages:get_list()[tabdata.selected_pkg]
	end

	if selected_pkg ~= nil then
		--check for screenshot beeing available
		local screenshotfilename = selected_pkg.path .. DIR_DELIM .. "screenshot.png"
		local screenshotfile, error = io.open(screenshotfilename, "r")

		local modscreenshot
		if error == nil then
			screenshotfile:close()
			modscreenshot = screenshotfilename
		end

		if modscreenshot == nil then
				modscreenshot = defaulttexturedir_esc .. "no_screenshot.png"
		end

		local info = core.get_content_info(selected_pkg.path)
		local desc = fgettext("No package description available")
		if info.description and info.description:trim() ~= "" then
			desc = info.description
		end

		retval = retval ..
				"image[5.5,0;3,2;" .. core.formspec_escape(modscreenshot) .. "]" ..
				"image[5.5,0;3,2;" .. defaulttexturedir_esc .. "gui" .. DIR_DELIM_esc .. "cdb_img_corners.png;15]" ..
				"label[8.25,0.6;" .. core.formspec_escape(selected_pkg.name) .. "]" ..
				"background9[5.6,2.3;6.2,2.4;" .. defaulttexturedir_esc .. "desc_bg.png;false;32]"

		if selected_pkg.type == "mod" then
			if selected_pkg.is_modpack then
				retval = retval ..
					btn_style("btn_mod_mgr_rename_modpack") ..
					"image_button[8.65,4.8;3.25,0.9;;btn_mod_mgr_rename_modpack;" ..
					fgettext("Rename") .. ";true;false]"
			else
				--show dependencies
				desc = desc .. "\n\n"
				local toadd_hard = table.concat(info.depends or {}, "\n")
				local toadd_soft = table.concat(info.optional_depends or {}, "\n")
				if toadd_hard == "" and toadd_soft == "" then
					desc = desc .. fgettext("No dependencies.")
				else
					if toadd_hard ~= "" then
						desc = desc ..fgettext("Dependencies:") ..
							"\n" .. toadd_hard
					end
					if toadd_soft ~= "" then
						if toadd_hard ~= "" then
							desc = desc .. "\n\n"
						end
						desc = desc .. fgettext("Optional dependencies:") ..
							"\n" .. toadd_soft
					end
				end
			end

		else
			if selected_pkg.type == "txp" then
				if selected_pkg.enabled then
					retval = retval ..
						btn_style("btn_mod_mgr_disable_txp") ..
						"image_button[8.65,4.8;3.25,0.9;;btn_mod_mgr_disable_txp;" ..
						fgettext("Disable Texture Pack") .. ";true;false]"
				else
					retval = retval ..
						btn_style("btn_mod_mgr_use_txp", "green") ..
						"image_button[8.65,4.8;3.25,0.9;;btn_mod_mgr_use_txp;" ..
						fgettext("Use Texture Pack") .. ";true;false]"
				end
			end
		end

		retval = retval .. "textarea[5.85,2.2;6.35,2.9;;" ..
			fgettext("Information:") .. ";" .. desc .. "]"

		if core.may_modify_path(selected_pkg.path) then
			retval = retval ..
				btn_style("btn_mod_mgr_delete_mod", "red") ..
				"image_button[5.5,4.8;3.25,0.9;;btn_mod_mgr_delete_mod;" ..
				fgettext("Uninstall Package") .. ";true;false]"
		end
	end
	return retval
end

--------------------------------------------------------------------------------
local function handle_buttons(tabview, fields, tabname, tabdata)
	if fields["pkglist"] ~= nil then
		local event = core.explode_table_event(fields["pkglist"])
		tabdata.selected_pkg = event.row
		return true
	end

	if fields["btn_contentdb"] ~= nil then
		local dlg = create_store_dlg()
		dlg:set_parent(tabview)
		tabview:hide()
		dlg:show()
		packages = nil
		return true
	end

	if fields["btn_mod_mgr_rename_modpack"] ~= nil then
		local mod = packages:get_list()[tabdata.selected_pkg]
		local dlg_renamemp = create_rename_modpack_dlg(mod)
		dlg_renamemp:set_parent(tabview)
		tabview:hide()
		dlg_renamemp:show()
		packages = nil
		return true
	end

	if fields["btn_mod_mgr_delete_mod"] ~= nil then
		local mod = packages:get_list()[tabdata.selected_pkg]
		local dlg_delmod = create_delete_content_dlg(mod)
		dlg_delmod:set_parent(tabview)
		tabview:hide()
		dlg_delmod:show()
		packages = nil
		return true
	end

	if fields.btn_mod_mgr_use_txp then
		local txp = packages:get_list()[tabdata.selected_pkg]
		core.settings:set("texture_path", txp.path)
		packages = nil
		return true
	end


	if fields.btn_mod_mgr_disable_txp then
		core.settings:set("texture_path", "")
		packages = nil
		return true
	end

	return false
end

--------------------------------------------------------------------------------
return {
	name = "content",
	caption = "", -- fgettext("Content"),
	cbf_formspec = get_formspec,
	cbf_button_handler = handle_buttons,
	on_change = pkgmgr.update_gamelist
}
