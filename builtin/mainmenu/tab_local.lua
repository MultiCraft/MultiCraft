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

local lang = core.settings:get("language")
if not lang or lang == "" then lang = os.getenv("LANG") end

local esc = core.formspec_escape
local defaulttexturedir = esc(defaulttexturedir)
local small_screen = (PLATFORM == "Android" or PLATFORM == "iOS") and not core.settings:get_bool("device_is_tablet")

local default_worlds = {
	{name = "World 1", mg_name = "v7p", seed = "15823438331521897617"},
	{name = "World 2", mg_name = "v7p", seed = "1841722166046826822"},
	{name = "World 3", mg_name = "v7p", seed = "CC"},
	{name = "World 4", mg_name = "valleys", seed = "8572"},
	{name = "World 5 Flat", mg_name = "flat", seed = "2"}
}

local function current_game()
	local last_game_id = core.settings:get("menu_last_game")
	local game = pkgmgr.find_by_gameid(last_game_id)

	return game
end

local function create_default_worlds()
	if #menudata.worldlist:get_list() > 0 then return end

	local _, gameindex = pkgmgr.find_by_gameid("default")
	if not gameindex or gameindex == 0 then return end

	-- Preserve the old map seed and mapgen values
	local old_map_seed = core.settings:get("fixed_map_seed")
	local old_mapgen = core.settings:get("mg_name")

	-- Create the worlds
	for _, world in ipairs(default_worlds) do
		core.settings:set("fixed_map_seed", world.seed)
		core.settings:set("mg_name", world.mg_name)
		core.create_world(world.name, gameindex)
	end

	-- Restore the old values
	if old_map_seed then
		core.settings:set("fixed_map_seed", old_map_seed)
	else
		core.settings:remove("fixed_map_seed")
	end
	if old_mapgen then
		core.settings:set("mg_name", old_mapgen)
	else
		core.settings:remove("mg_name")
	end

	menudata.worldlist:refresh()
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
						local selected = core.get_table_index("sp_worlds")
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

local function get_formspec(_, _, tab_data)
	local index = filterlist.get_current_index(menudata.worldlist,
				tonumber(core.settings:get("mainmenu_last_selected_world")))

	-- Default index
	if index == 0 then index = 1 end

	local creative_checkbox = core.settings:get_bool("creative_mode") and
			"creative_checkbox.png" or "blank.png"

	local creative_bg = "creative_bg.png"
	if lang and lang == "ru" then
		creative_bg = "creative_bg_" .. lang .. ".png"
	end

	local space = small_screen and ("\n"):rep(3) or ("\n"):rep(5)
	local retval =
			"style[world_delete;fgimg=" .. defaulttexturedir ..
				"world_delete.png;fgimg_hovered=" .. defaulttexturedir .. "world_delete_hover.png]" ..
			"image_button[-0.1,4.84;3.45,0.92;;world_delete;;true;false]" ..
			"tooltip[world_delete;".. fgettext("Delete") .. "]" ..

			"style[world_create;fgimg=" .. defaulttexturedir ..
				"world_new.png;fgimg_hovered=" .. defaulttexturedir .. "world_new_hover.png]" ..
			"image_button[3.15,4.84;3.45,0.92;;world_create;;true;false]" ..
			"tooltip[world_create;".. fgettext("New") .. "]"

	local world = menudata.worldlist:get_list()[index]
	local game = world and world.gameid ~= "default" and pkgmgr.find_by_gameid(world.gameid)
	if game and game.moddable then
		retval = retval ..
			btn_style("world_configure") ..
			"image_button[9,4.84;3,0.92;;world_configure;" .. fgettext("Select Mods") .. ";true;false]"
	end

	retval = retval ..
			btn_style("play") ..
			"style[play;font_size=*" .. (small_screen and 2.25 or 3) .. "]" ..
			"image_button[6.72,1.43;4.96,1.41;;play;" .. space .. " " ..
				fgettext("Play") .. space .. ";true;false]" ..
			"image[7,1.63;1,1;" .. defaulttexturedir .. "btn_play_icon.png]" ..
			"tooltip[play;".. fgettext("Play Game") .. "]" ..

			"image_button[7.2,3.09;4,0.83;" .. defaulttexturedir .. creative_bg .. ";;;true;false]" ..
			"style[cb_creative_mode;content_offset=0]" ..
			"image_button[7.2,3.09;4,0.83;" .. defaulttexturedir .. creative_checkbox ..
				";cb_creative_mode;;true;false]" ..

			"background9[0,0;6.5,4.8;" .. defaulttexturedir .. "worldlist_bg.png" .. ";false;40]" ..
			"tableoptions[background=#0000;border=false]" ..
			"table[0,0;6.28,4.64;sp_worlds;" .. menu_render_worldlist() .. ";" .. index .. "]"

	if core.settings:get("menu_last_game") == "default" and not tab_data.hidden then
		retval = retval ..
			"style[switch_local;fgimg=" .. defaulttexturedir .. "switch_local.png;fgimg_hovered=" ..
				defaulttexturedir .. "switch_local_hover.png]" ..
			"image_button[10.6,-0.1;1.5,1.5;;switch_local;;true;false]"

		if PLATFORM == "Android" then
			retval = retval ..
				"image_button[6.6,-0.1;1.5,1.5;" ..
					defaulttexturedir .. "gift_btn.png;upgrade;;true;false;" ..
					defaulttexturedir .. "gift_btn_pressed.png]"
		end
	elseif tab_data.hidden or pkgmgr.find_by_gameid("default") then
		retval = retval ..
			"style[switch_local_default;fgimg=" .. defaulttexturedir .. "switch_local_default.png;fgimg_hovered=" ..
				defaulttexturedir .. "switch_local_default_hover.png]" ..
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

