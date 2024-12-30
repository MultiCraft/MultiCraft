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

#include "lua_api/l_server.h"
#include "lua_api/l_internal.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "cpp_api/s_base.h"
#include "cpp_api/s_security.h"
#include "server.h"
#include "environment.h"
#include "remoteplayer.h"
#include "log.h"
#include <algorithm>

// request_shutdown()
int ModApiServer::l_request_shutdown(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char *msg = lua_tolstring(L, 1, NULL);
	bool reconnect = readParam<bool>(L, 2);
	float seconds_before_shutdown = lua_tonumber(L, 3);
	getServer(L)->requestShutdown(msg ? msg : "", reconnect, seconds_before_shutdown);
	return 0;
}

// get_server_status()
int ModApiServer::l_get_server_status(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushstring(L, getServer(L)->getStatusString().c_str());
	return 1;
}

// get_server_uptime()
int ModApiServer::l_get_server_uptime(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushnumber(L, getServer(L)->getUptime());
	return 1;
}


// print(text)
int ModApiServer::l_print(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string text;
	text = luaL_checkstring(L, 1);
	getServer(L)->printToConsoleOnly(text);
	return 0;
}

// chat_send_all(text)
int ModApiServer::l_chat_send_all(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char *text = luaL_checkstring(L, 1);
	// Get server from registry
	Server *server = getServer(L);
	// Send
	try {
		server->notifyPlayers(utf8_to_wide(text));
	} catch (PacketError &e) {
		warningstream << "Exception caught: " << e.what() << std::endl
			<< script_get_backtrace(L) << std::endl;
		server->notifyPlayers(utf8_to_wide(std::string("Internal error: ") + e.what()));
	}

	return 0;
}

// chat_send_player(name, text)
int ModApiServer::l_chat_send_player(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char *name = luaL_checkstring(L, 1);
	const char *text = luaL_checkstring(L, 2);

	// Get server from registry
	Server *server = getServer(L);
	// Send
	try {
		server->notifyPlayer(name, utf8_to_wide(text));
	} catch (PacketError &e) {
		warningstream << "Exception caught: " << e.what() << std::endl
			<< script_get_backtrace(L) << std::endl;
		server->notifyPlayer(name, utf8_to_wide(std::string("Internal error: ") + e.what()));
	}
	return 0;
}

// get_player_privs(name, text)
int ModApiServer::l_get_player_privs(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char *name = luaL_checkstring(L, 1);
	// Get server from registry
	Server *server = getServer(L);
	// Do it
	lua_newtable(L);
	int table = lua_gettop(L);
	std::set<std::string> privs_s = server->getPlayerEffectivePrivs(name);
	for (const std::string &privs_ : privs_s) {
		lua_pushboolean(L, true);
		lua_setfield(L, table, privs_.c_str());
	}
	lua_pushvalue(L, table);
	return 1;
}

// get_player_ip()
int ModApiServer::l_get_player_ip(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	Server *server = getServer(L);

	const char *name = luaL_checkstring(L, 1);
	RemotePlayer *player = server->getEnv().getPlayer(name);
	if (!player) {
		lua_pushnil(L); // no such player
		return 1;
	}

	lua_pushstring(L, server->getPeerAddress(player->getPeerId()).serializeString().c_str());
	return 1;
}

