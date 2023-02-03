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
				return true, core.gettext(name .. " disabled")
			end

			core.settings:set_bool(setting, true)
			local msg = name .. " enabled"
			if not no_priv and not core.get_privilege_list()[cmd] then
				msg = ("%s (note: no '%s' privilege)"):format(msg, cmd)
			end
			return true, core.gettext(msg)
		end,
	})
end

register_toggle_cmd("Fast mode", "fast", "fast_move")
register_toggle_cmd("Fly mode", "fly", "free_move")
register_toggle_cmd("Noclip mode", "noclip", "noclip")
register_toggle_cmd("Pitch move mode", "pitch", "pitch_move", true)
