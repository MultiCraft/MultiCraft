/*
MultiCraft
Copyright (C) 2013 sapier
Copyright (C) 2026 MultiCraft Development Team

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3.0 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "scripting_helper.h"
#include "lua_api/l_helper.h"
#include "lua_api/l_internal.h"
#include "common/c_converter.h"
#include "util/container.h"

MutexedQueue<UpdateNotification> g_helper_to_game;
MutexedQueue<UpdateNotification> g_game_to_helper;

int ModApiHelper::l_do_async_callback(lua_State *L)
{
	HelperScripting *script = getScriptApi<HelperScripting>(L);

	std::string serialized_func = readParam<std::string>(L, 1);
	std::string serialized_param = readParam<std::string>(L, 2);

	lua_pushinteger(L, script->queueAsync(serialized_func, serialized_param));

	return 1;
}

int ModApiHelper::l_notify_game(lua_State *L)
{
	UpdateNotification n;
	n.source = getScriptApiBase(L)->getType();
	n.channel = readParam<std::string>(L, 1);
	n.msg = readParam<std::string>(L, 2);

	if (n.source == ScriptingType::Helper)
		g_helper_to_game.push_back(n);
	else
		g_game_to_helper.push_back(n);

	return 0;
}

void ModApiHelper::Initialize(lua_State *L, int top)
{
	API_FCT(do_async_callback);
	API_FCT(notify_game);
}

void ModApiHelper::InitializeClient(lua_State *L, int top)
{
	registerFunction(L, "notify_helper", l_notify_game, top);
}
