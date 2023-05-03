--Minetest
--Copyright (C) 2013 sapier
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

local enabled_all = false

local function modname_valid(name)
	return not name:find("[^a-z0-9_]")
end

local function init_data(data)
	data.list = filterlist.create(
		pkgmgr.preparemodlist,
		pkgmgr.comparemod,
		function(element, uid)
			if element.name == uid then
				return true
			end
		end,
		function(element, criteria)
			if criteria.hide_game and
					element.is_game_content then
				return false
			end

			if criteria.hide_modpackcontents and
					element.modpack ~= nil then
				return false
			end
			return true
		end,
		{
			worldpath = data.worldspec.path,
			gameid = data.worldspec.gameid
		})

	if data.selected_mod > data.list:size() then
		data.selected_mod = 0
	end

	data.list:set_filtercriteria({
		hide_game = data.hide_gamemods,
		hide_modpackcontents = data.hide_modpackcontents
	})
	data.list:add_sort_mechanism("alphabetic", sort_mod_list)
	data.list:set_sortmode("alphabetic")
end

local function get_formspec(data)
	if not data.list then
		init_data(data)
	end

	local mod = data.list:get_list()[data.selected_mod] or {name = ""}

	local retval =
		"size[11.5,7.5]" ..
		"bgcolor[#0000]" ..
		"background9[0,0;0,0;" .. defaulttexturedir_esc .. "bg_common.png;true;40]" ..
		"background9[0.05,0.05;5.3,6.8;" .. defaulttexturedir_esc .. "desc_bg.png;false;32]" ..
		"label[0.1,0;" .. fgettext("World:") .. " " .. data.worldspec.name .. "]"

	if mod.is_modpack or mod.type == "game" then
		local info = core.formspec_escape(
			core.get_content_info(mod.path).description)
		if info == "" then
			if mod.is_modpack then
				info = fgettext("No modpack description provided.")
			else
				info = fgettext("No game description provided.")
			end
		end
		retval = retval ..
			"textarea[0.4,0.8;5.2,6.8;;" ..
				fgettext("Information:") .. ";" .. info .. "]"
	else
		local hard_deps, soft_deps = pkgmgr.get_dependencies(mod.path)
		local hard_deps_str = table.concat(hard_deps, ",")
		local soft_deps_str = table.concat(soft_deps, ",")

		retval = retval ..
			"label[0.1,0.7;" .. fgettext("Mod:") .. " " .. mod.name .. "]"

		if hard_deps_str == "" then
			if soft_deps_str == "" then
				retval = retval ..
					"label[0.1,1.25;" ..
					fgettext("No (optional) dependencies") .. "]"
			else
				retval = retval ..
					"label[0.1,1.25;" .. fgettext("No hard dependencies") ..
					"]" ..
					"label[0.1,1.75;" .. fgettext("Optional dependencies:") ..
					"]" ..
					"textlist[0.1,2.25;5,4;world_config_optdepends;" ..
					soft_deps_str .. ";0]"
			end
		else
			if soft_deps_str == "" then
				retval = retval ..
					"label[0.1,1.25;" .. fgettext("Dependencies:") .. "]" ..
					"textlist[0.1,1.75;5,4;world_config_depends;" ..
					hard_deps_str .. ";0]" ..
					"label[0.1,6;" .. fgettext("No optional dependencies") .. "]"
			else
				retval = retval ..
					"label[0.1,1.25;" .. fgettext("Dependencies:") .. "]" ..
					"textlist[0.1,1.75;5,2.125;world_config_depends;" ..
					hard_deps_str .. ";0]" ..
					"label[0.1,3.9;" .. fgettext("Optional dependencies:") ..
					"]" ..
					"textlist[0.1,4.375;5,1.8;world_config_optdepends;" ..
					soft_deps_str .. ";0]"
			end
		end
	end

	retval = retval ..
		btn_style("btn_config_world_save", "green") ..
		"button[5.5,7.1;3,0.5;btn_config_world_save;" ..
		fgettext("Save") .. "]" ..
		btn_style("btn_config_world_cancel") ..
		"button[8.5,7.1;3,0.5;btn_config_world_cancel;" ..
		fgettext("Cancel") .. "]" ..
		btn_style("btn_config_world_cdb") ..
		"button[-0.05,7.1;5.5,0.5;btn_config_world_cdb;" ..
		fgettext("Find More Mods") .. "]" ..
		"image[0.09,7.05;0.6,0.6;" .. defaulttexturedir_esc .. "gui" ..
		DIR_DELIM_esc .. "btn_download.png]"

	if mod.name ~= "" and not mod.is_game_content then
		if mod.is_modpack then

			if pkgmgr.is_modpack_entirely_enabled(data, mod.name) then
				retval = retval ..
					btn_style("btn_mod_disable", "yellow") ..
					"button[5.5,0.025;3.3,0.5;btn_mod_disable;" ..
					fgettext("Disable modpack") .. "]"
			else
				retval = retval ..
					btn_style("btn_mod_enable", "green") ..
					"button[5.5,0.025;3.3,0.5;btn_mod_enable;" ..
					fgettext("Enable modpack") .. "]"
			end
		else
			retval = retval ..
				"real_coordinates[true]" ..
				checkbox(7.3, 0.64, mod.enabled and "btn_mod_disable" or "btn_mod_enable",
					fgettext("Enabled"), mod.enabled) ..
				"real_coordinates[false]"
		end
	end
	if enabled_all then
		retval = retval ..
			btn_style("btn_disable_all_mods", "yellow") ..
			"button[8.8,0.025;2.695,0.5;btn_disable_all_mods;" ..
			fgettext("Disable all") .. "]"
	else
		retval = retval ..
			btn_style("btn_enable_all_mods", "green") ..
			"button[8.8,0.025;2.695,0.5;btn_enable_all_mods;" ..
			fgettext("Enable all") .. "]"
	end
	return retval ..
		"background9[5.6,0.85;5.8,6;" .. defaulttexturedir_esc .. "worldlist_bg.png;false;40]" ..
		"tablecolumns[color;tree;text]" ..
		"tableoptions[background=#0000;border=false]" ..
		scrollbar_style("world_config_modlist") ..
		"table[5.58,0.84;5.59,5.82;world_config_modlist;" ..
		pkgmgr.render_packagelist(data.list) .. ";" .. data.selected_mod .."]"
