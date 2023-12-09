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

mt_color_grey  = "#AAAAAA"
mt_color_blue  = "#6389FF"
mt_color_green = "#72FF63"
mt_color_dark_green = "#25C191"
mt_color_orange  = "#FF8800"

local menupath = core.get_mainmenu_path()
local basepath = core.get_builtin_path()
defaulttexturedir = core.get_texturepath_share() .. DIR_DELIM .. "base" ..
					DIR_DELIM .. "pack" .. DIR_DELIM
defaulttexturedir_esc = core.formspec_escape(defaulttexturedir)
DIR_DELIM_esc = core.formspec_escape(DIR_DELIM) -- for use in formspecs only

dofile(basepath .. "common" .. DIR_DELIM .. "filterlist.lua")
dofile(basepath .. "common" .. DIR_DELIM .. "btn_style.lua")
dofile(basepath .. "fstk" .. DIR_DELIM .. "buttonbar.lua")
dofile(basepath .. "fstk" .. DIR_DELIM .. "dialog.lua")
dofile(basepath .. "fstk" .. DIR_DELIM .. "tabview.lua")
dofile(basepath .. "fstk" .. DIR_DELIM .. "ui.lua")
dofile(menupath .. DIR_DELIM .. "async_event.lua")
dofile(menupath .. DIR_DELIM .. "common.lua")
dofile(menupath .. DIR_DELIM .. "pkgmgr.lua")
dofile(menupath .. DIR_DELIM .. "serverlistmgr.lua")
dofile(menupath .. DIR_DELIM .. "textures.lua")

dofile(menupath .. DIR_DELIM .. "dlg_config_world.lua")
dofile(menupath .. DIR_DELIM .. "dlg_settings_advanced.lua")
dofile(menupath .. DIR_DELIM .. "dlg_contentstore.lua")
dofile(menupath .. DIR_DELIM .. "dlg_create_world.lua")
dofile(menupath .. DIR_DELIM .. "dlg_delete_content.lua")
dofile(menupath .. DIR_DELIM .. "dlg_delete_world.lua")
dofile(menupath .. DIR_DELIM .. "dlg_rename_modpack.lua")
dofile(menupath .. DIR_DELIM .. "dlg_version_info.lua")

local tabs = {}

tabs.settings = dofile(menupath .. DIR_DELIM .. "tab_settings.lua")
tabs.content  = dofile(menupath .. DIR_DELIM .. "tab_content.lua")
tabs.credits  = dofile(menupath .. DIR_DELIM .. "tab_credits.lua")
tabs.local_game = dofile(menupath .. DIR_DELIM .. "tab_local.lua")
tabs.play_online = dofile(menupath .. DIR_DELIM .. "tab_online.lua")

local func = loadfile(basepath .. DIR_DELIM .. "hosting" .. DIR_DELIM .. "init.lua")

--------------------------------------------------------------------------------
local function main_event_handler(tabview, event)
	if event == "MenuQuit" and PLATFORM ~= "iOS" then
		core.close()
	end
	return true
end

--------------------------------------------------------------------------------
function menudata.init_tabs()
	-- Init gamedata
	gamedata.worldindex = 0

	menudata.worldlist = filterlist.create(
		core.get_worlds,
		compare_worlds,
		-- Unique id comparison function
		function(element, uid)
			return element.name == uid
		end,
		-- Filter function
		function(element, gameid)
			return element.gameid == gameid
		end
	)

	menudata.worldlist:add_sort_mechanism("alphabetic", sort_worlds_alphabetic)
	menudata.worldlist:set_sortmode("alphabetic")

	mm_texture.init()

	-- Create main tabview
	local tv_main = tabview_create("maintab", {x = 12, y = 5.4}, {x = 0.1, y = 0})

	tv_main:add_side_button({
		tooltip = fgettext("Browse online content"),
		tab_name_selected = "content",
		is_open_cdb = true,
		on_click = function(this)
			-- Show the content tab if no hidden games are installed
			for _, game in ipairs(pkgmgr.games) do
				if not game.hidden then
					this:set_tab("content")
					return
				end
			end

			-- Otherwise open the store dialog
			local dialog = create_store_dlg("game")
			dialog:set_parent(this)
			this:hide()
			dialog:show()
		end,
	})

	tv_main:add_side_button({
		tooltip = fgettext("Settings"),
		tab_name = "settings",
	})

	tv_main:add_side_button({
		tooltip = fgettext("Credits"),
		tab_name = "credits",
		texture_prefix = "authors"
	})

	tv_main:set_autosave_tab(true)
	tv_main:add(tabs.local_game)
	if func then
		func(tv_main)
	end
	tv_main:add(tabs.play_online)

	tv_main:add(tabs.content)
	tv_main:add(tabs.settings)
	tv_main:add(tabs.credits)

	tv_main:set_global_event_handler(main_event_handler)
	tv_main:set_fixed_size(false)

	local last_tab = core.settings:get("maintab_LAST")
	if last_tab and tv_main.current_tab ~= last_tab then
		tv_main:set_tab(last_tab)
	end

	if last_tab ~= "local" then
		core.set_clouds(false)
		mm_texture.set_dirt_bg()
	end

	-- In case the folder of the last selected game has been deleted,
	-- display "Minetest" as a header
	if tv_main.current_tab == "local" then
		local game = pkgmgr.find_by_gameid(core.settings:get("menu_last_game"))
		if game == nil then
			mm_texture.reset()
		end
	end

	ui.set_default("maintab")

	check_new_version()
	tv_main:show()

	ui.update()

--	core.sound_play("main_menu", true)
end

menudata.init_tabs()
