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

local mapgens = {"v7p", "valleys", "flat", "superflat"}
local mapgen_names = {"Default", "Valleys", "Flat", "Superflat"}

local function create_world_formspec()
	local current_seed = core.settings:get("fixed_map_seed") or ""
	local current_mg   = core.settings:get("mg_name")

	local selindex = math.max(table.indexof(mapgens, current_mg), 1)
	local mglist = table.concat(mapgen_names, ",")

	return "real_coordinates[true]formspec_version[3]" ..

		"image[3.5,1.1;8.5,0.8;" .. defaulttexturedir_esc .. "field_bg.png;32]" ..
		"style[te_world_name;border=false;bgcolor=transparent]" ..
		"field[3.55,1.1;8.4,0.8;te_world_name;" .. fgettext("World name") .. ":;]" ..

		"image[3.5,2.5;8.5,0.8;" .. defaulttexturedir_esc .. "field_bg.png;32]" ..
		"style[te_seed;border=false;bgcolor=transparent]" ..
		"field[3.55,2.5;8.4,0.8;te_seed;" .. fgettext("Seed") .. ":;".. current_seed .. "]" ..

		"label[3.5,3.7;" .. fgettext("Mapgen") .. ":]"..
		"dropdown[3.5,3.9;8.5,0.8;dd_mapgen;" .. mglist .. ";" .. selindex .. ";true]" ..

		btn_style("world_create_confirm", "green") ..
		"button[4.1,5.3;3.5,0.8;world_create_confirm;" .. fgettext("Create") .. "]" ..
		btn_style("world_create_cancel") ..
		"button[7.9,5.3;3.5,0.8;world_create_cancel;" .. fgettext("Cancel") .. "]"
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
				local mapgen = mapgens[tonumber(fields["dd_mapgen"])]
				if mapgen == "superflat" then
					core.settings:set("mg_name", "flat")
					old_mg_flags = core.settings:get("mg_flags")
					core.settings:set("mg_flags", "nocaves,nodungeons,nodecorations")
				else
					core.settings:set("mg_name", mapgen)
				end
				message = core.create_world(worldname,gameindex)

				-- Restore the old mg_flags setting if creating a superflat world
				if mapgen == "superflat" then
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
