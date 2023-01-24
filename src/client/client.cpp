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

#include <iostream>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <IFileSystem.h>
#include "client.h"
#include "network/clientopcodes.h"
#include "network/connection.h"
#include "network/networkpacket.h"
#include "threading/mutex_auto_lock.h"
#include "client/clientevent.h"
#include "client/gameui.h"
#include "client/renderingengine.h"
#include "client/sound.h"
#include "client/tile.h"
#include "util/auth.h"
#include "util/directiontables.h"
#include "util/pointedthing.h"
#include "util/serialize.h"
#include "util/string.h"
#include "util/srp.h"
#include "filesys.h"
#include "mapblock_mesh.h"
#include "mapblock.h"
#include "minimap.h"
#include "modchannels.h"
#include "content/mods.h"
#include "profiler.h"
#include "shader.h"
#include "gettext.h"
#include "clientmap.h"
#include "clientmedia.h"
#include "version.h"
#if USE_SQLITE
#include "database/database-sqlite3.h"
#endif
#include "serialization.h"
#include "guiscalingfilter.h"
#include "script/scripting_client.h"
#include "game.h"
#include "chatmessage.h"
#include "translation.h"

extern gui::IGUIEnvironment* guienv;

/*
	Utility classes
*/

u32 PacketCounter::sum() const
{
	u32 n = 0;
	for (const auto &it : m_packets)
		n += it.second;
	return n;
}

void PacketCounter::print(std::ostream &o) const
{
	for (const auto &it : m_packets) {
		auto name = it.first >= TOCLIENT_NUM_MSG_TYPES ? "?"
			: toClientCommandTable[it.first].name;
		o << "cmd " << it.first << " (" << name << ") count "
			<< it.second << std::endl;
	}
}

/*
	Client
*/

Client::Client(
		const char *playername,
		const std::string &password,
		const std::string &address_name,
		MapDrawControl &control,
		IWritableTextureSource *tsrc,
		IWritableShaderSource *shsrc,
		IWritableItemDefManager *itemdef,
		NodeDefManager *nodedef,
		ISoundManager *sound,
		MtEventManager *event,
		bool ipv6,
		GameUI *game_ui
):
	m_tsrc(tsrc),
	m_shsrc(shsrc),
	m_itemdef(itemdef),
	m_nodedef(nodedef),
	m_sound(sound),
	m_event(event),
	m_mesh_update_thread(this),
	m_env(
		new ClientMap(this, control, 666),
		tsrc, this
	),
	m_particle_manager(&m_env),
	m_con(new con::Connection(PROTOCOL_ID, 512, CONNECTION_TIMEOUT, ipv6, this)),
	m_address_name(address_name),
	m_server_ser_ver(SER_FMT_VER_INVALID),
	m_last_chat_message_sent(time(NULL)),
	m_password(password),
	m_chosen_auth_mech(AUTH_MECHANISM_NONE),
	m_media_downloader(new ClientMediaDownloader()),
	m_state(LC_Created),
	m_game_ui(game_ui),
	m_modchannel_mgr(new ModChannelMgr())
{
	// Add local player
	m_env.setLocalPlayer(new LocalPlayer(this, playername));

	if (g_settings->getBool("enable_minimap")) {
		m_minimap = new Minimap(this);
	}

	m_cache_save_interval = g_settings->getU16("server_map_save_interval");
	m_round_screen = g_settings->getU16("round_screen");
	m_hud_scaling = g_settings->getFloat("hud_scaling");
	m_inv_item_anim_enabled = g_settings->getBool("inventory_items_animations");
}

void Client::loadMods()
{
	// Don't load mods twice.
	// If client scripting is disabled by the client, don't load builtin or
	// client-provided mods.
	if (m_mods_loaded || !g_settings->getBool("enable_client_modding"))
		return;

	// If client scripting is disabled by the server, don't load builtin or
	// client-provided mods.
	// TODO Delete this code block when server-sent CSM and verifying of builtin are
	// complete.
	if (checkCSMRestrictionFlag(CSMRestrictionFlags::CSM_RF_LOAD_CLIENT_MODS)) {
		warningstream << "Client-provided mod loading is disabled by server." <<
			std::endl;
		return;
	}

	m_script = new ClientScripting(this);
	m_env.setScript(m_script);
	m_script->setEnv(&m_env);

	// Load builtin
	scanModIntoMemory(BUILTIN_MOD_NAME, getBuiltinLuaPath());
	m_script->loadModFromMemory(BUILTIN_MOD_NAME);

	ClientModConfiguration modconf(getClientModsLuaPath());
	m_mods = modconf.getMods();
	// complain about mods with unsatisfied dependencies
	if (!modconf.isConsistent()) {
		modconf.printUnsatisfiedModsError();
		return;
	}

	// Print mods
	infostream << "Client loading mods: ";
	for (const ModSpec &mod : m_mods)
		infostream << mod.name << " ";
	infostream << std::endl;

	// Load "mod" scripts
	for (const ModSpec &mod : m_mods) {
		if (!string_allowed(mod.name, MODNAME_ALLOWED_CHARS)) {
			throw ModError("Error loading mod \"" + mod.name +
				"\": Mod name does not follow naming conventions: "
					"Only characters [a-z0-9_] are allowed.");
		}
		scanModIntoMemory(mod.name, mod.path);
	}

	// Run them
	for (const ModSpec &mod : m_mods)
		m_script->loadModFromMemory(mod.name);

	// Mods are done loading. Unlock callbacks
	m_mods_loaded = true;

	// Run a callback when mods are loaded
	m_script->on_mods_loaded();

	// Create objects if they're ready
	if (m_state == LC_Ready)
		m_script->on_client_ready(m_env.getLocalPlayer());
	if (m_camera)
		m_script->on_camera_ready(m_camera);
	if (m_minimap)
		m_script->on_minimap_ready(m_minimap);
}

void Client::scanModSubfolder(const std::string &mod_name, const std::string &mod_path,
			std::string mod_subpath)
{
	std::string full_path = mod_path + DIR_DELIM + mod_subpath;
	std::vector<fs::DirListNode> mod = fs::GetDirListing(full_path);
	for (const fs::DirListNode &j : mod) {
		if (j.dir) {
			scanModSubfolder(mod_name, mod_path, mod_subpath + j.name + DIR_DELIM);
			continue;
		}
		std::replace(mod_subpath.begin(), mod_subpath.end(), DIR_DELIM_CHAR, '/');

		std::string real_path = full_path + j.name;
		std::string vfs_path = mod_name + ":" + mod_subpath + j.name;
		infostream << "Client::scanModSubfolder(): Loading \"" << real_path
				<< "\" as \"" << vfs_path << "\"." << std::endl;

		std::string contents;
		if (!fs::ReadFile(real_path, contents)) {
			errorstream << "Client::scanModSubfolder(): Can't read file \""
					<< real_path << "\"." << std::endl;
			continue;
		}

		m_mod_vfs.emplace(vfs_path, contents);
	}
}

const std::string &Client::getBuiltinLuaPath()
{
	static const std::string builtin_dir = porting::path_share + DIR_DELIM + "builtin";
	return builtin_dir;
}

const std::string &Client::getClientModsLuaPath()
{
	static const std::string clientmods_dir = porting::path_user + DIR_DELIM + "clientmods";
	return clientmods_dir;
}

const std::vector<ModSpec>& Client::getMods() const
{
	static std::vector<ModSpec> client_modspec_temp;
	return client_modspec_temp;
}

const ModSpec* Client::getModSpec(const std::string &modname) const
{
	return NULL;
}

void Client::Stop()
{
	m_shutdown = true;
	if (m_mods_loaded)
		m_script->on_shutdown();
	//request all client managed threads to stop
	m_mesh_update_thread.stop();
#if USE_SQLITE
	// Save local server map
	if (m_localdb) {
		infostream << "Local map saving ended." << std::endl;
		m_localdb->endSave();
	}
#endif

	if (m_mods_loaded)
		delete m_script;
}

