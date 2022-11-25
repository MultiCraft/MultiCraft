-- MultiCraft: builtin/common/btn_style.lua

local device_is_tablet = core.settings:get_bool("device_is_tablet", false)
local screen_density = core.get_screen_info().density
function is_high_dpi()
	if PLATFORM == "OSX" then
		return tonumber(core.settings:get("screen_dpi")) / 72 >= 2
	elseif PLATFORM == "iOS" and device_is_tablet then
		return screen_density >= 2
	else
		return screen_density >= 3
	end
end
--------------------------------------------------------------------------------
local DIR_DELIM_esc = DIR_DELIM_esc or core.formspec_escape(DIR_DELIM)
local button_path = (INIT == "mainmenu" and defaulttexturedir_esc or "") .. "gui" .. DIR_DELIM_esc

function btn_style(field, color)
	local btn_size = is_high_dpi() and ".x2" or ""
	color = (color and "_" .. color) or ""

	local retval =
		"style[" .. field .. ";border=false]" ..
		"style[" .. field .. ";bgimg=" .. button_path .. "gui_button" .. color .. btn_size ..
			".png;bgimg_middle=" .. (is_high_dpi() and 48 or 32) .. ";padding=" .. (is_high_dpi() and -30 or -20) .. "]"

	if color ~= "_gray" then
		retval = retval ..
			"style[" .. field .. ":hovered;bgimg=" .. button_path .. "gui_button" .. color .. "_hovered" .. btn_size ..
				".png]" ..
			"style[" .. field .. ":pressed;bgimg=" .. button_path .. "gui_button" .. color .. "_pressed" .. btn_size ..
				".png]"
	end

	return retval
end