local checked_worlds = false
local function refresh_worldlist()
	local gameid = core.settings:get("menu_last_game")
	if not gameid or gameid == "" or
			(gameid == "default" and not pkgmgr.find_by_gameid("default")) then
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
	if game then
		menudata.worldlist:set_filtercriteria(game.id)
		mm_texture.update("singleplayer", game)
		core.set_topleft_text(game.id == "default" and "" or "Powered by Minetest Engine")

		if game.id == "default" then
			local gamebar = ui.find_by_name("game_button_bar")
			if gamebar then
				gamebar:hide()
			end

			-- Create default worlds
			if not checked_worlds then
				create_default_worlds()
			end
			checked_worlds = true
		else
			singleplayer_refresh_gamebar()
			ui.find_by_name("game_button_bar"):show()
		end
	end
end

local function main_button_handler(this, fields, name, tab_data)
	assert(name == "local")

	local world_doubleclick = false

	if fields["sp_worlds"] ~= nil then
		local event = core.explode_table_event(fields["sp_worlds"])
		local selected = core.get_table_index("sp_worlds")

		if event.type == "DCL" then
			world_doubleclick = true
		end

		if event.type == "CHG" and selected ~= nil then
			core.settings:set("mainmenu_last_selected_world",
				menudata.worldlist:get_raw_index(selected))
			return true
		end
	end

	if menu_handle_key_up_down(fields,"sp_worlds","mainmenu_last_selected_world") then
		return true
	end

	if fields["cb_creative_mode"] then
		local creative_mode = core.settings:get_bool("creative_mode")
		core.settings:set("creative_mode", tostring(not creative_mode))
		core.settings:set("enable_damage", tostring(creative_mode))

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
		local game = current_game()
		local create_world_dlg = (game.id ~= "default") and create_create_world_dlg(true) or
			create_create_world_default_dlg(true)
		create_world_dlg:set_parent(this)
		this:hide()
		create_world_dlg:show()
		mm_texture.update("singleplayer", game)
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
				local delete_world_dlg = create_delete_world_dlg(world.name, index,
					world.gameid ~= "default" and world.gameid)
				delete_world_dlg:set_parent(this)
				this:hide()
				delete_world_dlg:show()
				mm_texture.update("singleplayer",current_game())
			end
		end

		return true
	end

	if fields["world_configure"] ~= nil then
		local selected = core.get_table_index("sp_worlds")
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

	if fields["switch_local"] then
		core.settings:remove("menu_last_game")
		refresh_worldlist()
		return true
	end

	if fields["switch_local_default"] then
		core.settings:set("menu_last_game", "default")
		if tab_data.hidden then
			this:set_tab("local_default")
		else
			refresh_worldlist()
		end

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
	if type == "ENTER" then
		refresh_worldlist()
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
