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
			return
				"image[4.9,0.3;2.5,2.5;" .. core.formspec_escape(defaulttexturedir ..
					"attention.png") .. "]" ..
				"image_button[1,2.85;10,0.8;" .. core.formspec_escape(defaulttexturedir ..
					"blank.png") .. ";;" .. fgettext("Reset all settings?") ..
					";true;false;]" ..
				btn_style("reset_confirm", "red") ..
				"button[3,4.8;3,0.5;reset_confirm;" .. fgettext("Reset") .. "]" ..
				btn_style("reset_cancel") ..
				"button[6,4.8;3,0.5;reset_cancel;" .. fgettext("Cancel") .. "]"
		end,
		function(this, fields)
			if fields["reset_confirm"] then
				for _, setting_name in ipairs(core.settings:get_names()) do
					if not setting_name:find(".", 1, true) and
							setting_name ~= "language" and
							setting_name ~= "maintab_LAST" then
						core.settings:remove(setting_name)
					end
				end

				core.settings:set("language", os.getenv("LANG"))

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

local labels = {
	node_highlighting = {
		fgettext("Node Outlining"),
		fgettext("Node Highlighting"),
		fgettext("None")
	}
}

local dd_options = {
	node_highlighting = {
		table.concat(labels.node_highlighting, ","),
		{"box", "halo", "none"}
	}
}

local getSettingIndex = {
	NodeHighlighting = function()
		local style = core.settings:get("node_highlighting")
		for idx, name in pairs(dd_options.node_highlighting[2]) do
			if style == name then return idx end
		end
		return 1
	end
}

-- Get a list of languages and language names
local path_locale = core.get_builtin_path() .. ".." .. DIR_DELIM .. "locale"
local languages = core.get_dir_list(path_locale, true)
local language_names = {}
for i = #languages, 1, -1 do
	local language = languages[i]
	local f = io.open(path_locale .. DIR_DELIM .. language .. DIR_DELIM ..
					  "LC_MESSAGES" .. DIR_DELIM .. "minetest.mo")
	if f then
		-- HACK
		local name = f:read("*a"):match("\nLanguage%-Team: ([^\\\n\"]+) <https://")
		language_names[language] = name or language
		f:close()
	else
		table.remove(languages, i)
	end
end

