--Minetest
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

--------------------------------------------------------------------------------

local labels = {
	leaves = {
		fgettext("Opaque Leaves"),
		fgettext("Simple Leaves"),
		fgettext("Fancy Leaves")
	},
	node_highlighting = {
		fgettext("Node Outlining"),
		fgettext("Node Highlighting"),
		fgettext("None")
	},
	filters = {
		fgettext("No Filter"),
		fgettext("Bilinear Filter"),
		fgettext("Trilinear Filter")
	},
	mipmap = {
		fgettext("No Mipmap"),
		fgettext("Mipmap"),
		fgettext("Mipmap + Aniso. Filter")
	},
	antialiasing = {
		fgettext("None"),
		fgettext("2x"),
		fgettext("4x"),
		fgettext("8x")
	}
}

local dd_options = {
	leaves = {"opaque", "simple", "fancy"},
	node_highlighting = {"box", "halo", "none"},
	filters = {"", "bilinear_filter", "trilinear_filter"},
	mipmap = {"", "mip_map", "anisotropic_filter"},
	antialiasing = {"0", "2", "4", "8"}
}

local getSettingIndex = {
	Leaves = function()
		local style = core.settings:get("leaves_style")
		for idx, name in pairs(dd_options.leaves) do
			if style == name then return idx end
		end
		return 1
	end,
	NodeHighlighting = function()
		local style = core.settings:get("node_highlighting")
		for idx, name in pairs(dd_options.node_highlighting) do
			if style == name then return idx end
		end
		return 1
	end,
	Filter = function()
		if core.settings:get(dd_options.filters[3]) == "true" then
			return 3
		elseif core.settings:get(dd_options.filters[3]) == "false" and
				core.settings:get(dd_options.filters[2]) == "true" then
			return 2
		end
		return 1
	end,
	Mipmap = function()
		if core.settings:get(dd_options.mipmap[3]) == "true" then
			return 3
		elseif core.settings:get(dd_options.mipmap[3]) == "false" and
				core.settings:get(dd_options.mipmap[2]) == "true" then
			return 2
		end
		return 1
	end,
	Antialiasing = function()
		local antialiasing_setting = core.settings:get("fsaa")
		for i = 1, #dd_options.antialiasing do
			if antialiasing_setting == dd_options.antialiasing[i] then
				return i
			end
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
local function formspec(tabview, name, tabdata)
	-- Dropdowns must be added last and in reverse order
	local dropdowns = {}

	local fs = {
		"formspec_version[4]",
		"real_coordinates[true]",
		"background9[0.625,0.6;4.7,5.2;", defaulttexturedir_esc, "desc_bg.png;false;32]",
		setting_cb(0.825, 1, "smooth_lighting", fgettext("Smooth Lighting")),
		setting_cb(0.825, 1.6, "enable_particles", fgettext("Particles")),
		setting_cb(0.825, 2.2, "enable_3d_clouds", fgettext("3D Clouds")),
		setting_cb(0.825, 2.8, "opaque_water", fgettext("Opaque Water")),
		setting_cb(0.825, 3.4, "connected_glass", fgettext("Connected Glass")),
	}

	dropdowns[#dropdowns + 1] = get_dropdown(0.825, 3.8, 4.3, "dd_node_highlighting",
			labels.node_highlighting, getSettingIndex.NodeHighlighting(),
			open_dropdown == "dd_node_highlighting")
	dropdowns[#dropdowns + 1] = get_dropdown(0.825, 4.8, 4.3, "dd_leaves_style",
			labels.leaves, getSettingIndex.Leaves(),
			open_dropdown == "dd_leaves_style")

	table.insert_all(fs, {
		"background9[5.6875,0.6;4.7,5.2;", defaulttexturedir_esc, "desc_bg.png;false;32]",
		"label[5.8875,1;", fgettext("Texturing:"), "]",
	})

	dropdowns[#dropdowns + 1] = get_dropdown(5.8875, 1.3, 4.3, "dd_filters",
		labels.filters, getSettingIndex.Filter(), open_dropdown == "dd_filters")
	dropdowns[#dropdowns + 1] = get_dropdown(5.8875, 2.3, 4.3, "dd_mipmap",
		labels.mipmap, getSettingIndex.Mipmap(), open_dropdown == "dd_mipmap")

	fs[#fs + 1] = "label[5.8875,3.4;" .. fgettext("Antialiasing:") .. "]"
	dropdowns[#dropdowns + 1] = get_dropdown(5.8875, 3.7, 4.3, "dd_antialiasing",
		labels.antialiasing, getSettingIndex.Antialiasing(), open_dropdown == "dd_antialiasing")

	table.insert_all(fs, {
		"label[5.8875,4.8;", fgettext("Screen:"), "]",
		checkbox(5.8875, 5.3, "cb_autosave_screensize", fgettext("Autosave Screen Size"),
			core.settings:get_bool("autosave_screensize")),
		"background9[10.75,0.6;4.7,5.2;", defaulttexturedir_esc, "desc_bg.png;false;32]",
	})

	local shader_y = 1
	if core.settings:get("touchscreen_threshold") ~= nil then
		fs[#fs + 1] = "label[10.95,1;" .. fgettext("Touchthreshold: (px)") .. "]"
		dropdowns[#dropdowns + 1] = get_dropdown(10.95, 1.3, 4.3, "dd_touchthreshold",
				{"0", "10", "20", "30", "40", "50"},
				(tonumber(core.settings:get("touchscreen_threshold")) / 10) + 1,
				open_dropdown == "dd_touchthreshold")
		shader_y = 2.9
	end

	local video_driver = core.settings:get("video_driver")
	local shaders_enabled = core.settings:get_bool("enable_shaders")
	if video_driver == "opengl" then
		fs[#fs + 1] = checkbox(10.95, shader_y, "cb_shaders", fgettext("Shaders"), shaders_enabled)
	elseif video_driver == "ogles2" then
		fs[#fs + 1] = checkbox(10.95, shader_y, "cb_shaders", fgettext("Shaders (experimental)"), shaders_enabled)
	else
		core.settings:set_bool("enable_shaders", false)
		shaders_enabled = false
		fs[#fs + 1] = disabled_cb(10.95, shader_y, nil, fgettext("Shaders (unavailable)"))
	end

	table.insert_all(fs, {
		btn_style("btn_change_keys"),
		"button[10.75,6.1;4.7,0.8;btn_change_keys;", fgettext("Change Keys"), "]",

		btn_style("btn_advanced_settings"),
		"button[0.625,6.1;4.7,0.8;btn_advanced_settings;", fgettext("All Settings"), "]",
	})

	local shader_cb = shaders_enabled and setting_cb or disabled_cb
	table.insert_all(fs, {
		shader_cb(10.95, shader_y + 0.6, "cb_tonemapping", fgettext("Tone Mapping"),
				core.settings:get_bool("tone_mapping")),
		shader_cb(10.95, shader_y + 1.2, "cb_waving_water", fgettext("Waving Liquids"),
				core.settings:get_bool("enable_waving_water")),
		shader_cb(10.95, shader_y + 1.8, "cb_waving_leaves", fgettext("Waving Leaves"),
				core.settings:get_bool("enable_waving_leaves")),
		shader_cb(10.95, shader_y + 2.4, "cb_waving_plants", fgettext("Waving Plants"),
				core.settings:get_bool("enable_waving_plants")),
	})

	-- Add the dropdowns in reverse order
	for i = #dropdowns, 1, -1 do
		fs[#fs + 1] = dropdowns[i]
	end
	fs[#fs + 1] = "real_coordinates[false]"

	return table.concat(fs)
end

--------------------------------------------------------------------------------
local function handle_settings_buttons(this, fields, tabname, tabdata)

	if fields["btn_advanced_settings"] ~= nil then
		local adv_settings_dlg = create_adv_settings_dlg()
		adv_settings_dlg:set_parent(this)
		this:hide()
		adv_settings_dlg:show()
		--mm_texture.update("singleplayer", current_game())
		return true
	end

	if fields["cb_shaders"] then
		if (core.settings:get("video_driver") == "direct3d8" or
				core.settings:get("video_driver") == "direct3d9") then
			core.settings:set("enable_shaders", "false")
			gamedata.errormessage = fgettext("To enable shaders the OpenGL driver needs to be used.")
		else
			core.settings:set_bool("enable_shaders", not core.settings:get_bool("enable_shaders"))
		end
		return true
	end

	for field in pairs(fields) do
		if field:sub(1, 3) == "cb_" then
			-- Checkboxes
			local setting_name = field:sub(4)
			core.settings:set_bool(setting_name, not core.settings:get_bool(setting_name))
			print('Updated', setting_name)
			return true
		elseif field:sub(1, 3) == "dd_" then
			-- Dropdown buttons
			open_dropdown = field
			return true
		elseif open_dropdown and field:sub(1, 9) == "dropdown_" then
			-- Dropdown fields
			local i = tonumber(field:sub(10))
			if i then
				if open_dropdown == "dd_leaves_style" then
					core.settings:set("leaves_style", dd_options.leaves[i])
				elseif open_dropdown == "dd_node_highlighting" then
					core.settings:set("node_highlighting", dd_options.node_highlighting[i])
				elseif open_dropdown == "dd_filters" then
					core.settings:set_bool("bilinear_filter", i == 2)
					core.settings:set_bool("trilinear_filter", i == 3)
				elseif open_dropdown == "dd_mipmap" then
					core.settings:set_bool("mip_map", i >= 2)
					core.settings:set_bool("anisotropic_filter", i >= 3)
				elseif open_dropdown == "dd_antialiasing" then
					core.settings:set("fsaa", dd_options.antialiasing[i])
				elseif open_dropdown == "dd_touchthreshold" then
					core.settings:set("touchscreen_threshold", (i - 1) * 10)
				end
			end

			open_dropdown = nil
			return true
		end
	end

	if fields["btn_change_keys"] then
		core.show_keys_menu()
		return true
	end
end

return {
	name = "settings",
	caption = "", -- fgettext("Settings"),
	cbf_formspec = formspec,
	cbf_button_handler = handle_settings_buttons
}
