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

#pragma once

#include "lua_api/l_base.h"

class ModApiHelper : public ModApiBase
{

private:
	static int l_do_async_callback(lua_State *L);

	static int l_notify_game(lua_State *L);

public:
	/**
	 * initialize this API module
	 * @param L lua stack to initialize
	 * @param top index (in lua stack) of global API table
	 */
	static void Initialize(lua_State *L, int top);

	static void InitializeClient(lua_State *L, int top);
};
