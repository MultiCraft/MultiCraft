/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2013-2020 Minetest core developers & community

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

#include "player_sao.h"
#include "nodedef.h"
#include "remoteplayer.h"
#include "scripting_server.h"
#include "server.h"
#include "serverenvironment.h"

PlayerSAO::PlayerSAO(ServerEnvironment *env_, RemotePlayer *player_, session_t peer_id_,
		bool is_singleplayer):
	UnitSAO(env_, v3f(0,0,0)),
	m_player(player_),
	m_peer_id(peer_id_),
	m_is_singleplayer(is_singleplayer)
{
	SANITY_CHECK(m_peer_id != PEER_ID_INEXISTENT);

	m_prop.hp_max = PLAYER_MAX_HP_DEFAULT;
	m_prop.breath_max = PLAYER_MAX_BREATH_DEFAULT;
	m_prop.physical = false;
	m_prop.collisionbox = aabb3f(-0.3f, 0.0f, -0.3f, 0.3f, 1.77f, 0.3f);
	m_prop.selectionbox = aabb3f(-0.3f, 0.0f, -0.3f, 0.3f, 1.77f, 0.3f);
	m_prop.pointable = true;
	// Start of default appearance, this should be overwritten by Lua
	m_prop.visual = "upright_sprite";
	m_prop.visual_size = v3f(1, 2, 1);
	m_prop.textures.clear();
	m_prop.textures.emplace_back("player.png");
	m_prop.textures.emplace_back("player_back.png");
	m_prop.colors.clear();
	m_prop.colors.emplace_back(255, 255, 255, 255);
	m_prop.spritediv = v2s16(1,1);
	m_prop.eye_height = 1.625f;
	// End of default appearance
	m_prop.is_visible = true;
	m_prop.backface_culling = false;
	m_prop.makes_footstep_sound = true;
	m_prop.stepheight = PLAYER_DEFAULT_STEPHEIGHT * BS;
	m_prop.show_on_minimap = true;
	m_hp = m_prop.hp_max;
	m_breath = m_prop.breath_max;
	// Disable zoom in survival mode using a value of 0
	m_prop.zoom_fov = g_settings->getBool("creative_mode") ? 15.0f : 0.0f;

	if (!g_settings->getBool("enable_damage"))
		m_armor_groups["immortal"] = 1;
}

void PlayerSAO::finalize(RemotePlayer *player, const std::set<std::string> &privs)
{
	assert(player);
	m_player = player;
	m_privs = privs;
}

v3f PlayerSAO::getEyeOffset() const
{
	return v3f(0, BS * m_prop.eye_height, 0);
}

std::string PlayerSAO::getDescription()
{
	return std::string("player ") + m_player->getName();
}

// Called after id has been set and has been inserted in environment
void PlayerSAO::addedToEnvironment(u32 dtime_s)
{
	ServerActiveObject::addedToEnvironment(dtime_s);
	ServerActiveObject::setBasePosition(m_base_position);
	m_player->setPlayerSAO(this);
	m_player->setPeerId(m_peer_id);
	m_last_good_position = m_base_position;
}

// Called before removing from environment
void PlayerSAO::removingFromEnvironment()
{
	ServerActiveObject::removingFromEnvironment();
	if (m_player->getPlayerSAO() == this) {
		unlinkPlayerSessionAndSave();
		for (u32 attached_particle_spawner : m_attached_particle_spawners) {
			m_env->deleteParticleSpawner(attached_particle_spawner, false);
		}
	}
}

