--[[
	From Better HUD mod
	Copyright (C) BlockMen (2013-2016)
	Copyright (C) MultiCraft Development Team (2019-2020)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published by
	the Free Software Foundation; either version 3.0 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License along
	with this program; if not, write to the Free Software Foundation, Inc.,
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
]]

hud, hud_id = {}, {}

local items, sb_bg = {}, {}

local function throw_error(msg)
	core.log("error", "HUD: " .. msg)
end

--
-- API
--

function hud.register(name, def)
	if not name or not def then
		throw_error("Not enough parameters given")
		return false
	end

	if items[name] then
		throw_error("A statbar with name " .. name .. " already exists")
		return false
	end

	-- actually register
	-- add background first since draworder is based on id
	if def.hud_elem_type == "statbar" and def.background then
		sb_bg[name] = table.copy(def)
		sb_bg[name].text = def.background
		if not def.autohide_bg and def.max then
			sb_bg[name].number = def.max
		end
	end
	-- add item itself
	items[name] = def

	return true
end

function hud.change_item(player, name, def)
	if not player or not player:is_player() then
		return false
	end

	if not name or not def then
		throw_error("Not enough parameters given")
		return false
	end

	local i_name = player:get_player_name() .. "_" .. name
	local elem = hud_id[i_name]
	local item = items[name]

	if not elem then
	--	throw_error("Given HUD element " .. dump(name) .. " does not exist")
		return false
	end

	if def.number then
		if item.hud_elem_type ~= "statbar" then
			throw_error("Attempted to change an statbar HUD parameter for text HUD element " .. dump(name))
			return false
		end

		if item.max and def.number > item.max then
			def.number = item.max
		end
		player:hud_change(elem.id, "number", def.number)

		-- hide background when set
		if item.autohide_bg then
			local bg = hud_id[i_name .. "_bg"]
			if not bg then
				throw_error("Given HUD element " .. dump(name) .. "_bg does not exist")
				return false
			end

			local num = def.number ~= 0 and (item.max or item.number) or 0
			player:hud_change(bg.id, "number", num)
		end
	elseif def.text then
		player:hud_change(elem.id, "text", def.text)
	elseif def.offset then
		player:hud_change(elem.id, "offset", def.offset)
	end

	return true
end

function hud.remove_item(player, name)
	if not player or not player:is_player() then
		return false
	end

	if not name then
		throw_error("Not enough parameters given")
		return false
	end

	local i_name = player:get_player_name() .. "_" .. name
	local elem = hud_id[i_name]

	if not elem then
		throw_error("Given HUD element " .. dump(name) .. " does not exist")
		return false
	end

	player:hud_remove(elem.id)

	return true
end

--
-- Add registered HUD items to joining players
--

-- Following code is placed here to keep HUD ids internal
local function add_hud_item(player, name, def)
	if not player or not name or not def then
		throw_error("Not enough parameters given")
		return false
	end

	local i_name = player:get_player_name() .. "_" .. name
	hud_id[i_name] = {}
	hud_id[i_name].id = player:hud_add(def)
end

core.register_on_joinplayer(function(player)
	-- add the backgrounds for statbars
	for name, item in pairs(sb_bg) do
		add_hud_item(player, name .. "_bg", item)
	end
	-- and finally the actual HUD items
	for name, item in pairs(items) do
		add_hud_item(player, name, item)
	end
end)

core.register_on_leaveplayer(function(player)
	local player_name = player:get_player_name()
	for name, _ in pairs(sb_bg) do
		sb_bg[player_name .. "_" .. name] = nil
	end
	for name, _ in pairs(items) do
		hud_id[player_name .. "_" .. name] = nil
	end
end)
