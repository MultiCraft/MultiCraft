--[[
Minetest
Copyright (C) 2018-2020 SmallJoker
Copyright (C) 2022 MultiCraft Development Team

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

local update_download_url -- Filled by HTTP callback

local defaulttexturedir = core.formspec_escape(defaulttexturedir)

local function version_info_formspec(data)
	return ([[
		style_type[image_button;content_offset=0]
		image[4.9,0;2.5,2.5;%s]
		image_button[1,2.2;10,0.8;%s;;%s;false;false]
		hypertext[1.3,3;10,1.5;;<center>%s</center>]
		style[version_check_remind;bgcolor=yellow]
		button[2,4.5;4,0.8;version_check_remind;%s]
		style[version_check_visit;bgcolor=green]
		button[6,4.5;4,0.8;version_check_visit;%s]
	]]):format(
		defaulttexturedir .. "logo.png",
		defaulttexturedir .. "blank.png",
		fgettext("A new MultiCraft version is available!"),
		core.formspec_escape(data.changelog),
		fgettext("Remind me later"),
		fgettext("Update now")
	)
end

local function version_info_buttonhandler(this, fields)
	if fields.version_check_remind then
		-- Only wait for the check interval
		core.settings:set("update_last_known", "")
		this:delete()
		return true
	end
	-- if fields.version_check_never then
	-- 	core.settings:set("update_last_known", "disabled")
	-- 	this:delete()
	-- 	return true
	-- end
	if fields.version_check_visit then
		core.open_url(update_download_url)
		-- this:delete()
		-- return true
	end

	return false
end


function create_version_info_dlg(changelog)
	assert(type(changelog) == "string")

	local retval = dialog_create("version_info",
		version_info_formspec,
		version_info_buttonhandler,
		nil, true)
	retval.data.changelog = changelog

	return retval
end

local function get_version_code(version_string)
	-- Format: Major.Minor.Patch
	-- Convert to MMMNNNPPP
	local ver_major, ver_minor, ver_patch = version_string:match("^(%d+).(%d+).(%d+)")

	if not ver_patch then
		core.log("error", "Failed to read version numbers (invalid tag format?)")
		return
	end

	return (ver_major * 1000 + ver_minor) * 1000 + ver_patch
end

local function on_version_info_received(update_info)
	local known_update = tonumber(core.settings:get("update_last_known")) or 0

	-- Parse the latest version number
	local new_number = get_version_code(update_info.latest_version or "")
	if not new_number then
		core.log("error", "dlg_version_info.lua: Failed to read version number")
		return
	end

	-- Compare with the current number
	local client_version = core.get_version()
	local cur_number = get_version_code(client_version.string)
	if not cur_number or new_number <= known_update or new_number < cur_number then
		return
	end

	-- Also consider updating from 1.2.3-dev to 1.2.3
	if new_number == cur_number and not client_version.is_dev then
		return
	end

--	core.settings:set("update_last_known", tostring(new_number))

	local tabs = ui.find_by_name("maintab")
	tabs:hide()

	update_download_url = update_info.url or "https://multicraft.world/downloads"
	local version_info_dlg = create_version_info_dlg(update_info.changelog or "")
	version_info_dlg:set_parent(tabs)
	version_info_dlg:show()

	ui.update()
end

function check_new_version()
	local url = core.settings:get("update_information_url")
	if core.settings:get("update_last_known") == "disabled" or
			url == "" then
		-- Never show any updates
		return
	end

	local time_now = os.time()
	local time_checked = tonumber(core.settings:get("update_last_checked")) or 0
	if time_now - time_checked < 2 * 24 * 3600 then
		-- Check interval of 2 entire days
		return
	end

	core.settings:set("update_last_checked", tostring(time_now))

	core.handle_async(function(params)
		local http = core.get_http_api()
		return http.fetch_sync(params)
	end, { url = url }, function(result)
		local json = result.succeeded and core.parse_json(result.data)
		if type(json) ~= "table" then
			core.log("error", "Failed to read JSON output from " .. url ..
					", status code = " .. result.code)
			return
		elseif not json[PLATFORM] or not json[PLATFORM].latest_version then
			-- No platform-specific update information, don't display anything
			return
		end

		on_version_info_received(json[PLATFORM])
	end)
end