std::string PlayerSAO::getClientInitializationData(u16 protocol_version)
{
	std::ostringstream os(std::ios::binary);

	// Protocol >= 15
	writeU8(os, 1); // version
	os << serializeString16(m_player->getName()); // name
	writeU8(os, 1); // is_player
	writeS16(os, getId()); // id
	if (protocol_version >= 37) {
		writeV3F32(os, m_base_position);
		writeV3F32(os, m_rotation);
		writeU16(os, getHP());
	} else {
		writeV3F1000(os, m_base_position + v3f(0, BS, 0));
		writeF1000(os, m_rotation.Y);

		// HP is sent as a signed integer
		const u16 hp = getHP();
		if (hp > S16_MAX)
			writeS16(os, S16_MAX);
		else
			writeS16(os, static_cast<s16>(hp));
	}

	std::ostringstream msg_os(std::ios::binary);
	msg_os << serializeString32(getPropertyPacket(
		protocol_version)); // message 1
	msg_os << serializeString32(generateUpdateArmorGroupsCommand()); // 2
	msg_os << serializeString32(generateUpdateAnimationCommand(
		protocol_version)); // 3
	for (const auto &bone_pos : m_bone_position) {
		msg_os << serializeString32(generateUpdateBonePositionCommand(
			bone_pos.first, bone_pos.second.X, bone_pos.second.Y,
			protocol_version)); // 3 + N
	}
	msg_os << serializeString32(generateUpdateAttachmentCommand(
		protocol_version)); // 4 + m_bone_position.size
	msg_os << serializeString32(generateUpdatePhysicsOverrideCommand(
		protocol_version)); // 5 + m_bone_position.size

	int message_count = 5 + m_bone_position.size();

	for (const auto &id : getAttachmentChildIds()) {
		if (ServerActiveObject *obj = m_env->getActiveObject(id)) {
			message_count++;
			msg_os << serializeString32(obj->generateUpdateInfantCommand(
				id, protocol_version));
		}
	}

	writeU8(os, message_count);
	std::string serialized = msg_os.str();
	os.write(serialized.c_str(), serialized.size());

	// return result
	return os.str();
}

void PlayerSAO::getStaticData(std::string * result) const
{
	FATAL_ERROR("This function shall not be called for PlayerSAO");
}

