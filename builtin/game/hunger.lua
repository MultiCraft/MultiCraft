--[[
	From Stamina mod
	Copyright (C) BlockMen (2013-2015)
	Copyright (C) Auke Kok <sofar@foo-projects.org> (2016)
	Copyright (C) Minetest Mods Team (2016-2019)
	Copyright (C) MultiCraft Development Team (2016-2022)

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

if not core.settings:get_bool("enable_damage")
or not core.settings:get_bool("enable_hunger") then
	return
end

hunger = {
	hud = {}
}

local function get_setting(key, default)
	local setting = core.settings:get("hunger." .. key)
	return tonumber(setting) or default
end

hunger.settings = {
	-- see settingtypes.txt for descriptions
	tick = get_setting("tick", 600),
	tick_min = get_setting("tick_min", 4),
	health_tick = get_setting("health_tick", 4),
	move_tick = get_setting("move_tick", 0.5),
	poison_tick = get_setting("poison_tick", 1),
	exhaust_dig = get_setting("exhaust_dig", 2),
	exhaust_place = get_setting("exhaust_place", 1),
	exhaust_move = get_setting("exhaust_move", 2),
	exhaust_jump = get_setting("exhaust_jump", 4),
	exhaust_craft = get_setting("exhaust_craft", 2),
	exhaust_punch = get_setting("exhaust_punch", 5),
	exhaust_lvl = get_setting("exhaust_lvl", 192),
	heal = get_setting("heal", 1),
	heal_lvl = get_setting("heal_lvl", 5),
	starve = get_setting("starve", 1),
	starve_lvl = get_setting("starve_lvl", 2),
	level_max = get_setting("level_max", 21),
	visual_max = get_setting("visual_max", 20)
}
local settings = hunger.settings

local min, max = math.min, math.max
local vlength = vector.length
local tcopy = table.copy

local attribute = {
	saturation = "hunger:level",
	poisoned = "hunger:poisoned",
	exhaustion = "hunger:exhaustion"
}

local function is_player(player)
	return (
		core.is_player(player) and
		not player.is_fake_player and
		not core.is_creative_enabled(player:get_player_name())
	)
end

local function get_hud_id(player)
	return hunger.hud[player:get_player_name()]
end

local function set_hud_id(player, hud_id)
	hunger.hud[player:get_player_name()] = hud_id
end

--- SATURATION API ---
function hunger.get_saturation(player)
	if not is_player(player) then
		return settings.level_max
	end

	-- This uses get_string so that nil is returned when there is no value as
	-- get_string() will return an empty string and tonumber("") == nil.
	return tonumber(player:get_meta():get_string(attribute.saturation))
end

function hunger.set_hud_level(player, level)
	if not is_player(player) then
		return
	end

	player:hud_change(get_hud_id(player), "number", min(settings.visual_max, level))
end

function hunger.set_saturation(player, level)
	player:get_meta():set_int(attribute.saturation, level)
	hunger.set_hud_level(player, level)
end

hunger.registered_on_update_saturations = {}
function hunger.register_on_update_saturation(fun)
	local saturations = hunger.registered_on_update_saturations
	saturations[#saturations + 1] = fun
end

function hunger.update_saturation(player, level)
	if not is_player(player) then
		return
	end

	for _, callback in ipairs(hunger.registered_on_update_saturations) do
		local result = callback(player, level)
		if result then
			return result
		end
	end

	local old = hunger.get_saturation(player)

	if not old or old == level then -- To suppress HUD update
		return
	end

	-- players without interact priv cannot eat
	if old < settings.heal_lvl and not core.check_player_privs(player, {interact=true}) then
		return
	end

	hunger.set_saturation(player, level)
end

function hunger.change_saturation(player, change)
	if not change or change == 0 or not is_player(player) then
		return false
	end

	local old = hunger.get_saturation(player)

	if not old then
		return false
	end

	local level = old + change
	level = max(level, 0)
	level = min(level, settings.level_max)
	hunger.update_saturation(player, level)
	return true
end

hunger.change = hunger.change_saturation -- for backwards compatibility
--- END SATURATION API ---

--- POISON API ---
function hunger.is_poisoned(player)
	if not is_player(player) then
		return false
	end

	local poisoned = player:get_meta():get_string(attribute.poisoned)
	return poisoned and poisoned == "yes"
end

function hunger.set_hud_poisoned(player, poisoned)
	if not is_player(player) then
		return
	end

	local texture = poisoned and "hunger_poisen.png" or "hunger.png"
	player:hud_change(get_hud_id(player), "text", texture)
end

function hunger.set_poisoned(player, poisoned)
	if not is_player(player) then
		return
	end

	hunger.set_hud_poisoned(player, poisoned)
	local attr = poisoned and "yes" or "no"
	player:get_meta():set_string(attribute.poisoned, attr)
end

local function poison_tick(name, ticks, interval, elapsed)
	local player = core.get_player_by_name(name)
	if not player or not hunger.is_poisoned(player) then
		return
	elseif elapsed > ticks then
		hunger.set_poisoned(player, false)
	else
		local hp = player:get_hp() - 1
		if hp > 0 then
			player:set_hp(hp)
		end
		core.after(interval, poison_tick, name, ticks, interval, elapsed + 1)
	end
end

hunger.registered_on_poisons = {}
function hunger.register_on_poison(fun)
	local poison = hunger.registered_on_poisons
	poison[#poison + 1] = fun
end

function hunger.poison(player, ticks, interval)
	if not is_player(player) then
		return
	end

	for _, fun in ipairs(hunger.registered_on_poisons) do
		local rv = fun(player, ticks, interval)
		if rv == true then
			return
		end
	end
	hunger.set_poisoned(player, true)
	poison_tick(player:get_player_name(), ticks, interval, 0)
end
--- END POISON API ---

--- EXHAUSTION API ---
hunger.exhaustion_reasons = {
	craft = "craft",
	dig = "dig",
	heal = "heal",
	jump = "jump",
	move = "move",
	place = "place",
	punch = "punch"
}

function hunger.get_exhaustion(player)
	return player:get_meta():get_int(attribute.exhaustion)
end

function hunger.set_exhaustion(player, exhaustion)
	player:get_meta():set_int(attribute.exhaustion, exhaustion)
end

hunger.registered_on_exhaust_players = {}
function hunger.register_on_exhaust_player(fun)
	local exhaust = hunger.registered_on_exhaust_players
	exhaust[#exhaust + 1] = fun
end

function hunger.exhaust_player(player, change, cause)
	if not is_player(player) then
		return
	end

	for _, callback in ipairs(hunger.registered_on_exhaust_players) do
		local result = callback(player, change, cause)
		if result then
			return result
		end
	end

	local exhaustion = hunger.get_exhaustion(player) or 0
	exhaustion = exhaustion + change

	if exhaustion >= settings.exhaust_lvl then
		exhaustion = exhaustion - settings.exhaust_lvl
		hunger.change(player, -1)
	end

	hunger.set_exhaustion(player, exhaustion)
end
--- END EXHAUSTION API ---

-- Time based hunger functions
local connected_players = core.get_connected_players

local function move_tick()
	for _, player in ipairs(connected_players()) do
		local controls = player:get_player_control()
		local is_moving = controls.up or controls.down or controls.left or controls.right
		local velocity = player:get_velocity()
		velocity.y = 0
		local horizontal_speed = vlength(velocity)
		local has_velocity = horizontal_speed > 0.05

		if controls.jump then
			hunger.exhaust_player(player, settings.exhaust_jump, hunger.exhaustion_reasons.jump)
		elseif is_moving and has_velocity then
			hunger.exhaust_player(player, settings.exhaust_move, hunger.exhaustion_reasons.move)
		end

	end
end

local function hunger_tick()
	-- lower saturation by 1 point after settings.tick seconds
	for _, player in ipairs(connected_players()) do
		local saturation = hunger.get_saturation(player) or 0
		if saturation > settings.tick_min then
			hunger.update_saturation(player, saturation - 1)
		end
	end
end

local function health_tick()
	-- heal or damage player, depending on saturation
	for _, player in ipairs(connected_players()) do
		local air = player:get_breath() or 0
		local hp = player:get_hp() or 0
		local saturation = hunger.get_saturation(player) or 0

		-- don't heal if dead, drowning, or poisoned
		local should_heal = (
			saturation >= settings.heal_lvl and
			hp > 0 and
			hp < 20 and
			air > 0
			and not hunger.is_poisoned(player)
		)
		-- or damage player by 1 hp if saturation is < 2 (of 21)
		local is_starving = (
			saturation < settings.starve_lvl and
			hp > 0
		)

		if should_heal then
			player:set_hp(hp + settings.heal)
		elseif is_starving then
			player:set_hp(hp - settings.starve)
			hunger.set_hud_level(player, 0)
			core.after(0.5, function()
				hunger.set_hud_level(player, min(settings.visual_max, saturation))
			end)
		end
	end
end

local hunger_timer = 0
local health_timer = 0
local action_timer = 0

core.register_globalstep(function(dtime)
	hunger_timer = hunger_timer + dtime
	health_timer = health_timer + dtime
	action_timer = action_timer + dtime

	if action_timer > settings.move_tick then
		action_timer = 0
		move_tick()
	end

	if hunger_timer > settings.tick then
		hunger_timer = 0
		hunger_tick()
	end

	if health_timer > settings.health_tick then
		health_timer = 0
		health_tick()
	end
end)

function hunger.item_eat(hp_change, user, poison)
	if not poison then
		hunger.change_saturation(user, hp_change)
		hunger.set_exhaustion(user, 0)
	else
		hunger.change_saturation(user, hp_change)
		hunger.poison(user, -poison, settings.poison_tick)
	end
end

hunger.bar_definition = {
	hud_elem_type = "statbar",
	position = {x = 0.5, y = 1},
	text = "hunger.png",
	text2 = "hunger_gone.png",
	number = settings.visual_max,
	item = settings.visual_max,
	size = {x = 24, y = 24},
	offset = {x = -265, y = -126}
}

core.register_on_joinplayer(function(player)
	local level = hunger.get_saturation(player)
	if not level then
		level = settings.level_max
		player:get_meta():set_int(attribute.saturation, level)
	end

	-- add HUD
	if hunger.bar_definition then
		local def = tcopy(hunger.bar_definition)
		def.number = min(settings.visual_max, level)
		local id = player:hud_add(def)
		set_hud_id(player, id)
	end

	-- reset poisoned
	hunger.set_poisoned(player, false)
end)

core.register_on_leaveplayer(function(player)
	set_hud_id(player, nil)
end)

local exhaust = hunger.exhaust_player
core.register_on_placenode(function(_, _, player)
	exhaust(player, settings.exhaust_place, hunger.exhaustion_reasons.place)
end)
core.register_on_dignode(function(_, _, player)
	exhaust(player, settings.exhaust_dig, hunger.exhaustion_reasons.dig)
end)
core.register_on_craft(function(_, player)
	exhaust(player, settings.exhaust_craft, hunger.exhaustion_reasons.craft)
end)
core.register_on_punchplayer(function(_, hitter)
	exhaust(hitter, settings.exhaust_punch, hunger.exhaustion_reasons.punch)
end)
core.register_on_respawnplayer(function(player)
	hunger.update_saturation(player, settings.level_max)
	hunger.set_exhaustion(player, 0)
	hunger.set_poisoned(player, false)
end)
