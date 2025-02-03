--[[
Minetest
Copyright (C) 2018-2020 SmallJoker, 2022 rubenwardy
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

if not core.get_http_api then
	function check_new_version()
	end
	return
end

local esc = core.formspec_escape

local LANG = core.settings:get("language")
if not (LANG and (LANG ~= "")) then LANG = os.getenv("LANG") end
if not (LANG and (LANG ~= "")) then LANG = "en" end

local style_exists = core.global_exists("btn_style")
local function version_info_formspec(data)
	local changes = data.changes

	-- Hack to work around https://github.com/minetest/minetest/issues/11727
	if changes:sub(2, 2) == " " then
		local idx = changes:find("\n", 3, true) or #changes + 1
		changes = "\194\160" .. changes:sub(1, idx - 1) .. "\194\160" ..
			changes:sub(idx)
	end

	changes = changes:gsub("\\n", "\n")

	return ([[
		style_type[image_button;content_offset=0]
		image[4.9,0;2.5,2.5;%slogo.png]
		style[msg;content_offset=0]
		image_button[1,2;10,0.8;;msg;%s;false;false]
		hypertext[1.3,2.6;10,2;;<center>%s</center>]
		%s
		button[2,4.5;4,0.8;version_check_remind;%s]
		%s
		button[6,4.5;4,0.8;version_check_visit;%s]
	]]):format(
		defaulttexturedir_esc,
		esc(data.title),
		esc(changes),
		(style_exists and btn_style("version_check_remind", "yellow") or ""),
		fgettext("Cancel"),
		(style_exists and btn_style("version_check_visit", "green") or ""),
		fgettext("Update")
	) -- "Remind me later", "Update now"
end

local function version_info_buttonhandler(this, fields)
	if fields.version_check_remind then
		-- Erase last known, user will be reminded again at next check
		core.settings:remove("update_last_known")
		this:delete()
		return true
	end
	-- if fields.version_check_never then
	-- 	core.settings:set("update_last_checked", "disabled")
	-- 	this:delete()
	-- 	return true
	-- end
	if fields.version_check_visit then
		if type(this.data.url) == "string" then
			core.open_url(this.data.url)
		end
		-- this:delete()
		-- return true
	end

	return false
end

local function create_version_info_dlg(title, changes, url)
	assert(type(title) == "string")
	assert(type(changes) == "string")
	assert(type(url) == "string")

	local retval = dialog_create("version_info",
		version_info_formspec,
		version_info_buttonhandler,
		nil, true)

	retval.data.title = title
	retval.data.changes = changes
	retval.data.url = url

	return retval
end

local function get_version_code(version_string)
	-- Format: Major.Minor.Patch
	-- Convert to MMMNNNPPP
	local ver_major, ver_minor, ver_patch = version_string:match("^(%d+).(%d+).(%d+)")

	if not ver_patch then
		core.log("error", "Failed to parse version numbers (invalid tag format?)")
		return
	end

	return (ver_major * 1000 + ver_minor) * 1000 + ver_patch
end

local function on_version_info_received(update_info)
	local maintab = ui.find_by_name("maintab")
	if maintab.hidden then
		-- Another dialog is open, abort.
		return
	end

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

	-- core.settings:set("update_last_known", tostring(new_number))

	-- Show version info dialog (once)
	maintab:hide()

	local url = update_info.url or "https://multicraft.world/downloads"
	if not url:find("://", 1, true) then
		url = "https://" .. url
	end

	local version_info_dlg = create_version_info_dlg(update_info.title or "", update_info.changes or "", url)
	version_info_dlg:set_parent(maintab)
	version_info_dlg:show()

	ui.update()
end

function check_new_version()
	local url = core.settings:get("update_information_url")
	if core.settings:get("update_last_checked") == "disabled" or
			url == "" then
		-- Never show any updates
		return
	end

	local time_now = os.time()
--	local time_checked = tonumber(core.settings:get("update_last_checked")) or 0
--	if time_now - time_checked < 2 * 24 * 3600 then
--		-- Check interval of 2 entire days
--		return
--	end

	core.settings:set("update_last_checked", tostring(time_now))

	url = ("%s?lang=%s&platform=%s"):format(url, LANG, PLATFORM)

	core.handle_async(function(params)
		local http = core.get_http_api()
		return http.fetch_sync(params)
	end, { url = url }, function(result)
		local json = result.succeeded and core.parse_json(result.data)
		if type(json) ~= "table" then
			core.log("error", "Failed to read JSON output from " .. url ..
					", status code = " .. result.code)
			return
		elseif not json.latest_version then
			-- No platform-specific update information, don't display anything
			return
		end

		on_version_info_received(json)
	end)
end