void PlayerSAO::step(float dtime, bool send_recommended)
{
	if (!isImmortal() && m_drowning_interval.step(dtime, 2.0f)) {
		// Get nose/mouth position, approximate with eye position
		v3s16 p = floatToInt(getEyePosition(), BS);
		MapNode n = m_env->getMap().getNode(p);
		const ContentFeatures &c = m_env->getGameDef()->ndef()->get(n);
		// If node generates drown
		const bool noclip = m_privs.count("noclip") && (!m_is_singleplayer || g_settings->getBool("noclip"));
		int drowning = (c.walkable && c.drawtype != NDT_NODEBOX && c.drawtype != NDT_AIRLIKE && c.drawtype != NDT_PLANTLIKE) ? 1 : c.drowning;
		if (drowning > 0 && m_hp > 0 && !noclip) {
			if (m_breath > 0)
				setBreath(m_breath - 1);

			// No more breath, damage player
			if (m_breath == 0) {
				PlayerHPChangeReason reason(PlayerHPChangeReason::DROWNING);
				setHP(m_hp - drowning, reason);
				m_env->getGameDef()->SendPlayerHPOrDie(this, reason);
			}
		}
	}

	if (m_breathing_interval.step(dtime, 0.5f) && !isImmortal()) {
		// Get nose/mouth position, approximate with eye position
		v3s16 p = floatToInt(getEyePosition(), BS);
		MapNode n = m_env->getMap().getNode(p);
		const ContentFeatures &c = m_env->getGameDef()->ndef()->get(n);
		// If player is alive & not drowning & not in ignore & not immortal, breathe
		int drowning = (c.walkable && c.drawtype != NDT_NODEBOX) ? 1 : c.drowning;
		if (m_breath < m_prop.breath_max && drowning == 0 &&
				n.getContent() != CONTENT_IGNORE && m_hp > 0)
			setBreath(m_breath + 1);
	}

	if (!isImmortal() && m_node_hurt_interval.step(dtime, 1.0f)) {
		u32 damage_per_second = 0;
		std::string nodename;
		// Lowest and highest damage points are 0.1 within collisionbox
		float dam_top = m_prop.collisionbox.MaxEdge.Y - 0.1f;

		// Sequence of damage points, starting 0.1 above feet and progressing
		// upwards in 1 node intervals, stopping below top damage point.
		for (float dam_height = 0.1f; dam_height < dam_top; dam_height++) {
			v3s16 p = floatToInt(m_base_position +
				v3f(0.0f, dam_height * BS, 0.0f), BS);
			MapNode n = m_env->getMap().getNode(p);
			const ContentFeatures &c = m_env->getGameDef()->ndef()->get(n);
			if (c.damage_per_second > damage_per_second) {
				damage_per_second = c.damage_per_second;
				nodename = c.name;
			}
		}

		// Top damage point
		v3s16 ptop = floatToInt(m_base_position +
			v3f(0.0f, dam_top * BS, 0.0f), BS);
		MapNode ntop = m_env->getMap().getNode(ptop);
		const ContentFeatures &c = m_env->getGameDef()->ndef()->get(ntop);
		if (c.damage_per_second > damage_per_second) {
			damage_per_second = c.damage_per_second;
			nodename = c.name;
		}

		if (damage_per_second != 0 && m_hp > 0) {
			s32 newhp = (s32)m_hp - (s32)damage_per_second;
			PlayerHPChangeReason reason(PlayerHPChangeReason::NODE_DAMAGE, nodename);
			setHP(newhp, reason);
			m_env->getGameDef()->SendPlayerHPOrDie(this, reason);
		}
	}

	if (!m_properties_sent) {
		m_properties_sent = true;
		std::string str = getPropertyPacket(37);
		std::string legacy_str = getPropertyPacket(32);
		// create message and add to list
		m_messages_out.emplace(getId(), true, str, legacy_str);
		m_env->getScriptIface()->player_event(this, "properties_changed");
	}

	// If attached, check that our parent is still there. If it isn't, detach.
	if (m_attachment_parent_id && !isAttached()) {
		// This is handled when objects are removed from the map
		warningstream << "PlayerSAO::step() id=" << m_id <<
			" is attached to nonexistent parent. This is a bug." << std::endl;
		clearParentAttachment();
		setBasePosition(m_last_good_position);
		m_env->getGameDef()->SendMovePlayer(m_peer_id);
	}

	//dstream<<"PlayerSAO::step: dtime: "<<dtime<<std::endl;

	// Set lag pool maximums based on estimated lag
	const float LAG_POOL_MIN = 5.0f;
	float lag_pool_max = m_env->getMaxLagEstimate() * 2.0f;
	if(lag_pool_max < LAG_POOL_MIN)
		lag_pool_max = LAG_POOL_MIN;
	m_dig_pool.setMax(lag_pool_max);
	m_move_pool.setMax(lag_pool_max);

	// Increment cheat prevention timers
	m_dig_pool.add(dtime);
	m_move_pool.add(dtime);
	m_time_from_last_teleport += dtime;
	m_time_from_last_punch += dtime;
	m_nocheat_dig_time += dtime;
	m_max_speed_override_time = MYMAX(m_max_speed_override_time - dtime, 0.0f);

	// Each frame, parent position is copied if the object is attached,
	// otherwise it's calculated normally.
	// If the object gets detached this comes into effect automatically from
	// the last known origin.
	if (auto *parent = getParent()) {
		v3f pos = parent->getBasePosition();
		m_last_good_position = pos;
		setBasePosition(pos);

		if (m_player)
			m_player->setSpeed(v3f());
	}

	if (!send_recommended)
		return;

	if (m_position_not_sent) {
		m_position_not_sent = false;
		float update_interval = m_env->getSendRecommendedInterval();
		v3f pos;
		// When attached, the position is only sent to clients where the
		// parent isn't known
		if (isAttached())
			pos = m_last_good_position;
		else
			pos = m_base_position;

		std::string str = generateUpdatePositionCommand(
			pos,
			v3f(0.0f, 0.0f, 0.0f),
			v3f(0.0f, 0.0f, 0.0f),
			m_rotation,
			true,
			false,
			update_interval,
			37
		);

		std::string legacy_str = generateUpdatePositionCommand(
			pos + v3f(0.0f, BS, 0.0f),
			v3f(0.0f, 0.0f, 0.0f),
			v3f(0.0f, 0.0f, 0.0f),
			m_rotation,
			true,
			false,
			update_interval,
			32
		);

		// create message and add to list
		m_messages_out.emplace(getId(), false, str, legacy_str);
	}

	if (!m_physics_override_sent) {
		m_physics_override_sent = true;
		// create message and add to list
		m_messages_out.emplace(getId(), true,
			generateUpdatePhysicsOverrideCommand(37),
			generateUpdatePhysicsOverrideCommand(32));
	}

	sendOutdatedData();
}

