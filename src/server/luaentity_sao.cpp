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

#include "luaentity_sao.h"
#include "collision.h"
#include "constants.h"
#include "player_sao.h"
#include "scripting_server.h"
#include "server.h"
#include "serverenvironment.h"

LuaEntitySAO::LuaEntitySAO(ServerEnvironment *env, v3f pos, const std::string &data)
	: UnitSAO(env, pos)
{
	std::string name;
	std::string state;
	u16 hp = 1;
	v3f velocity;
	v3f rotation;

	while (!data.empty()) { // breakable, run for one iteration
		std::istringstream is(data, std::ios::binary);
		// 'version' does not allow to incrementally extend the parameter list thus
		// we need another variable to build on top of 'version=1'. Ugly hack but works™
		u8 version2 = 0;
		u8 version = readU8(is);

		name = deSerializeString16(is);
		state = deSerializeString32(is);

		if (version < 1)
			break;

		hp = readU16(is);
		velocity = readV3F1000(is);
		// yaw must be yaw to be backwards-compatible
		rotation.Y = readF1000(is);

		if (is.good()) // EOF for old formats
			version2 = readU8(is);

		if (version2 < 1) // PROTOCOL_VERSION < 37
			break;

		// version2 >= 1
		rotation.X = readF1000(is);
		rotation.Z = readF1000(is);

		// if (version2 < 2)
		//     break;
		// <read new values>
		break;
	}
	// create object
	infostream << "LuaEntitySAO::create(name=\"" << name << "\" state=\""
			 << state << "\")" << std::endl;

	m_init_name = name;
	m_init_state = state;
	m_hp = hp;
	m_velocity = velocity;
	m_rotation = rotation;
}

LuaEntitySAO::~LuaEntitySAO()
{
	if(m_registered){
		m_env->getScriptIface()->luaentity_Remove(m_id);
	}

	for (u32 attached_particle_spawner : m_attached_particle_spawners) {
		m_env->deleteParticleSpawner(attached_particle_spawner, false);
	}
}

void LuaEntitySAO::addedToEnvironment(u32 dtime_s)
{
	ServerActiveObject::addedToEnvironment(dtime_s);

	// Create entity from name
	m_registered = m_env->getScriptIface()->
		luaentity_Add(m_id, m_init_name.c_str());

	if(m_registered){
		// Get properties
		m_env->getScriptIface()->
			luaentity_GetProperties(m_id, this, &m_prop);
		// Initialize HP from properties
		m_hp = m_prop.hp_max;
		// Activate entity, supplying serialized state
		m_env->getScriptIface()->
			luaentity_Activate(m_id, m_init_state, dtime_s);
	} else {
		m_prop.infotext = m_init_name;
#ifdef NDEBUG
		markForRemoval(); // remove unknown entities
#endif
	}
}

void LuaEntitySAO::dispatchScriptDeactivate()
{
	// Ensure that this is in fact a registered entity,
	// and that it isn't already gone.
	// The latter also prevents this from ever being called twice.
	if (m_registered && !isGone())
		m_env->getScriptIface()->luaentity_Deactivate(m_id);
}

