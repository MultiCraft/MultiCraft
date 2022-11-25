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

local small_screen = (PLATFORM == "Android" or PLATFORM == "iOS") and not core.settings:get_bool("device_is_tablet")

local default_worlds = {
	{name = "World 1", mg_name = "v7p", seed = "15823438331521897617"},
	{name = "World 2", mg_name = "v7p", seed = "1841722166046826822"},
	{name = "World 3", mg_name = "v7p", seed = "CC"},
	{name = "World 4", mg_name = "valleys", seed = "8572"},
	{name = "World 5 Flat", mg_name = "flat", seed = "2"}
}

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

local checked_worlds = false
local function get_formspec(this)
	-- Only check the worlds once (on restart)
	if not checked_worlds then
		create_default_worlds()
	end
	checked_worlds = true

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

	local creative_checkbox = core.settings:get_bool("creative_mode") and
			"creative_checkbox.png" or "blank.png"

	local creative_bg = "creative_bg.png"
	if lang and lang == "ru" then
		creative_bg = "creative_bg_" .. lang .. ".png"
	end

	local space = small_screen and ("\n"):rep(3) or ("\n"):rep(5)
	local retval =
			"style[world_delete;fgimg=" .. defaulttexturedir_esc ..
			"world_delete.png;fgimg_hovered=" .. defaulttexturedir_esc .. "world_delete_hover.png]" ..
			"image_button[-0.1,4.84;3.45,0.92;;world_delete;;true;false]" ..
			"tooltip[world_delete;".. fgettext("Delete") .. "]" ..

			"style[world_create;fgimg=" .. defaulttexturedir_esc ..
				"world_new.png;fgimg_hovered=" .. defaulttexturedir_esc .. "world_new_hover.png]" ..
			"image_button[3.15,4.84;3.45,0.92;;world_create;;true;false]" ..
			"tooltip[world_create;".. fgettext("New") .. "]" ..

			btn_style("play") ..
			"style[play;font_size=*" .. (small_screen and 2.25 or 3) .. "]" ..
			"image_button[6.72,1.43;4.96,1.41;;play;" .. space .. " " ..
				fgettext("Play") .. space .. ";true;false]" ..
			"image[7,1.63;1,1;" .. defaulttexturedir_esc .. "btn_play_icon.png]" ..
			"tooltip[play;".. fgettext("Play Game") .. "]" ..

			"image_button[7.2,3.09;4,0.83;" .. defaulttexturedir_esc .. creative_bg .. ";;;true;false]" ..
			"style[cb_creative_mode;content_offset=0]" ..
			"image_button[7.2,3.09;4,0.83;" .. defaulttexturedir_esc .. creative_checkbox ..
				";cb_creative_mode;;true;false]" ..

			"background9[0,0;6.5,4.8;" .. defaulttexturedir_esc .. "worldlist_bg.png;false;40]" ..
			"tableoptions[background=#0000;border=false]" ..
			"table[0,0;6.28,4.64;sp_worlds;" .. menu_render_worldlist() .. ";" .. index .. "]" ..

			"style[switch_local;fgimg=" .. defaulttexturedir_esc .. "switch_local.png;fgimg_hovered=" ..
				defaulttexturedir_esc .. "switch_local_hover.png]" ..
			"image_button[10.6,-0.1;1.5,1.5;;switch_local;;true;false]"

	if PLATFORM == "Android" then
		retval = retval ..
			"image_button[6.6,-0.1;1.5,1.5;" ..
				defaulttexturedir_esc .. "gift_btn.png;upgrade;;true;false;" ..
				defaulttexturedir_esc .. "gift_btn_pressed.png]"
	end

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

	if fields["upgrade"] then
		core.upgrade("")
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