// get_player_information(name)
int ModApiServer::l_get_player_information(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	Server *server = getServer(L);

	const char *name = luaL_checkstring(L, 1);
	RemotePlayer *player = server->getEnv().getPlayer(name);
	if (!player) {
		lua_pushnil(L); // no such player
		return 1;
	}

	/*
		Be careful not to introduce a depdendency on the connection to
		the peer here. This function is >>REQUIRED<< to still be able to return
		values even when the peer unexpectedly disappears.
		Hence all the ConInfo values here are optional.
	*/

	auto getConInfo = [&] (con::rtt_stat_type type, float *value) -> bool {
		return server->getClientConInfo(player->getPeerId(), type, value);
	};

	float min_rtt, max_rtt, avg_rtt, min_jitter, max_jitter, avg_jitter;
	bool have_con_info =
		getConInfo(con::MIN_RTT, &min_rtt) &&
		getConInfo(con::MAX_RTT, &max_rtt) &&
		getConInfo(con::AVG_RTT, &avg_rtt) &&
		getConInfo(con::MIN_JITTER, &min_jitter) &&
		getConInfo(con::MAX_JITTER, &max_jitter) &&
		getConInfo(con::AVG_JITTER, &avg_jitter);

	ClientInfo info;
	if (!server->getClientInfo(player->getPeerId(), info)) {
		warningstream << FUNCTION_NAME << ": no client info?!" << std::endl;
		lua_pushnil(L); // error
		return 1;
	}

	lua_newtable(L);
	int table = lua_gettop(L);

	lua_pushstring(L,"address");
	lua_pushstring(L, info.addr.serializeString().c_str());
	lua_settable(L, table);

	lua_pushstring(L,"ip_version");
	if (info.addr.getFamily() == AF_INET) {
		lua_pushnumber(L, 4);
	} else if (info.addr.getFamily() == AF_INET6) {
		lua_pushnumber(L, 6);
	} else {
		lua_pushnumber(L, 0);
	}
	lua_settable(L, table);

	if (have_con_info) { // may be missing
		lua_pushstring(L, "min_rtt");
		lua_pushnumber(L, min_rtt);
		lua_settable(L, table);

		lua_pushstring(L, "max_rtt");
		lua_pushnumber(L, max_rtt);
		lua_settable(L, table);

		lua_pushstring(L, "avg_rtt");
		lua_pushnumber(L, avg_rtt);
		lua_settable(L, table);

		lua_pushstring(L, "min_jitter");
		lua_pushnumber(L, min_jitter);
		lua_settable(L, table);

		lua_pushstring(L, "max_jitter");
		lua_pushnumber(L, max_jitter);
		lua_settable(L, table);

		lua_pushstring(L, "avg_jitter");
		lua_pushnumber(L, avg_jitter);
		lua_settable(L, table);
	}

	lua_pushstring(L,"connection_uptime");
	lua_pushnumber(L, info.uptime);
	lua_settable(L, table);

	lua_pushstring(L,"protocol_version");
	lua_pushnumber(L, info.prot_vers);
	lua_settable(L, table);

	lua_pushstring(L, "formspec_version");
	lua_pushnumber(L, player->formspec_version);
	lua_settable(L, table);

	lua_pushstring(L, "lang_code");
	lua_pushstring(L, info.lang_code.c_str());
	lua_settable(L, table);

	lua_pushstring(L,"serialization_version");
	lua_pushnumber(L, info.ser_vers);
	lua_settable(L, table);

	lua_pushstring(L,"major");
	lua_pushnumber(L, info.major);
	lua_settable(L, table);

	lua_pushstring(L,"minor");
	lua_pushnumber(L, info.minor);
	lua_settable(L, table);

	lua_pushstring(L,"patch");
	lua_pushnumber(L, info.patch);
	lua_settable(L, table);

	lua_pushstring(L,"version_string");
	lua_pushstring(L, info.vers_string.c_str());
	lua_settable(L, table);

	lua_pushstring(L,"platform");
	lua_pushstring(L, info.platform.c_str());
	lua_settable(L, table);

	lua_pushstring(L,"sysinfo");
	lua_pushstring(L, info.sysinfo.c_str());
	lua_settable(L, table);

	lua_pushstring(L,"state");
	lua_pushstring(L, ClientInterface::state2Name(info.state).c_str());
	lua_settable(L, table);

	return 1;
}

// get_ban_list()
int ModApiServer::l_get_ban_list(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushstring(L, getServer(L)->getBanDescription("").c_str());
	return 1;
}

// get_ban_description()
int ModApiServer::l_get_ban_description(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char * ip_or_name = luaL_checkstring(L, 1);
	lua_pushstring(L, getServer(L)->getBanDescription(std::string(ip_or_name)).c_str());
	return 1;
}