bool Client::isShutdown()
{
	return m_shutdown || !m_mesh_update_thread.isRunning();
}

Client::~Client()
{
	m_shutdown = true;
	m_con->Disconnect();

	deleteAuthData();

	m_mesh_update_thread.stop();
	m_mesh_update_thread.wait();
	while (!m_mesh_update_thread.m_queue_out.empty()) {
		MeshUpdateResult r = m_mesh_update_thread.m_queue_out.pop_frontNoEx();
		delete r.mesh;
	}


	delete m_inventory_from_server;

	// Delete detached inventories
	for (auto &m_detached_inventorie : m_detached_inventories) {
		delete m_detached_inventorie.second;
	}

	// cleanup 3d model meshes on client shutdown
	while (RenderingEngine::get_mesh_cache()->getMeshCount() != 0) {
		scene::IAnimatedMesh *mesh = RenderingEngine::get_mesh_cache()->getMeshByIndex(0);

		if (mesh)
			RenderingEngine::get_mesh_cache()->removeMesh(mesh);
	}

	delete m_minimap;
	m_minimap = nullptr;

	delete m_media_downloader;
}

void Client::connect(Address address, bool is_local_server)
{
	initLocalMapSaving(address, m_address_name, is_local_server);

	// Since we use TryReceive() a timeout here would be ineffective anyway
	m_con->SetTimeoutMs(0);
	m_con->Connect(address);
}

void Client::step(float dtime)
{
	// Limit a bit
	if (dtime > 2.0)
		dtime = 2.0;

	m_animation_time += dtime;
	if(m_animation_time > 60.0)
		m_animation_time -= 60.0;

	m_time_of_day_update_timer += dtime;

	ReceiveAll();

	/*
		Packet counter
	*/
	{
		float &counter = m_packetcounter_timer;
		counter -= dtime;
		if(counter <= 0.0f)
		{
			counter = 30.0f;
			u32 sum = m_packetcounter.sum();
			float avg = sum / counter;

			infostream << "Client packetcounter (" << counter << "s): "
					<< "sum=" << sum << " avg=" << avg << "/s" << std::endl;
			m_packetcounter.print(infostream);
			m_packetcounter.clear();
		}
	}

	// UGLY hack to fix 2 second startup delay caused by non existent
	// server client startup synchronization in local server or singleplayer mode
	static bool initial_step = true;
	if (initial_step) {
		initial_step = false;
	}
	else if(m_state == LC_Created) {
		if (m_is_registration_confirmation_state) {
			// Waiting confirmation
			return;
		}
		float &counter = m_connection_reinit_timer;
		counter -= dtime;
		if(counter <= 0.0) {
			counter = 2.0;

			LocalPlayer *myplayer = m_env.getLocalPlayer();
			FATAL_ERROR_IF(myplayer == NULL, "Local player not found in environment.");

			sendInit(myplayer->getName());
		}

		// Not connected, return
		return;
	}

	/*
		Do stuff if connected
	*/

	/*
		Run Map's timers and unload unused data
	*/
	const float map_timer_and_unload_dtime = 5.25;
	if(m_map_timer_and_unload_interval.step(dtime, map_timer_and_unload_dtime)) {
		std::vector<v3s16> deleted_blocks;
		m_env.getMap().timerUpdate(map_timer_and_unload_dtime,
			g_settings->getFloat("client_unload_unused_data_timeout"),
			g_settings->getS32("client_mapblock_limit"),
			&deleted_blocks);

		/*
			Send info to server
			NOTE: This loop is intentionally iterated the way it is.
		*/

		std::vector<v3s16>::iterator i = deleted_blocks.begin();
		std::vector<v3s16> sendlist;
		for(;;) {
			if(sendlist.size() == 255 || i == deleted_blocks.end()) {
				if(sendlist.empty())
					break;
				/*
					[0] u16 command
					[2] u8 count
					[3] v3s16 pos_0
					[3+6] v3s16 pos_1
					...
				*/

				sendDeletedBlocks(sendlist);

				if(i == deleted_blocks.end())
					break;

				sendlist.clear();
			}

			sendlist.push_back(*i);
			++i;
		}
	}

	/*
		Send pending messages on out chat queue
	*/
	if (!m_out_chat_queue.empty() && canSendChatMessage()) {
		sendChatMessage(m_out_chat_queue.front());
		m_out_chat_queue.pop();
	}

	/*
		Handle environment
	*/
	LocalPlayer *player = m_env.getLocalPlayer();

	// Step environment (also handles player controls)
	m_env.step(dtime);
	m_sound->step(dtime);

	/*
		Get events
	*/
	while (m_env.hasClientEnvEvents()) {
		ClientEnvEvent envEvent = m_env.getClientEnvEvent();

		if (envEvent.type == CEE_PLAYER_DAMAGE) {
			u16 damage = envEvent.player_damage.amount;

			if (envEvent.player_damage.send_to_server)
				sendDamage(damage);

			// Add to ClientEvent queue
			ClientEvent *event = new ClientEvent();
			event->type = CE_PLAYER_DAMAGE;
			event->player_damage.amount = damage;
			m_client_event_queue.push(event);
		}
	}

	/*
		Print some info
	*/
	float &counter = m_avg_rtt_timer;
	counter += dtime;
	if(counter >= 10) {
		counter = 0.0;
		// connectedAndInitialized() is true, peer exists.
		float avg_rtt = getRTT();
		infostream << "Client: avg_rtt=" << avg_rtt << std::endl;
	}

	/*
		Send player position to server
	*/
	{
		float &counter = m_playerpos_send_timer;
		counter += dtime;
		if((m_state == LC_Ready) && (counter >= m_recommended_send_interval))
		{
			counter = 0.0;
			sendPlayerPos();
		}
	}

	/*
		Replace updated meshes
	*/
	{
		int num_processed_meshes = 0;
		std::vector<v3s16> blocks_to_ack;
		while (!m_mesh_update_thread.m_queue_out.empty())
		{
			num_processed_meshes++;

			MinimapMapblock *minimap_mapblock = NULL;
			bool do_mapper_update = true;

			MeshUpdateResult r = m_mesh_update_thread.m_queue_out.pop_frontNoEx();
			MapBlock *block = m_env.getMap().getBlockNoCreateNoEx(r.p);
			if (block) {
				// Delete the old mesh
				delete block->mesh;
				block->mesh = nullptr;

				if (r.mesh) {
					minimap_mapblock = r.mesh->moveMinimapMapblock();
					if (minimap_mapblock == NULL)
						do_mapper_update = false;

					bool is_empty = true;
					for (int l = 0; l < MAX_TILE_LAYERS; l++)
						if (r.mesh->getMesh(l)->getMeshBufferCount() != 0)
							is_empty = false;

					if (is_empty)
						delete r.mesh;
					else
						// Replace with the new mesh
						block->mesh = r.mesh;
				}
			} else {
				delete r.mesh;
			}

			if (m_minimap && do_mapper_update)
				m_minimap->addBlock(r.p, minimap_mapblock);

			if (r.ack_block_to_server) {
				if (blocks_to_ack.size() == 255) {
					sendGotBlocks(blocks_to_ack);
					blocks_to_ack.clear();
				}

				blocks_to_ack.emplace_back(r.p);
			}
		}
		if (blocks_to_ack.size() > 0) {
				// Acknowledge block(s)
				sendGotBlocks(blocks_to_ack);
		}

		if (num_processed_meshes > 0)
			g_profiler->graphAdd("num_processed_meshes", num_processed_meshes);
	}

	/*
		Load fetched media
	*/
	if (m_media_downloader && m_media_downloader->isStarted()) {
		m_media_downloader->step(this);
		if (m_media_downloader->isDone()) {
			delete m_media_downloader;
			m_media_downloader = NULL;
		}
	}

	/*
		If the server didn't update the inventory in a while, revert
		the local inventory (so the player notices the lag problem
		and knows something is wrong).
	*/
	if (m_inventory_from_server) {
		float interval = 10.0f;
		float count_before = std::floor(m_inventory_from_server_age / interval);

		m_inventory_from_server_age += dtime;

		float count_after = std::floor(m_inventory_from_server_age / interval);

		if (count_after != count_before) {
			// Do this every <interval> seconds after TOCLIENT_INVENTORY
			// Reset the locally changed inventory to the authoritative inventory
			player->inventory = *m_inventory_from_server;
			m_update_wielded_item = true;
		}
	}

	/*
		Update positions of sounds attached to objects
	*/
	{
		for (auto &m_sounds_to_object : m_sounds_to_objects) {
			int client_id = m_sounds_to_object.first;
			u16 object_id = m_sounds_to_object.second;
			ClientActiveObject *cao = m_env.getActiveObject(object_id);
			if (!cao)
				continue;
			m_sound->updateSoundPosition(client_id, cao->getPosition());
		}
	}

	/*
		Handle removed remotely initiated sounds
	*/
	m_removed_sounds_check_timer += dtime;
	if(m_removed_sounds_check_timer >= 2.32) {
		m_removed_sounds_check_timer = 0;
		// Find removed sounds and clear references to them
		std::vector<s32> removed_server_ids;
		for (std::unordered_map<s32, int>::iterator i = m_sounds_server_to_client.begin();
				i != m_sounds_server_to_client.end();) {
			s32 server_id = i->first;
			int client_id = i->second;
			++i;
			if(!m_sound->soundExists(client_id)) {
				m_sounds_server_to_client.erase(server_id);
				m_sounds_client_to_server.erase(client_id);
				m_sounds_to_objects.erase(client_id);
				removed_server_ids.push_back(server_id);
			}
		}

		// Sync to server
		if(!removed_server_ids.empty()) {
			sendRemovedSounds(removed_server_ids);
		}
	}

	m_mod_storage_save_timer -= dtime;
	if (m_mod_storage_save_timer <= 0.0f) {
		m_mod_storage_save_timer = g_settings->getFloat("server_map_save_interval");
		int n = 0;
		for (std::unordered_map<std::string, ModMetadata *>::const_iterator
				it = m_mod_storages.begin(); it != m_mod_storages.end(); ++it) {
			if (it->second->isModified()) {
				it->second->save(getModStoragePath());
				n++;
			}
		}
		if (n > 0)
			infostream << "Saved " << n << " modified mod storages." << std::endl;
	}

#if USE_SQLITE
	// Write server map
	if (m_localdb && m_localdb_save_interval.step(dtime,
			m_cache_save_interval)) {
		m_localdb->endSave();
		m_localdb->beginSave();
	}
#endif
}

