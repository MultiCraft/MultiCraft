--Minetest
--Copyright (C) 2018-20 rubenwardy
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

local esc = core.formspec_escape

if not core.get_http_api then
	function create_store_dlg()
		return messagebox("store",
				fgettext("ContentDB is not available when Minetest was compiled without cURL"))
	end
	return
end

-- Unordered preserves the original order of the ContentDB API,
-- before the package list is ordered based on installed state.
local store = { packages = {}, packages_full = {}, packages_full_unordered = {} }

local http = core.get_http_api()

-- Screenshot
local screenshot_dir = core.get_cache_path() .. DIR_DELIM .. "cdb"
if not core.create_dir(screenshot_dir) then
	core.log("warning", "Failed to create ContentDB screenshot cache dir")
end
local screenshot_downloading = {}
local screenshot_downloaded = {}

-- Filter
local search_string = ""
local cur_page = 1
local num_per_page = 5
local filter_type = 1
local dropdown_open = false
local filter_types_titles = {
	fgettext("All packages"),
	fgettext("Games"),
	fgettext("Mods"),
	fgettext("Texture packs"),
}

local number_downloading = 0
local download_queue = {}

local filter_types_type = {
	nil,
	"game",
	"mod",
	"txp",
}


local function download_package(param)
	if core.download_file(param.package.url, param.filename) then
		return {
			filename = param.filename,
			successful = true,
		}
	else
		core.log("error", "downloading " .. dump(param.package.url) .. " failed")
		return {
			successful = false,
		}
	end
end

local function start_install(package)
	local params = {
		package = package,
		filename = os.tempfolder() .. "_MODNAME_" .. package.name .. ".zip",
	}

	number_downloading = number_downloading + 1

	local function callback(result)
		if result.successful then
			local path, msg = pkgmgr.install(package.type,
					result.filename, package.name,
					package.path)
			if not path then
				gamedata.errormessage = msg
			else
				core.log("action", "Installed package to " .. path)

				local conf_path
				local name_is_title = false
				if package.type == "mod" then
					local actual_type = pkgmgr.get_folder_type(path)
					if actual_type.type == "modpack" then
						conf_path = path .. DIR_DELIM .. "modpack.conf"
					else
						conf_path = path .. DIR_DELIM .. "mod.conf"
					end
				elseif package.type == "game" then
					conf_path = path .. DIR_DELIM .. "game.conf"
					name_is_title = true
				elseif package.type == "txp" then
					conf_path = path .. DIR_DELIM .. "texture_pack.conf"
				end

				if conf_path then
					local conf = Settings(conf_path)
					if name_is_title then
						conf:set("name",   package.title)
					else
						conf:set("title",  package.title)
						conf:set("name",   package.name)
					end
					if not conf:get("description") then
						conf:set("description", package.short_description)
					end
					conf:set("author",     package.author)
					conf:set("release",    package.release)
					conf:write()
				end
			end
			os.remove(result.filename)
		else
			gamedata.errormessage = fgettext("Failed to download $1", package.name)
		end

		package.downloading = false

		number_downloading = number_downloading - 1

		local next = download_queue[1]
		if next then
			table.remove(download_queue, 1)

			start_install(next)
		end

		ui.update()
	end

	package.queued = false
	package.downloading = true

	if not core.handle_async(download_package, params, callback) then
		core.log("error", "ERROR: async event failed")
		gamedata.errormessage = fgettext("Failed to download $1", package.name)
		return
	end
end

local function queue_download(package)
	local max_concurrent_downloads = tonumber(core.settings:get("contentdb_max_concurrent_downloads"))
	if number_downloading < max_concurrent_downloads then
		start_install(package)
	else
		table.insert(download_queue, package)
		package.queued = true
	end
end