// ban_player()
int ModApiServer::l_ban_player(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	Server *server = getServer(L);

	const char *name = luaL_checkstring(L, 1);
	RemotePlayer *player = server->getEnv().getPlayer(name);
	if (!player) {
		lua_pushboolean(L, false); // no such player
		return 1;
	}

	std::string ip_str = server->getPeerAddress(player->getPeerId()).serializeString();
	server->setIpBanned(ip_str, name);
	lua_pushboolean(L, true);
	return 1;
}

// kick_player(name, [reason]) -> success
int ModApiServer::l_kick_player(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char *name = luaL_checkstring(L, 1);
	std::string message("Kicked");
	if (lua_isstring(L, 2))
		message.append(": ").append(readParam<std::string>(L, 2));
	else
		message.append(".");

	Server *server = getServer(L);

	RemotePlayer *player = server->getEnv().getPlayer(name);
	if (!player) {
		lua_pushboolean(L, false); // No such player
		return 1;
	}

	server->DenyAccess(player->getPeerId(), SERVER_ACCESSDENIED_CUSTOM_STRING, message);
	lua_pushboolean(L, true);
	return 1;
}

int ModApiServer::l_remove_player(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string name = luaL_checkstring(L, 1);
	ServerEnvironment *s_env = dynamic_cast<ServerEnvironment *>(getEnv(L));
	assert(s_env);

	RemotePlayer *player = s_env->getPlayer(name.c_str());
	if (!player)
		lua_pushinteger(L, s_env->removePlayerFromDatabase(name) ? 0 : 1);
	else
		lua_pushinteger(L, 2);

	return 1;
}

// unban_player_or_ip()
int ModApiServer::l_unban_player_or_ip(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char * ip_or_name = luaL_checkstring(L, 1);
	getServer(L)->unsetIpBanned(ip_or_name);
	lua_pushboolean(L, true);
	return 1;
}

// show_formspec(playername,formname,formspec)
int ModApiServer::l_show_formspec(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char *playername = luaL_checkstring(L, 1);
	const char *formname = luaL_checkstring(L, 2);
	const char *formspec = luaL_checkstring(L, 3);

	if(getServer(L)->showFormspec(playername,formspec,formname))
	{
		lua_pushboolean(L, true);
	}else{
		lua_pushboolean(L, false);
	}
	return 1;
}

// get_current_modname()
int ModApiServer::l_get_current_modname(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_rawgeti(L, LUA_REGISTRYINDEX, CUSTOM_RIDX_CURRENT_MOD_NAME);
	return 1;
}

// get_modpath(modname)
int ModApiServer::l_get_modpath(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string modname = luaL_checkstring(L, 1);
	const ModSpec *mod = getServer(L)->getModSpec(modname);
	if (!mod) {
		lua_pushnil(L);
		return 1;
	}
	lua_pushstring(L, mod->path.c_str());
	return 1;
}

// get_modnames()
// the returned list is sorted alphabetically for you
int ModApiServer::l_get_modnames(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	// Get a list of mods
	std::vector<std::string> modlist;
	getServer(L)->getModNames(modlist);

	std::sort(modlist.begin(), modlist.end());

	// Package them up for Lua
	lua_createtable(L, modlist.size(), 0);
	std::vector<std::string>::iterator iter = modlist.begin();
	for (u16 i = 0; iter != modlist.end(); ++iter) {
		lua_pushstring(L, iter->c_str());
		lua_rawseti(L, -2, ++i);
	}
	return 1;
}

// get_worldpath()
int ModApiServer::l_get_worldpath(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string worldpath = getServer(L)->getWorldPath();
	lua_pushstring(L, worldpath.c_str());
	return 1;
}

// sound_play(spec, parameters, [ephemeral])
int ModApiServer::l_sound_play(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	SimpleSoundSpec spec;
	read_soundspec(L, 1, spec);
	ServerSoundParams params;
	read_server_sound_params(L, 2, params);
	bool ephemeral = lua_gettop(L) > 2 && readParam<bool>(L, 3);
	if (ephemeral) {
		getServer(L)->playSound(spec, params, true);
		lua_pushnil(L);
	} else {
		s32 handle = getServer(L)->playSound(spec, params);
		lua_pushinteger(L, handle);
	}
	return 1;
}