bool Client::loadMedia(const std::string &data, const std::string &filename,
	bool from_media_push)
{
	std::string name;

	const char *image_ext[] = {
		".png", ".jpg", ".bmp", ".tga",
		".pcx", ".ppm", ".psd", ".wal", ".rgb",
		NULL
	};
	name = removeStringEnd(filename, image_ext);
	if (!name.empty()) {
		TRACESTREAM(<< "Client: Attempting to load image "
			<< "file \"" << filename << "\"" << std::endl);

		io::IFileSystem *irrfs = RenderingEngine::get_filesystem();
		video::IVideoDriver *vdrv = RenderingEngine::get_video_driver();

		// Silly irrlicht's const-incorrectness
		Buffer<char> data_rw(data.c_str(), data.size());

		// Create an irrlicht memory file
		io::IReadFile *rfile = irrfs->createMemoryReadFile(
				*data_rw, data_rw.getSize(), "_tempreadfile");

		FATAL_ERROR_IF(!rfile, "Could not create irrlicht memory file.");

		// Read image
		video::IImage *img = vdrv->createImageFromFile(rfile);
		if (!img) {
			errorstream<<"Client: Cannot create image from data of "
					<<"file \""<<filename<<"\""<<std::endl;
			rfile->drop();
			return false;
		}

		m_tsrc->insertSourceImage(filename, img);
		img->drop();
		rfile->drop();
		return true;
	}

	const char *sound_ext[] = {
		".0.ogg", ".1.ogg", ".2.ogg", ".3.ogg", ".4.ogg",
		".5.ogg", ".6.ogg", ".7.ogg", ".8.ogg", ".9.ogg",
		".ogg", NULL
	};
	name = removeStringEnd(filename, sound_ext);
	if (!name.empty()) {
		TRACESTREAM(<< "Client: Attempting to load sound "
			<< "file \"" << filename << "\"" << std::endl);
		return m_sound->loadSoundData(name, data);
	}

	const char *model_ext[] = {
		".x", ".b3d", ".md2", ".obj",
		NULL
	};
	name = removeStringEnd(filename, model_ext);
	if (!name.empty()) {
		verbosestream<<"Client: Storing model into memory: "
				<<"\""<<filename<<"\""<<std::endl;
		if(m_mesh_data.count(filename))
			errorstream<<"Multiple models with name \""<<filename.c_str()
					<<"\" found; replacing previous model"<<std::endl;
		m_mesh_data[filename] = data;
		return true;
	}

	const char *translate_ext[] = {
		".tr", NULL
	};
	name = removeStringEnd(filename, translate_ext);
	if (!name.empty()) {
		if (from_media_push)
			return false;
		TRACESTREAM(<< "Client: Loading translation: "
				<< "\"" << filename << "\"" << std::endl);
		g_client_translations->loadTranslation(data);
		return true;
	}

	errorstream << "Client: Don't know how to load file \""
		<< filename << "\"" << std::endl;
	return false;
}

// Virtual methods from con::PeerHandler
void Client::peerAdded(con::Peer *peer)
{
	infostream << "Client::peerAdded(): peer->id="
			<< peer->id << std::endl;
}
void Client::deletingPeer(con::Peer *peer, bool timeout)
{
	infostream << "Client::deletingPeer(): "
			"Server Peer is getting deleted "
			<< "(timeout=" << timeout << ")" << std::endl;

	if (timeout) {
		m_access_denied = true;
		m_access_denied_reason = gettext("Connection timed out.");
	}
}

/*
	u16 command
	u16 number of files requested
	for each file {
		u16 length of name
		string name
	}
*/
void Client::request_media(const std::vector<std::string> &file_requests)
{
	std::ostringstream os(std::ios_base::binary);
	writeU16(os, TOSERVER_REQUEST_MEDIA);
	size_t file_requests_size = file_requests.size();

	FATAL_ERROR_IF(file_requests_size > 0xFFFF, "Unsupported number of file requests");

	// Packet dynamicly resized
	NetworkPacket pkt(TOSERVER_REQUEST_MEDIA, 2 + 0);

	pkt << (u16) (file_requests_size & 0xFFFF);

	for (const std::string &file_request : file_requests) {
		pkt << file_request;
	}

	Send(&pkt);

	infostream << "Client: Sending media request list to server ("
			<< file_requests.size() << " files. packet size)" << std::endl;
}

void Client::initLocalMapSaving(const Address &address,
		const std::string &hostname,
		bool is_local_server)
{
	if (!g_settings->getBool("enable_local_map_saving") || is_local_server) {
		return;
	}

	std::string world_path;
#define set_world_path(hostname) \
	world_path = porting::path_user \
		+ DIR_DELIM + "worlds" \
		+ DIR_DELIM + "server_" \
		+ hostname + "_" + std::to_string(address.getPort());

	set_world_path(hostname);
	if (!fs::IsDir(world_path)) {
		std::string hostname_escaped = hostname;
		str_replace(hostname_escaped, ':', '_');
		set_world_path(hostname_escaped);
	}
#undef set_world_path
	fs::CreateAllDirs(world_path);

#if USE_SQLITE
	m_localdb = new MapDatabaseSQLite3(world_path);
	m_localdb->beginSave();
	actionstream << "Local map saving started, map will be saved at '" << world_path << "'" << std::endl;
#endif
}

