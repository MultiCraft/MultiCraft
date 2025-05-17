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

#include "config.h"

#if USE_LEVELDB

#include "database-leveldb.h"

#include "log.h"
#include "filesys.h"
#include "exceptions.h"
#include "remoteplayer.h"
#include "server/player_sao.h"
#include "util/serialize.h"
#include "util/string.h"

#include "leveldb/db.h"


#define ENSURE_STATUS_OK(s) \
	if (!(s).ok()) { \
		throw DatabaseException(std::string("LevelDB error: ") + \
				(s).ToString()); \
	}


Database_LevelDB::Database_LevelDB(const std::string &savedir)
{
	leveldb::Options options;
	options.create_if_missing = true;
#ifdef SERVER
	options.write_buffer_size = 16 * 1024 * 1024; // x4 from default
	options.max_file_size = 8 * 1024 * 1024; // x4 from default
#endif
	leveldb::Status status = leveldb::DB::Open(options,
		savedir + DIR_DELIM + "map.db", &m_database);
	ENSURE_STATUS_OK(status);
}

Database_LevelDB::~Database_LevelDB()
{
	delete m_database;
}

bool Database_LevelDB::saveBlock(const v3s16 &pos, const std::string &data)
{
	leveldb::Status status = m_database->Put(leveldb::WriteOptions(),
			i64tos(getBlockAsInteger(pos)), data);
	if (!status.ok()) {
		warningstream << "saveBlock: LevelDB error saving block "
			<< PP(pos) << ": " << status.ToString() << std::endl;
		return false;
	}

	return true;
}

void Database_LevelDB::loadBlock(const v3s16 &pos, std::string *block)
{
	std::string datastr;
	leveldb::Status status = m_database->Get(leveldb::ReadOptions(),
		i64tos(getBlockAsInteger(pos)), &datastr);

	*block = (status.ok()) ? datastr : "";
}

bool Database_LevelDB::deleteBlock(const v3s16 &pos)
{
	leveldb::Status status = m_database->Delete(leveldb::WriteOptions(),
			i64tos(getBlockAsInteger(pos)));
	if (!status.ok()) {
		warningstream << "deleteBlock: LevelDB error deleting block "
			<< PP(pos) << ": " << status.ToString() << std::endl;
		return false;
	}

	return true;
}

void Database_LevelDB::listAllLoadableBlocks(std::vector<v3s16> &dst)
{
	leveldb::Iterator* it = m_database->NewIterator(leveldb::ReadOptions());
	for (it->SeekToFirst(); it->Valid(); it->Next()) {
		dst.push_back(getIntegerAsBlock(stoi64(it->key().ToString())));
	}
	ENSURE_STATUS_OK(it->status());  // Check for any errors found during the scan
	delete it;
}

PlayerDatabaseLevelDB::PlayerDatabaseLevelDB(const std::string &savedir)
{
	leveldb::Options options;
	options.create_if_missing = true;
	leveldb::Status status = leveldb::DB::Open(options,
		savedir + DIR_DELIM + "players.db", &m_database);
	ENSURE_STATUS_OK(status);
}

PlayerDatabaseLevelDB::~PlayerDatabaseLevelDB()
{
	delete m_database;
}

void PlayerDatabaseLevelDB::savePlayer(RemotePlayer *player)
{
	/*
	u8 version = 1
	u16 hp
	v3f position
	f32 pitch
	f32 yaw
	u16 breath
	u32 attribute_count
	for each attribute {
		std::string name
		std::string (long) value
	}
	std::string (long) serialized_inventory
	*/

	std::ostringstream os;
	writeU8(os, 1);

	PlayerSAO *sao = player->getPlayerSAO();
	sanity_check(sao);
	writeU16(os, sao->getHP());
	writeV3F32(os, sao->getBasePosition());
	writeF32(os, sao->getLookPitch());
	writeF32(os, sao->getRotation().Y);
	writeU16(os, sao->getBreath());

	StringMap stringvars = sao->getMeta().getStrings();
	writeU32(os, stringvars.size());
	for (const auto &it : stringvars) {
		os << serializeString16(it.first);
		os << serializeString32(it.second);
	}

	player->inventory.serialize(os);

	leveldb::Status status = m_database->Put(leveldb::WriteOptions(),
		player->getName(), os.str());
	ENSURE_STATUS_OK(status);
	player->onSuccessfulSave();
}