local function get_raw_dependencies(package)
	if package.raw_deps then
		return package.raw_deps
	end

	local url_fmt = "/api/packages/%s/dependencies/?only_hard=1&protocol_version=%s&engine_version=%s&platform=%s"
	local version = core.get_version()
	local base_url = core.settings:get("contentdb_url")
	local url = base_url .. url_fmt:format(package.id, core.get_max_supp_proto(), version.string, PLATFORM)

	local response = http.fetch_sync({ url = url })
	if not response.succeeded then
		return
	end

	local data = core.parse_json(response.data) or {}

	local content_lookup = {}
	for _, pkg in pairs(store.packages_full) do
		content_lookup[pkg.id] = pkg
	end

	for id, raw_deps in pairs(data) do
		local package2 = content_lookup[id:lower()]
		if package2 and not package2.raw_deps then
			package2.raw_deps = raw_deps

			for _, dep in pairs(raw_deps) do
				local packages = {}
				for i=1, #dep.packages do
					packages[#packages + 1] = content_lookup[dep.packages[i]:lower()]
				end
				dep.packages = packages
			end
		end
	end

	return package.raw_deps
end

local function has_hard_deps(raw_deps)
	for i=1, #raw_deps do
		if not raw_deps[i].is_optional then
			return true
		end
	end

	return false
end

-- Recursively resolve dependencies, given the installed mods
local function resolve_dependencies_2(raw_deps, installed_mods, out)
	local function resolve_dep(dep)
		-- Check whether it's already installed
		if installed_mods[dep.name] then
			return {
				is_optional = dep.is_optional,
				name = dep.name,
				installed = true,
			}
		end

		-- Find exact name matches
		local fallback
		for _, package in pairs(dep.packages) do
			if package.type ~= "game" then
				if package.name == dep.name then
					return {
						is_optional = dep.is_optional,
						name = dep.name,
						installed = false,
						package = package,
					}
				elseif not fallback then
					fallback = package
				end
			end
		end

		-- Otherwise, find the first mod that fulfils it
		if fallback then
			return {
				is_optional = dep.is_optional,
				name = dep.name,
				installed = false,
				package = fallback,
			}
		end

		return {
			is_optional = dep.is_optional,
			name = dep.name,
			installed = false,
		}
	end

	for _, dep in pairs(raw_deps) do
		if not dep.is_optional and not out[dep.name] then
			local result  = resolve_dep(dep)
			out[dep.name] = result
			if result and result.package and not result.installed then
				local raw_deps2 = get_raw_dependencies(result.package)
				if raw_deps2 then
					resolve_dependencies_2(raw_deps2, installed_mods, out)
				end
			end
		end
	end

	return true
end

-- Resolve dependencies for a package, calls the recursive version.
local function resolve_dependencies(raw_deps, game)
	assert(game)

	local installed_mods = {}

	local mods = {}
	pkgmgr.get_game_mods(game, mods)
	for _, mod in pairs(mods) do
		installed_mods[mod.name] = true
	end

	for _, mod in pairs(pkgmgr.global_mods:get_list()) do
		installed_mods[mod.name] = true
	end

	local out = {}
	if not resolve_dependencies_2(raw_deps, installed_mods, out) then
		return nil
	end

	local retval = {}
	for _, dep in pairs(out) do
		retval[#retval + 1] = dep
	end

	table.sort(retval, function(a, b)
		return a.name < b.name
	end)

	return retval
end

local install_dialog = {}
function install_dialog.get_formspec()
	local package = install_dialog.package
	local raw_deps = install_dialog.raw_deps
	local will_install_deps = install_dialog.will_install_deps

	local selected_game_idx = 1
	local selected_gameid = core.settings:get("menu_last_game")
	local games = table.copy(pkgmgr.games)
	for i=1, #games do
		if selected_gameid and games[i].id == selected_gameid then
			selected_game_idx = i
		end

		games[i] = esc(games[i].name)
	end

	local selected_game = pkgmgr.games[selected_game_idx]
	local deps_to_install = 0
	local deps_not_found = 0

	install_dialog.dependencies = resolve_dependencies(raw_deps, selected_game)
	local formatted_deps = {}
	for _, dep in pairs(install_dialog.dependencies) do
		formatted_deps[#formatted_deps + 1] = "#fff"
		formatted_deps[#formatted_deps + 1] = esc(dep.name)
		if dep.installed then
			formatted_deps[#formatted_deps + 1] = "#ccf"
			formatted_deps[#formatted_deps + 1] = fgettext("Already installed")
		elseif dep.package then
			formatted_deps[#formatted_deps + 1] = "#cfc"
			formatted_deps[#formatted_deps + 1] = fgettext("$1 by $2", dep.package.title, dep.package.author)
			deps_to_install = deps_to_install + 1
		else
			formatted_deps[#formatted_deps + 1] = "#f00"
			formatted_deps[#formatted_deps + 1] = fgettext("Not found")
			deps_not_found = deps_not_found + 1
		end
	end

	local message_bg = "#3333"
	local message
	if will_install_deps then
		message = fgettext("$1 and $2 dependencies will be installed.", package.title, deps_to_install)
	else
		message = fgettext("$1 will be installed, and $2 dependencies will be skipped.", package.title, deps_to_install)
	end
	if deps_not_found > 0 then
		message = fgettext("$1 required dependencies could not be found.", deps_not_found) ..
				" " .. fgettext("Please check that the base game is correct.", deps_not_found) ..
				"\n" .. message
		message_bg = mt_color_orange
	end

	local formspec = {
		"formspec_version[3]",
		"size[7,7.85]",
		"bgcolor[#0000]",
		"background9[0,0;0,0;", defaulttexturedir_esc, "bg_common.png", ";true;40]",
		"style[title;border=false]",
		"button[0,0;7,0.5;title;", fgettext("Install $1", package.title) , "]",

		"container[0.375,0.70]",

		"label[0,0.25;", fgettext("Base Game:"), "]",
		"dropdown[2,0;4.25,0.5;gameid;", table.concat(games, ","), ";", selected_game_idx, "]",

		"label[0,0.8;", fgettext("Dependencies:"), "]",

		"tablecolumns[color;text;color;text]",
		scrollbar_style("packages") ..
		"table[0,1.1;6.25,3;packages;", table.concat(formatted_deps, ","), "]",

		"container_end[]",

		"checkbox[0.375,5.1;will_install_deps;",
			fgettext("Install missing dependencies"), ";",
			will_install_deps and "true" or "false", "]",

		"box[0.04,5.4;6.94,1.2;", message_bg, "]",
		"textarea[0.375,5.5;6.25,1;;;", message, "]",

		btn_style("install_all"),
		"button[0.5,6.85;2.8,0.8;install_all;", fgettext("Install"), "]",
		btn_style("cancel"),
		"button[3.75,6.85;2.8,0.8;cancel;", fgettext("Cancel"), "]",
	}

	return table.concat(formspec, "")
end

function install_dialog.handle_submit(this, fields)
	if fields.cancel then
		this:delete()
		return true
	end

	if fields.will_install_deps ~= nil then
		install_dialog.will_install_deps = core.is_yes(fields.will_install_deps)
		return true
	end

	if fields.install_all then
		queue_download(install_dialog.package)

		if install_dialog.will_install_deps then
			for _, dep in pairs(install_dialog.dependencies) do
				if not dep.is_optional and not dep.installed and dep.package then
					queue_download(dep.package)
				end
			end
		end

		this:delete()
		return true
	end

	if fields.gameid then
		for _, game in pairs(pkgmgr.games) do
			if game.name == fields.gameid then
				core.settings:set("menu_last_game", game.id)
				break
			end
		end
		return true
	end

	return false
end

function install_dialog.create(package, raw_deps)
	install_dialog.dependencies = nil
	install_dialog.package = package
	install_dialog.raw_deps = raw_deps
	install_dialog.will_install_deps = true
	return dialog_create("install_dialog",
			install_dialog.get_formspec,
			install_dialog.handle_submit,
			nil)
end


local confirm_overwrite = {}
function confirm_overwrite.get_formspec()
	local package = confirm_overwrite.package

	return "size[11.5,4.5]bgcolor[#0000]" ..
			"background9[0,0;0,0;" .. defaulttexturedir_esc .. "bg_common.png;true;40]" ..
			"label[2,2;" ..
			fgettext("\"$1\" already exists. Would you like to overwrite it?", package.name) .. "]"..
			btn_style("install", "red") ..
			"button[3.25,3.5;2.5,0.5;install;" .. fgettext("Overwrite") .. "]" ..
			btn_style("cancel") ..
			"button[5.75,3.5;2.5,0.5;cancel;" .. fgettext("Cancel") .. "]"
end

function confirm_overwrite.handle_submit(this, fields)
	if fields.cancel then
		this:delete()
		return true
	end

	if fields.install then
		this:delete()
		confirm_overwrite.callback()
		return true
	end

	return false
end

function confirm_overwrite.create(package, callback)
	assert(type(package) == "table")
	assert(type(callback) == "function")

	confirm_overwrite.package = package
	confirm_overwrite.callback = callback
	return dialog_create("confirm_overwrite",
		confirm_overwrite.get_formspec,
		confirm_overwrite.handle_submit,
		nil)
end


local function get_file_extension(path)
	local parts = path:split(".")
	return parts[#parts]
end

local function get_screenshot(package)
	if not package.thumbnail then
		return defaulttexturedir_esc .. "no_screenshot.png"
	elseif screenshot_downloading[package.thumbnail] then
		return defaulttexturedir_esc .. "loading_screenshot.png"
	end

	-- Get tmp screenshot path
	local ext = get_file_extension(package.thumbnail)
	local filepath = screenshot_dir .. DIR_DELIM ..
		("%s-%s-%s.%s"):format(package.type, package.author, package.name, ext)

	-- Return if already downloaded
	local file = io.open(filepath, "r")
	if file then
		file:close()
		return esc(filepath)
	end

	-- Show error if we've failed to download before
	if screenshot_downloaded[package.thumbnail] then
		return defaulttexturedir_esc .. "error_screenshot.png"
	end

	-- Download

	local function download_screenshot(params)
		return core.download_file(params.url, params.dest)
	end
	local function callback(success)
		screenshot_downloading[package.thumbnail] = nil
		screenshot_downloaded[package.thumbnail] = true
		if not success then
			core.log("warning", "Screenshot download failed for some reason")
		end
		ui.update()
	end
	if core.handle_async(download_screenshot,
			{ dest = filepath, url = package.thumbnail }, callback) then
		screenshot_downloading[package.thumbnail] = true
	else
		core.log("error", "ERROR: async event failed")
		return defaulttexturedir_esc .. "error_screenshot.png"
	end

	return defaulttexturedir_esc .. "loading_screenshot.png"
end

function store.load()
	local version = core.get_version()
	local base_url = core.settings:get("contentdb_url")
	local url = base_url ..
		"/api/packages/?type=mod&type=game&type=txp&protocol_version=" ..
		core.get_max_supp_proto() .. "&engine_version=" .. version.string .. "&platform=" .. PLATFORM

	for _, item in pairs(core.settings:get("contentdb_flag_blacklist"):split(",")) do
		item = item:trim()
		if item ~= "" then
			url = url .. "&hide=" .. item
		end
	end

	local timeout = tonumber(core.settings:get("curl_file_download_timeout"))
	local response = http.fetch_sync({ url = url, timeout = timeout })
	if not response.succeeded then
		return
	end

	store.packages_full = core.parse_json(response.data) or {}

	for _, package in pairs(store.packages_full) do
		package.url = base_url .. "/packages/" ..
				package.author .. "/" .. package.name ..
				"/releases/" .. package.release .. "/download/"

		local name_len = #package.name
		if package.type == "game" and name_len > 5 and package.name:sub(name_len - 4) == "_game" then
			package.id = package.author:lower() .. "/" .. package.name:sub(1, name_len - 5)
		else
			package.id = package.author:lower() .. "/" .. package.name
		end
	end

	store.packages_full_unordered = store.packages_full
	store.packages = store.packages_full
	store.loaded = true
end

function store.update_paths()
	local mod_hash = {}
	pkgmgr.refresh_globals()
	for _, mod in pairs(pkgmgr.global_mods:get_list()) do
		if mod.author and mod.release > 0 then
			mod_hash[mod.author:lower() .. "/" .. mod.name] = mod
		end
	end

	local game_hash = {}
	pkgmgr.update_gamelist()
	for _, game in pairs(pkgmgr.games) do
		if game.author ~= "" and game.release > 0 then
			game_hash[game.author:lower() .. "/" .. game.id] = game
		end
	end

	local txp_hash = {}
	for _, txp in pairs(pkgmgr.get_texture_packs()) do
		if txp.author and txp.release > 0 then
			txp_hash[txp.author:lower() .. "/" .. txp.name] = txp
		end
	end

	for _, package in pairs(store.packages_full) do
		local content
		if package.type == "mod" then
			content = mod_hash[package.id]
		elseif package.type == "game" then
			content = game_hash[package.id]
		elseif package.type == "txp" then
			content = txp_hash[package.id]
		end

		if content then
			package.path = content.path
			package.installed_release = content.release or 0
		else
			package.path = nil
		end
	end
end

function store.sort_packages()
	local ret = {}

	-- Add installed content
	for i=1, #store.packages_full_unordered do
		local package = store.packages_full_unordered[i]
		if package.path then
			ret[#ret + 1] = package
		end
	end

	-- Sort installed content by title
	table.sort(ret, function(a, b)
		return a.title < b.title
	end)

	-- Add uninstalled content
	for i=1, #store.packages_full_unordered do
		local package = store.packages_full_unordered[i]
		if not package.path then
			ret[#ret + 1] = package
		end
	end

	store.packages_full = ret
end

function store.filter_packages(query)
	if query == "" and filter_type == 1 then
		store.packages = store.packages_full
		return
	end

	local keywords = {}
	for word in query:lower():gmatch("%S+") do
		table.insert(keywords, word)
	end

	local function matches_keywords(package)
		for k = 1, #keywords do
			local keyword = keywords[k]

			if string.find(package.name:lower(), keyword, 1, true) or
					string.find(package.title:lower(), keyword, 1, true) or
					string.find(package.author:lower(), keyword, 1, true) or
					string.find(package.short_description:lower(), keyword, 1, true) then
				return true
			end
		end

		return false
	end

	store.packages = {}
	for _, package in pairs(store.packages_full) do
		if (query == "" or matches_keywords(package)) and
				(filter_type == 1 or package.type == filter_types_type[filter_type]) then
			store.packages[#store.packages + 1] = package
		end
	end
end

function store.get_formspec(dlgdata)
	store.update_paths()

	dlgdata.pagemax = math.max(math.ceil(#store.packages / num_per_page), 1)
	if cur_page > dlgdata.pagemax then
		cur_page = 1
	end

	local W = 15.75
	local H = 9.5
	local formspec
	if #store.packages_full > 0 then
		formspec = {
			"formspec_version[3]",
			"size[15.75,9.5]",
			"bgcolor[#0000]",
			"background9[0,0;0,0;", defaulttexturedir_esc, "bg_common.png;true;40]",

			"style[status,downloading,queued;border=false]",

			"container[0.375,0.375]",
			"image[0,0;7.7,0.8;", defaulttexturedir_esc, "field_bg.png;32]",
			"style[Dsearch_string;border=false;bgcolor=transparent]",
			"field[0.1,0;6.65,0.8;Dsearch_string;;", esc(search_string), "]",
			"set_focus[Dsearch_string;true]",
			"image_button[6.9,0.05;0.7,0.7;", defaulttexturedir_esc, "clear.png;clear;;true;false]",
			"container_end[]",

			-- Page nav buttons
			"container[0,", H - 0.8 - 0.375, "]",
			btn_style("back"),
			"button[0.375,0;5,0.8;back;< ", fgettext("Back to Main Menu"), "]",

			"container[", W - 0.375 - 0.8*4 - 2,  ",0]",
			btn_style("pstart"),
			"image_button[-0.1,0;0.8,0.8;", defaulttexturedir_esc, "start_icon.png;pstart;;true;false]",
			btn_style("pback"),
			"image_button[0.8,0;0.8,0.8;", defaulttexturedir_esc, "prev_icon.png;pback;;true;false]",
			"style[pagenum;border=false]",
			"button[1.5,0;2,0.8;pagenum;", core.colorize("#FFDF00", tostring(cur_page)),
				" / ", core.colorize("#FFDF00", tostring(dlgdata.pagemax)), "]",
			btn_style("pnext"),
			"image_button[3.5,0;0.8,0.8;", defaulttexturedir_esc, "next_icon.png;pnext;;true;false]",
			btn_style("pend"),
			"image_button[4.4,0;0.8,0.8;", defaulttexturedir_esc, "end_icon.png;pend;;true;false]",
			"container_end[]",

			"container_end[]",
		}

		if number_downloading > 0 then
			formspec[#formspec + 1] = "style[downloading;content_offset=0]"
			formspec[#formspec + 1] = "button[12.4,0.375;3.1,0.8;downloading;"
			if #download_queue > 0 then
				formspec[#formspec + 1] = fgettext("$1 downloading,\n$2 queued", number_downloading, #download_queue)
			else
				formspec[#formspec + 1] = fgettext("$1 downloading...", number_downloading)
			end
			formspec[#formspec + 1] = "]"
		else
			local num_avail_updates = 0
			for i=1, #store.packages_full do
				local package = store.packages_full[i]
				if package.path and package.installed_release < package.release and
						not (package.downloading or package.queued) then
					num_avail_updates = num_avail_updates + 1
				end
			end

			if num_avail_updates == 0 then
				formspec[#formspec + 1] = "style[status;content_offset=0]"
				formspec[#formspec + 1] = "button[12.4,0.375;3.1,0.8;status;"
				formspec[#formspec + 1] = fgettext("No updates")
				formspec[#formspec + 1] = "]"
			else
				formspec[#formspec + 1] = "style[update_all;content_offset=0]"
				formspec[#formspec + 1] = btn_style("update_all")
				formspec[#formspec + 1] = "button[12.4,0.375;3.1,0.8;update_all;"
				formspec[#formspec + 1] = fgettext("Update All [$1]", num_avail_updates)
				formspec[#formspec + 1] = "]"
			end
		end

		if #store.packages == 0 then
			formspec[#formspec + 1] = "style[msg;content_offset=0]"
			formspec[#formspec + 1] = "image_button[1,4.25;13.75,1;;msg;"
			formspec[#formspec + 1] = fgettext("No results")
			formspec[#formspec + 1] = ";false;false]"
		end
	else
		formspec = {
			"size[12,6.4]",
			"bgcolor[#0000]",
			"background9[0,0;0,0;", defaulttexturedir_esc, "bg_common.png;true;40]",
			"label[4,3;", fgettext("No packages could be retrieved"), "]",
			btn_style("back"),
			"button[0-0.11,5.8;5.5,0.9;back;< ", fgettext("Back to Main Menu"), "]",
		}
	end

	-- download/queued tooltips always have the same message
	local tooltip_colors = ";#dff6f5;#302c2e]"
	formspec[#formspec + 1] = "tooltip[downloading;" .. fgettext("Downloading...") .. tooltip_colors
	formspec[#formspec + 1] = "tooltip[queued;" .. fgettext("Queued") .. tooltip_colors

	local start_idx = (cur_page - 1) * num_per_page + 1
	for i=start_idx, math.min(#store.packages, start_idx+num_per_page-1) do
		local package = store.packages[i]
		local container_y = (i - start_idx) * 1.375 + (2*0.375 + 0.8)
		formspec[#formspec + 1] = "container[0.375,"
		formspec[#formspec + 1] = container_y
		formspec[#formspec + 1] = "]"

		-- image
		formspec[#formspec + 1] = "image[0,0;1.5,1;"
		formspec[#formspec + 1] = get_screenshot(package)
		formspec[#formspec + 1] = "]"

		formspec[#formspec + 1] = "image[-0.01,-0.01;1.52,1.02;"
		formspec[#formspec + 1] = defaulttexturedir_esc
		formspec[#formspec + 1] = "gui"
		formspec[#formspec + 1] = DIR_DELIM_esc
		formspec[#formspec + 1] = "cdb_img_corners.png;15]"

		-- title
		formspec[#formspec + 1] = "label[1.875,0.1;"
		formspec[#formspec + 1] = esc(
				core.colorize(mt_color_green, package.title) ..
				core.colorize("#BFBFBF", " by " .. package.author))
		formspec[#formspec + 1] = "]"

		-- buttons
		local left_base = "image_button[-1.55,0;0.7,0.7;" .. defaulttexturedir_esc
		formspec[#formspec + 1] = "container["
		formspec[#formspec + 1] = W - 0.375*2
		formspec[#formspec + 1] = ",0.1]"

		if package.downloading then
			formspec[#formspec + 1] = "animated_image[-1.7,-0.15;1,1;downloading;"
			formspec[#formspec + 1] = defaulttexturedir_esc
			formspec[#formspec + 1] = "cdb_downloading.png;4;300;]"
		elseif package.queued then
			formspec[#formspec + 1] = left_base
			formspec[#formspec + 1] = defaulttexturedir_esc
			formspec[#formspec + 1] = "cdb_queued.png;queued]"
		elseif not package.path then
			local elem_name = "install_" .. i .. ";"
			formspec[#formspec + 1] = btn_style("install_" .. i, "green", true)
			formspec[#formspec + 1] = left_base .. "cdb_add.png;" .. elem_name .. "]"
			formspec[#formspec + 1] = "tooltip[" .. elem_name .. fgettext("Install") .. tooltip_colors
		else
			if package.installed_release < package.release then

				-- The install_ action also handles updating
				local elem_name = "install_" .. i .. ";"
				formspec[#formspec + 1] = btn_style("install_" .. i, nil, true)
				formspec[#formspec + 1] = left_base .. "cdb_update.png;" .. elem_name .. "]"
				formspec[#formspec + 1] = "tooltip[" .. elem_name .. fgettext("Update") .. tooltip_colors
			else

				local elem_name = "uninstall_" .. i .. ";"
				formspec[#formspec + 1] = btn_style("uninstall_" .. i, "red", true)
				formspec[#formspec + 1] = left_base .. "cdb_clear.png;" .. elem_name .. "]"
				formspec[#formspec + 1] = "tooltip[" .. elem_name .. fgettext("Uninstall") .. tooltip_colors
			end
		end

		local web_elem_name = "view_" .. i .. ";"
		formspec[#formspec + 1] = btn_style("view_" .. i, nil, true)
		formspec[#formspec + 1] = "image_button[-0.7,0;0.7,0.7;" ..
			defaulttexturedir_esc .. "cdb_viewonline.png;" .. web_elem_name .. "]"
		formspec[#formspec + 1] = "tooltip[" .. web_elem_name ..
			fgettext("View more information in a web browser") .. tooltip_colors
		formspec[#formspec + 1] = "container_end[]"

		-- description
		local description_width = W - 0.375*5 - 0.85 - 2*0.7
		formspec[#formspec + 1] = "style_type[textarea;font_size=-2]"
		formspec[#formspec + 1] = "textarea[1.855,0.3;"
		formspec[#formspec + 1] = tostring(description_width)
		formspec[#formspec + 1] = ",0.8;;;"
		formspec[#formspec + 1] = esc(package.short_description)
		formspec[#formspec + 1] = "]"
		formspec[#formspec + 1] = "style_type[textarea;font_size=]"

		formspec[#formspec + 1] = "container_end[]"
	end

	-- Add the dropdown last so that it is over top of everything else
	if #store.packages_full > 0 then
		formspec[#formspec + 1] = get_dropdown(8.22, 0.375, 4, "change_type",
			filter_types_titles, filter_type, dropdown_open)
	end

	return table.concat(formspec, "")
end

function store.handle_submit(this, fields)
	if fields.clear then
		search_string = ""
		cur_page = 1
		store.filter_packages("")
		return true
	end

	if fields.back then
		this:delete()
		return true
	end

	if fields.pstart then
		cur_page = 1
		return true
	end

	if fields.pend then
		cur_page = this.data.pagemax
		return true
	end

	if fields.pnext then
		cur_page = cur_page + 1
		if cur_page > this.data.pagemax then
			cur_page = 1
		end
		return true
	end

	if fields.pback then
		if cur_page == 1 then
			cur_page = this.data.pagemax
		else
			cur_page = cur_page - 1
		end
		return true
	end

	if fields.change_type then
		dropdown_open = true
		return true
	end

	for field in pairs(fields) do
		if field:sub(1, 9) == "dropdown_" then
			dropdown_open = false
			local new_type = tonumber(field:sub(10))
			if new_type and new_type ~= filter_type then
				filter_type = new_type
				cur_page = 1
				store.filter_packages(search_string)
			end
			return true
		end
	end

	if fields.update_all then
		for i=1, #store.packages_full do
			local package = store.packages_full[i]
			if package.path and package.installed_release < package.release and
					not (package.downloading or package.queued) then
				queue_download(package)
			end
		end
		return true
	end

	local start_idx = (cur_page - 1) * num_per_page + 1
	assert(start_idx ~= nil)
	for i=start_idx, math.min(#store.packages, start_idx+num_per_page-1) do
		local package = store.packages[i]
		assert(package)

		if fields["install_" .. i] then
			local install_parent
			if package.type == "mod" then
				install_parent = core.get_modpath()
			elseif package.type == "game" then
				install_parent = core.get_gamepath()
			elseif package.type == "txp" then
				install_parent = core.get_texturepath()
			else
				error("Unknown package type: " .. package.type)
			end


			local function on_confirm()
				local deps = get_raw_dependencies(package)
				if deps and has_hard_deps(deps) and #pkgmgr.games > 0 then
					local dlg = install_dialog.create(package, deps)
					dlg:set_parent(this)
					this:hide()
					dlg:show()
				else
					queue_download(package)
				end
			end

			if not package.path and core.is_dir(install_parent .. DIR_DELIM .. package.name) then
				local dlg = confirm_overwrite.create(package, on_confirm)
				dlg:set_parent(this)
				this:hide()
				dlg:show()
			else
				on_confirm()
			end

			return true
		end

		if fields["uninstall_" .. i] then
			local dlg = create_delete_content_dlg(package)
			dlg:set_parent(this)
			this:hide()
			dlg:show()
			return true
		end

		if fields["view_" .. i] then
			local url = ("%s/packages/%s/%s?protocol_version=%d&platform=%s"):format(
					core.settings:get("contentdb_url"),
					package.author, package.name, core.get_max_supp_proto(), PLATFORM)
			core.open_url(url)
			return true
		end
	end

	-- Should be last
	if fields.Dsearch_string then
		search_string = fields.Dsearch_string:trim()
		cur_page = 1
		store.filter_packages(search_string)
		return true
	end

	return false
end

function create_store_dlg(type)
	if not store.loaded or #store.packages_full == 0 then
		store.load()
	end

	store.update_paths()
	store.sort_packages()

	search_string = ""
	cur_page = 1

	if type then
		-- table.indexof does not work on tables that contain `nil`
		for i, v in pairs(filter_types_type) do
			if v == type then
				filter_type = i
				break
			end
		end
	end

	store.filter_packages(search_string)

	return dialog_create("store",
			store.get_formspec,
			store.handle_submit,
			nil)
end