void Client::ReceiveAll()
{
	NetworkPacket pkt;
	u64 start_ms = porting::getTimeMs();
	const u64 budget = 100;
	for(;;) {
		// Limit time even if there would be huge amounts of data to
		// process
		if (porting::getTimeMs() > start_ms + budget) {
			infostream << "Client::ReceiveAll(): "
					"Packet processing budget exceeded." << std::endl;
			break;
		}

		pkt.clear();
		try {
			if (!m_con->TryReceive(&pkt))
				break;
			ProcessData(&pkt);
		} catch (const con::InvalidIncomingDataException &e) {
			infostream << "Client::ReceiveAll(): "
					"InvalidIncomingDataException: what()="
					 << e.what() << std::endl;
		}
	}
}

inline void Client::handleCommand(NetworkPacket* pkt)
{
	const ToClientCommandHandler& opHandle = toClientCommandTable[pkt->getCommand()];
	(this->*opHandle.handler)(pkt);
}

/*
	sender_peer_id given to this shall be quaranteed to be a valid peer
*/
void Client::ProcessData(NetworkPacket *pkt)
{
	ToClientCommand command = (ToClientCommand) pkt->getCommand();
	u32 sender_peer_id = pkt->getPeerId();

	//infostream<<"Client: received command="<<command<<std::endl;
	m_packetcounter.add((u16)command);
	g_profiler->graphAdd("client_received_packets", 1);

	/*
		If this check is removed, be sure to change the queue
		system to know the ids
	*/
	if(sender_peer_id != PEER_ID_SERVER) {
		infostream << "Client::ProcessData(): Discarding data not "
			"coming from server: peer_id=" << sender_peer_id
			<< std::endl;
		return;
	}

	// Command must be handled into ToClientCommandHandler
	if (command >= TOCLIENT_NUM_MSG_TYPES) {
		infostream << "Client: Ignoring unknown command "
			<< command << std::endl;
		return;
	}

	/*
	 * Those packets are handled before m_server_ser_ver is set, it's normal
	 * But we must use the new ToClientConnectionState in the future,
	 * as a byte mask
	 */
	if(toClientCommandTable[command].state == TOCLIENT_STATE_NOT_CONNECTED) {
		handleCommand(pkt);
		return;
	}

	if(m_server_ser_ver == SER_FMT_VER_INVALID) {
		infostream << "Client: Server serialization"
				" format invalid or not initialized."
				" Skipping incoming command=" << command << std::endl;
		return;
	}

	/*
	  Handle runtime commands
	*/

	handleCommand(pkt);
}

void Client::Send(NetworkPacket* pkt)
{
	m_con->Send(PEER_ID_SERVER,
		serverCommandFactoryTable[pkt->getCommand()].channel,
		pkt,
		serverCommandFactoryTable[pkt->getCommand()].reliable);
}

// Will fill up 12 + 12 + 4 + 4 + 4 bytes
void writePlayerPos(LocalPlayer *myplayer, ClientMap *clientMap, NetworkPacket *pkt)
{
	v3f pf           = myplayer->getPosition() * 100;
	v3f sf           = myplayer->getSpeed() * 100;
	s32 pitch        = myplayer->getPitch() * 100;
	s32 yaw          = myplayer->getYaw() * 100;
	u32 keyPressed   = myplayer->keyPressed;
	// scaled by 80, so that pi can fit into a u8
	u8 fov           = clientMap->getCameraFov() * 80;
	u8 wanted_range  = MYMIN(255,
			std::ceil(clientMap->getControl().wanted_range / MAP_BLOCKSIZE));

	v3s32 position(pf.X, pf.Y, pf.Z);
	v3s32 speed(sf.X, sf.Y, sf.Z);

	/*
		Format:
		[0] v3s32 position*100
		[12] v3s32 speed*100
		[12+12] s32 pitch*100
		[12+12+4] s32 yaw*100
		[12+12+4+4] u32 keyPressed
		[12+12+4+4+4] u8 fov*80
		[12+12+4+4+4+1] u8 ceil(wanted_range / MAP_BLOCKSIZE)
	*/
	*pkt << position << speed << pitch << yaw << keyPressed;
	*pkt << fov << wanted_range;
}

void Client::interact(InteractAction action, const PointedThing& pointed)
{
	if(m_state != LC_Ready) {
		errorstream << "Client::interact() "
				"Canceled (not connected)"
				<< std::endl;
		return;
	}

	LocalPlayer *myplayer = m_env.getLocalPlayer();
	if (myplayer == NULL)
		return;

	/*
		[0] u16 command
		[2] u8 action
		[3] u16 item
		[5] u32 length of the next item (plen)
		[9] serialized PointedThing
		[9 + plen] player position information
	*/

	NetworkPacket pkt(TOSERVER_INTERACT, 1 + 2 + 0);

	pkt << (u8)action;
	pkt << myplayer->getWieldIndex();

	std::ostringstream tmp_os(std::ios::binary);
	pointed.serialize(tmp_os);

	pkt.putLongString(tmp_os.str());

	writePlayerPos(myplayer, &m_env.getClientMap(), &pkt);

	Send(&pkt);
}

void Client::deleteAuthData()
{
	if (!m_auth_data)
		return;

	switch (m_chosen_auth_mech) {
		case AUTH_MECHANISM_FIRST_SRP:
			break;
		case AUTH_MECHANISM_SRP:
		case AUTH_MECHANISM_LEGACY_PASSWORD:
			srp_user_delete((SRPUser *) m_auth_data);
			m_auth_data = NULL;
			break;
		case AUTH_MECHANISM_NONE:
			break;
	}
	m_chosen_auth_mech = AUTH_MECHANISM_NONE;
}


AuthMechanism Client::choseAuthMech(const u32 mechs)
{
	if (mechs & AUTH_MECHANISM_SRP)
		return AUTH_MECHANISM_SRP;

	if (mechs & AUTH_MECHANISM_FIRST_SRP)
		return AUTH_MECHANISM_FIRST_SRP;

	if (mechs & AUTH_MECHANISM_LEGACY_PASSWORD)
		return AUTH_MECHANISM_LEGACY_PASSWORD;

	return AUTH_MECHANISM_NONE;
}

void Client::sendInit(const std::string &playerName)
{
	NetworkPacket pkt(TOSERVER_INIT, 1 + 2 + 2 + (1 + playerName.size()) + 1);

	// we don't support network compression yet
	u16 supp_comp_modes = NETPROTO_COMPRESSION_NONE;

	pkt << (u8) SER_FMT_VER_HIGHEST_READ << (u16) supp_comp_modes;
	pkt << (u16) CLIENT_PROTOCOL_VERSION_MIN << (u16) CLIENT_PROTOCOL_VERSION_MAX;
	pkt << playerName << (u8) 1;

	Send(&pkt);
}

void Client::promptConfirmRegistration(AuthMechanism chosen_auth_mechanism)
{
	m_chosen_auth_mech = chosen_auth_mechanism;
	m_is_registration_confirmation_state = true;
}

void Client::confirmRegistration()
{
	m_is_registration_confirmation_state = false;
	startAuth(m_chosen_auth_mech);
}

