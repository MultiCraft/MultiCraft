-- MultiCraft: builtin/common/btn_style.lua
-- luacheck: read_globals PLATFORM

local fmt = string.format

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

local DIR_DELIM_esc = core.formspec_escape(DIR_DELIM)
local button_path = (INIT == "mainmenu" and defaulttexturedir_esc or "") .. "gui" .. DIR_DELIM_esc

function btn_style(field, color, no_padding)
	local high_dpi = is_high_dpi()
	local btn_size = high_dpi and ".x2" or ""
	color = (color and "_" .. color) or ""

	local bgimg_middle = high_dpi and 48 or 32
	local retval =
		"style[" .. field .. ";border=false]" ..
		"style[" .. field .. ";bgimg=" .. button_path .. "gui_button" .. color .. btn_size ..
			".png;bgimg_middle=" .. bgimg_middle .. ";padding=" ..
			(no_padding and -bgimg_middle or (high_dpi and -36 or -24)) .. "]"

	if color ~= "_gray" and color:sub(-8) ~= "_pressed" then
		retval = retval ..
			"style[" .. field .. ":hovered;bgimg=" .. button_path .. "gui_button" .. color .. "_hovered" .. btn_size ..
				".png]" ..
			"style[" .. field .. ":pressed;bgimg=" .. button_path .. "gui_button" .. color .. "_pressed" .. btn_size ..
				".png]"
	end

	return retval
end
--------------------------------------------------------------------------------
function get_dropdown(x, y, w, name, items, selected_idx, dropdown_open, max_items)
	local fs = {}
	fs[#fs + 1] = fmt("style[%s;bgimg=%s%s;bgimg_middle=32;padding=-24;border=false]",
		name, button_path, dropdown_open and "dropdown_open.png" or "dropdown.png")
	fs[#fs + 1] = fmt("button[%s,%s;%s,0.8;%s;%s]", x, y, w, name, items[selected_idx])
	fs[#fs + 1] = fmt("image[%s,%s;0.3375,0.225;%sdropdown_arrow.png]",
		x + w - 0.2 - 0.3375, y + 0.325, button_path)

	if dropdown_open then
		-- Make clicking outside of the dropdown close the menu
		fs[#fs + 1] = "image_button[-50,-50;100,100;;dropdown_close;;true;false]"

		max_items = max_items or 6
		local scroll_container = #items > max_items
		if scroll_container then
			fs[#fs + 1] = fmt("scroll_container[%s,%s;%s,%s;scrbar;vertical]", x, y + 0.79, w, 0.79 * max_items)
		else
			fs[#fs + 1] = fmt("container[%s,%s]", x, y + 0.79)
		end

		-- Add a button for each dropdown entry
		for i, entry in ipairs(items) do
			local btn_name = "dropdown_" .. i
			local suffix = i <= 6 and i == #items and "_end" or ""
			fs[#fs + 1] = fmt("style[%s;bgimg=%sdropdown_bg%s.png;bgimg_middle=32;padding=-24;border=false;noclip=%s]",
				btn_name, button_path, suffix, scroll_container and "false" or "true")
			fs[#fs + 1] = fmt("style[%s:hovered,%s:pressed;bgimg=%sdropdown_bg%s_hover.png]",
				btn_name, btn_name, button_path, suffix)

			-- 0.79 is used to prevent any 1px gaps between entries
			fs[#fs + 1] = fmt("button[0,%s;%s,0.8;%s;%s]", (i - 1) * 0.79, w, btn_name, entry)
		end

		if scroll_container then
			fs[#fs + 1] = "scroll_container_end[]"
			local scrollbar_max = math.ceil((#items - max_items) * 0.79 * 10)
			fs[#fs + 1] = fmt("scrollbaroptions[max=%s;thumbsize=%s]", scrollbar_max, scrollbar_max * 0.75)
			fs[#fs + 1] = fmt("scrollbar[%s,%s;0.7,%s;vertical;scrbar;0;" ..
				"%sscrollbar_bg.png,%sscrollbar_slider.png,%sscrollbar_up.png,%sscrollbar_down.png]",
				x + w - 0.76, y + 0.84, max_items * 0.79 - 0.11, button_path, button_path, button_path, button_path)
			fs[#fs + 1] = fmt("image[%s,%s;%s,0.79;%sdropdown_fg_end.png;32]", x, y + 0.79 * max_items + 0.02, w, button_path)
		else
			fs[#fs + 1] = "container_end[]"
		end
	end

	return table.concat(fs)
end
--------------------------------------------------------------------------------
function checkbox(x, y, name, label, checked, small)
	-- Note: checkbox[] sends a "true" or "false" value in fields but this
	-- doesn't, code will have to be changed to toggle the value and redraw the
	-- formspec instead
	return ([[
		image[%s,%.3f;0.4,0.4;%sgui%scheckbox%s.png]
		label[%s,%.3f;%s]
		image_button[%s,%.2f;%s,0.5;;%s;;false;false]
	]]):format(
		x, y - 0.2, defaulttexturedir_esc, DIR_DELIM_esc, checked and "_checked" or "",
		x + 0.6, y, label,
		x, y - 0.25, small and 4.3 or 7, name
	)
end
