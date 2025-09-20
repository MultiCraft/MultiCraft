--
-- SSCSM: Server-Sent Client-Side Mods
-- Initial code sent to the client
--
-- Copyright © 2019-2021 by luk3yx
-- Copyright © 2020-2021 MultiCraft Development Team
-- License: GNU LGPL 3.0+
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

-- Make sure both table.unpack and unpack exist.
if table.unpack then
	unpack = table.unpack
else
	table.unpack = unpack -- luacheck: ignore
end

-- Make sure a few basic functions exist, these may have been blocked because
-- of security or laziness.
if not rawget   then function rawget(n, name) return n[name] end end
if not rawset   then function rawset(n, k, v) n[k] = v end end
if not rawequal then function rawequal(a, b) return a == b end end

-- Older versions of the CSM don't provide assert(), this function exists for
-- compatibility.
if not assert then
	function assert(value, ...)
		if value then
			return value, ...
		else
			error(... or 'assertion failed!', 2)
		end
	end
end

-- Create the API
sscsm = {}
function sscsm.global_exists(name)
	return rawget(_G, name) ~= nil
end

if sscsm.global_exists('minetest') then
	core = minetest
else
	minetest = assert(core, 'No "minetest" global found!')
end

core.global_exists = sscsm.global_exists

-- Add print()
function print(...)
	local msg = '[SSCSM] '
	for i = 1, select('#', ...) do
		if i > 1 then msg = msg .. '\t' end
		msg = msg .. tostring(select(i, ...))
	end
	core.log('none', msg)
end

-- Add register_on_mods_loaded
do
	local funcs = {}
	function sscsm.register_on_mods_loaded(callback)
		if funcs then table.insert(funcs, callback) end
	end

	function sscsm._done_loading_()
		sscsm._done_loading_ = nil
		for _, func in ipairs(funcs) do func() end
		funcs = nil
	end
end

-- Helper functions
if not core.get_node then
	function core.get_node(pos)
		return core.get_node_or_nil(pos) or {name = 'ignore', param1 = 0,
			param2 = 0}
	end
end

-- Make core.run_server_chatcommand allow param to be unspecified.
function core.run_server_chatcommand(cmd, param)
	core.send_chat_message('/' .. cmd .. ' ' .. (param or ''))
end

-- Register "server-side" chatcommands
-- Can allow instantaneous responses in some cases.
sscsm.registered_chatcommands = {}
local function on_chat_message(msg)
	if msg:sub(1, 1) ~= '/' then return false end

	local cmd, param = msg:match('^/([^ ]+) *(.*)')
	if not cmd then
		core.display_chat_message('-!- Empty command')
		return true
	end

	if not sscsm.registered_chatcommands[cmd] then return false end

	local _, res = sscsm.registered_chatcommands[cmd].func(param or '')
	if res then core.display_chat_message(tostring(res)) end

	return true
end

function sscsm.register_chatcommand(cmd, def)
	if type(def) == 'function' then
		def = {func = def}
	elseif type(def.func) ~= 'function' then
		error('Invalid definition passed to sscsm.register_chatcommand.')
	end

	sscsm.registered_chatcommands[cmd] = def

	if on_chat_message then
		core.register_on_sending_chat_message(on_chat_message)
		on_chat_message = nil
	end
end

function sscsm.unregister_chatcommand(cmd)
	sscsm.registered_chatcommands[cmd] = nil
end

-- This function exists for backwards compatibility reasons
function sscsm.get_player_control()
	local c = core.localplayer:get_control()
	c.LMB, c.RMB = c.dig, c.place
	return c
end

-- Call func(...) every <interval> seconds.
local function sscsm_every(interval, func, ...)
	core.after(interval, sscsm_every, interval, func, ...)
	return func(...)
end

function sscsm.every(interval, func, ...)
	assert(type(interval) == 'number' and type(func) == 'function',
		'Invalid sscsm.every() invocation.')
	return sscsm_every(interval, func, ...)
end

-- Allow SSCSMs to know about CSM restriction flags.
-- "__FLAGS__" is replaced with the actual value in init.lua.
-- luacheck: globals __FLAGS__ __MAPGEN_LIMIT__
local flags = __FLAGS__
sscsm.restriction_flags = assert(flags)
sscsm.restrictions = {
	chat_messages = math.floor(flags / 2) % 2 == 1,
	read_itemdefs = math.floor(flags / 4) % 2 == 1,
	read_nodedefs = math.floor(flags / 8) % 2 == 1,
	lookup_nodes_limit = math.floor(flags / 16) % 2 == 1,
	read_playerinfo = math.floor(flags / 32) % 2 == 1,
}
sscsm.restrictions.lookup_nodes = sscsm.restrictions.lookup_nodes_limit