void LuaEntitySAO::step(float dtime, bool send_recommended)
{
	if(!m_properties_sent)
	{
		m_properties_sent = true;
		std::string str = getPropertyPacket();
		// create message and add to list
		m_messages_out.emplace(getId(), true, str);
	}

	// If attached, check that our parent is still there. If it isn't, detach.
	if (m_attachment_parent_id && !isAttached()) {
		// This is handled when objects are removed from the map
		warningstream << "LuaEntitySAO::step() id=" << m_id <<
			" is attached to nonexistent parent. This is a bug." << std::endl;
		clearParentAttachment();
		sendPosition(false, true);
	}

	m_last_sent_position_timer += dtime;

	collisionMoveResult moveresult, *moveresult_p = nullptr;

	// Each frame, parent position is copied if the object is attached, otherwise it's calculated normally
	// If the object gets detached this comes into effect automatically from the last known origin
	if (auto *parent = getParent()) {
		m_base_position = parent->getBasePosition();
		m_velocity = v3f(0,0,0);
		m_acceleration = v3f(0,0,0);
	} else {
		if(m_prop.physical){
			aabb3f box = m_prop.collisionbox;
			box.MinEdge *= BS;
			box.MaxEdge *= BS;
			f32 pos_max_d = BS*0.25; // Distance per iteration
			v3f p_pos = m_base_position;
			v3f p_velocity = m_velocity;
			v3f p_acceleration = m_acceleration;
			moveresult = collisionMoveSimple(m_env, m_env->getGameDef(),
					pos_max_d, box, m_prop.stepheight, dtime,
					&p_pos, &p_velocity, p_acceleration,
					this, m_prop.collideWithObjects);
			moveresult_p = &moveresult;

			// Apply results
			m_base_position = p_pos;
			m_velocity = p_velocity;
			m_acceleration = p_acceleration;
		} else {
			m_base_position += dtime * m_velocity + 0.5 * dtime
					* dtime * m_acceleration;
			m_velocity += dtime * m_acceleration;
		}

		if (m_prop.automatic_face_movement_dir &&
				(fabs(m_velocity.Z) > 0.001 || fabs(m_velocity.X) > 0.001)) {
			float target_yaw = atan2(m_velocity.Z, m_velocity.X) * 180 / M_PI
				+ m_prop.automatic_face_movement_dir_offset;
			float max_rotation_per_sec =
					m_prop.automatic_face_movement_max_rotation_per_sec;

			if (max_rotation_per_sec > 0) {
				m_rotation.Y = wrapDegrees_0_360(m_rotation.Y);
				wrappedApproachShortest(m_rotation.Y, target_yaw,
					dtime * max_rotation_per_sec, 360.f);
			} else {
				// Negative values of max_rotation_per_sec mean disabled.
				m_rotation.Y = target_yaw;
			}
		}
	}

	if(m_registered) {
		m_env->getScriptIface()->luaentity_Step(m_id, dtime, moveresult_p);
	}

	if (!send_recommended)
		return;

	if(!isAttached())
	{
		// TODO: force send when acceleration changes enough?
		float minchange = 0.2*BS;
		if(m_last_sent_position_timer > 1.0){
			minchange = 0.01*BS;
		} else if(m_last_sent_position_timer > 0.2){
			minchange = 0.05*BS;
		}
		float move_d = m_base_position.getDistanceFrom(m_last_sent_position);
		move_d += m_last_sent_move_precision;
		float vel_d = m_velocity.getDistanceFrom(m_last_sent_velocity);
		if (move_d > minchange || vel_d > minchange ||
				std::fabs(m_rotation.X - m_last_sent_rotation.X) > 1.0f ||
				std::fabs(m_rotation.Y - m_last_sent_rotation.Y) > 1.0f ||
				std::fabs(m_rotation.Z - m_last_sent_rotation.Z) > 1.0f) {

			sendPosition(true, false);
		}
	}

	sendOutdatedData();
}

std::string LuaEntitySAO::getClientInitializationData(u16 protocol_version)
{
	std::ostringstream os(std::ios::binary);

	// PROTOCOL_VERSION >= 37
	writeU8(os, 1); // version
	os << serializeString16(""); // name
	writeU8(os, 0); // is_player
	writeU16(os, getId()); //id
	writeV3F32(os, m_base_position);
	writeV3F32(os, m_rotation);
	writeU16(os, m_hp);

	std::ostringstream msg_os(std::ios::binary);
	msg_os << serializeString32(getPropertyPacket()); // message 1
	msg_os << serializeString32(generateUpdateArmorGroupsCommand()); // 2
	msg_os << serializeString32(generateUpdateAnimationCommand()); // 3
	for (const auto &bone_pos : m_bone_position) {
		msg_os << serializeString32(generateUpdateBonePositionCommand(
			bone_pos.first, bone_pos.second.X, bone_pos.second.Y)); // 3 + N
	}
	msg_os << serializeString32(generateUpdateAttachmentCommand()); // 4 + m_bone_position.size

	int message_count = 4 + m_bone_position.size();

	for (const auto &id : getAttachmentChildIds()) {
		if (ServerActiveObject *obj = m_env->getActiveObject(id)) {
			message_count++;
			msg_os << serializeString32(obj->generateUpdateInfantCommand(
				id, protocol_version));
		}
	}

	msg_os << serializeString32(generateSetTextureModCommand());
	message_count++;

	writeU8(os, message_count);
	std::string serialized = msg_os.str();
	os.write(serialized.c_str(), serialized.size());

	// return result
	return os.str();
}

