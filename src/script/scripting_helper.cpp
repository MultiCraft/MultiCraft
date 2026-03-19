/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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
#include "cpp_api/s_internal.h"
#include "lua_api/l_base.h"
#include "lua_api/l_helper.h"
#include "lua_api/l_http.h"
#include "lua_api/l_mainmenu.h"
#include "lua_api/l_noise.h"
#include "lua_api/l_util.h"
#include "lua_api/l_settings.h"
#include "log.h"
#include "filesys.h"

extern "C" {
#include "lualib.h"
}
#define HELPER_NUM_ASYNC_THREADS 4

// Set in main.cpp
HelperScripting *g_helper_script = nullptr;

HelperScripting::HelperScripting() : ScriptApiBase(ScriptingType::Helper)
{
	SCRIPTAPI_PRECHECKHEADER

	lua_getglobal(L, "core");
	int top = lua_gettop(L);

	// Initialize our lua_api modules
	initializeModApi(L, top);
	lua_pop(L, 1);

	// Push builtin initialization type
	lua_pushstring(L, "helper");
	lua_setglobal(L, "INIT");

	loadScript(porting::path_share + DIR_DELIM "builtin" DIR_DELIM "init.lua");

	infostream << "SCRIPTAPI: Initialized helper modules" << std::endl;
}

/******************************************************************************/
void HelperScripting::initializeModApi(lua_State *L, int top)
{
	registerLuaClasses(L, top);

	// Initialize mod API modules
	ModApiMainMenu::InitializeAsync(L, top);
	ModApiUtil::InitializeMainMenu(L, top);
	ModApiHttp::Initialize(L, top);
	ModApiHelper::Initialize(L, top);

	asyncEngine.registerStateInitializer(registerLuaClasses);
	asyncEngine.registerStateInitializer(ModApiMainMenu::InitializeAsync);
	asyncEngine.registerStateInitializer(ModApiUtil::InitializeAsync);
	asyncEngine.registerStateInitializer(ModApiHttp::InitializeAsync);
}

/******************************************************************************/
void HelperScripting::registerLuaClasses(lua_State *L, int top)
{
	LuaSecureRandom::Register(L);
	LuaSettings::Register(L);
}

/******************************************************************************/
void HelperScripting::step()
{
	asyncEngine.step(getStack());

	process_update_notifications();
}

/******************************************************************************/
unsigned int HelperScripting::queueAsync(
		const std::string &serialized_func, const std::string &serialized_param)
{
	// Lazily initialize async environment if/when it is needed
	if (!asyncEngine.isInitialized())
		asyncEngine.initialize(HELPER_NUM_ASYNC_THREADS);

	return asyncEngine.queueAsyncJob(serialized_func, serialized_param);
}