end

local function handle_buttons(this, fields)
	if fields.world_config_modlist then
		local event = core.explode_table_event(fields.world_config_modlist)
		this.data.selected_mod = event.row
		core.settings:set("world_config_selected_mod", event.row)

		if event.type == "DCL" then
			pkgmgr.enable_mod(this)
		end

		return true
	end

	if fields.key_enter then
		pkgmgr.enable_mod(this)
		return true
	end

	if fields.btn_mod_enable ~= nil or
			fields.btn_mod_disable then
		pkgmgr.enable_mod(this, fields.btn_mod_enable ~= nil)
		return true
	end

	if fields.btn_config_world_save then
		local filename = this.data.worldspec.path .. DIR_DELIM .. "world.mt"

		local worldfile = Settings(filename)
		local mods = worldfile:to_table()

		local rawlist = this.data.list:get_raw_list()

		for i = 1, #rawlist do
			local mod = rawlist[i]
			if not mod.is_modpack and
					not mod.is_game_content then
				if modname_valid(mod.name) then
					worldfile:set("load_mod_" .. mod.name,
						mod.enabled and "true" or "false")
				elseif mod.enabled then
					gamedata.errormessage = fgettext_ne("Failed to enable mo" ..
							"d \"$1\" as it contains disallowed characters. " ..
							"Only characters [a-z0-9_] are allowed.",
							mod.name)
				end
				mods["load_mod_" .. mod.name] = nil
			end
		end

		-- Remove mods that are not present anymore
		for key in pairs(mods) do
			if key:sub(1, 9) == "load_mod_" then
				worldfile:remove(key)
			end
		end

		if not worldfile:write() then
			core.log("error", "Failed to write world config file")
		end

		this:delete()
		return true
	end

	if fields.btn_config_world_cancel then
		this:delete()
		return true
	end

	if fields.btn_config_world_cdb then
		this.data.list = nil

		local dlg = create_store_dlg("mod")
		dlg:set_parent(this)
		this:hide()
		dlg:show()
		return true
	end

	if fields.btn_enable_all_mods then
		local list = this.data.list:get_raw_list()

		for i = 1, #list do
			if not list[i].is_game_content
					and not list[i].is_modpack then
				list[i].enabled = true
			end
		end
		enabled_all = true
		return true
	end

	if fields.btn_disable_all_mods then
		local list = this.data.list:get_raw_list()

		for i = 1, #list do
			if not list[i].is_game_content
					and not list[i].is_modpack then
				list[i].enabled = false
			end
		end
		enabled_all = false
		return true
	end

	return false
end

function create_configure_world_dlg(worldidx)
	local dlg = dialog_create("sp_config_world", get_formspec, handle_buttons)

	dlg.data.selected_mod = tonumber(
			core.settings:get("world_config_selected_mod")) or 0

	dlg.data.worldspec = core.get_worlds()[worldidx]
	if not dlg.data.worldspec then
		dlg:delete()
		return
	end

	dlg.data.worldconfig = pkgmgr.get_worldconfig(dlg.data.worldspec.path)

	if not dlg.data.worldconfig or not dlg.data.worldconfig.id or
			dlg.data.worldconfig.id == "" then
		dlg:delete()
		return
	end

	return dlg
end