std::string PlayerSAO::generateUpdatePhysicsOverrideCommand(const u16 protocol_version) const
{
	std::ostringstream os(std::ios::binary);
	// command
	writeU8(os, AO_CMD_SET_PHYSICS_OVERRIDE);
	// parameters
	writeF(os, m_physics_override_speed, protocol_version);
	writeF(os, m_physics_override_jump, protocol_version);
	writeF(os, m_physics_override_gravity, protocol_version);
	// these are sent inverted so we get true when the server sends nothing
	writeU8(os, !m_physics_override_sneak);
	writeU8(os, !m_physics_override_sneak_glitch);
	writeU8(os, !m_physics_override_new_move);
	return os.str();
}

void PlayerSAO::setBasePosition(const v3f &position)
{
	if (m_player && position != m_base_position)
		m_player->setDirty(true);

	// This needs to be ran for attachments too
	ServerActiveObject::setBasePosition(position);

	// Updating is not wanted/required for player migration
	if (m_env) {
		m_position_not_sent = true;
	}
}

void PlayerSAO::setPos(const v3f &pos)
{
	if(isAttached())
		return;

	// Send mapblock of target location
	v3s16 blockpos = v3s16(pos.X / MAP_BLOCKSIZE, pos.Y / MAP_BLOCKSIZE, pos.Z / MAP_BLOCKSIZE);
	m_env->getGameDef()->SendBlock(m_peer_id, blockpos);

	setBasePosition(pos);
	// Movement caused by this command is always valid
	m_last_good_position = pos;
	m_move_pool.empty();
	m_time_from_last_teleport = 0.0;
	m_env->getGameDef()->SendMovePlayer(m_peer_id);
}

void PlayerSAO::moveTo(v3f pos, bool continuous)
{
	if(isAttached())
		return;

	setBasePosition(pos);
	// Movement caused by this command is always valid
	m_last_good_position = pos;
	m_move_pool.empty();
	m_time_from_last_teleport = 0.0;
	m_env->getGameDef()->SendMovePlayer(m_peer_id);
}

void PlayerSAO::setPlayerYaw(const float yaw)
{
	v3f rotation(0, yaw, 0);
	if (m_player && yaw != m_rotation.Y)
		m_player->setDirty(true);

	// Set player model yaw, not look view
	UnitSAO::setRotation(rotation);
}

void PlayerSAO::setFov(const float fov)
{
	if (m_player && fov != m_fov)
		m_player->setDirty(true);

	m_fov = fov;
}

void PlayerSAO::setWantedRange(const s16 range)
{
	if (m_player && range != m_wanted_range)
		m_player->setDirty(true);

	m_wanted_range = range;
}

void PlayerSAO::setPlayerYawAndSend(const float yaw)
{
	setPlayerYaw(yaw);
	m_env->getGameDef()->SendMovePlayer(m_peer_id);
}

void PlayerSAO::setLookPitch(const float pitch)
{
	if (m_player && pitch != m_pitch)
		m_player->setDirty(true);

	m_pitch = pitch;
}

void PlayerSAO::setLookPitchAndSend(const float pitch)
{
	setLookPitch(pitch);
	m_env->getGameDef()->SendMovePlayer(m_peer_id);
}

u16 PlayerSAO::punch(v3f dir,
	const ToolCapabilities *toolcap,
	ServerActiveObject *puncher,
	float time_from_last_punch)
{
	if (!toolcap)
		return 0;

	FATAL_ERROR_IF(!puncher, "Punch action called without SAO");

	// No effect if PvP disabled or if immortal
	if (isImmortal() || !g_settings->getBool("enable_pvp")) {
		if (puncher->getType() == ACTIVEOBJECT_TYPE_PLAYER) {
			// create message and add to list
			sendPunchCommand();
			return 0;
		}
	}

	s32 old_hp = getHP();
	HitParams hitparams = getHitParams(m_armor_groups, toolcap,
			time_from_last_punch);

	PlayerSAO *playersao = m_player->getPlayerSAO();

	bool damage_handled = m_env->getScriptIface()->on_punchplayer(playersao,
				puncher, time_from_last_punch, toolcap, dir,
				hitparams.hp);

	if (!damage_handled) {
		setHP((s32)getHP() - (s32)hitparams.hp,
				PlayerHPChangeReason(PlayerHPChangeReason::PLAYER_PUNCH, puncher));
	} else { // override client prediction
		if (puncher->getType() == ACTIVEOBJECT_TYPE_PLAYER) {
			// create message and add to list
			sendPunchCommand();
		}
	}

	actionstream << puncher->getDescription() << " (id=" << puncher->getId() <<
		", hp=" << puncher->getHP() << ") punched " <<
		getDescription() << " (id=" << m_id << ", hp=" << m_hp <<
		"), damage=" << (old_hp - (s32)getHP()) <<
		(damage_handled ? " (handled by Lua)" : "") << std::endl;

	return hitparams.wear;
}

