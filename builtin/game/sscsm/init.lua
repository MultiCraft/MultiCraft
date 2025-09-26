--
-- SSCSM: Server-Sent Client-Side Mods
--
-- Copyright © 2019-2025 by luk3yx
-- Copyright © 2020-2025 MultiCraft Development Team
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

sscsm = {}

local format = string.format
local modpath = core.get_builtin_path() .. "game/sscsm"

sscsm.minify_code = dofile(modpath .. "/minify.lua")

-- Register code
sscsm.registered_csms = {}
local csm_order

-- Recalculate the CSM loading order
-- TODO: Make this nicer
local function recalc_csm_order()
	local staging = {}
	local order = {":init"}
	local unsatisfied = {}
	for name, def in pairs(sscsm.registered_csms) do
		assert(name == def.name)
		if name:sub(1, 1) ~= ":" then
			if not def.depends or #def.depends == 0 then
				table.insert(staging, name)
			else
				unsatisfied[name] = {}
				for _, mod in ipairs(def.depends) do
					if mod:sub(1, 1) ~= ":" then
						unsatisfied[name][mod] = true
					end
				end
			end
		end
	end
	while #staging > 0 do
		local name = staging[1]
		for name2, u in pairs(unsatisfied) do
			if u[name] then
				u[name] = nil
				if not next(u) then
					table.insert(staging, name2)
				end
			end
		end

		table.insert(order, name)
		table.remove(staging, 1)
	end

	for name, u in pairs(unsatisfied) do
		if next(u) then
			local msg = 'SSCSM "' .. name .. '" has unsatisfied dependencies: '
			local n = false
			for dep, _ in pairs(u) do
				if n then msg = msg .. ", " else n = true end
				msg = msg .. '"' .. dep .. '"'
			end
			core.log("error", msg)
		end
	end

	-- Set csm_order
	table.insert(order, ":cleanup")
	csm_order = order
end

