-- cache setting
local enable_damage = core.settings:get_bool("enable_damage")
local disable_health = false

local health_bar_definition = {
	hud_elem_type = "statbar",
	position      = {x =  0.5, y =  1},
	offset        = {x = -247, y = -94},
	size          = {x =  24,  y =  24},
	text          = "heart.png",
	background    = "heart_gone.png",
	number        = 20
}

local breath_bar_definition = {
	hud_elem_type = "statbar",
	position      = {x =  0.5, y = 1},
	offset        = {x =  8,  y = -120},
	size          = {x =  24,  y = 24},
	text          = "bubble.png",
	number        = 20
}

local hud_ids = {}

local function update_builtin_statbars(player)
	local name = player:get_player_name()

	if name == "" then
		return
	end

	local flags = player:hud_get_flags()
	if not hud_ids[name] then
		hud_ids[name] = {}
		-- flags are not transmitted to client on connect, we need to make sure
		-- our current flags are transmitted by sending them actively
		player:hud_set_flags(flags)
	end
	local hud_id = hud_ids[name]

	if flags.healthbar and not disable_health then
		hud.change_item(player, "health", {number = player:get_hp()})
	end

	if flags.breathbar and player:get_breath() < 10 then
		local number = player:get_breath() * 2
		if hud_id.id_breathbar == nil then
			local hud_def = table.copy(breath_bar_definition)
			hud_def.number = number
			hud_id.id_breathbar = player:hud_add(hud_def)
		else
			player:hud_change(hud_id.id_breathbar, "number", number)
	end
	elseif hud_id.id_breathbar then
		player:hud_remove(hud_id.id_breathbar)
		hud_id.id_breathbar = nil
	end
end

local function cleanup_builtin_statbars(player)
	local name = player:get_player_name()

	if name == "" then
		return
	end

	hud_ids[name] = nil
end

local function player_event_handler(player,eventname)
	assert(player:is_player())

	local name = player:get_player_name()

	if name == "" then
		return
	end

	if eventname == "health_changed" then
		update_builtin_statbars(player)

		if hud_ids[name].id_healthbar then
			return true
		end
	end

	if eventname == "breath_changed" then
		update_builtin_statbars(player)

		if hud_ids[name].id_breathbar then
			return true
		end
	end

	if eventname == "hud_changed" or eventname == "properties_changed" then
		update_builtin_statbars(player)
		return true
	end

	return false
end

function core.hud_replace_builtin(hud_name, definition)
	if hud_name == "health" then
		disable_health = true
		return true
	end

	if type(definition) ~= "table" or
			definition.hud_elem_type ~= "statbar" then
		return false
	end

	if hud_name == "breath" then
		breath_bar_definition = definition

		for name, ids in pairs(hud_ids) do
			local player = core.get_player_by_name(name)
			if player and ids.id_breathbar then
				player:hud_remove(ids.id_breathbar)
				ids.id_breathbar = nil
				update_builtin_statbars(player)
			end
		end
		return true
	end

	return false
end

-- Append "update_builtin_statbars" as late as possible
-- This ensures that the HUD is hidden when the flags are updated in this callback
if enable_damage then
	core.register_on_mods_loaded(function()
		if not disable_health then
			hud.register("health", health_bar_definition)
			core.register_on_joinplayer(update_builtin_statbars)
		end
	end)
	core.register_on_leaveplayer(cleanup_builtin_statbars)
	core.register_playerevent(player_event_handler)
end