void PlayerSAO::rightClick(ServerActiveObject *clicker)
{
	m_env->getScriptIface()->on_rightclickplayer(this, clicker);
}

void PlayerSAO::setHP(s32 hp, const PlayerHPChangeReason &reason)
{
	if (hp == (s32)m_hp)
		return; // Nothing to do

	if (m_hp <= 0 && hp < (s32)m_hp)
		return; // Cannot take more damage

	{
		s32 hp_change = m_env->getScriptIface()->on_player_hpchange(this, hp - m_hp, reason);
		if (hp_change == 0)
			return;

		hp = m_hp + hp_change;
	}

	s32 oldhp = m_hp;
	hp = rangelim(hp, 0, m_prop.hp_max);

	if (hp < oldhp && isImmortal())
		return; // Do not allow immortal players to be damaged

	m_hp = hp;

	// Update properties on death
	if ((hp == 0) != (oldhp == 0))
		m_properties_sent = false;
}

void PlayerSAO::setBreath(const u16 breath, bool send)
{
	if (m_player && breath != m_breath)
		m_player->setDirty(true);

	m_breath = rangelim(breath, 0, m_prop.breath_max);

	if (send)
		m_env->getGameDef()->SendPlayerBreath(this);
}

Inventory *PlayerSAO::getInventory() const
{
	return m_player ? &m_player->inventory : nullptr;
}

InventoryLocation PlayerSAO::getInventoryLocation() const
{
	InventoryLocation loc;
	loc.setPlayer(m_player->getName());
	return loc;
}

u16 PlayerSAO::getWieldIndex() const
{
	return m_player->getWieldIndex();
}

ItemStack PlayerSAO::getWieldedItem(ItemStack *selected, ItemStack *hand) const
{
	return m_player->getWieldedItem(selected, hand);
}

bool PlayerSAO::setWieldedItem(const ItemStack &item)
{
	InventoryList *mlist = m_player->inventory.getList(getWieldList());
	if (mlist) {
		mlist->changeItem(m_player->getWieldIndex(), item);
		return true;
	}
	return false;
}

void PlayerSAO::disconnected()
{
	m_peer_id = PEER_ID_INEXISTENT;
	markForRemoval();
}

void PlayerSAO::unlinkPlayerSessionAndSave()
{
	assert(m_player->getPlayerSAO() == this);
	m_player->setPeerId(PEER_ID_INEXISTENT);
	m_env->savePlayer(m_player);
	m_player->setPlayerSAO(NULL);
	m_env->removePlayer(m_player);
}

std::string PlayerSAO::getPropertyPacket(const u16 protocol_version)
{
	m_prop.is_visible = (true);

	ObjectProperties prop = m_prop;

	// Use the renamed model if compat_send_original_model is enabled
	if (protocol_version < 37 && m_env->getCompatSendOriginalModel() &&
			m_env->isCompatPlayerModel(m_prop.mesh)) {
		prop.mesh = "_mc_compat_" + m_prop.mesh;
	}

	// Remove a one-node offset from a copy of the object properties for MT 0.4
	if (protocol_version < 37) {
		prop.selectionbox.MinEdge.Y -= 1.0f;
		prop.selectionbox.MaxEdge.Y -= 1.0f;
	}
	return generateSetPropertiesCommand(prop, protocol_version);
}

