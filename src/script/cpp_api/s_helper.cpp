/*
MultiCraft
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2017 nerzhul, Loic Blot <loic.blot@unix-experience.fr>
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

#include "s_internal.h"
#include "lua_api/l_helper.h"
#include "scripting_helper.h"
#include "gui/guiEngine.h"
#include "util/container.h"

extern MutexedQueue<UpdateNotification> g_helper_to_game;
extern MutexedQueue<UpdateNotification> g_game_to_helper;

void ScriptApiHelper::process_update_notifications()
{
	SCRIPTAPI_PRECHECKHEADER

	const bool is_helper = getType() == ScriptingType::Helper;
	MutexedQueue<UpdateNotification> *queue =
			is_helper ? &g_game_to_helper : &g_helper_to_game;

	if (is_helper) {
		UpdateNotification n;
		if (GUIEngine::readUpdate(&n.channel, &n.msg)) {
			n.source = ScriptingType::Helper;
			queue->push_back(n);
		}
	}

	while (true) {
		UpdateNotification n = queue->pop_frontNoEx(0);
		if (n.source == ScriptingType::Invalid)
			break;

		try {
			lua_getglobal(L, "core");
			lua_getfield(L, -1, "registered_on_update");
			lua_pushlstring(L, n.channel.c_str(), n.channel.size());
			lua_pushlstring(L, n.msg.c_str(), n.msg.size());

			if (is_helper) {
				std::string source;
				switch (n.source) {
				case ScriptingType::Helper:
					source = "helper";
					break;
				case ScriptingType::MainMenu:
					source = "main_menu";
					break;
				case ScriptingType::Client:
					source = "client";
					break;
				}
				lua_pushlstring(L, source.c_str(), source.size());
				runCallbacks(3, RUN_CALLBACKS_MODE_OR_SC);
			} else {
				runCallbacks(2, RUN_CALLBACKS_MODE_FIRST);
			}
		} catch (LuaError &e) {
			if (is_helper && n.source == ScriptingType::Helper)
				// If there's a Lua error, just pass the message onto the
				// game
				g_helper_to_game.push_back(n);
			throw;
		}
	}
}
