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

local small_screen = (PLATFORM == "Android" or PLATFORM == "iOS") and not core.settings:get_bool("device_is_tablet")

local function menu_render_worldlist()
	local retval = {}

	local creative = core.settings:get_bool("creative_mode", false)
	local damage = core.settings:get_bool("enable_damage", true)

	for _, world in ipairs(menudata.worldlist:get_list()) do
		if world.creative_mode == nil or world.enable_damage == nil then
			-- There's a built-in menu_worldmt function that can read from
			-- world.mt but it would read from the file once for each setting
			-- read
			local world_conf = Settings(world.path .. DIR_DELIM .. "world.mt")
			world.creative_mode = world_conf:get_bool("creative_mode", creative)
			world.enable_damage = world_conf:get_bool("enable_damage", damage)
		end

		retval[#retval + 1] = world.creative_mode and "5" or "4"
		retval[#retval + 1] = core.formspec_escape(world.name)
	end

	return table.concat(retval, ",")
end

local function get_formspec(this)
	local index = filterlist.get_current_index(menudata.worldlist,
				tonumber(core.settings:get("mainmenu_last_selected_world")))
	if not index or index < 1 then
		local selected = core.get_table_index("sp_worlds")
		if selected ~= nil and selected < #menudata.worldlist:get_list() then
			index = selected
		else
			index = 1
		end
	end

	local space = small_screen and ("\n"):rep(3) or ("\n"):rep(5)
	local c_label = utf8.gsub(fgettext("Creative mode"), "(%w)(%w+)",
		function(a, b) return utf8.upper(a) .. b end)
	local retval =
			"style[world_delete,world_create;font_size=*" ..
				(small_screen and 1.2 or 1.5) .. "]" ..
			btn_style("world_delete", "left") ..
			"image_button[-0.12,4.85;3.48,0.9;;world_delete;" .. fgettext("Delete") .. ";true;false]" ..
			"image[0.1,5;0.5,0.5;" .. defaulttexturedir_esc .. "gui" .. DIR_DELIM_esc .. "world_delete.png]" ..

			btn_style("world_create", "right") ..
			"image_button[3.14,4.85;3.48,0.9;;world_create;".. fgettext("Create") .. ";true;false]" ..
			"image[3.35,5;0.5,0.5;" .. defaulttexturedir_esc .. "gui" .. DIR_DELIM_esc .. "world_create.png]" ..

			btn_style("play") ..
			"style[play;font_size=*" .. (small_screen and 2.25 or 3) .. "]" ..
			"image_button[6.72,1.43;4.96,1.41;;play;" .. space .. " " ..
				fgettext("Play") .. space .. ";true;false]" ..
			"image[7,1.63;1,1;" .. defaulttexturedir_esc .. "btn_play_icon.png]" ..

			"style[cb_creative_mode;content_offset=0;font_size=*" .. (small_screen and 1.2 or 1.5) ..
				";textcolor=#53659C]" ..
			"image_button[6.86,3.09;4.65,0.83;" .. defaulttexturedir_esc .. "creative_bg.png;cb_creative_mode;;true;false]" ..
			"image[6.96,3.19;0.55,0.55;" .. defaulttexturedir_esc .. "gui" .. DIR_DELIM_esc ..
				(core.settings:get_bool("creative_mode") and "checkbox_checked" or "checkbox") .. ".png]" ..
			"image_button[7.31,3.09;4.2,0.83;;cb_creative_mode;" .. c_label .. ";true;false]" ..

			"background9[0,0;6.5,4.8;" .. defaulttexturedir_esc .. "worldlist_bg.png;false;40]" ..
			"tableoptions[background=#0000;border=false]" ..
			"tablecolumns[" .. image_column(fgettext("Creative mode")) .. ";text]" ..
			scrollbar_style("sp_worlds") ..
			"table[0,0;6.28,4.64;sp_worlds;" .. menu_render_worldlist() .. ";" .. index .. "]"

	local enable_server = core.settings:get_bool("enable_server")
	if enable_server then
		retval = retval ..
			"checkbox[6.6,5;cb_server;".. fgettext("Create Server") ..";" ..
				dump(enable_server) .. "]"
	end

	if enable_server then
		if core.settings:get_bool("server_announce") then
			retval = retval ..
				"checkbox[9.3,5;cb_server_announce;" .. fgettext("Announce Server") .. ";true]"
		end

		retval = retval ..
			-- Name / Password
			"field[6.9,4.6;2.8,0.5;te_playername;" .. fgettext("Name") .. ":;" ..
				core.formspec_escape(core.settings:get("name")) .. "]" ..
			"pwdfield[9.6,4.6;2.8,0.5;te_passwd;" .. fgettext("Password") .. ":]"
	end

	return retval
end

local function main_button_handler(this, fields, name)
	assert(name == "local_default")

	local world_doubleclick = false

	if fields["sp_worlds"] ~= nil then
		local event = core.explode_table_event(fields["sp_worlds"])
		local selected = core.get_table_index("sp_worlds")

		if event.type == "DCL" then
			world_doubleclick = true
		end

		if event.type == "CHG" and selected ~= nil then
			local world = menudata.worldlist:get_list()[selected]
			if world and world.creative_mode ~= nil and
					world.enable_damage ~= nil then
				core.settings:set_bool("creative_mode", world.creative_mode)
				core.settings:set_bool("enable_damage", world.enable_damage)
			end

			core.settings:set("mainmenu_last_selected_world",
				menudata.worldlist:get_raw_index(selected))
			return true
		end
	end

	if menu_handle_key_up_down(fields,"sp_worlds","mainmenu_last_selected_world") then
		return true
	end

	if fields["cb_creative_mode"] then
		local creative_mode = core.settings:get_bool("creative_mode", false)
		core.settings:set_bool("creative_mode", not creative_mode)
		core.settings:set_bool("enable_damage", creative_mode)

		local selected = core.get_table_index("sp_worlds")
		local world = menudata.worldlist:get_list()[selected]
		if world then
			-- Update the cached values
			world.creative_mode = not creative_mode
			world.enable_damage = creative_mode

			-- Update the settings in world.mt
			local world_conf = Settings(world.path .. DIR_DELIM .. "world.mt")
			world_conf:set_bool("creative_mode", not creative_mode)
			world_conf:set_bool("enable_damage", creative_mode)
			world_conf:write()
		end

		return true
	end

	if fields["cb_server"] then
		core.settings:set("enable_server", fields["cb_server"])

		return true
	end

	if fields["cb_server_announce"] then
		core.settings:set("server_announce", fields["cb_server_announce"])
		local selected = core.get_table_index("srv_worlds")
		menu_worldmt(selected, "server_announce", fields["cb_server_announce"])

		return true
	end

	if fields["play"] ~= nil or world_doubleclick or fields["key_enter"] then
		local selected = core.get_table_index("sp_worlds")
		gamedata.selected_world = menudata.worldlist:get_raw_index(selected)
		core.settings:set("maintab_LAST", "local_default")
		core.settings:set("mainmenu_last_selected_world", gamedata.selected_world)

		if core.settings:get_bool("enable_server") then
			if selected ~= nil and gamedata.selected_world ~= 0 then
				gamedata.playername = fields["te_playername"]
				gamedata.password   = fields["te_passwd"]
				gamedata.port       = fields["te_serverport"]
				gamedata.address    = ""

				core.settings:set_bool("auto_connect", false)
				if fields["port"] ~= nil then
					core.settings:set("port",fields["port"])
				end
				if fields["te_serveraddr"] ~= nil then
					core.settings:set("bind_address",fields["te_serveraddr"])
				end

				--update last game
				local world = menudata.worldlist:get_raw_element(gamedata.selected_world)
				if world then
					core.settings:set("menu_last_game", "default")
				end

				core.start()
			else
				gamedata.errormessage =
					fgettext("No world created or selected!")
			end
		else
			if selected ~= nil and gamedata.selected_world ~= 0 then
				gamedata.singleplayer = true
				core.settings:set_bool("auto_connect", true)
				core.settings:set("connect_time", os.time())
				core.start()
			else
				gamedata.errormessage =
					fgettext("No world created or selected!")
			end
			return true
		end
	end

	if fields["world_create"] ~= nil then
		core.settings:set("menu_last_game", "default")
		local create_world_dlg = create_create_world_default_dlg(true)
		create_world_dlg:set_parent(this)
		this:hide()
		create_world_dlg:show()
		return true
	end

	if fields["world_delete"] ~= nil then
		local selected = core.get_table_index("sp_worlds")
		if selected ~= nil and
			selected <= menudata.worldlist:size() then
			local world = menudata.worldlist:get_list()[selected]
			if world ~= nil and
				world.name ~= nil and
				world.name ~= "" then
				local index = menudata.worldlist:get_raw_index(selected)
				local delete_world_dlg = create_delete_world_dlg(world.name,index)
				delete_world_dlg:set_parent(this)
				this:hide()
				delete_world_dlg:show()
			end
		end

		return true
	end

	if fields["switch_local"] then
		this:set_tab("local")
		return true
	end

--[[if fields["world_configure"] ~= nil then
		local selected = core.get_table_index("sp_worlds")
		if selected ~= nil then
			local configdialog =
				create_configure_world_dlg(
						menudata.worldlist:get_raw_index(selected))

			if (configdialog ~= nil) then
				configdialog:set_parent(this)
				this:hide()
				configdialog:show()
			end
		end

		return true
	end]]
end

local function on_change(type, _, _, this)
	if (type == "ENTER") then
		local gameid = core.settings:get("menu_last_game")

		local game = pkgmgr.find_by_gameid(gameid)
		if game then
			if gameid ~= "default"  then
				this:set_tab("local")
			else
				mm_texture.update("singleplayer",game)
				menudata.worldlist:set_filtercriteria("default")

				-- Update creative_mode and enable_damage settings
				local index = filterlist.get_current_index(menudata.worldlist,
						tonumber(core.settings:get("mainmenu_last_selected_world")))
				local world = menudata.worldlist:get_list()[index] or menudata.worldlist:get_list()[1]
				if world then
					if world.creative_mode == nil or world.enable_damage == nil then
						local world_conf = Settings(world.path .. DIR_DELIM .. "world.mt")
						world.creative_mode = world_conf:get_bool("creative_mode")
						world.enable_damage = world_conf:get_bool("enable_damage")
					end

					core.settings:set_bool("creative_mode", world.creative_mode)
					core.settings:set_bool("enable_damage", world.enable_damage)
				end
			end
		end
	end
end

--------------------------------------------------------------------------------
return {
	name = "local_default",
	caption = fgettext("Singleplayer"),
	cbf_formspec = get_formspec,
	cbf_button_handler = main_button_handler,
	on_change = on_change
}
