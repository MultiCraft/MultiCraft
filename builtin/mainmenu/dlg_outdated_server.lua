--MultiCraft
--Copyright (C) 2022 MultiCraft Development Team
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

local blank = core.formspec_escape(defaulttexturedir .. "blank.png")
local function outdated_server_formspec(this)
	return ([[
		style_type[image_button;content_offset=0]
		image[4.9,0.3;2.5,2.5;%s]
		image_button[1,2.5;10,0.8;%s;;%s;false;false]
		image_button[1,3.2;10,0.8;%s;;%s;false;false]
		%s
		button[2,4.5;4,0.8;cancel;%s]
		%s
		button[6,4.5;4,0.8;continue;%s]
	]]):format(
		core.formspec_escape(defaulttexturedir .. "attention.png"),
		blank,
		fgettext("The server you are trying to connect to is outdated!"),
		blank,
		fgettext("Support for older servers may be removed at any time."),
		btn_style("cancel"),
		fgettext("Cancel"),
		btn_style("continue", "yellow"),
		fgettext("Join anyway")
	)
end

local function outdated_server_buttonhandler(this, fields)
	if fields.cancel then
		this:delete()
		return true
	end

	if fields.continue then
		serverlistmgr.add_favorite(this.server)

		gamedata.servername = this.server.name
		gamedata.serverdescription = this.server.description

		core.settings:set_bool("auto_connect", false)
		core.settings:set("connect_time", os.time())
		core.settings:set("maintab_LAST", "online")
		core.settings:set("address", gamedata.address)
		core.settings:set("remote_port", gamedata.port)

		core.start()
	end
end


function create_outdated_server_dlg(server)
	local retval = dialog_create("outdated_server_dlg",
					outdated_server_formspec,
					outdated_server_buttonhandler,
					nil, true)
	retval.server = server

	return retval
end