-- Add core.get_csm_restrictions() if it doesn't exist already.
if not core.get_csm_restrictions then
	function core.get_csm_restrictions()
		return table.copy(sscsm.restrictions)
	end
end

local mapgen_limit = __MAPGEN_LIMIT__
function core.is_valid_pos(pos)
	if not pos or type(pos) ~= "table" then
		return false
	end
	for _, v in ipairs({"x", "y", "z"}) do
		if not pos[v] or pos[v] ~= pos[v] or
				pos[v] < -mapgen_limit or pos[v] > mapgen_limit then
			return false
		end
	end

	return true
end

-- SSCSM communication
-- A lot of this is copied from init.lua.
local function validate_channel(channel)
	if type(channel) ~= 'string' then
		error('SSCSM com channels must be strings!', 3)
	end
	if channel:find('\001', nil, true) then
		error('SSCSM com channels cannot contain U+0001!', 3)
	end
end

function sscsm.com_send(channel, msg)
	assert(not sscsm.restrictions.chat_messages, 'Server restrictions ' ..
		'prevent SSCSM com messages from being sent!')
	validate_channel(channel)
	if type(msg) == 'string' then
		msg = '\002' .. msg
	else
		msg = core.write_json(msg)
	end
	core.run_server_chatcommand('admin', '\001SSCSM_COM\001' .. channel ..
		'\001' .. msg)
end

local registered_on_receive = {}
function sscsm.register_on_com_receive(channel, func)
	if not registered_on_receive[channel] then
		registered_on_receive[channel] = {}
	end
	table.insert(registered_on_receive[channel], func)
end

-- Load split messages
local incoming_messages = {}
local function load_split_message(chan, msg)
	local id, i, l, pkt = msg:match('^\1([^\1]+)\1([^\1]+)\1([^\1]+)\1(.*)$')
	id, i, l = tonumber(id), tonumber(i), tonumber(l)

	if not incoming_messages[id] then
		incoming_messages[id] = {}
	end
	local msgs = incoming_messages[id]
	msgs[i] = pkt

	-- Return true if all the messages have been received
	if #msgs < l then return end
	for j = 1, l do
		if not msgs[j] then
			return
		end
	end
	incoming_messages[id] = nil
	return table.concat(msgs, '')
end

-- Detect messages and handle them
core.register_on_receiving_chat_message(function(message)
	if type(message) == 'table' then
		message = message.message
	end

	local chan, msg = message:match('^\001SSCSM_COM\001([^\001]*)\001(.*)$')
	if not chan or not msg then return end

	-- Get the callbacks
	local callbacks = registered_on_receive[chan]
	if not callbacks then return true end

	-- Handle split messages
	local prefix = msg:byte(1)
	if prefix == 1 then
		msg = load_split_message(chan, msg)
		if not msg then
			return true
		end
		prefix = msg:byte(1)
	end

	-- Decompress messages
	if prefix == 3 or prefix == 4 then
		msg = minetest.decompress(minetest.decode_base64(msg:sub(2)),
			prefix == 4 and "zstd" or "deflate")
		prefix = msg:byte(1)
	end

	-- Load the message
	if prefix == 2 then
		msg = msg:sub(2)
	else
		msg = core.parse_json(msg)
	end

	-- Run callbacks
	for _, func in ipairs(callbacks) do
		local ok, err = pcall(func, msg)
		if not ok then
			sscsm.com_send("sscsm:error", tostring(err))
		end
	end
	return true
end)

sscsm.register_on_mods_loaded(function()
	sscsm.com_send('sscsm:com_test', {flags = sscsm.restriction_flags})
end)

-- Call leave_mod_channel for legacy clients
if sscsm.global_exists('leave_mod_channel') then
	sscsm.register_on_mods_loaded(leave_mod_channel)
	join_mod_channel = nil
	leave_mod_channel = nil
end

if core.global_exists("set_error_handler") then
	set_error_handler(function(err)
		sscsm.com_send("sscsm:error", tostring(err))
	end)
	set_error_handler = nil
end

if not core.global_exists("utf8") or not utf8.lower then
	utf8 = string
end