void Client::startAuth(AuthMechanism chosen_auth_mechanism)
{
	m_chosen_auth_mech = chosen_auth_mechanism;

	switch (chosen_auth_mechanism) {
		case AUTH_MECHANISM_FIRST_SRP: {
			// send srp verifier to server
			std::string verifier;
			std::string salt;
			generate_srp_verifier_and_salt(getPlayerName(), m_password,
				&verifier, &salt);

			NetworkPacket resp_pkt(TOSERVER_FIRST_SRP, 0);
			resp_pkt << salt << verifier << (u8)((m_password.empty()) ? 1 : 0);

			Send(&resp_pkt);
			break;
		}
		case AUTH_MECHANISM_SRP:
		case AUTH_MECHANISM_LEGACY_PASSWORD: {
			u8 based_on = 1;

			if (chosen_auth_mechanism == AUTH_MECHANISM_LEGACY_PASSWORD) {
				m_password = translate_password(getPlayerName(), m_password);
				based_on = 0;
			}

			std::string playername_u = lowercase(getPlayerName());
			m_auth_data = srp_user_new(SRP_SHA256, SRP_NG_2048,
				getPlayerName().c_str(), playername_u.c_str(),
				(const unsigned char *) m_password.c_str(),
				m_password.length(), NULL, NULL);
			char *bytes_A = 0;
			size_t len_A = 0;
			SRP_Result res = srp_user_start_authentication(
				(struct SRPUser *) m_auth_data, NULL, NULL, 0,
				(unsigned char **) &bytes_A, &len_A);
			FATAL_ERROR_IF(res != SRP_OK, "Creating local SRP user failed.");

			NetworkPacket resp_pkt(TOSERVER_SRP_BYTES_A, 0);
			resp_pkt << std::string(bytes_A, len_A) << based_on;
			Send(&resp_pkt);
			break;
		}
		case AUTH_MECHANISM_NONE:
			break; // not handled in this method
	}
}

void Client::sendDeletedBlocks(std::vector<v3s16> &blocks)
{
	NetworkPacket pkt(TOSERVER_DELETEDBLOCKS, 1 + sizeof(v3s16) * blocks.size());

	pkt << (u8) blocks.size();

	for (const v3s16 &block : blocks) {
		pkt << block;
	}

	Send(&pkt);
}

void Client::sendGotBlocks(const std::vector<v3s16> &blocks)
{
	NetworkPacket pkt(TOSERVER_GOTBLOCKS, 1 + 6 * blocks.size());
	pkt << (u8) blocks.size();
	for (const v3s16 &block : blocks)
		pkt << block;

	Send(&pkt);
}

void Client::sendRemovedSounds(std::vector<s32> &soundList)
{
	size_t server_ids = soundList.size();
	assert(server_ids <= 0xFFFF);

	NetworkPacket pkt(TOSERVER_REMOVED_SOUNDS, 2 + server_ids * 4);

	pkt << (u16) (server_ids & 0xFFFF);

	for (s32 sound_id : soundList)
		pkt << sound_id;

	Send(&pkt);
}

void Client::sendNodemetaFields(v3s16 p, const std::string &formname,
		const StringMap &fields)
{
	size_t fields_size = fields.size();

	FATAL_ERROR_IF(fields_size > 0xFFFF, "Unsupported number of nodemeta fields");

	NetworkPacket pkt(TOSERVER_NODEMETA_FIELDS, 0);

	pkt << p << formname << (u16) (fields_size & 0xFFFF);

	StringMap::const_iterator it;
	for (it = fields.begin(); it != fields.end(); ++it) {
		const std::string &name = it->first;
		const std::string &value = it->second;
		pkt << name;
		pkt.putLongString(value);
	}

	Send(&pkt);
}

void Client::sendInventoryFields(const std::string &formname,
		const StringMap &fields)
{
	size_t fields_size = fields.size();
	FATAL_ERROR_IF(fields_size > 0xFFFF, "Unsupported number of inventory fields");

	NetworkPacket pkt(TOSERVER_INVENTORY_FIELDS, 0);
	pkt << formname << (u16) (fields_size & 0xFFFF);

	StringMap::const_iterator it;
	for (it = fields.begin(); it != fields.end(); ++it) {
		const std::string &name  = it->first;
		const std::string &value = it->second;
		pkt << name;
		pkt.putLongString(value);
	}

	Send(&pkt);
}

void Client::sendInventoryAction(InventoryAction *a)
{
	std::ostringstream os(std::ios_base::binary);

	a->serialize(os);

	// Make data buffer
	std::string s = os.str();

	NetworkPacket pkt(TOSERVER_INVENTORY_ACTION, s.size());
	pkt.putRawString(s.c_str(),s.size());

	Send(&pkt);
}

bool Client::canSendChatMessage() const
{
	u32 now = time(NULL);
	float time_passed = now - m_last_chat_message_sent;

	float virt_chat_message_allowance = m_chat_message_allowance + time_passed *
			(CLIENT_CHAT_MESSAGE_LIMIT_PER_10S / 8.0f);

	if (virt_chat_message_allowance < 1.0f)
		return false;

	return true;
}

void Client::sendChatMessage(const std::wstring &message)
{
	// Exempt SSCSM com messages from limits
	if (message.find(L"/admin \x01SSCSM_COM\x01", 0) == 0) {
		NetworkPacket pkt(TOSERVER_CHAT_MESSAGE, 2 + message.size() * sizeof(u16));
		pkt << message;
		Send(&pkt);
		return;
	}

	const s16 max_queue_size = g_settings->getS16("max_out_chat_queue_size");
	if (canSendChatMessage()) {
		u32 now = time(NULL);
		float time_passed = now - m_last_chat_message_sent;
		m_last_chat_message_sent = now;

		m_chat_message_allowance += time_passed * (CLIENT_CHAT_MESSAGE_LIMIT_PER_10S / 8.0f);
		if (m_chat_message_allowance > CLIENT_CHAT_MESSAGE_LIMIT_PER_10S)
			m_chat_message_allowance = CLIENT_CHAT_MESSAGE_LIMIT_PER_10S;

		m_chat_message_allowance -= 1.0f;

		NetworkPacket pkt(TOSERVER_CHAT_MESSAGE, 2 + message.size() * sizeof(u16));

		pkt << message;

		Send(&pkt);
	} else if (m_out_chat_queue.size() < (u16) max_queue_size || max_queue_size == -1) {
		m_out_chat_queue.push(message);
	} else {
		infostream << "Could not queue chat message because maximum out chat queue size ("
				<< max_queue_size << ") is reached." << std::endl;
	}
}

void Client::clearOutChatQueue()
{
	m_out_chat_queue = std::queue<std::wstring>();
}

void Client::sendChangePassword(const std::string &oldpassword,
	const std::string &newpassword, const bool close_form)
{
	LocalPlayer *player = m_env.getLocalPlayer();
	if (player == NULL)
		return;

	if (close_form) {
		auto formspec = m_game_ui->getFormspecGUI();
		if (formspec)
			formspec->quitMenu();
	}

	// get into sudo mode and then send new password to server
	m_password = oldpassword;
	m_new_password = newpassword;
	startAuth(choseAuthMech(m_sudo_auth_methods));
}


void Client::sendDamage(u16 damage)
{
	NetworkPacket pkt(TOSERVER_DAMAGE, sizeof(u16));
	pkt << damage;
	Send(&pkt);
}

void Client::sendRespawn()
{
	NetworkPacket pkt(TOSERVER_RESPAWN, 0);
	Send(&pkt);
}

void Client::sendReady()
{
	const char *platform_name = porting::getPlatformName();
	const std::string sysinfo = porting::get_sysinfo();
	const size_t version_len = strlen(g_version_hash) + 1 + strlen(platform_name) + 1 + sysinfo.size();
	NetworkPacket pkt(TOSERVER_CLIENT_READY,
			1 + 1 + 1 + 1 + 2 + sizeof(char) * version_len + 2);

	pkt << (u8) VERSION_MAJOR << (u8) VERSION_MINOR << (u8) VERSION_PATCH
		<< (u8) 0 << (u16) version_len;

	pkt.putRawString(g_version_hash, (u16) strlen(g_version_hash));
	pkt << (u8) 0;
	pkt.putRawString(platform_name, (u16) strlen(platform_name));
	pkt << (u8) 0;
	pkt.putRawString(sysinfo.c_str(), sysinfo.size());
	pkt << (u16)FORMSPEC_API_VERSION;
	Send(&pkt);
}