languages[#languages + 1] = "en"
language_names.en = "English"

-- Sort the languages list based on their human readable name
table.sort(languages, function(a, b)
	return language_names[a] < language_names[b]
end)

local language_name_list = {}
for i, language in ipairs(languages) do
	language_name_list[i] = core.formspec_escape(language_names[language])
end
local language_dropdown = table.concat(language_name_list, ",")

local lang_idx = table.indexof(languages, fgettext("LANG_CODE"))
if lang_idx < 0 then
	lang_idx = table.indexof(languages, "en")
end

local function formspec(tabview, name, tabdata)
	local fps = tonumber(core.settings:get("fps_max"))
	local range = tonumber(core.settings:get("viewing_range"))
	local sensitivity = tonumber(core.settings:get("mouse_sensitivity")) * 2000
	local touchtarget = core.settings:get_bool("touchtarget") or false
	local fancy_leaves = core.settings:get("leaves_style") == "fancy"
	local arm_inertia = core.settings:get_bool("arm_inertia") or false
	local sound = tonumber(core.settings:get("sound_volume")) ~= 0 and true or false

	local tab_string =
		"box[-0.1,0;3.85,5.5;#999999]" ..
		"checkbox[0.15,-0.05;cb_smooth_lighting;" .. fgettext("Smooth Lighting") .. ";"
				.. dump(core.settings:get_bool("smooth_lighting")) .. "]" ..
		"checkbox[0.15,0.5;cb_particles;" .. fgettext("Particles") .. ";"
				.. dump(core.settings:get_bool("enable_particles")) .. "]" ..
		"checkbox[0.15,1.1;cb_3d_clouds;" .. fgettext("3D Clouds") .. ";"
				.. dump(core.settings:get_bool("enable_3d_clouds")) .. "]" ..
	--[["checkbox[0.15,1.7;cb_opaque_water;" .. fgettext("Opaque Water") .. ";"
				.. dump(core.settings:get_bool("opaque_water")) .. "]" ..
		"checkbox[0.15,2.0;cb_connected_glass;" .. fgettext("Connected Glass") .. ";"
				.. dump(core.settings:get_bool("connected_glass")) .. "]" ..]]
		"checkbox[0.15,1.7;cb_fog;" .. fgettext("Fog") .. ";"
				.. dump(core.settings:get_bool("enable_fog")) .. "]" ..
		"checkbox[0.15,2.3;cb_inventory_items_animations;" .. fgettext("Inv. animations") .. ";"
				.. dump(core.settings:get_bool("inventory_items_animations")) .. "]" ..
		"checkbox[0.15,2.9;cb_fancy_leaves;" .. fgettext("Fancy Leaves") .. ";"
				.. dump(fancy_leaves) .. "]" ..
		"checkbox[0.15,3.5;cb_touchtarget;" .. fgettext("Touchtarget") .. ";"
				.. dump(touchtarget) .. "]" ..
		"checkbox[0.15,4.1;cb_arm_inertia;" .. fgettext("Arm inertia") .. ";"
			.. dump(arm_inertia) .. "]" ..
		"checkbox[0.15,4.7;cb_sound;" .. fgettext("Sound") .. ";"
				.. dump(sound) .. "]" ..

		"box[4,0;3.75,5.5;#999999]" ..

		"label[4.25,0.15;" .. fgettext("Maximum FPS") .. ":]" ..
		"dropdown[4.25,0.6;3.5;dd_fps_max;30,35,45,60;" ..
			tonumber(fps <= 30 and 1 or fps == 35 and 2 or fps == 45 and 3 or 4) .. "]" ..

		"label[4.25,1.5;" .. fgettext("Viewing range") .. ":]" ..
		"dropdown[4.25,1.95;3.5;dd_viewing_range;30,40,60,80,100,125,150,175,200;" ..
			tonumber(range <= 30 and 1 or range == 40 and 2 or range == 60 and 3 or
			range == 80 and 4 or range == 100 and 5 or range == 125 and 6 or
			range == 150 and 7 or range == 175 and 8 or 9) .. "]" ..

		"label[4.25,2.85;" .. fgettext("Node highlighting") .. ":]" ..
		"dropdown[4.25,3.3;3.5;dd_node_highlighting;" .. dd_options.node_highlighting[1] .. ";"
				.. getSettingIndex.NodeHighlighting() .. "]" ..

		"label[4.25,4.2;" .. fgettext("Mouse sensitivity") .. ":]" ..
		"scrollbar[4.25,4.65;3.23,0.5;horizontal;sb_sensitivity;" .. sensitivity .. "]" ..

		"box[8,0;3.85,3.25;#999999]"

	local video_driver = core.settings:get("video_driver")
	local shaders_enabled = video_driver == "opengl" or video_driver == "ogles2"
	core.settings:set_bool("enable_shaders", shaders_enabled)
	if shaders_enabled then
		tab_string = tab_string ..
			"label[8.25,0.15;" .. fgettext("Shaders") .. "]"
	else
		tab_string = tab_string ..
			"label[8.25,0.15;" .. core.colorize("#888888",
					fgettext("Shaders (unavailable)")) .. "]"
	end

--[[tab_string = tab_string ..
		btn_style("btn_change_keys") ..
		"button[8,3.22;3.95,1;btn_change_keys;"
		.. fgettext("Change Keys") .. "]"

	tab_string = tab_string ..
		btn_style("btn_advanced_settings") ..
		"button[8,4.57;3.95,1;btn_advanced_settings;"
		.. fgettext("All Settings") .. "]"]]

	if shaders_enabled then
		tab_string = tab_string ..
			"checkbox[8.25,0.55;cb_tonemapping;" .. fgettext("Tone Mapping") .. ";"
					.. dump(core.settings:get_bool("tone_mapping")) .. "]" ..
			"checkbox[8.25,1.15;cb_waving_water;" .. fgettext("Waving liquids") .. ";"
					.. dump(core.settings:get_bool("enable_waving_water")) .. "]" ..
			"checkbox[8.25,1.75;cb_waving_leaves;" .. fgettext("Waving leaves") .. ";"
					.. dump(core.settings:get_bool("enable_waving_leaves")) .. "]" ..
			"checkbox[8.25,2.35;cb_waving_plants;" .. fgettext("Waving plants") .. ";"
					.. dump(core.settings:get_bool("enable_waving_plants")) .. "]"
	else
		tab_string = tab_string ..
			"label[8.38,0.75;" .. core.colorize("#888888",
					fgettext("Tone Mapping")) .. "]" ..
			"label[8.38,1.35;" .. core.colorize("#888888",
					fgettext("Waving Liquids")) .. "]" ..
			"label[8.38,1.95;" .. core.colorize("#888888",
					fgettext("Waving Leaves")) .. "]" ..
			"label[8.38,2.55;" .. core.colorize("#888888",
					fgettext("Waving Plants")) .. "]"
	end

	tab_string = tab_string ..
		"label[8.25,3.35;" .. fgettext("Language") .. ":]" ..
		"dropdown[8.25,3.8;3.58;dd_language;" .. language_dropdown .. ";" ..
			lang_idx .. ";true]" ..
		btn_style("btn_reset") ..
		"button[8.25,4.81;3.5,0.8;btn_reset;" .. fgettext("Reset all settings") .. "]"

	return tab_string
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
	if fields["cb_smooth_lighting"] then
		core.settings:set("smooth_lighting", fields["cb_smooth_lighting"])
		return true
	end
	if fields["cb_particles"] then
		core.settings:set("enable_particles", fields["cb_particles"])
		return true
	end
	if fields["cb_3d_clouds"] then
		core.settings:set("enable_3d_clouds", fields["cb_3d_clouds"])
		return true
	end
	if fields["cb_opaque_water"] then
		core.settings:set("opaque_water", fields["cb_opaque_water"])
		return true
	end
--[[if fields["cb_connected_glass"] then
		core.settings:set("connected_glass", fields["cb_connected_glass"])
		return true
	end]]
	if fields["cb_fog"] then
		core.settings:set("enable_fog", fields["cb_fog"])
		return true
	end
	if fields["cb_inventory_items_animations"] then
		core.settings:set("inventory_items_animations", fields["cb_inventory_items_animations"])
		return true
	end
	if fields["cb_fancy_leaves"] then
		core.settings:set("leaves_style", fields["cb_fancy_leaves"] and "fancy" or "opaque")
		return true
	end
	if fields["cb_touchtarget"] then
		core.settings:set("touchtarget", fields["cb_touchtarget"])
		return true
	end
	if fields["cb_arm_inertia"] then
		core.settings:set("arm_inertia", fields["cb_arm_inertia"])
		return true
	end
	if fields["cb_sound"] then
		core.settings:set("sound_volume", (minetest.is_yes(fields["cb_sound"]) and "1.0") or "0.0")
		return true
	end
	if fields["cb_shaders"] then
		core.settings:set("enable_shaders", fields["cb_shaders"])
		return true
	end
	if fields["cb_tonemapping"] then
		core.settings:set("tone_mapping", fields["cb_tonemapping"])
		return true
	end
	if fields["cb_waving_water"] then
		core.settings:set("enable_waving_water", fields["cb_waving_water"])
		return true
	end
	if fields["cb_waving_leaves"] then
		core.settings:set("enable_waving_leaves", fields["cb_waving_leaves"])
	end
	if fields["cb_waving_plants"] then
		core.settings:set("enable_waving_plants", fields["cb_waving_plants"])
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

	-- Note dropdowns have to be handled LAST!
	local ddhandled = false

	if fields["cb_touchscreen_target"] then
		core.settings:set("touchtarget", fields["cb_touchscreen_target"])
		ddhandled = true
	end
	for i = 1, #labels.node_highlighting do
		if fields["dd_node_highlighting"] == labels.node_highlighting[i] then
			core.settings:set("node_highlighting", dd_options.node_highlighting[2][i])
			ddhandled = true
		end
	end
	if fields["dd_fps_max"] then
		core.settings:set("fps_max", fields["dd_fps_max"])
		ddhandled = true
	end
	if fields["dd_viewing_range"] then
		core.settings:set("viewing_range", fields["dd_viewing_range"])
		ddhandled = true
	end
	if fields["sb_sensitivity"] then
		-- reset old setting
		core.settings:remove("touchscreen_threshold")

		local event = core.explode_scrollbar_event(fields["sb_sensitivity"])
		if event.type == "CHG" then
			core.settings:set("mouse_sensitivity", event.value / 2000)

			-- The formspec cannot be updated or the scrollbar movement will
			-- break.
			ddhandled = false
		end
	end
	if fields["dd_language"] then
		local new_idx = tonumber(fields["dd_language"])
		if lang_idx ~= new_idx then
			core.settings:set("language", languages[new_idx] or "")
			ddhandled = true

			-- Reload the main menu so that everything uses the new language
			dofile(core.get_builtin_path() .. "init.lua")
		end
	end

	return ddhandled
end

return {
	name = "settings",
	caption = "", -- fgettext("Settings"),
	cbf_formspec = formspec,
	cbf_button_handler = handle_settings_buttons
}