void LuaEntitySAO::getStaticData(std::string *result) const
{
	verbosestream<<FUNCTION_NAME<<std::endl;
	std::ostringstream os(std::ios::binary);
	// version must be 1 to keep backwards-compatibility. See version2
	writeU8(os, 1);
	// name
	os<<serializeString16(m_init_name);
	// state
	if(m_registered){
		std::string state = m_env->getScriptIface()->
			luaentity_GetStaticdata(m_id);
		os<<serializeString32(state);
	} else {
		os<<serializeString32(m_init_state);
	}
	writeU16(os, m_hp);
	writeV3F1000(os, m_velocity);
	// yaw
	writeF1000(os, m_rotation.Y);

	// version2. Increase this variable for new values
	writeU8(os, 1); // PROTOCOL_VERSION >= 37

	writeF1000(os, m_rotation.X);
	writeF1000(os, m_rotation.Z);

	// <write new values>

	*result = os.str();
}

u16 LuaEntitySAO::punch(v3f dir,
		const ToolCapabilities *toolcap,
		ServerActiveObject *puncher,
		float time_from_last_punch)
{
	if (!m_registered) {
		// Delete unknown LuaEntities when punched
		markForRemoval();
		return 0;
	}

	FATAL_ERROR_IF(!puncher, "Punch action called without SAO");

	s32 old_hp = getHP();
	ItemStack selected_item, hand_item;
	ItemStack tool_item = puncher->getWieldedItem(&selected_item, &hand_item);

	PunchDamageResult result = getPunchDamage(
			m_armor_groups,
			toolcap,
			&tool_item,
			time_from_last_punch);

	bool damage_handled = m_env->getScriptIface()->luaentity_Punch(m_id, puncher,
			time_from_last_punch, toolcap, dir, result.did_punch ? result.damage : 0);

	if (!damage_handled) {
		if (result.did_punch) {
			setHP((s32)getHP() - result.damage,
				PlayerHPChangeReason(PlayerHPChangeReason::PLAYER_PUNCH, puncher));

			// create message and add to list
			sendPunchCommand();
		}
	}

	if (getHP() == 0 && !isGone()) {
		clearParentAttachment();
		clearChildAttachments();
		m_env->getScriptIface()->luaentity_on_death(m_id, puncher);
		markForRemoval();
	}

	actionstream << puncher->getDescription() << " (id=" << puncher->getId() <<
			", hp=" << puncher->getHP() << ") punched " <<
			getDescription() << " (id=" << m_id << ", hp=" << m_hp <<
			"), damage=" << (old_hp - (s32)getHP()) <<
			(damage_handled ? " (handled by Lua)" : "") << std::endl;

	// TODO: give Lua control over wear
	return result.wear;
}

void LuaEntitySAO::rightClick(ServerActiveObject *clicker)
{
	if (!m_registered)
		return;

	m_env->getScriptIface()->luaentity_Rightclick(m_id, clicker);
}

void LuaEntitySAO::setPos(const v3f &pos)
{
	if(isAttached())
		return;
	m_base_position = pos;
	sendPosition(false, true);
}

void LuaEntitySAO::moveTo(v3f pos, bool continuous)
{
	if(isAttached())
		return;
	m_base_position = pos;
	if(!continuous)
		sendPosition(true, true);
}

float LuaEntitySAO::getMinimumSavedMovement()
{
	return 0.1 * BS;
}

std::string LuaEntitySAO::getDescription()
{
	std::ostringstream oss;
	oss << "LuaEntitySAO \"" << m_init_name << "\" ";
	auto pos = floatToInt(m_base_position, BS);
	oss << "at " << PP(pos);
	return oss.str();
}

