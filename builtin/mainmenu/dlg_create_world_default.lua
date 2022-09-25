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

local function create_world_formspec()
	local mapgens = core.get_mapgen_names()

	local current_seed = core.settings:get("fixed_map_seed") or ""
	local current_mg   = core.settings:get("mg_name")
	local gameid       = core.settings:get("menu_last_game")

	local gameidx = 0
	if gameid ~= nil then
		local _
		_, gameidx = pkgmgr.find_by_gameid(gameid)

		if gameidx == nil then
			gameidx = 0
		end
	end

	local game_by_gameidx = core.get_game(gameidx)
	if game_by_gameidx ~= nil then
		local allowed_mapgens = {"v7p", "flat", "valleys"}
		for i = #mapgens, 1, -1 do
			if table.indexof(allowed_mapgens, mapgens[i]) == -1 then
				table.remove(mapgens, i)
			end
		end

		mapgens[#mapgens + 1] = "superflat"
	end

	local mglist = ""
	local selindex = 1
	local i = 1
	for _, v in pairs(mapgens) do
		if current_mg == v then
			selindex = i
		end
		i = i + 1
		mglist = mglist .. v .. ","
	end
	mglist = mglist:sub(1, -2)

	return
		"label[1.5,0.9;" .. fgettext("World name") .. ":" .. "]"..
		"field[4.5,1.2;6,0.5;te_world_name;;]" ..

		"label[1.5,1.9;" .. fgettext("Seed") .. ":" .. "]"..
		"field[4.5,2.2;6,0.5;te_seed;;".. current_seed .. "]" ..

		"label[1.5,2.9;" .. fgettext("Mapgen") .. ":" .. "]"..
		"dropdown[4.2,2.75;6.3;dd_mapgen;" .. mglist .. ";" .. selindex .. "]" ..

		btn_style("world_create_confirm", "green") ..
		"button[3.5,4.4;2.5,0.5;world_create_confirm;" .. fgettext("Create") .. "]" ..
		btn_style("world_create_cancel") ..
		"button[6,4.4;2.5,0.5;world_create_cancel;" .. fgettext("Cancel") .. "]"
end

local function create_world_buttonhandler(this, fields)
	if fields["world_create_confirm"] or
		fields["key_enter"] then
		local worldname = fields["te_world_name"]
		local gameid = core.settings:get("menu_last_game")
		local gameindex = 0
		if gameid ~= nil then
			local _
			_, gameindex = pkgmgr.find_by_gameid(gameid)

			if gameindex == nil then
				gameindex = 0
			end
		end

		if gameindex ~= 0 then
			-- For unnamed worlds use the generated name 'World <number>',
			-- where the number increments: it is set to 1 larger than the largest
			-- generated name number found.
			if worldname == "" then
				menudata.worldlist:set_filtercriteria(nil) -- to count all existing worlds
				local worldnum_max = 0
				for _, world in ipairs(menudata.worldlist:get_list()) do
					-- Match "World 1" and "World 1 a" (but not "World 1a")
					local worldnum = world.name:match("^World (%d+)$") or world.name:match("^World (%d+) ")
					if worldnum then
						worldnum_max = math.max(worldnum_max, tonumber(worldnum))
					end
				end
				worldname = "World " .. worldnum_max + 1
			end

			if fields["te_seed"] then
				core.settings:set("fixed_map_seed", fields["te_seed"])
			end

			local message
			if not menudata.worldlist:uid_exists_raw(worldname) then
				local old_mg_flags
				if fields["dd_mapgen"] == "superflat" then
					core.settings:set("mg_name", "flat")
					old_mg_flags = core.settings:get("mg_flags")
					core.settings:set("mg_flags", "nocaves,nodungeons,nodecorations")
				else
					core.settings:set("mg_name", fields["dd_mapgen"])
				end
				message = core.create_world(worldname,gameindex)

				-- Restore the old mg_flags setting if creating a superflat world
				if fields["dd_mapgen"] == "superflat" then
					core.settings:set("mg_name", "superflat")
					if old_mg_flags then
						core.settings:set("mg_flags", old_mg_flags)
					else
						core.settings:remove("mg_flags")
					end
				end
			else
				message = fgettext("A world named \"$1\" already exists", worldname)
			end

			if message ~= nil then
				gamedata.errormessage = message
			else
				if this.data.update_worldlist_filter then
					menudata.worldlist:set_filtercriteria(pkgmgr.games[gameindex].id)
					mm_texture.update("singleplayer", pkgmgr.games[gameindex].id)
				end
				menudata.worldlist:refresh()
				core.settings:set("mainmenu_last_selected_world",
									menudata.worldlist:raw_index_by_uid(worldname))
			end
		else
			gamedata.errormessage = fgettext("No game selected")
		end

		this:delete()
		return true
	end

	if fields["world_create_cancel"] then
		this:delete()
		return true
	end

	return false
end


function create_create_world_default_dlg(update_worldlistfilter)
	local retval = dialog_create("sp_create_world",
					create_world_formspec,
					create_world_buttonhandler,
					nil, true)
	retval.update_worldlist_filter = update_worldlistfilter

	return retval
end