void PlayerSAO::setMaxSpeedOverride(const v3f &vel)
{
	if (m_max_speed_override_time == 0.0f)
		m_max_speed_override = vel;
	else
		m_max_speed_override += vel;
	if (m_player) {
		float accel = MYMIN(m_player->movement_acceleration_default,
				m_player->movement_acceleration_air);
		m_max_speed_override_time = m_max_speed_override.getLength() / accel / BS;
	}
}

bool PlayerSAO::checkMovementCheat()
{
	if (m_is_singleplayer ||
			isAttached() ||
			g_settings->getBool("disable_anticheat")) {
		m_last_good_position = m_base_position;
		return false;
	}

	bool cheated = false;
	/*
		Check player movements

		NOTE: Actually the server should handle player physics like the
		client does and compare player's position to what is calculated
		on our side. This is required when eg. players fly due to an
		explosion. Altough a node-based alternative might be possible
		too, and much more lightweight.
	*/

	float override_max_H, override_max_V;
	if (m_max_speed_override_time > 0.0f) {
		override_max_H = MYMAX(fabs(m_max_speed_override.X), fabs(m_max_speed_override.Z));
		override_max_V = fabs(m_max_speed_override.Y);
	} else {
		override_max_H = override_max_V = 0.0f;
	}

	float player_max_walk = 0; // horizontal movement
	float player_max_jump = 0; // vertical upwards movement

	if (m_privs.count("fast") != 0)
		player_max_walk = m_player->movement_speed_fast; // Fast speed
	else
		player_max_walk = m_player->movement_speed_walk; // Normal speed
	player_max_walk *= m_physics_override_speed;
	player_max_walk = MYMAX(player_max_walk, override_max_H);

	player_max_jump = m_player->movement_speed_jump * m_physics_override_jump;
	// FIXME: Bouncy nodes cause practically unbound increase in Y speed,
	//        until this can be verified correctly, tolerate higher jumping speeds
	player_max_jump *= 2.0;
	player_max_jump = MYMAX(player_max_jump, override_max_V);

	// Don't divide by zero!
	if (player_max_walk < 0.0001f)
		player_max_walk = 0.0001f;
	if (player_max_jump < 0.0001f)
		player_max_jump = 0.0001f;

	v3f diff = (m_base_position - m_last_good_position);
	float d_vert = diff.Y;
	diff.Y = 0;
	float d_horiz = diff.getLength();
	float required_time = d_horiz / player_max_walk;

	// FIXME: Checking downwards movement is not easily possible currently,
	//        the server could calculate speed differences to examine the gravity
	if (d_vert > 0) {
		// In certain cases (water, ladders) walking speed is applied vertically
		float s = MYMAX(player_max_jump, player_max_walk);
		required_time = MYMAX(required_time, d_vert / s);
	}

	if (m_move_pool.grab(required_time)) {
		m_last_good_position = m_base_position;
	} else {
		const float LAG_POOL_MIN = 5.0;
		float lag_pool_max = m_env->getMaxLagEstimate() * 2.0;
		lag_pool_max = MYMAX(lag_pool_max, LAG_POOL_MIN);
		if (m_time_from_last_teleport > lag_pool_max) {
			actionstream << "Server: " << m_player->getName()
					<< " moved too fast: V=" << d_vert << ", H=" << d_horiz
					<< "; resetting position." << std::endl;
			cheated = true;
		}
		setBasePosition(m_last_good_position);
	}
	return cheated;
}

bool PlayerSAO::getCollisionBox(aabb3f *toset) const
{
	//update collision box
	toset->MinEdge = m_prop.collisionbox.MinEdge * BS;
	toset->MaxEdge = m_prop.collisionbox.MaxEdge * BS;

	toset->MinEdge += m_base_position;
	toset->MaxEdge += m_base_position;
	return true;
}

bool PlayerSAO::getSelectionBox(aabb3f *toset) const
{
	if (!m_prop.is_visible || !m_prop.pointable) {
		return false;
	}

	toset->MinEdge = m_prop.selectionbox.MinEdge * BS;
	toset->MaxEdge = m_prop.selectionbox.MaxEdge * BS;

	return true;
}

float PlayerSAO::getZoomFOV() const
{
	return m_prop.zoom_fov;
}