-- Register SSCSMs
local block_colon = false
sscsm.registered_csms = {}
function sscsm.register(def)
	-- Read files now in case MT decides to block access later.
	if not def.code and def.file then
		local f = io.open(def.file, "rb")
		if not f then
			error('Invalid "file" parameter passed to sscsm.register_csm.', 2)
		end
		def.code = f:read("*a")
		f:close()
		def.file = nil
	end

	if type(def.name) ~= "string" or def.name:find("\n")
			or (def.name:sub(1, 1) == ":" and block_colon) then
		error('Invalid "name" parameter passed to sscsm.register_csm.', 2)
	end

	if type(def.code) ~= "string" then
		error('Invalid "code" parameter passed to sscsm.register_csm.', 2)
	end

	if block_colon then
		def.code = "local minetest=core;" .. def.code
	end
	def.code = sscsm.minify_code(def.code)
	if (#def.name + #def.code) > 65300 then
		error("The code (or name) passed to sscsm.register_csm is too large."
			.. " Consider refactoring your SSCSM code.", 2)
	end

	-- Copy the table to prevent mods from betraying our trust.
	sscsm.registered_csms[def.name] = table.copy(def)
	if csm_order then recalc_csm_order() end
end

function sscsm.unregister(name)
	sscsm.registered_csms[name] = nil
	if csm_order then recalc_csm_order() end
end

-- Recalculate the CSM order once all other mods are loaded
core.register_on_mods_loaded(recalc_csm_order)

-- Handle players joining
local has_sscsms = {}
local sscsms_sent = {}
local supports_zstd = {}
local v2_mod_channels = {}
local v2_chunk_size = 65530
core.register_on_modchannel_message(function(channel_name, sender, message)
	if not sender or sscsms_sent[sender] then return end

	local v2_channel = v2_mod_channels[sender]
	if channel_name ~= "sscsm:v2_" .. sender or not v2_channel then
		return
	end

	local method
	if message == "0" then
		method = "deflate"
	elseif message == "1" then
		method = "zstd"
		supports_zstd[sender] = true
	else
		-- Unsupported protocol version
		return
	end

	-- New protocol (compressed)
	local blob = {}

	for _, name in ipairs(csm_order) do
		local def = sscsm.registered_csms[name]
		if not def.is_enabled_for or def.is_enabled_for(sender) then
			blob[#blob + 1] = name .. "\n" .. def.code
		end
	end

	local compressed = core.encode_base64(core.compress(table.concat(blob, "\0"), method))

	sscsms_sent[sender] = true
	core.log("info", "[SSCSM] Sending CSMs on request for " .. sender ..
		" (v2 protocol)...")
	-- This is not optimal but the compressed code is probably only one or
	-- two chunks anyway
	while #compressed > v2_chunk_size do
		v2_channel:send_all("C" .. compressed:sub(1, v2_chunk_size))
		compressed = compressed:sub(v2_chunk_size + 1)
	end
	v2_channel:send_all("E" .. compressed)

	-- No point staying in the v2 mod channel now that SSCSMs are sent
	v2_channel:leave()
	v2_mod_channels[sender] = nil
end)

core.register_on_joinplayer(function(player)
	-- Join player-specific mod channels to avoid relaying messages to players
	-- that don't need them
	local name = player:get_player_name()
	v2_mod_channels[name] = core.mod_channel_join("sscsm:v2_" .. name)
end)

core.register_on_leaveplayer(function(player)
	local name = player:get_player_name()
	has_sscsms[name] = nil
	sscsms_sent[name] = nil
	supports_zstd[name] = nil

	-- Leave the v2 mod channel
	if v2_mod_channels[name] then
		v2_mod_channels[name]:leave()
		v2_mod_channels[name] = nil
	end
end)

-- Register the SSCSM "builtins"
sscsm.register({
	name = ":init",
	file = modpath .. "/client.lua"
})

sscsm.register({
	name = ":cleanup",
	code = "sscsm._done_loading_()"
})

block_colon = true

-- Set the CSM restriction flags
local flags = tonumber(core.settings:get("csm_restriction_flags"))
if not flags or flags ~= flags then
	flags = 62
end
flags = math.floor(math.max(flags, 0)) % 64

do
	local def = sscsm.registered_csms[":init"]
	def.code = def.code:gsub("__FLAGS__", tostring(flags))

	local mapgen_limit = tonumber(core.settings:get("mapgen_limit"))
	def.code = def.code:gsub("__MAPGEN_LIMIT__", tostring(mapgen_limit))
end

if math.floor(flags / 2) % 2 == 1 then
	core.log("warning", "[SSCSM] SSCSMs enabled, however CSMs cannot "
		.. "send chat messages! This will prevent SSCSMs from sending "
		.. "messages to the server.")
	sscsm.com_write_only = true
else
	sscsm.com_write_only = false
end

-- SSCSM communication
local function validate_channel(channel)
	if type(channel) ~= "string" then
		error("SSCSM com channels must be strings!", 3)
	end
	if channel:find("\001", nil, true) then
		error("SSCSM com channels cannot contain U+0001!", 3)
	end
end

local msgids = {}
function sscsm.com_send(pname, channel, msg)
	if core.is_player(pname) then
		pname = pname:get_player_name()
	end
	validate_channel(channel)
	if type(msg) == "string" then
		msg = "\002" .. msg
	else
		msg = assert(core.write_json(msg))
	end

	-- Compress long messages
	if #msg > 512 then
		-- Chat messages can't contain binary data so base64 is used
		local method = supports_zstd[pname] and "zstd" or "deflate"
		local compressed_msg = minetest.encode_base64(minetest.compress(msg, method))

		-- Only use the compressed message if it's shorter
		if #msg > #compressed_msg + 1 then
			msg = (supports_zstd[pname] and "\004" or "\003") .. compressed_msg
		end
	end

	-- Short messages can be sent all at once
	local prefix = format("\001SSCSM_COM\001%s\001", channel)
	if #msg < 65300 then
		core.chat_send_player(pname, prefix .. msg)
		return
	end

	-- You should never send messages over 128MB to clients
	assert(#msg < 134217728)

	-- Otherwise split the message into multiple chunks
	prefix = prefix .. "\001"
	local id = #msgids + 1
	local i = 0
	msgids[id] = true
	local total_msgs = math.ceil(#msg / 65000)
	repeat
		i = i + 1
		core.chat_send_player(pname, prefix .. id .. "\001" .. i ..
			"\001" .. total_msgs .. "\001" .. msg:sub(1, 65000))
		msg = msg:sub(65001)
	until msg == ""

	-- Allow the ID to be reused on the next globalstep.
	core.after(0, function()
		msgids[id] = nil
	end)
end

local registered_on_receive = {}
function sscsm.register_on_com_receive(channel, func)
	if not registered_on_receive[channel] then
		registered_on_receive[channel] = {}
	end
	table.insert(registered_on_receive[channel], func)
end

local admin_func = core.registered_chatcommands["admin"].func
core.override_chatcommand("admin", {
	func = function(name, param)
		local chan, msg = param:match("^\001SSCSM_COM\001([^\001]*)\001(.*)$")
		if not chan or not msg then
			return admin_func(name, param)
		end

		-- Get the callbacks
		local callbacks = registered_on_receive[chan]
		if not callbacks then return end

		-- Load the message
		if msg:sub(1, 1) == "\002" then
			msg = msg:sub(2)
		else
			msg = core.parse_json(msg)
		end

		-- Run callbacks
		for _, func in ipairs(callbacks) do
			func(name, msg)
		end
	end,
})

-- Add a callback for sscsm:com_test
local registered_on_loaded = {}
function sscsm.register_on_sscsms_loaded(func)
	table.insert(registered_on_loaded, func)
end

sscsm.register_on_com_receive("sscsm:com_test", function(name, msg)
	if type(msg) ~= "table" or msg.flags ~= flags or has_sscsms[name] then
		return
	end
	has_sscsms[name] = true
	for _, func in ipairs(registered_on_loaded) do
		func(name)
	end
end)

sscsm.register_on_com_receive("sscsm:error", function(name, msg)
	minetest.log("error", "[SSCSM] Error reported from " .. name .. ": " ..
		tostring(msg))
end)

function sscsm.has_sscsms_enabled(name)
	return has_sscsms[name] or false
end

function sscsm.com_send_all(channel, msg)
	for name, _ in pairs(has_sscsms) do
		sscsm.com_send(name, channel, msg)
	end
end
