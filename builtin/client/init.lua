-- Minetest: builtin/client/init.lua
local scriptpath = core.get_builtin_path()
local clientpath = scriptpath.."client"..DIR_DELIM
local commonpath = scriptpath.."common"..DIR_DELIM

dofile(clientpath .. "register.lua")
dofile(commonpath .. "after.lua")
dofile(commonpath .. "chatcommands.lua")
dofile(clientpath .. "chatcommands.lua")
dofile(commonpath .. "vector.lua")
dofile(commonpath .. "btn_style.lua")
dofile(clientpath .. "death_formspec.lua")
dofile(clientpath .. "sscsm.lua")
dofile(clientpath .. "misc.lua")