void Client::sendPlayerPos()
{
	LocalPlayer *player = m_env.getLocalPlayer();
	if (!player)
		return;

	ClientMap &map = m_env.getClientMap();
	u8 camera_fov   = map.getCameraFov();
	u8 wanted_range = map.getControl().wanted_range;

	// Save bandwidth by only updating position when
	// player is not dead and something changed

	if (m_activeobjects_received && player->isDead())
		return;

	if (
			player->last_position     == player->getPosition() &&
			player->last_speed        == player->getSpeed()    &&
			player->last_pitch        == player->getPitch()    &&
			player->last_yaw          == player->getYaw()      &&
			player->last_keyPressed   == player->keyPressed    &&
			player->last_camera_fov   == camera_fov            &&
			player->last_wanted_range == wanted_range)
		return;

	player->last_position     = player->getPosition();
	player->last_speed        = player->getSpeed();
	player->last_pitch        = player->getPitch();
	player->last_yaw          = player->getYaw();
	player->last_keyPressed   = player->keyPressed;
	player->last_camera_fov   = camera_fov;
	player->last_wanted_range = wanted_range;

	NetworkPacket pkt(TOSERVER_PLAYERPOS, 12 + 12 + 4 + 4 + 4 + 1 + 1);

	writePlayerPos(player, &map, &pkt);

	Send(&pkt);
}

void Client::removeNode(v3s16 p)
{
	std::map<v3s16, MapBlock*> modified_blocks;

	try {
		m_env.getMap().removeNodeAndUpdate(p, modified_blocks);
	}
	catch(InvalidPositionException &e) {
	}

	for (const auto &modified_block : modified_blocks) {
		addUpdateMeshTaskWithEdge(modified_block.first, false, true);
	}
}

/**
 * Helper function for Client Side Modding
 * CSM restrictions are applied there, this should not be used for core engine
 * @param p
 * @param is_valid_position
 * @return
 */
MapNode Client::CSMGetNode(v3s16 p, bool *is_valid_position)
{
	if (checkCSMRestrictionFlag(CSMRestrictionFlags::CSM_RF_LOOKUP_NODES)) {
		v3s16 ppos = floatToInt(m_env.getLocalPlayer()->getPosition(), BS);
		if ((u32) ppos.getDistanceFrom(p) > m_csm_restriction_noderange) {
			*is_valid_position = false;
			return {};
		}
	}
	return m_env.getMap().getNode(p, is_valid_position);
}

int Client::CSMClampRadius(v3s16 pos, int radius)
{
	if (!checkCSMRestrictionFlag(CSMRestrictionFlags::CSM_RF_LOOKUP_NODES))
		return radius;
	// This is approximate and will cause some allowed nodes to be excluded
	v3s16 ppos = floatToInt(m_env.getLocalPlayer()->getPosition(), BS);
	u32 distance = ppos.getDistanceFrom(pos);
	if (distance >= m_csm_restriction_noderange)
		return 0;
	return std::min<int>(radius, m_csm_restriction_noderange - distance);
}

v3s16 Client::CSMClampPos(v3s16 pos)
{
	if (!checkCSMRestrictionFlag(CSMRestrictionFlags::CSM_RF_LOOKUP_NODES))
		return pos;
	v3s16 ppos = floatToInt(m_env.getLocalPlayer()->getPosition(), BS);
	const int range = m_csm_restriction_noderange;
	return v3s16(
		core::clamp<int>(pos.X, (int)ppos.X - range, (int)ppos.X + range),
		core::clamp<int>(pos.Y, (int)ppos.Y - range, (int)ppos.Y + range),
		core::clamp<int>(pos.Z, (int)ppos.Z - range, (int)ppos.Z + range)
	);
}

void Client::addNode(v3s16 p, MapNode n, bool remove_metadata)
{
	//TimeTaker timer1("Client::addNode()");

	std::map<v3s16, MapBlock*> modified_blocks;

	try {
		//TimeTaker timer3("Client::addNode(): addNodeAndUpdate");
		m_env.getMap().addNodeAndUpdate(p, n, modified_blocks, remove_metadata);
	}
	catch(InvalidPositionException &e) {
	}

	for (const auto &modified_block : modified_blocks) {
		addUpdateMeshTaskWithEdge(modified_block.first, false, true);
	}
}

void Client::setPlayerControl(PlayerControl &control)
{
	LocalPlayer *player = m_env.getLocalPlayer();
	assert(player);
	player->control = control;
}

void Client::setPlayerItem(u16 item)
{
	m_env.getLocalPlayer()->setWieldIndex(item);
	m_update_wielded_item = true;

	NetworkPacket pkt(TOSERVER_PLAYERITEM, 2);
	pkt << item;
	Send(&pkt);
}

// Returns true once after the inventory of the local player
// has been updated from the server.
bool Client::updateWieldedItem()
{
	if (!m_update_wielded_item)
		return false;

	m_update_wielded_item = false;

	LocalPlayer *player = m_env.getLocalPlayer();
	assert(player);
	if (auto *list = player->inventory.getList("main"))
		list->setModified(false);
	if (auto *list = player->inventory.getList("hand"))
		list->setModified(false);

	return true;
}

Inventory* Client::getInventory(const InventoryLocation &loc)
{
	switch(loc.type){
	case InventoryLocation::UNDEFINED:
	{}
	break;
	case InventoryLocation::CURRENT_PLAYER:
	{
		LocalPlayer *player = m_env.getLocalPlayer();
		assert(player);
		return &player->inventory;
	}
	break;
	case InventoryLocation::PLAYER:
	{
		// Check if we are working with local player inventory
		LocalPlayer *player = m_env.getLocalPlayer();
		if (!player || strcmp(player->getName(), loc.name.c_str()) != 0)
			return NULL;
		return &player->inventory;
	}
	break;
	case InventoryLocation::NODEMETA:
	{
		NodeMetadata *meta = m_env.getMap().getNodeMetadata(loc.p);
		if(!meta)
			return NULL;
		return meta->getInventory();
	}
	break;
	case InventoryLocation::DETACHED:
	{
		if (m_detached_inventories.count(loc.name) == 0)
			return NULL;
		return m_detached_inventories[loc.name];
	}
	break;
	default:
		FATAL_ERROR("Invalid inventory location type.");
		break;
	}
	return NULL;
}

void Client::inventoryAction(InventoryAction *a)
{
	/*
		Send it to the server
	*/
	sendInventoryAction(a);

	/*
		Predict some local inventory changes
	*/
	a->clientApply(this, this);

	// Remove it
	delete a;
}

float Client::getAnimationTime()
{
	return m_animation_time;
}

int Client::getCrackLevel()
{
	return m_crack_level;
}

v3s16 Client::getCrackPos()
{
	return m_crack_pos;
}

void Client::setCrack(int level, v3s16 pos)
{
	int old_crack_level = m_crack_level;
	v3s16 old_crack_pos = m_crack_pos;

	m_crack_level = level;
	m_crack_pos = pos;

	if(old_crack_level >= 0 && (level < 0 || pos != old_crack_pos))
	{
		// remove old crack
		addUpdateMeshTaskForNode(old_crack_pos, false, true);
	}
	if(level >= 0 && (old_crack_level < 0 || pos != old_crack_pos))
	{
		// add new crack
		addUpdateMeshTaskForNode(pos, false, true);
	}
}

u16 Client::getHP()
{
	LocalPlayer *player = m_env.getLocalPlayer();
	assert(player);
	return player->hp;
}