// sound_stop(handle)
int ModApiServer::l_sound_stop(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	s32 handle = luaL_checkinteger(L, 1);
	getServer(L)->stopSound(handle);
	return 0;
}

int ModApiServer::l_sound_fade(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	s32 handle = luaL_checkinteger(L, 1);
	float step = readParam<float>(L, 2);
	float gain = readParam<float>(L, 3);
	getServer(L)->fadeSound(handle, step, gain);
	return 0;
}

// dynamic_add_media(filepath)
int ModApiServer::l_dynamic_add_media_raw(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	if (!getEnv(L))
		throw LuaError("Dynamic media cannot be added before server has started up");

	std::string filepath = readParam<std::string>(L, 1);
	CHECK_SECURE_PATH(L, filepath.c_str(), false);

	std::vector<RemotePlayer*> sent_to;
	bool ok = getServer(L)->dynamicAddMedia(filepath, sent_to);
	if (ok) {
		// (see wrapper code in builtin)
		lua_createtable(L, sent_to.size(), 0);
		int i = 0;
		for (RemotePlayer *player : sent_to) {
			lua_pushstring(L, player->getName());
			lua_rawseti(L, -2, ++i);
		}
	} else {
		lua_pushboolean(L, false);
	}

	return 1;
}

// static_add_media(filepath, filename)
int ModApiServer::l_static_add_media(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const std::string filename = luaL_checkstring(L, 1);
	const std::string filepath = luaL_checkstring(L, 2);

	Server *server = getServer(L);
	server->addMediaFile(filename, filepath);
	return 0;
}

// is_singleplayer()
int ModApiServer::l_is_singleplayer(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushboolean(L, getServer(L)->isSingleplayer());
	return 1;
}

// notify_authentication_modified(name)
int ModApiServer::l_notify_authentication_modified(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string name;
	if(lua_isstring(L, 1))
		name = readParam<std::string>(L, 1);
	getServer(L)->reportPrivsModified(name);
	return 0;
}

// get_last_run_mod()
int ModApiServer::l_get_last_run_mod(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_rawgeti(L, LUA_REGISTRYINDEX, CUSTOM_RIDX_CURRENT_MOD_NAME);
	std::string current_mod = readParam<std::string>(L, -1, "");
	if (current_mod.empty()) {
		lua_pop(L, 1);
		lua_pushstring(L, getScriptApiBase(L)->getOrigin().c_str());
	}
	return 1;
}

// set_last_run_mod(modname)
int ModApiServer::l_set_last_run_mod(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
#ifdef SCRIPTAPI_DEBUG
	const char *mod = lua_tostring(L, 1);
	getScriptApiBase(L)->setOriginDirect(mod);
	//printf(">>>> last mod set from Lua: %s\n", mod);
#endif
	return 0;
}

void ModApiServer::Initialize(lua_State *L, int top)
{
	API_FCT(request_shutdown);
	API_FCT(get_server_status);
	API_FCT(get_server_uptime);
	API_FCT(get_worldpath);
	API_FCT(is_singleplayer);

	API_FCT(get_current_modname);
	API_FCT(get_modpath);
	API_FCT(get_modnames);

	API_FCT(print);

	API_FCT(chat_send_all);
	API_FCT(chat_send_player);
	API_FCT(show_formspec);
	API_FCT(sound_play);
	API_FCT(sound_stop);
	API_FCT(sound_fade);
	API_FCT(dynamic_add_media_raw);
	API_FCT(static_add_media);

	API_FCT(get_player_information);
	API_FCT(get_player_privs);
	API_FCT(get_player_ip);
	API_FCT(get_ban_list);
	API_FCT(get_ban_description);
	API_FCT(ban_player);
	API_FCT(kick_player);
	API_FCT(remove_player);
	API_FCT(unban_player_or_ip);
	API_FCT(notify_authentication_modified);

	API_FCT(get_last_run_mod);
	API_FCT(set_last_run_mod);
}
