function core.setting_get_pos(name)
	local value = core.settings:get(name)
	if not value then
		return nil
	end
	return core.string_to_pos(value)
end

local function register_toggle_cmd(name, cmd, setting, no_priv)
	core.register_chatcommand(cmd, {
		description = "Toggles " .. name,
		func = function()
			if core.settings:get_bool(setting) then
				core.settings:set_bool(setting, false)
				return true, "Disabled " .. name
			elseif no_priv or core.get_privilege_list()[cmd] then
				core.settings:set_bool(setting, true)
				return true, "Enabled " .. name
			end
			return false, "You do not have the " .. cmd .. " privilege."
		end,
	})
end

register_toggle_cmd("fast mode", "fast", "fast_move")
register_toggle_cmd("noclip mode", "noclip", "noclip")
register_toggle_cmd("fly mode", "fly", "free_move")
register_toggle_cmd("pitch move mode", "pitch", "pitch_move", true)