bool Client::getChatMessage(std::wstring &res)
{
	if (m_chat_queue.empty())
		return false;

	ChatMessage *chatMessage = m_chat_queue.front();
	m_chat_queue.pop();

	res = L"";

	switch (chatMessage->type) {
		case CHATMESSAGE_TYPE_RAW:
		case CHATMESSAGE_TYPE_ANNOUNCE:
		case CHATMESSAGE_TYPE_SYSTEM:
			res = chatMessage->message;
			break;
		case CHATMESSAGE_TYPE_NORMAL: {
			if (!chatMessage->sender.empty())
				res = L"<" + chatMessage->sender + L"> " + chatMessage->message;
			else
				res = chatMessage->message;
			break;
		}
		default:
			break;
	}

	delete chatMessage;
	return true;
}

void Client::typeChatMessage(const std::wstring &message)
{
	// Discard empty line
	if (message.empty())
		return;

	// If message was consumed by script API, don't send it to server
	if (m_mods_loaded && m_script->on_sending_message(wide_to_utf8(message)))
		return;

	// Send to others
	sendChatMessage(message);
}

void Client::addUpdateMeshTask(v3s16 p, bool ack_to_server, bool urgent)
{
	// Check if the block exists to begin with. In the case when a non-existing
	// neighbor is automatically added, it may not. In that case we don't want
	// to tell the mesh update thread about it.
	MapBlock *b = m_env.getMap().getBlockNoCreateNoEx(p);
	if (b == NULL)
		return;

	m_mesh_update_thread.updateBlock(&m_env.getMap(), p, ack_to_server, urgent);
}

void Client::addUpdateMeshTaskWithEdge(v3s16 blockpos, bool ack_to_server, bool urgent)
{
	try{
		addUpdateMeshTask(blockpos, ack_to_server, urgent);
	}
	catch(InvalidPositionException &e){}

	// Leading edge
	for (int i=0;i<6;i++)
	{
		try{
			v3s16 p = blockpos + g_6dirs[i];
			addUpdateMeshTask(p, false, urgent);
		}
		catch(InvalidPositionException &e){}
	}
}

void Client::addUpdateMeshTaskForNode(v3s16 nodepos, bool ack_to_server, bool urgent)
{
	{
		v3s16 p = nodepos;
		infostream<<"Client::addUpdateMeshTaskForNode(): "
				<<"("<<p.X<<","<<p.Y<<","<<p.Z<<")"
				<<std::endl;
	}

	v3s16 blockpos          = getNodeBlockPos(nodepos);
	v3s16 blockpos_relative = blockpos * MAP_BLOCKSIZE;

	try{
		addUpdateMeshTask(blockpos, ack_to_server, urgent);
	}
	catch(InvalidPositionException &e) {}

	// Leading edge
	if(nodepos.X == blockpos_relative.X){
		try{
			v3s16 p = blockpos + v3s16(-1,0,0);
			addUpdateMeshTask(p, false, urgent);
		}
		catch(InvalidPositionException &e){}
	}

	if(nodepos.Y == blockpos_relative.Y){
		try{
			v3s16 p = blockpos + v3s16(0,-1,0);
			addUpdateMeshTask(p, false, urgent);
		}
		catch(InvalidPositionException &e){}
	}

	if(nodepos.Z == blockpos_relative.Z){
		try{
			v3s16 p = blockpos + v3s16(0,0,-1);
			addUpdateMeshTask(p, false, urgent);
		}
		catch(InvalidPositionException &e){}
	}
}

ClientEvent *Client::getClientEvent()
{
	FATAL_ERROR_IF(m_client_event_queue.empty(),
			"Cannot getClientEvent, queue is empty.");

	ClientEvent *event = m_client_event_queue.front();
	m_client_event_queue.pop();
	return event;
}

const Address Client::getServerAddress()
{
	return m_con->GetPeerAddress(PEER_ID_SERVER);
}

float Client::mediaReceiveProgress()
{
	if (m_media_downloader)
		return m_media_downloader->getProgress();

	return 1.0; // downloader only exists when not yet done
}

typedef struct TextureUpdateArgs {
	gui::IGUIEnvironment *guienv;
	u64 last_time_ms;
	u16 last_percent;
	const wchar_t* text_base;
	ITextureSource *tsrc;
} TextureUpdateArgs;

void texture_update_progress(void *args, u32 progress, u32 max_progress)
{
		TextureUpdateArgs* targs = (TextureUpdateArgs*) args;
		u16 cur_percent = ceil(progress / (double) max_progress * 100.);

		// update the loading menu -- if neccessary
		bool do_draw = false;
		u64 time_ms = targs->last_time_ms;
		if (cur_percent != targs->last_percent) {
			targs->last_percent = cur_percent;
			time_ms = porting::getTimeMs();
			// only draw when the user will notice something:
			do_draw = (time_ms - targs->last_time_ms > 100);
		}

		if (do_draw) {
			targs->last_time_ms = time_ms;
			std::basic_stringstream<wchar_t> strm;
			strm << targs->text_base << " " << targs->last_percent << "%...";
			RenderingEngine::draw_load_screen(strm.str(), targs->guienv, targs->tsrc, 0,
				72 + (u16) ((18. / 100.) * (double) targs->last_percent));
		}
}

void Client::afterContentReceived()
{
	infostream<<"Client::afterContentReceived() started"<<std::endl;
	assert(m_itemdef_received); // pre-condition
	assert(m_nodedef_received); // pre-condition
	assert(mediaReceived()); // pre-condition

	const wchar_t* text = wgettext("Loading textures...");

	// Clear cached pre-scaled 2D GUI images, as this cache
	// might have images with the same name but different
	// content from previous sessions.
	guiScalingCacheClear();

	// Rebuild inherited images and recreate textures
	infostream<<"- Rebuilding images and textures"<<std::endl;
	RenderingEngine::draw_load_screen(text, guienv, m_tsrc, 0, 70);
	m_tsrc->rebuildImagesAndTextures();
	delete[] text;

	// Rebuild shaders
	infostream<<"- Rebuilding shaders"<<std::endl;
	text = wgettext("Rebuilding shaders...");
	RenderingEngine::draw_load_screen(text, guienv, m_tsrc, 0, 71);
	m_shsrc->rebuildShaders();
	delete[] text;

	// Update node aliases
	infostream<<"- Updating node aliases"<<std::endl;
	text = wgettext("Initializing nodes...");
	RenderingEngine::draw_load_screen(text, guienv, m_tsrc, 0, 72);
	m_nodedef->updateAliases(m_itemdef);
	for (const auto &path : getTextureDirs()) {
		TextureOverrideSource override_source(path + DIR_DELIM + "override.txt");
		m_nodedef->applyTextureOverrides(override_source.getNodeTileOverrides());
		m_itemdef->applyTextureOverrides(override_source.getItemTextureOverrides());
	}
	m_nodedef->setNodeRegistrationStatus(true);
	m_nodedef->runNodeResolveCallbacks();
	delete[] text;

	// Update node textures and assign shaders to each tile
	infostream<<"- Updating node textures"<<std::endl;
	TextureUpdateArgs tu_args;
	tu_args.guienv = guienv;
	tu_args.last_time_ms = porting::getTimeMs();
	tu_args.last_percent = 0;
	tu_args.text_base =  wgettext("Initializing nodes");
	tu_args.tsrc = m_tsrc;
	m_nodedef->updateTextures(this, texture_update_progress, &tu_args);
	delete[] tu_args.text_base;

	// Start mesh update thread after setting up content definitions
	infostream<<"- Starting mesh update thread"<<std::endl;
	m_mesh_update_thread.start();

	m_state = LC_Ready;
	sendReady();

	if (m_mods_loaded)
		m_script->on_client_ready(m_env.getLocalPlayer());

	text = wgettext("Done!");
	RenderingEngine::draw_load_screen(text, guienv, m_tsrc, 0, 100);
	infostream<<"Client::afterContentReceived() done"<<std::endl;
	delete[] text;
}

float Client::getRTT()
{
	return m_con->getPeerStat(PEER_ID_SERVER,con::AVG_RTT);
}

