--
-- The commands a player may use, for the chat console of clients
--
-- Copyright © 2026 MultiCraft Development Team
--
-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 3.0 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU Lesser General Public License for more details.
--
-- You should have received a copy of the GNU Lesser General Public License
-- along with this program; if not, write to the Free Software Foundation,
-- Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
--

-- The console asks on a mod channel for the commands the player may use, with their syntax,
-- the same ones /help lists
local CHANNEL = "__builtin:command_list"
local clients = {}

-- A long list goes compressed, as SSCSM sends it: zstd in base64 after \004
local function pack(json)
	if #json > 512 then
		local packed = "\004" .. core.encode_base64(core.compress(json, "zstd"))
		if #packed < #json then
			return packed
		end
	end
	return json
end

local function send_list(name)
	local list = {}
	for cmd, def in pairs(core.registered_chatcommands) do
		if core.check_player_privs(name, def.privs) then
			list[#list + 1] = {cmd, def.params, def.description}
		end
	end
	local message = pack(core.write_json(list))
	-- A chat message holds 65535 characters, a longer list goes without descriptions, and one
	-- still longer does not go
	if #message > 65000 then
		for _, command in ipairs(list) do
			command[3] = ""
		end
		message = pack(core.write_json(list))
	end
	if #message > 65000 then
		return
	end
	-- In a chat message, as mod channel messages go to all players on the channel
	core.chat_send_player(name, "\001" .. CHANNEL .. "\001" .. message)
end

core.register_on_modchannel_message(function(channel_name, sender)
	if channel_name == CHANNEL and sender and sender ~= "" then
		clients[sender] = true
		send_list(sender)
	end
end)

-- Privileges change one by one, as with /grant all, the list goes once after them
local function resend_list(name)
	if clients[name] == true then
		clients[name] = "pending"
		core.after(0, function()
			if clients[name] == "pending" then
				clients[name] = true
				send_list(name)
			end
		end)
	end
	-- The callbacks of mods run only after a true
	return true
end

core.register_on_priv_grant(resend_list)
core.register_on_priv_revoke(resend_list)
core.register_on_leaveplayer(function(player)
	clients[player:get_player_name()] = nil
end)
