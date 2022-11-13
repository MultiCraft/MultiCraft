--Minetest
--Copyright (C) 2014 sapier
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

ui = {}
ui.childlist = {}
ui.default = nil
-- Whether fstk is currently showing its own formspec instead of active ui elements.
ui.overridden = false

--------------------------------------------------------------------------------
function ui.add(child)
	--TODO check child
	ui.childlist[child.name] = child

	return child.name
end

--------------------------------------------------------------------------------
function ui.delete(child)

	if ui.childlist[child.name] == nil then
		return false
	end

	ui.childlist[child.name] = nil
	return true
end

--------------------------------------------------------------------------------
function ui.set_default(name)
	ui.default = name
end

--------------------------------------------------------------------------------
function ui.find_by_name(name)
	return ui.childlist[name]
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Internal functions not to be called from user
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local maintab = core.settings:get("maintab_LAST")
local connect_time = tonumber(core.settings:get("connect_time")) or 0

function ui.update()
	ui.overridden = false
	local formspec = {}

	-- handle errors
	if gamedata ~= nil and gamedata.reconnect_requested then
		local error_message = core.formspec_escape(
				gamedata.errormessage or "<none available>")
		formspec = {
			"formspec_version[3]",
			"size[14,8.25]",
			"bgcolor[#0000]",
			"background9[0,0;0,0;", defaulttexturedir_esc, "bg_common.png;true;40]",
			"set_focus[btn_reconnect_yes;true]",
			"textarea[0.6,0.5;12.8,0.6;;;", fgettext("The server has requested a reconnect:"), "]",
			"background9[0.4,1.1;13.2,5.2;", defaulttexturedir_esc,
				"worldlist_bg.png;false;40]",
			"textarea[0.6,1.3;12.8,4.8;;;", error_message, "]",
			btn_style("btn_reconnect_yes") ..
			"button[2,6.725;4,1;btn_reconnect_yes;" .. fgettext("Reconnect") .. "]",
			btn_style("btn_reconnect_no") ..
			"button[8,6.725;4,1;btn_reconnect_no;" .. fgettext("Main menu") .. "]"
		}
		ui.overridden = true
	elseif gamedata ~= nil and gamedata.errormessage ~= nil then
		local error_message = core.formspec_escape(gamedata.errormessage)

		local error_title
		local mod_error = gamedata.errormessage:find("ModError") or gamedata.errormessage:find("LuaError")
		if mod_error then
			error_title = fgettext("An error occurred in a Lua script:")
		else
			error_title = fgettext("An error occurred:")
		end
		local restart_btn
		if (maintab == "local" or maintab == "local_default") and mod_error and
				core.get_us_time() - connect_time > 30 then
			restart_btn =
				btn_style("btn_reconnect_yes") ..
				"button[2,6.725;4,1;btn_reconnect_yes;" .. fgettext("Restart") .. "]" ..
				btn_style("btn_reconnect_no") ..
				"button[8,6.725;4,1;btn_reconnect_no;" .. fgettext("Main menu") .. "]" ..
				"set_focus[btn_reconnect_yes;true]"
		else
			restart_btn =
				btn_style("btn_reconnect_no") ..
				"button[5,6.725;4,1;btn_reconnect_no;" .. fgettext("OK") .. "]" ..
				"set_focus[btn_reconnect_no;true]"
		end
		formspec = {
			"formspec_version[3]",
			"size[14,8.25]",
			"bgcolor[#0000]",
			"background9[0,0;0,0;", defaulttexturedir_esc, "bg_common.png;true;40]",
			"textarea[0.6,0.5;12.8,0.6;;;", error_title, "]",
			"background9[0.4,1.1;13.2,5.2;", defaulttexturedir_esc,
				"worldlist_bg.png;false;40]",
			"textarea[0.6,1.3;12.8,4.8;;;", error_message, "]",
			restart_btn
		}
		ui.overridden = true
	else
		local active_toplevel_ui_elements = 0
		for key,value in pairs(ui.childlist) do
			if (value.type == "toplevel") then
				local retval = value:get_formspec()

				if retval ~= nil and retval ~= "" then
					active_toplevel_ui_elements = active_toplevel_ui_elements + 1
					table.insert(formspec, retval)
				end
			end
		end

		-- no need to show addons if there ain't a toplevel element
		if (active_toplevel_ui_elements > 0) then
			for key,value in pairs(ui.childlist) do
				if (value.type == "addon") then
					local retval = value:get_formspec()

					if retval ~= nil and retval ~= "" then
						table.insert(formspec, retval)
					end
				end
			end
		end

		if (active_toplevel_ui_elements > 1) then
			core.log("warning", "more than one active ui "..
				"element, self most likely isn't intended")
		end

		if (active_toplevel_ui_elements == 0) then
			core.log("warning", "no toplevel ui element "..
					"active; switching to default")
			ui.childlist[ui.default]:show()
			formspec = {ui.childlist[ui.default]:get_formspec()}
		end
	end
	core.update_formspec(table.concat(formspec))
end

--------------------------------------------------------------------------------
function ui.handle_buttons(fields)
	for key,value in pairs(ui.childlist) do

		local retval = value:handle_buttons(fields)

		if retval then
			ui.update()
			return
		end
	end
end


--------------------------------------------------------------------------------
function ui.handle_events(event)

	for key,value in pairs(ui.childlist) do

		if value.handle_events ~= nil then
			local retval = value:handle_events(event)

			if retval then
				return retval
			end
		end
	end
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- initialize callbacks
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
core.button_handler = function(fields)
	if fields["btn_reconnect_yes"] then
		if maintab == "local" or maintab == "local_default" then
			gamedata.singleplayer = true
			gamedata.selected_world =
				tonumber(core.settings:get("mainmenu_last_selected_world"))
		end
		core.settings:set("connect_time", core.get_us_time())
		gamedata.reconnect_requested = false
		gamedata.errormessage = nil
		gamedata.do_reconnect = true
		core.start()
		return
	elseif fields["btn_reconnect_no"] then
		gamedata.errormessage = nil
		gamedata.reconnect_requested = false
		ui.update()
		return
	end

	if ui.handle_buttons(fields) then
		ui.update()
	end
end

--------------------------------------------------------------------------------
core.event_handler = function(event)
	-- Handle error messages
	if ui.overridden then
		if event == "MenuQuit" then
			gamedata.errormessage = nil
			gamedata.reconnect_requested = false
			ui.update()
		end
		return
	end

	if ui.handle_events(event) then
		ui.update()
		return
	end

	if event == "Refresh" then
		ui.update()
		return
	end
end

--------------------------------------------------------------------------------
if core.settings:get("just_reconnected") then
	core.settings:remove("just_reconnected")
elseif gamedata and gamedata.errormessage == "AsyncErr: Failed to bind socket (port already in use?)" and
		(maintab == "local" or maintab == "local_default") and not core.settings:get_bool("enable_server") then
	core.settings:set("just_reconnected", "true")
	gamedata.singleplayer = true
	gamedata.selected_world =
		tonumber(core.settings:get("mainmenu_last_selected_world"))
	gamedata.errormessage = nil
	gamedata.do_reconnect = true
	core.start()
end
