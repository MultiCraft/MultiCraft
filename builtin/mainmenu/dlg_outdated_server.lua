--Minetest
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

local bg = core.formspec_escape(defaulttexturedir .. "bg_common.png")
local function outdated_server_formspec(this)
	local message = fgettext("The server you are trying to connect to is outdated!") .. "\n" ..
		fgettext("Support for older servers may be removed at any time.")

	return ([[
		size[14,5.4,false]
		container[1,0]
		bgcolor[#0000]
		background9[-0.2,-0.26;12.4,6.15;%s;false;40]
		textarea[1,1;10,4;;;%s]
		button[2,4.5;4,0.8;cancel;%s]
		button[6,4.5;4,0.8;continue;%s]
		container_end[]
	]]):format(bg, message, fgettext("Cancel"), fgettext("Join anyway"))
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
					nil)
	retval.server = server

	return retval
end
