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
local password_tmp = ""

local esc = core.formspec_escape
local lower = utf8.lower
local small_screen = (PLATFORM == "Android" or PLATFORM == "iOS") and not core.settings:get_bool("device_is_tablet")

local function get_formspec(tabview, name, tabdata)
	-- Update the cached supported proto info,
	-- it may have changed after a change by the settings menu.
	common_update_cached_supp_proto()
	local selected
	if menudata.search_result then
		selected = menudata.search_result[tabdata.selected]
	else
		selected = serverlistmgr.servers[tabdata.selected]
	end

	if not tabdata.search_for then
		tabdata.search_for = ""
	end

	local address = core.settings:get("address")
	local port = tonumber(core.settings:get("remote_port"))
	if port and port ~= 30000 then
		address = address .. ":" .. port
	end

	local retval =
		-- Search
		"formspec_version[3]" ..
		"image[-0.11,4.93;8.02,0.81;" .. defaulttexturedir_esc .. "field_bg.png;32]" ..
		"style[Dte_search;border=false;bgcolor=transparent]" ..
		"field[0.25,5.25;5.9,0.83;Dte_search;;" .. esc(tabdata.search_for) .. "]" ..
		"image_button[5.62,4.93;0.83,0.83;" .. defaulttexturedir_esc ..
			"clear.png;btn_mp_clear;;true;false]" ..
		btn_style("btn_mp_refresh") ..
		"image_button[6.37,4.93;0.83,0.83;" .. defaulttexturedir_esc ..
			"refresh.png;btn_mp_refresh;;true;false]" ..

		-- Address / Port
		"image[7.1,0.09;6,0.8;" .. defaulttexturedir_esc .. "field_bg.png;32]" ..
		"style[te_address;border=false;bgcolor=transparent]" ..
		"field[7.45,0.55;4.9,0.5;te_address;" .. fgettext("Address / Port") .. ":;" ..
			esc(address) .. "]" ..

		-- Name
		"image[7.1,1.25;2.95,0.8;" .. defaulttexturedir_esc .. "field_bg.png;32]" ..
		"style[te_name;border=false;bgcolor=transparent]" ..
		"field[7.45,1.7;2.45,0.5;te_name;" .. fgettext("Name") .. ":;" ..
			esc(core.settings:get("name")) .. "]" ..

		-- Password
		"image[9.55,1.25;2.95,0.8;" .. defaulttexturedir_esc .. "field_bg.png;32]" ..
		"style[te_pwd;border=false;bgcolor=transparent]" ..
		"pwdfield[9.9,1.7;2.45,0.5;te_pwd;" .. fgettext("Password") .. ":;" ..
			esc(password_tmp) .. "]" ..

		-- Description Background
		"background9[7.2,2.2;4.8,2.65;" .. defaulttexturedir_esc .. "desc_bg.png;false;32]" ..

		-- Connect
		btn_style("btn_mp_connect") ..
		"style[btn_mp_connect;font_size=*" .. (small_screen and 1.5 or 2) .. "]" ..
		"image_button[8.8,4.88;3.3,0.9;;btn_mp_connect;" ..
			("\n"):rep(3) .. " " .. fgettext("Play") .. ("\n"):rep(3) .. ";true;false]" .. -- Connect
		"image[9,5;0.6,0.6;" .. defaulttexturedir_esc .. "btn_play_icon.png]" ..
		"tooltip[btn_mp_connect;" .. fgettext("Connect") .. "]"

	if tabdata.selected and selected then
		if gamedata.fav then
			retval = retval ..
				btn_style("btn_delete_favorite", "red") ..
				"image_button[7.1,4.93;0.83,0.83;" .. defaulttexturedir_esc ..
					"trash.png;btn_delete_favorite;;true;false;" .. defaulttexturedir_esc .. "trash_pressed.png]" ..
					"tooltip[btn_delete_favorite;" .. fgettext("Del. Favorite") .. "]"
		end
		if selected.description then
			retval = retval ..
				"textarea[7.5,2.2;4.8,3;;" .. esc((gamedata.serverdescription or ""), true) .. ";]"
		end
	end

	--favorites
	retval = retval ..
		"background9[0,-0.1;7.1,5;" ..
			defaulttexturedir_esc .. "worldlist_bg.png;false;40]" ..
		"tableoptions[background=#0000;border=false]" ..
		"tablecolumns[" ..
		image_column(fgettext("Favorite")) .. ",align=center;" ..
		image_column(fgettext("Lag")) .. ",padding=0.5;" ..
		"color,span=3;" ..
		"text,align=right;" ..               -- clients
		"text,align=center,padding=0.25;" .. -- "/"
		"text,align=right,padding=0.25;" ..  -- clients_max
		image_column(fgettext("Server mode")) .. ",padding=0.5;" ..
		"color,span=1;" ..
		"text,padding=0.5]" ..
		"table[-0.02,-0.1;6.91,4.87;favorites;"

	if menudata.search_result then
		local favs = serverlistmgr.get_favorites()
		for i = 1, #menudata.search_result do
			local server = menudata.search_result[i]
			for fav_id = 1, #favs do
				if server.address == favs[fav_id].address and
						server.port == favs[fav_id].port then
					server.is_favorite = true
				end
			end

			if i ~= 1 then
				retval = retval .. ","
			end

			retval = retval .. render_serverlist_row(server, server.is_favorite)
		end
	elseif #serverlistmgr.servers > 0 then
		local favs = serverlistmgr.get_favorites()
		if #favs > 0 then
			for i = 1, #favs do
				for j = 1, #serverlistmgr.servers do
					if serverlistmgr.servers[j] and serverlistmgr.servers[j].address == favs[i].address and
							serverlistmgr.servers[j].port == favs[i].port then
						table.insert(serverlistmgr.servers, i, table.remove(serverlistmgr.servers, j))
					end
				end
				if serverlistmgr.servers[i] and favs[i].address ~= serverlistmgr.servers[i].address then
					table.insert(serverlistmgr.servers, i, favs[i])
				end
			end
		end

		retval = retval .. render_serverlist_row(serverlistmgr.servers[1], (#favs > 0))
		for i = 2, #serverlistmgr.servers do
			retval = retval .. "," .. render_serverlist_row(serverlistmgr.servers[i], (i <= #favs))
		end
	end

	if tabdata.selected then
		retval = retval .. ";" .. tabdata.selected .. "]"
	else
		retval = retval .. ";0]"
	end

	return retval
end

--------------------------------------------------------------------------------
local function main_button_handler(tabview, fields, name, tabdata)
	local serverlist = menudata.search_result or serverlistmgr.servers

	if fields.te_name then
		gamedata.playername = fields.te_name
		core.settings:set("name", fields.te_name)
	end

	if fields.te_pwd then
		password_tmp = fields.te_pwd
	end

	if fields.favorites then
		local event = core.explode_table_event(fields.favorites)
		local fav = serverlist[event.row]

		if event.type == "DCL" then
			if event.row <= #serverlist then
				if not is_server_protocol_compat_or_error(
							fav.proto_min, fav.proto_max) then
					return true
				end

				gamedata.address    = fav.address
				gamedata.port       = fav.port
				gamedata.playername = fields.te_name
				gamedata.selected_world = 0

				if fields.te_pwd then
					gamedata.password = fields.te_pwd
				end

				gamedata.servername        = fav.name
				gamedata.serverdescription = fav.description

				if gamedata.address and gamedata.port then
					core.settings:set("address", gamedata.address)
					core.settings:set("remote_port", gamedata.port)
					core.start()
				end
			end
			return true
		end

		if event.type == "CHG" then
			if event.row <= #serverlist then
				gamedata.fav = false
				local favs = serverlistmgr.get_favorites()
				local address = fav.address
				local port    = fav.port
				gamedata.serverdescription = fav.description

				for i = 1, #favs do
					if fav.address == favs[i].address and
							fav.port == favs[i].port then
						gamedata.fav = true
					end
				end

				if address and port then
					core.settings:set("address", address)
					core.settings:set("remote_port", port)
				end
				tabdata.selected = event.row
			end
			return true
		end
	end

	if fields.key_up or fields.key_down then
		local fav_idx = core.get_table_index("favorites")
		local fav = serverlist[fav_idx]

		if fav_idx then
			if fields.key_up and fav_idx > 1 then
				fav_idx = fav_idx - 1
			elseif fields.key_down and fav_idx < #serverlistmgr.servers then
				fav_idx = fav_idx + 1
			end
		else
			fav_idx = 1
		end

		if not serverlistmgr.servers or not fav then
			tabdata.selected = 0
			return true
		end

		local address = fav.address
		local port    = fav.port
		gamedata.serverdescription = fav.description
		if address and port then
			core.settings:set("address", address)
			core.settings:set("remote_port", port)
		end

		tabdata.selected = fav_idx
		return true
	end

	if fields.btn_delete_favorite then
		local current_favorite = core.get_table_index("favorites")
		if not current_favorite then return end

		serverlistmgr.delete_favorite(serverlistmgr.servers[current_favorite])
		serverlistmgr.sync()
		tabdata.selected = nil

		core.settings:set("address", "")
		core.settings:set("remote_port", "30000")
		return true
	end

	if fields.btn_mp_clear then
		tabdata.search_for = ""
		menudata.search_result = nil
		return true
	end

	if fields.btn_mp_refresh then
		serverlistmgr.sync()
		return true
	end

	if fields.Dte_search and not
			(fields.btn_mp_connect or fields.key_enter) then
		tabdata.selected = 1
		local input = lower(fields.Dte_search or "")
		tabdata.search_for = fields.Dte_search

		if #serverlistmgr.servers < 2 then
			return true
		end

		menudata.search_result = {}

		-- setup the keyword list
		local keywords = {}
		for word in input:gmatch("%S+") do
			word = word:gsub("(%W)", "%%%1")
			table.insert(keywords, word)
		end

		if #keywords == 0 then
			menudata.search_result = nil
			return true
		end

		-- Search the serverlist
		local search_result = {}
		for i = 1, #serverlistmgr.servers do
			local server = serverlistmgr.servers[i]
			local found = 0
			for k = 1, #keywords do
				local keyword = keywords[k]
				if server.name then
					local sername = lower(server.name)
					local _, count = sername:gsub(keyword, keyword)
					found = found + count * 4
				end

				if server.description then
					local desc = lower(server.description)
					local _, count = desc:gsub(keyword, keyword)
					found = found + count * 2
				end
			end
			if found > 0 then
				local points = (#serverlistmgr.servers - i) / 5 + found
				server.points = points
				table.insert(search_result, server)
			end
		end
		if #search_result > 0 then
			table.sort(search_result, function(a, b)
				return a.points > b.points
			end)
			menudata.search_result = search_result
			local first_server = search_result[1]
			if first_server.address and first_server.port then
				core.settings:set("address",     first_server.address)
				core.settings:set("remote_port", first_server.port)
				gamedata.serverdescription = first_server.description
			end
		end
		return true
	end

	if (fields.btn_mp_connect or fields.key_enter)
			and fields.te_address ~= "" then
		gamedata.playername = fields.te_name
		gamedata.password   = fields.te_pwd

		-- Allow entering "address:port"
		local address, port = fields.te_address:match("^(.+):([0-9]+)$")
		gamedata.address    = address or fields.te_address
		gamedata.port       = tonumber(port) or 30000

		gamedata.selected_world = 0
		local fav_idx = core.get_table_index("favorites")
		local fav = serverlist[fav_idx]

		if fav_idx and fav_idx <= #serverlist and
				fav.address == gamedata.address and
				fav.port    == gamedata.port then
			if not is_server_protocol_compat_or_error(
						fav.proto_min, fav.proto_max) then
				return true
			end

			serverlistmgr.add_favorite(fav)

			gamedata.servername        = fav.name
			gamedata.serverdescription = fav.description
		else
			gamedata.servername        = ""
			gamedata.serverdescription = ""

			serverlistmgr.add_favorite({
				address = gamedata.address,
				port = gamedata.port,
			})
		end

		local auto_connect = false
		for _, server in pairs(serverlist) do
			if server.server_id == "multicraft" and server.address == gamedata.address then
				auto_connect = true
				break
			end
		end

		core.settings:set_bool("auto_connect", auto_connect)
		core.settings:set("connect_time", os.time())
		core.settings:set("maintab_LAST", "online")
		core.settings:set("address",     gamedata.address)
		core.settings:set("remote_port", gamedata.port)

		core.start()
		return true
	end
	return false
end

local function on_change(type, old_tab, new_tab)
	if type == "LEAVE" then return end
	serverlistmgr.sync()
end

--------------------------------------------------------------------------------
return {
	name = "online",
	caption = fgettext("Join Game"),
	cbf_formspec = get_formspec,
	cbf_button_handler = main_button_handler,
	on_change = on_change
}
