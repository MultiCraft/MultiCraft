--Minetest
--Copyright (C) 2020-2022 MultiCraft Development Team
--Copyright (C) 2013 sapier
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

local function create_confirm_reset_dlg()
	return dialog_create("reset_all_settings",
		function()
			return table.concat({
				"real_coordinates[true]",
				"image[6.5,0.8;2.5,2.5;", defaulttexturedir_esc, "attention.png]",

				"style[msg;content_offset=0]",
				"image_button[1,3.5;13.5,0.8;;msg;",
					fgettext("Reset all settings?"), ";false;false]",

				btn_style("reset_confirm", "red"),
				"image_button[4.1,5.3;3.5,0.8;;reset_confirm;",
					fgettext("Reset"), ";true;false]",

				btn_style("reset_cancel"),
				"image_button[7.9,5.3;3.5,0.8;;reset_cancel;",
					fgettext("Cancel"), ";true;false]",
			})
		end,
		function(this, fields)
			if fields["reset_confirm"] then
				for _, setting_name in ipairs(core.settings:get_names()) do
					if not setting_name:find(".", 1, true) and
							setting_name ~= "maintab_LAST" then
						core.settings:remove(setting_name)
					end
				end

				-- Reload the entire main menu
				dofile(core.get_builtin_path() .. "init.lua")
				return true
			end

			if fields["reset_cancel"] then
				this:delete()
				return true
			end
		end,
		nil, true)
end

--------------------------------------------------------------------------------

local languages, lang_idx, language_labels = get_language_list()

local node_highlighting_labels = {
	fgettext("Node Outlining"),
	fgettext("Node Highlighting"),
	fgettext("None")
}

local fps_max_labels = {"30", "60", "90", [-1] = "45"}

local dd_options = {
	-- "30 FPS" actually sets 35 FPS for some reason
	fps_max = {"35", "60", "90"},
	language = languages,
	node_highlighting = {"box", "halo", "none"},
	viewing_range = {"30", "40", "60", "80", "100", "125", "150", "175", "200"},
}

local getSettingIndex = {
	NodeHighlighting = function()
		local style = core.settings:get("node_highlighting")
		for idx, name in pairs(dd_options.node_highlighting) do
			if style == name then return idx end
		end
		return 1
	end
}

local function setting_cb(x, y, setting, label)
	return checkbox(x, y, "cb_" .. setting, label, core.settings:get_bool(setting), true)
end

local function disabled_cb(x, y, _, label)
	return ("label[%s,%s;%s]"):format(x + 0.6, y, core.colorize("#888", label))
end

