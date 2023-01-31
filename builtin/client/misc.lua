function core.setting_get_pos(name)
	local value = core.settings:get(name)
	if not value then
		return nil
	end
	return core.string_to_pos(value)
end

local function register_toggle_cmd(name, priv, setting)
	core.register_chatcommand(priv, {
		description = "Toggles " .. name,
		func = function()
			if core.settings:get_bool(setting) then
				core.settings:set_bool(setting, false)
				return true, "Disabled " .. name
			elseif core.get_privilege_list()[priv] then
				core.settings:set_bool(setting, true)
				return true, "Enabled " .. name
			end
			return false, "You do not have the " .. priv .. " privilege."
		end,
	})
end

register_toggle_cmd("fast mode", "fast", "fast_move")
register_toggle_cmd("noclip mode", "noclip", "noclip")
