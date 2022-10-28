local COLOR_BLUE = "#7AF"
local COLOR_GREEN = "#7F7"
local COLOR_GRAY = "#BBB"

-- Only use bg_common.png with MultiCraft clients
local function get_background(name)
	local info = minetest.get_player_information(name)
	return info.major == 2 and "background9[0,0;14,8;bg_common.png;true;40]" or ""
end

local LIST_FORMSPEC = [[
		size[13,6.5]%s
		label[0,-0.1;%s]
		tablecolumns[color;tree;text;text]
		table[0,0.5;12.8,5.5;list;%s;0]
		button_exit[5,6;3,1;quit;%s]
	]]

local LIST_FORMSPEC_DESCRIPTION = [[
		size[13,7.5]%s
		label[0,-0.1;%s]
		tablecolumns[color;tree;text;text]
		table[0,0.5;12.8,4.8;list;%s;%i]
		box[0,5.5;12.8,1.5;#000]
		textarea[0.3,5.5;13.05,1.9;;%s;]
		button_exit[5,7;3,1;quit;%s]
	]]

local formspec_escape = core.formspec_escape
local check_player_privs = core.check_player_privs


-- CHAT COMMANDS FORMSPEC

local mod_cmds = {}

local function load_mod_command_tree()
	mod_cmds = {}

	for name, def in pairs(core.registered_chatcommands) do
		mod_cmds[def.mod_origin] = mod_cmds[def.mod_origin] or {}
		local cmds = mod_cmds[def.mod_origin]

		-- Could be simplified, but avoid the priv checks whenever possible
		cmds[#cmds + 1] = { name, def }
	end
	local sorted_mod_cmds = {}
	for modname, cmds in pairs(mod_cmds) do
		table.sort(cmds, function(a, b) return a[1] < b[1] end)
		sorted_mod_cmds[#sorted_mod_cmds + 1] = { modname, cmds }
	end
	table.sort(sorted_mod_cmds, function(a, b) return a[1] < b[1] end)
	mod_cmds = sorted_mod_cmds
end

core.after(0, load_mod_command_tree)

local function build_chatcommands_formspec(name, sel, copy)
	local rows = {}
	rows[1] = "#FFF,0,Command,Parameters"

	local description = "For more information, click on any entry in the list.\n" ..
		"Double-click to copy the entry to the chat history."

	for i, data in ipairs(mod_cmds) do
		for _, cmds in ipairs(data[2]) do
			local has_priv = check_player_privs(name, cmds[2].privs)
			if has_priv then
				rows[#rows + 1] = COLOR_BLUE .. ",0," .. formspec_escape(data[1]) .. ","
				break
			end
		end
		for _, cmds in ipairs(data[2]) do
			local has_priv = check_player_privs(name, cmds[2].privs)
			if has_priv then
				rows[#rows + 1] = ("%s,1,%s,%s"):format(
				--	has_priv and COLOR_GREEN or COLOR_GRAY,
					COLOR_GREEN,
					cmds[1], formspec_escape(cmds[2].params))
				if sel == #rows then
					description = cmds[2].description
					if copy then
						core.chat_send_player(name, ("Command: %s %s"):format(
							core.colorize("#0FF", "/" .. cmds[1]), cmds[2].params))
					end
				end
			end
		end
	end

	return LIST_FORMSPEC_DESCRIPTION:format(
			get_background(name),
			"Available commands: (see also: /help <cmd>)",
			table.concat(rows, ","), sel or 0,
			description, "Close"
		)
end


-- PRIVILEGES FORMSPEC

local function build_privs_formspec(name)
	local privs = {}
	for priv_name, def in pairs(core.registered_privileges) do
		privs[#privs + 1] = { priv_name, def }
	end
	table.sort(privs, function(a, b) return a[1] < b[1] end)

	local rows = {}
	rows[1] = "#FFF,0,Privilege,Description"

	local player_privs = core.get_player_privs(name)
	for i, data in ipairs(privs) do
		rows[#rows + 1] = ("%s,0,%s,%s"):format(
			player_privs[data[1]] and COLOR_GREEN or COLOR_GRAY,
				data[1], formspec_escape(data[2].description))
	end

	return LIST_FORMSPEC:format(
			get_background(name),
			"Available privileges:",
			table.concat(rows, ","),
			"Close"
		)
end


-- DETAILED CHAT COMMAND INFORMATION

core.register_on_player_receive_fields(function(player, formname, fields)
	if formname ~= "__builtin:help_cmds" or fields.quit then
		return
	end

	local event = core.explode_table_event(fields.list)
	if event.type ~= "INV" then
		local name = player:get_player_name()
		core.show_formspec(name, "__builtin:help_cmds",
			build_chatcommands_formspec(name, event.row, event.type == "DCL"))
	end
end)


local help_command = core.registered_chatcommands["help"]
local old_help_func = help_command.func

help_command.func = function(name, param)
	local admin = core.settings:get("name")

	-- If the admin ran help, put the output in the chat buffer as well to
	-- work with the server terminal
	if param == "privs" then
		core.show_formspec(name, "__builtin:help_privs",
			build_privs_formspec(name))
		if name ~= admin then
			return true
		end
	end
	if param == "" or param == "all" then
		core.show_formspec(name, "__builtin:help_cmds",
			build_chatcommands_formspec(name))
		if name ~= admin then
			return true
		end
	end

	return old_help_func(name, param)
end
