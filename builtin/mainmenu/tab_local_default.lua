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

local function get_formspec()
	menudata.worldlist:set_filtercriteria("default")

	local index = filterlist.get_current_index(menudata.worldlist,
				tonumber(core.settings:get("mainmenu_last_selected_world")))
	if not index or index < 1 then
		local selected = core.get_table_index("sp_worlds")
		if selected ~= nil and selected < #menudata.worldlist:get_list() then
			index = selected
		else
			index = #menudata.worldlist:get_list()
		end
	end

	local creative_checkbox = core.settings:get_bool("creative_mode") and
			"creative_checkbox" or "blank"

	local creative_bg = "creative_bg.png"
	if lang and lang == "ru" then
		creative_bg = "creative_bg_" .. lang .. ".png"
	end

	local retval =
			"image_button[0,4.84;3.31,0.92;" ..
				core.formspec_escape(defaulttexturedir ..
					"blank.png") .. ";world_delete;;true;false]" ..
			"tooltip[world_delete;".. fgettext("Delete") .. "]" ..
			"image_button[3.14,4.84;3.3,0.92;" ..
				core.formspec_escape(defaulttexturedir ..
					"blank.png") .. ";world_create;;true;false]" ..
			"tooltip[world_create;".. fgettext("New") .. "]" ..

			"image_button[6.72,1.43;4.96,1.41;" ..
				core.formspec_escape(defaulttexturedir ..
					"blank.png") .. ";play;;true;false]" ..
			"tooltip[play;".. fgettext("Play Game") .. "]" ..

			"image_button[7.2,3.09;4,0.83;" ..
				core.formspec_escape(defaulttexturedir) .. creative_bg ..
				";;;true;false]" ..
			"image_button[7.2,3.09;4,0.83;" ..
				core.formspec_escape(defaulttexturedir) .. creative_checkbox ..
					".png;cb_creative_mode;;true;false]" ..

			"tableoptions[background=#27233F;border=false]" ..
			"table[-0.01,0;6.28,4.64;sp_worlds;" ..
			menu_render_worldlist() ..
			";" .. index .. "]"


	if PLATFORM ~= "Android" and PLATFORM ~= "iOS" then
		retval = retval ..
				"checkbox[6.6,5;cb_server;".. fgettext("Create Server") ..";" ..
					dump(core.settings:get_bool("enable_server")) .. "]"
	end

	if core.settings:get_bool("enable_server") then
		retval = retval ..
				"checkbox[6.6,0.65;cb_server_announce;" .. fgettext("Announce Server") .. ";" ..
					dump(core.settings:get_bool("server_announce")) .. "]" ..

				-- Name / Password
				"label[6.6,-0.3;" .. fgettext("Name") .. ":" .. "]" ..
				"label[9.3,-0.3;" .. fgettext("Password") .. ":" .. "]" ..
				"field[6.9,0.6;2.8,0.5;te_playername;;" ..
					core.formspec_escape(core.settings:get("name")) .. "]" ..
				"pwdfield[9.6,0.6;2.8,0.5;te_passwd;]"
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

--------------------------------------------------------------------------------
return {
	name = "local_default",
	caption = fgettext("Singleplayer"),
	cbf_formspec = get_formspec,
	cbf_button_handler = main_button_handler
}