local open_dropdown
local guitexturedir = defaulttexturedir_esc .. "gui" .. DIR_DELIM_esc
local function formspec(tabview, name, tabdata)
	local fps = tonumber(core.settings:get("fps_max"))
	local range = tonumber(core.settings:get("viewing_range"))
	local sensitivity = tonumber(core.settings:get("touch_sensitivity") or 0) * 2000
	local touchtarget = core.settings:get_bool("touchtarget", false)
	local fancy_leaves = core.settings:get("leaves_style") == "fancy"
	local sound = tonumber(core.settings:get("sound_volume")) ~= 0

	local video_driver = core.settings:get("video_driver")
	local shaders_enabled = video_driver == "opengl" or video_driver == "ogles2"
	core.settings:set_bool("enable_shaders", shaders_enabled)

	local open_dropdown_fs
	local function dropdown(x, y, w, name, items, selected_idx, max_items, container_pos)
		local dd = get_dropdown(x, y, w, name, items, selected_idx, open_dropdown == name, max_items)
		if open_dropdown == name then
			open_dropdown_fs = dd
			-- Items positioned inside scroll containers are very slightly
			-- offset from the same item in a regular container
			if container_pos then
				open_dropdown_fs = "scroll_container[" .. container_pos .. ";" .. w + x + 1 .. ",10;;vertical;0]" ..
					open_dropdown_fs ..
					"scroll_container_end[]"
			end
			return ""
		end
		return dd
	end

	local shader_cb = shaders_enabled and setting_cb or disabled_cb
	local fs = {
		"formspec_version[4]",
		"real_coordinates[true]",
		"background9[0.5,0.5;4.8,6.4;", defaulttexturedir_esc, "desc_bg.png;false;32]",

		-- A scroll container is used so that long labels are clipped
		"scroll_container[0.5,0.5;4.8,6.4;;vertical;0]",

		setting_cb(0.3, 0.5, "smooth_lighting", fgettext("Smooth Lighting")),
		setting_cb(0.3, 1.175, "enable_particles", fgettext("Particles")),
		setting_cb(0.3, 1.85, "enable_3d_clouds", fgettext("3D Clouds")),
		-- setting_cb(0.3, y, "opaque_water", fgettext("Opaque Water")),
		-- setting_cb(0.3, y, "connected_glass", fgettext("Connected Glass")),
		setting_cb(0.3, 2.525, "enable_fog", fgettext("Fog")),
		setting_cb(0.3, 3.2, "inventory_items_animations", fgettext("Inv. animations")),

		-- Some checkboxes don't directly have a boolean setting so they need
		-- to be handled separately
		checkbox(0.3, 3.875, "fancy_leaves", fgettext("Fancy Leaves"), fancy_leaves, true),
		checkbox(0.3, 4.55, "crosshair", fgettext("Crosshair"), not touchtarget, true),
		setting_cb(0.3, 5.225, "arm_inertia", fgettext("Arm inertia")),
		checkbox(0.3, 5.9, "sound", fgettext("Sound"), sound, true),

		"scroll_container_end[]",

		-- Middle column
		"background9[5.6,0.5;4.8,6.4;", defaulttexturedir_esc, "desc_bg.png;false;32]",
		"scroll_container[5.6,0.5;4.8,6.4;;vertical;0]",
		"label[0.3,0.5;", fgettext("Maximum FPS"), ":]",
		dropdown(0.3, 0.8, 4.2, "dd_fps_max", fps_max_labels,
			fps <= 35 and 1 or fps == 45 and -1 or fps == 60 and 2 or 3, nil, "5.6,0.5"),

		"label[0.3,2;", fgettext("Viewing range"), ":]",
		dropdown(0.3, 2.3, 4.2, "dd_viewing_range", dd_options.viewing_range,
			range <= 30 and 1 or range == 40 and 2 or range == 60 and 3 or
			range == 80 and 4 or range == 100 and 5 or range == 125 and 6 or
			range == 150 and 7 or range == 175 and 8 or 9, 4.5, "5.6,0.5"),

		"label[0.3,3.5;", fgettext("Node highlighting"), ":]",
		dropdown(0.3, 3.8, 4.2, "dd_node_highlighting", node_highlighting_labels,
			getSettingIndex.NodeHighlighting(), nil, "5.6,0.5"),

		"label[0.3,5;", fgettext("Mouse sensitivity"), ":]",
		"scrollbar[0.3,5.3;4.2,0.8;horizontal;sb_sensitivity;", tostring(sensitivity), ";",
			guitexturedir, "scrollbar_horiz_bg.png,", guitexturedir, "scrollbar_slider.png,",
			guitexturedir, "scrollbar_minus.png,", guitexturedir, "scrollbar_plus.png]",
		"scroll_container_end[]",

		-- Right column
		"background9[10.7,0.5;4.8,1.9;", defaulttexturedir_esc, "desc_bg.png;false;32]",
		"label[11,1;", fgettext("Language"), ":]",
		dropdown(11, 1.3, 4.2, "dd_language", language_labels, lang_idx, 6.4),

		"background9[10.7,2.6;4.8,4.3;", defaulttexturedir_esc, "desc_bg.png;false;32]",
		"scroll_container[10.7,2.6;4.8,4.3;;vertical;0]",
		"label[0.3,0.5;", shaders_enabled and fgettext("Shaders") or
			core.colorize("#888888", fgettext("Shaders (unavailable)")), "]",
		shader_cb(0.3, 1, "tone_mapping", fgettext("Tone Mapping")),
		shader_cb(0.3, 1.6, "enable_waving_water", fgettext("Waving liquids")),
		shader_cb(0.3, 2.2, "enable_waving_leaves", fgettext("Waving leaves")),
		shader_cb(0.3, 2.8, "enable_waving_plants", fgettext("Waving plants")),
		"scroll_container_end[]",

		btn_style("btn_reset"),
		"button[11,5.8;4.2,0.8;btn_reset;", fgettext("Reset all settings"), "]",
	}

	-- Show the open dropdown (if any) last
	fs[#fs + 1] = open_dropdown_fs
	fs[#fs + 1] = "real_coordinates[false]"

	return table.concat(fs)
end

--------------------------------------------------------------------------------
local function handle_settings_buttons(this, fields, tabname, tabdata)
--[[if fields["btn_advanced_settings"] ~= nil then
		local adv_settings_dlg = create_adv_settings_dlg()
		adv_settings_dlg:set_parent(this)
		this:hide()
		adv_settings_dlg:show()
		return true
	end]]

	for field in pairs(fields) do
		if field:sub(1, 3) == "cb_" then
			-- Checkboxes
			local setting_name = field:sub(4)
			core.settings:set_bool(setting_name, not core.settings:get_bool(setting_name))
			return true
		elseif field:sub(1, 3) == "dd_" then
			-- Dropdown buttons
			open_dropdown = field
			return true
		elseif open_dropdown and field:sub(1, 9) == "dropdown_" then
			-- Dropdown fields
			local i = tonumber(field:sub(10))
			local setting = open_dropdown:sub(4)
			if i and dd_options[setting] then
				core.settings:set(setting, dd_options[setting][i])

				-- Reload the main menu so that everything uses the new language
				if setting == "language" then
					dofile(core.get_builtin_path() .. "init.lua")
				end
			end

			open_dropdown = nil
			return true
		end
	end

	-- Special checkboxes
	if fields["fancy_leaves"] then
		core.settings:set("leaves_style", core.settings:get("leaves_style") == "fancy" and "opaque" or "fancy")
		return true
	end
	if fields["crosshair"] then
		core.settings:set_bool("touchtarget", not core.settings:get_bool("touchtarget"))
		return true
	end
	if fields["sound"] then
		core.settings:set("sound_volume", tonumber(core.settings:get("sound_volume")) == 0 and "1.0" or "0.0")
		return true
	end

--[[if fields["btn_change_keys"] then
		core.show_keys_menu()
		return true
	end]]

	if fields["btn_reset"] then
		local reset_dlg = create_confirm_reset_dlg()
		reset_dlg:set_parent(this)
		this:hide()
		reset_dlg:show()
		return true
	end

	if fields["sb_sensitivity"] then
		-- reset old setting
		core.settings:remove("touchscreen_threshold")

		local event = core.explode_scrollbar_event(fields["sb_sensitivity"])
		if event.type == "CHG" then
			core.settings:set("touch_sensitivity", event.value / 2000)
		end
	end
end

return {
	name = "settings",
	caption = "", -- fgettext("Settings"),
	cbf_formspec = formspec,
	cbf_button_handler = handle_settings_buttons
}
