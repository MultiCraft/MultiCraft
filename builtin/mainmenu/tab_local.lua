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

local esc = core.formspec_escape
local small_screen = (PLATFORM == "Android" or PLATFORM == "iOS") and not core.settings:get_bool("device_is_tablet")

local function current_game()
	local last_game_id = core.settings:get("menu_last_game")
	local game = pkgmgr.find_by_gameid(last_game_id)

	return game
end

local function singleplayer_refresh_gamebar()
	local old_bar = ui.find_by_name("game_button_bar")

	if old_bar ~= nil then
		old_bar:delete()
	end

	local function game_buttonbar_button_handler(fields)
		--[[if fields.game_open_cdb then
			local maintab = ui.find_by_name("maintab")
			local dlg = create_store_dlg("game")
			dlg:set_parent(maintab)
			maintab:hide()
			dlg:show()
			return true
		end]]

		for key, value in pairs(fields) do
			for j=1, #pkgmgr.games do
				if ("game_btnbar_" .. pkgmgr.games[j].id == key) then
					mm_texture.update("singleplayer", pkgmgr.games[j])
				--	core.set_topleft_text(pkgmgr.games[j].name)
					core.settings:set("menu_last_game",pkgmgr.games[j].id)
					menudata.worldlist:set_filtercriteria(pkgmgr.games[j].id)
					--[[local index = filterlist.get_current_index(menudata.worldlist,
						tonumber(core.settings:get("mainmenu_last_selected_world")))
					if not index or index < 1 then
						local selected = get_index()
						if selected ~= nil and selected < #menudata.worldlist:get_list() then
							index = selected
						else
							index = #menudata.worldlist:get_list()
						end
					end]]

					return true
				end
			end
		end
	end

	local btnbar = buttonbar_create("game_button_bar",
		game_buttonbar_button_handler,
		{x=-0.15, y=0.18}, "vertical", {x=1, y=6.14})

	for i=1, #pkgmgr.games do
		if pkgmgr.games[i].id ~= "default" then
			local btn_name = "game_btnbar_" .. pkgmgr.games[i].id

			local image = nil
			local text = nil
			local tooltip = esc(pkgmgr.games[i].name)

			if pkgmgr.games[i].menuicon_path ~= nil and
				pkgmgr.games[i].menuicon_path ~= "" then
				image = esc(pkgmgr.games[i].menuicon_path)
			else
				local part1 = pkgmgr.games[i].id:sub(1,5)
				local part2 = pkgmgr.games[i].id:sub(6,10)
				local part3 = pkgmgr.games[i].id:sub(11)

				text = part1 .. "\n" .. part2
				if part3 ~= nil and
					part3 ~= "" then
					text = text .. "\n" .. part3
				end
			end

			btnbar:add_button(btn_name, text, image, tooltip)
		end
	end

	btnbar:add_button("game_open_cdb", "", "", fgettext("Install games from ContentDB"), true)
end

local function get_index()
	local index = filterlist.get_current_index(menudata.worldlist,
				tonumber(core.settings:get("mainmenu_last_selected_world")))

	-- Default index
	if index == 0 then index = 1 end

	return index
end

local function get_formspec(_, _, tab_data)
	local index = get_index()

	local space = small_screen and ("\n"):rep(3) or ("\n"):rep(5)
	local retval =
			"style[world_delete,world_create,world_configure;font_size=*" ..
				(small_screen and 1.2 or 1.5) .. "]" ..
			btn_style("world_delete", "left") ..
			"image_button[-0.12,4.85;3.48,0.9;;world_delete;" .. fgettext("Delete") .. ";true;false]" ..
			"image[0.1,5;0.5,0.5;" .. defaulttexturedir_esc .. "gui" .. DIR_DELIM_esc .. "world_delete.png]" ..

			btn_style("world_create", "right") ..
			"image_button[3.14,4.85;3.48,0.9;;world_create;".. fgettext("Create") .. ";true;false]" ..
			"image[3.35,5;0.5,0.5;" .. defaulttexturedir_esc .. "gui" .. DIR_DELIM_esc .. "world_create.png]"

	local world = menudata.worldlist:get_list()[index]
	local game = world and pkgmgr.find_by_gameid(world.gameid)
	if game and game.moddable then
		retval = retval ..
			btn_style("world_configure") ..
			"image_button[8.1,4.85;4,0.9;;world_configure;" .. fgettext("Select Mods") .. ";true;false]" ..
			"image[8.3,5.02;0.5,0.5;" .. defaulttexturedir_esc .. "gui" .. DIR_DELIM_esc .. "world_settings.png]"
	end

	local c_label = utf8.gsub(fgettext("Creative mode"), "(%w)(%w+)",
		function(a, b) return utf8.upper(a) .. b end)
	retval = retval ..
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

			"real_coordinates[true]" ..
			menu_render_worldlist2(index) ..
			"real_coordinates[false]"

	if tab_data.hidden then
		retval = retval ..
			btn_style("switch_local_default") ..
			"style[switch_local_default;fgimg=" .. defaulttexturedir_esc .. "switch_local_default.png;fgimg_hovered=" ..
				defaulttexturedir_esc .. "switch_local_default_hover.png;padding=" .. (is_high_dpi() and -42 or -30) .. "]" ..
			"image_button[10.6,-0.1;1.5,1.5;;switch_local_default;;true;false]"
	end

	local enable_server = core.settings:get_bool("enable_server")
	if enable_server then
		retval = retval ..
			"checkbox[6.6,5;cb_server;".. fgettext("Create Server") ..";" ..
				dump(enable_server) .. "]"

		if core.settings:get_bool("server_announce") then
			retval = retval ..
				"checkbox[9.3,5;cb_server_announce;" .. fgettext("Announce Server") .. ";true]"
		end

		retval = retval ..
			-- Name / Password
			"field[6.9,4.6;2.8,0.5;te_playername;" .. fgettext("Name") .. ":;" ..
				esc(core.settings:get("name")) .. "]" ..
			"pwdfield[9.6,4.6;2.8,0.5;te_passwd;" .. fgettext("Password") .. ":]"
	end

	return retval