float Client::getCurRate()
{
	return (m_con->getLocalStat(con::CUR_INC_RATE) +
			m_con->getLocalStat(con::CUR_DL_RATE));
}

void Client::makeScreenshot()
{
	irr::video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	irr::video::IImage* const raw_image = driver->createScreenShot();

	if (!raw_image)
		return;

	time_t t = time(NULL);
	struct tm *tm = localtime(&t);

	char timetstamp_c[64];
	strftime(timetstamp_c, sizeof(timetstamp_c), "%Y%m%d_%H%M%S", tm);

	std::string screenshot_dir;

	if (fs::IsPathAbsolute(g_settings->get("screenshot_path")))
		screenshot_dir = g_settings->get("screenshot_path");
	else
		screenshot_dir = porting::path_user + DIR_DELIM + g_settings->get("screenshot_path");

	std::string filename_base = screenshot_dir
			+ DIR_DELIM
			+ std::string("screenshot_")
			+ std::string(timetstamp_c);
	std::string filename_ext = "." + g_settings->get("screenshot_format");
	std::string filename;

	// Create the directory if it doesn't already exist.
	// Otherwise, saving the screenshot would fail.
	fs::CreateDir(screenshot_dir);

	u32 quality = (u32)g_settings->getS32("screenshot_quality");
	quality = MYMIN(MYMAX(quality, 0), 100) / 100.0 * 255;

	// Try to find a unique filename
	unsigned serial = 0;

	while (serial < SCREENSHOT_MAX_SERIAL_TRIES) {
		filename = filename_base + (serial > 0 ? ("_" + itos(serial)) : "") + filename_ext;
		std::ifstream tmp(filename.c_str());
		if (!tmp.good())
			break;	// File did not apparently exist, we'll go with it
		serial++;
	}

	if (serial == SCREENSHOT_MAX_SERIAL_TRIES) {
		infostream << "Could not find suitable filename for screenshot" << std::endl;
	} else {
		irr::video::IImage* const image =
				driver->createImage(video::ECF_R8G8B8, raw_image->getDimension());

		if (image) {
			raw_image->copyTo(image);

			std::ostringstream sstr;
			if (driver->writeImageToFile(image, filename.c_str(), quality)) {
				sstr << "Saved screenshot to '" << filename << "'";
			} else {
				sstr << "Failed to save screenshot '" << filename << "'";
			}
			pushToChatQueue(new ChatMessage(CHATMESSAGE_TYPE_SYSTEM,
					utf8_to_wide(sstr.str())));
			infostream << sstr.str() << std::endl;
			image->drop();
		}
	}

	raw_image->drop();
}

bool Client::shouldShowMinimap() const
{
	return !m_minimap_disabled_by_server;
}

void Client::pushToEventQueue(ClientEvent *event)
{
	m_client_event_queue.push(event);
}

void Client::showMinimap(const bool show)
{
	m_game_ui->showMinimap(show);
}

// IGameDef interface
// Under envlock
IItemDefManager* Client::getItemDefManager()
{
	return m_itemdef;
}
const NodeDefManager* Client::getNodeDefManager()
{
	return m_nodedef;
}
ICraftDefManager* Client::getCraftDefManager()
{
	return NULL;
	//return m_craftdef;
}
ITextureSource* Client::getTextureSource()
{
	return m_tsrc;
}
IWritableShaderSource* Client::getShaderSource()
{
	return m_shsrc;
}

u16 Client::allocateUnknownNodeId(const std::string &name)
{
	errorstream << "Client::allocateUnknownNodeId(): "
			<< "Client cannot allocate node IDs" << std::endl;
	FATAL_ERROR("Client allocated unknown node");

	return CONTENT_IGNORE;
}
ISoundManager* Client::getSoundManager()
{
	return m_sound;
}
MtEventManager* Client::getEventManager()
{
	return m_event;
}

ParticleManager* Client::getParticleManager()
{
	return &m_particle_manager;
}

scene::IAnimatedMesh* Client::getMesh(const std::string &filename, bool cache)
{
	StringMap::const_iterator it = m_mesh_data.find(filename);
	if (it == m_mesh_data.end()) {
		errorstream << "Client::getMesh(): Mesh not found: \"" << filename
			<< "\"" << std::endl;
		return NULL;
	}
	const std::string &data    = it->second;

	// Create the mesh, remove it from cache and return it
	// This allows unique vertex colors and other properties for each instance
	Buffer<char> data_rw(data.c_str(), data.size()); // Const-incorrect Irrlicht
	io::IReadFile *rfile   = RenderingEngine::get_filesystem()->createMemoryReadFile(
			*data_rw, data_rw.getSize(), filename.c_str());
	FATAL_ERROR_IF(!rfile, "Could not create/open RAM file");

	scene::IAnimatedMesh *mesh = RenderingEngine::get_scene_manager()->getMesh(rfile);
	rfile->drop();
	mesh->grab();
	if (!cache)
		RenderingEngine::get_mesh_cache()->removeMesh(mesh);
	return mesh;
}

const std::string* Client::getModFile(std::string filename)
{
	// strip dir delimiter from beginning of path
	auto pos = filename.find_first_of(':');
	if (pos == std::string::npos)
		return nullptr;
	pos++;
	auto pos2 = filename.find_first_not_of('/', pos);
	if (pos2 > pos)
		filename.erase(pos, pos2 - pos);

	StringMap::const_iterator it = m_mod_vfs.find(filename);
	if (it == m_mod_vfs.end())
		return nullptr;
	return &it->second;
}

bool Client::registerModStorage(ModMetadata *storage)
{
	if (m_mod_storages.find(storage->getModName()) != m_mod_storages.end()) {
		errorstream << "Unable to register same mod storage twice. Storage name: "
				<< storage->getModName() << std::endl;
		return false;
	}

	m_mod_storages[storage->getModName()] = storage;
	return true;
}

void Client::unregisterModStorage(const std::string &name)
{
	std::unordered_map<std::string, ModMetadata *>::const_iterator it =
		m_mod_storages.find(name);
	if (it != m_mod_storages.end()) {
		// Save unconditionaly on unregistration
		it->second->save(getModStoragePath());
		m_mod_storages.erase(name);
	}
}

std::string Client::getModStoragePath() const
{
	return porting::path_user + DIR_DELIM + "client" + DIR_DELIM + "mod_storage";
}

/*
 * Mod channels
 */

bool Client::joinModChannel(const std::string &channel)
{
	if (m_modchannel_mgr->channelRegistered(channel))
		return false;

	NetworkPacket pkt(TOSERVER_MODCHANNEL_JOIN, 2 + channel.size());
	pkt << channel;
	Send(&pkt);

	m_modchannel_mgr->joinChannel(channel, 0);
	return true;
}

bool Client::leaveModChannel(const std::string &channel)
{
	if (!m_modchannel_mgr->channelRegistered(channel))
		return false;

	NetworkPacket pkt(TOSERVER_MODCHANNEL_LEAVE, 2 + channel.size());
	pkt << channel;
	Send(&pkt);

	m_modchannel_mgr->leaveChannel(channel, 0);
	return true;
}

bool Client::sendModChannelMessage(const std::string &channel, const std::string &message)
{
	if (!m_modchannel_mgr->canWriteOnChannel(channel))
		return false;

	if (message.size() > STRING_MAX_LEN) {
		warningstream << "ModChannel message too long, dropping before sending "
				<< " (" << message.size() << " > " << STRING_MAX_LEN << ", channel: "
				<< channel << ")" << std::endl;
		return false;
	}

	// @TODO: do some client rate limiting
	NetworkPacket pkt(TOSERVER_MODCHANNEL_MSG, 2 + channel.size() + 2 + message.size());
	pkt << channel << message;
	Send(&pkt);
	return true;
}

ModChannel* Client::getModChannel(const std::string &channel)
{
	return m_modchannel_mgr->getModChannel(channel);
}