bool PlayerDatabaseLevelDB::removePlayer(const std::string &name)
{
	leveldb::Status s = m_database->Delete(leveldb::WriteOptions(), name);
	return s.ok();
}

bool PlayerDatabaseLevelDB::loadPlayer(RemotePlayer *player, PlayerSAO *sao)
{
	std::string raw;
	leveldb::Status s = m_database->Get(leveldb::ReadOptions(),
		player->getName(), &raw);
	if (!s.ok())
		return false;
	std::istringstream is(raw);

	if (readU8(is) > 1)
		return false;

	sao->setHPRaw(readU16(is));
	sao->setBasePosition(readV3F32(is));
	sao->setLookPitch(readF32(is));
	sao->setPlayerYaw(readF32(is));
	sao->setBreath(readU16(is), false);

	u32 attribute_count = readU32(is);
	for (u32 i = 0; i < attribute_count; i++) {
		std::string name = deSerializeString16(is);
		std::string value = deSerializeString32(is);
		sao->getMeta().setString(name, value);
	}
	sao->getMeta().setModified(false);

	// This should always be last.
	try {
		player->inventory.deSerialize(is);
	} catch (SerializationError &e) {
		errorstream << "Failed to deserialize player inventory. player_name="
			<< player->getName() << " " << e.what() << std::endl;
	}

	return true;
}

void PlayerDatabaseLevelDB::listPlayers(std::vector<std::string> &res)
{
	leveldb::Iterator* it = m_database->NewIterator(leveldb::ReadOptions());
	res.clear();
	for (it->SeekToFirst(); it->Valid(); it->Next()) {
		res.push_back(it->key().ToString());
	}
	delete it;
}

AuthDatabaseLevelDB::AuthDatabaseLevelDB(const std::string &savedir)
{
	leveldb::Options options;
	options.create_if_missing = true;
	leveldb::Status status = leveldb::DB::Open(options,
		savedir + DIR_DELIM + "auth.db", &m_database);
	ENSURE_STATUS_OK(status);
}

AuthDatabaseLevelDB::~AuthDatabaseLevelDB()
{
	delete m_database;
}

bool AuthDatabaseLevelDB::getAuth(const std::string &name, AuthEntry &res)
{
	std::string raw;
	leveldb::Status s = m_database->Get(leveldb::ReadOptions(), name, &raw);
	if (!s.ok())
		return false;
	std::istringstream is(raw);

	/*
	u8 version = 1
	std::string password
	u16 number of privileges
	for each privilege {
		std::string privilege
	}
	s64 last_login
	*/

	if (readU8(is) > 1)
		return false;

	res.id = 1;
	res.name = name;
	res.password = deSerializeString16(is);

	u16 privilege_count = readU16(is);
	res.privileges.clear();
	res.privileges.reserve(privilege_count);
	for (u16 i = 0; i < privilege_count; i++) {
		res.privileges.push_back(deSerializeString16(is));
	}

	res.last_login = readS64(is);
	return true;
}

bool AuthDatabaseLevelDB::saveAuth(const AuthEntry &authEntry)
{
	std::ostringstream os;
	writeU8(os, 1);
	os << serializeString16(authEntry.password);

	size_t privilege_count = authEntry.privileges.size();
	FATAL_ERROR_IF(privilege_count > U16_MAX,
		"Unsupported number of privileges");
	writeU16(os, privilege_count);
	for (const std::string &privilege : authEntry.privileges) {
		os << serializeString16(privilege);
	}

	writeS64(os, authEntry.last_login);
	leveldb::Status s = m_database->Put(leveldb::WriteOptions(),
		authEntry.name, os.str());
	return s.ok();
}

bool AuthDatabaseLevelDB::createAuth(AuthEntry &authEntry)
{
	return saveAuth(authEntry);
}

bool AuthDatabaseLevelDB::deleteAuth(const std::string &name)
{
	leveldb::Status s = m_database->Delete(leveldb::WriteOptions(), name);
	return s.ok();
}

void AuthDatabaseLevelDB::listNames(std::vector<std::string> &res)
{
	leveldb::Iterator* it = m_database->NewIterator(leveldb::ReadOptions());
	res.clear();
	for (it->SeekToFirst(); it->Valid(); it->Next()) {
		res.emplace_back(it->key().ToString());
	}
	delete it;
}

void AuthDatabaseLevelDB::reload()
{
	// No-op for LevelDB.
}

#endif // USE_LEVELDB
