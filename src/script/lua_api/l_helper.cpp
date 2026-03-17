/*
Minetest
Copyright (C) 2013 sapier

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

int ModApiHelper::l_do_async_callback(lua_State *L)
{
	HelperScripting *script = getScriptApi<HelperScripting>(L);

	std::string serialized_func = readParam<std::string>(L, 1);
	std::string serialized_param = readParam<std::string>(L, 2);

	lua_pushinteger(L, script->queueAsync(serialized_func, serialized_param));

	return 1;
}

void ModApiHelper::Initialize(lua_State *L, int top)
{
	API_FCT(do_async_callback);
}