end

local function main_button_handler(this, fields, name, tab_data)
	assert(name == "local")

	if menu_handle_key_up_down(fields,"sp_worlds","mainmenu_last_selected_world") then
		return true
	end

	if fields["cb_creative_mode"] then
		local creative_mode = core.settings:get_bool("creative_mode", false)
		core.settings:set_bool("creative_mode", not creative_mode)
		core.settings:set_bool("enable_damage", creative_mode)

		local selected = get_index()
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

	if fields["play"] ~= nil or fields["key_enter"] then
		local selected = get_index()
		gamedata.selected_world = menudata.worldlist:get_raw_index(selected)
		core.settings:set("maintab_LAST", "local")
		core.settings:set("mainmenu_last_selected_world", gamedata.selected_world)

		if selected == nil or gamedata.selected_world == 0 then
			gamedata.errormessage =
					fgettext("No world created or selected!")
			return true
		end

		-- Update last game
		local world = menudata.worldlist:get_raw_element(gamedata.selected_world)
		if world then
			local game = pkgmgr.find_by_gameid(world.gameid)
			core.settings:set("menu_last_game", (game and game.id or ""))

			-- Disable all mods on games that aren't moddable
			if game and not game.moddable then
				local conf = Settings(world.path .. DIR_DELIM .. "world.mt")
				local needs_update = false
				for _, key in ipairs(conf:get_names()) do
					if key:sub(1, 9) == "load_mod_" and conf:get_bool(key) then
						conf:set_bool(key, false)
						needs_update = true
					end
				end

				if needs_update then
					conf:write()
				end
			end
		end

		if core.settings:get_bool("enable_server") then
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
		else
			gamedata.singleplayer = true
			core.settings:set_bool("auto_connect", true)
			core.settings:set("connect_time", os.time())
			core.start()
		end

		core.start()
		return true
	end

	if fields["world_create"] ~= nil then
		local dlg
		if #pkgmgr.games > 1 or (pkgmgr.games[1] and pkgmgr.games[1].id ~= "default") then
			mm_texture.update("singleplayer", current_game())
			dlg = create_create_world_dlg(true)
		else
			dlg = create_store_dlg("game")
		end

		dlg:set_parent(this)
		this:hide()
		dlg:show()
		return true
	end

	if fields["world_delete"] ~= nil then
		local selected = get_index()
		if selected ~= nil and
			selected <= menudata.worldlist:size() then
			local world = menudata.worldlist:get_list()[selected]
			if world ~= nil and
				world.name ~= nil and
				world.name ~= "" then
				local index = menudata.worldlist:get_raw_index(selected)
				local game = pkgmgr.find_by_gameid(world.gameid)
				local delete_world_dlg = create_delete_world_dlg(world.name, index, game.name)
				delete_world_dlg:set_parent(this)
				this:hide()
				delete_world_dlg:show()
				mm_texture.update("singleplayer",current_game())
			end
		end

		return true
	end

	if fields["world_configure"] ~= nil then
		local selected = get_index()
		if selected ~= nil then
			local configdialog =
				create_configure_world_dlg(
						menudata.worldlist:get_raw_index(selected))

			if (configdialog ~= nil) then
				configdialog:set_parent(this)
				this:hide()
				configdialog:show()
				mm_texture.update("singleplayer",current_game())
			end
		end

		return true
	end

	if fields["switch_local_default"] then
		core.settings:set("menu_last_game", "default")
		this:set_tab("local_default")

		return true
	end

	if fields["game_open_cdb"] then
		if #pkgmgr.games > 1 or (pkgmgr.games[1] and pkgmgr.games[1].id ~= "default") then
			this:set_tab("content")
		else
			local dlg = create_store_dlg("game")
			dlg:set_parent(this)
			this:hide()
			dlg:show()
		end

		return true
	end
end

local function on_change(type, old_tab, new_tab)
	if (type == "ENTER") then
		local gameid = core.settings:get("menu_last_game")
		if not gameid or gameid == "" or gameid == "default" then
			local game_set
			for _, game in ipairs(pkgmgr.games) do
				local name = game.id
				if name and name ~= "default" then
					core.settings:set("menu_last_game", name)
					game_set = true
					break
				end
			end
			if not game_set then
				menudata.worldlist:set_filtercriteria("empty")
			end
		end

		local game = current_game()

		if game and game.id ~= "default" then
			menudata.worldlist:set_filtercriteria(game.id)
			mm_texture.update("singleplayer",game)
		else
			mm_texture.reset()
		end

		core.set_topleft_text("Powered by Minetest Engine")

		singleplayer_refresh_gamebar()
		ui.find_by_name("game_button_bar"):show()

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
	else
		menudata.worldlist:set_filtercriteria(nil)
		local gamebar = ui.find_by_name("game_button_bar")
		if gamebar then
			gamebar:hide()
		end
		core.set_topleft_text("")
		mm_texture.update(new_tab,nil)
	end
end

--------------------------------------------------------------------------------
return {
	name = "local",
	caption = fgettext("Singleplayer"),
	cbf_formspec = get_formspec,
	cbf_button_handler = main_button_handler,
	on_change = on_change
}