void LuaEntitySAO::setHP(s32 hp, const PlayerHPChangeReason &reason)
{
	m_hp = rangelim(hp, 0, U16_MAX);
}

u16 LuaEntitySAO::getHP() const
{
	return m_hp;
}

void LuaEntitySAO::setVelocity(v3f velocity)
{
	m_velocity = velocity;
}

v3f LuaEntitySAO::getVelocity()
{
	return m_velocity;
}

void LuaEntitySAO::setAcceleration(v3f acceleration)
{
	m_acceleration = acceleration;
}

v3f LuaEntitySAO::getAcceleration()
{
	return m_acceleration;
}

void LuaEntitySAO::setTextureMod(const std::string &mod)
{
	m_current_texture_modifier = mod;
	// create message and add to list
	m_messages_out.emplace(getId(), true, generateSetTextureModCommand());
}

std::string LuaEntitySAO::getTextureMod() const
{
	return m_current_texture_modifier;
}


std::string LuaEntitySAO::generateSetTextureModCommand() const
{
	std::ostringstream os(std::ios::binary);
	// command
	writeU8(os, AO_CMD_SET_TEXTURE_MOD);
	// parameters
	os << serializeString16(m_current_texture_modifier);
	return os.str();
}

std::string LuaEntitySAO::generateSetSpriteCommand(v2s16 p, u16 num_frames,
	f32 framelength, bool select_horiz_by_yawpitch)
{
	std::ostringstream os(std::ios::binary);
	// command
	writeU8(os, AO_CMD_SET_SPRITE);
	// parameters
	writeV2S16(os, p);
	writeU16(os, num_frames);
	writeF32(os, framelength);
	writeU8(os, select_horiz_by_yawpitch);
	return os.str();
}

void LuaEntitySAO::setSprite(v2s16 p, int num_frames, float framelength,
		bool select_horiz_by_yawpitch)
{
	std::string str = generateSetSpriteCommand(
		p,
		num_frames,
		framelength,
		select_horiz_by_yawpitch
	);
	// create message and add to list
	m_messages_out.emplace(getId(), true, str);
}

std::string LuaEntitySAO::getName()
{
	return m_init_name;
}

std::string LuaEntitySAO::getPropertyPacket()
{
	return generateSetPropertiesCommand(m_prop);
}

void LuaEntitySAO::sendPosition(bool do_interpolate, bool is_movement_end)
{
	// If the object is attached client-side, don't waste bandwidth sending its position to clients
	if(isAttached())
		return;

	// Send attachment updates instantly to the client prior updating position
	sendOutdatedData();

	m_last_sent_move_precision = m_base_position.getDistanceFrom(
			m_last_sent_position);
	m_last_sent_position_timer = 0;
	m_last_sent_position = m_base_position;
	m_last_sent_velocity = m_velocity;
	//m_last_sent_acceleration = m_acceleration;
	m_last_sent_rotation = m_rotation;

	float update_interval = m_env->getSendRecommendedInterval();

	std::string str = generateUpdatePositionCommand(
		m_base_position,
		m_velocity,
		m_acceleration,
		m_rotation,
		do_interpolate,
		is_movement_end,
		update_interval
	);
	// create message and add to list
	m_messages_out.emplace(getId(), false, str);
}

bool LuaEntitySAO::getCollisionBox(aabb3f *toset) const
{
	if (m_prop.physical)
	{
		//update collision box
		toset->MinEdge = m_prop.collisionbox.MinEdge * BS;
		toset->MaxEdge = m_prop.collisionbox.MaxEdge * BS;

		toset->MinEdge += m_base_position;
		toset->MaxEdge += m_base_position;

		return true;
	}

	return false;
}

bool LuaEntitySAO::getSelectionBox(aabb3f *toset) const
{
	if (!m_prop.is_visible || !m_prop.pointable) {
		return false;
	}

	toset->MinEdge = m_prop.selectionbox.MinEdge * BS;
	toset->MaxEdge = m_prop.selectionbox.MaxEdge * BS;

	return true;
}

bool LuaEntitySAO::collideWithObjects() const
{
	return m_prop.collideWithObjects;
}
