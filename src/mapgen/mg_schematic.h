/*
Minetest
Copyright (C) 2014-2018 kwolekr, Ryan Kwolek <kwolekr@minetest.net>
Copyright (C) 2015-2018 paramat

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

#include <map>
#include "mg_decoration.h"
#include "util/string.h"

class Map;
class ServerMap;
class Mapgen;
class MMVManip;
class PseudoRandom;
class NodeResolver;
class Server;

/*
	Minetest Schematic File Format

	All values are stored in big-endian byte order.
	[u32] signature: 'MTSM'
	[u16] version: 4
	[u16] size X
	[u16] size Y
	[u16] size Z
	For each Y:
		[u8] slice probability value
	[Name-ID table] Name ID Mapping Table
		[u16] name-id count
		For each name-id mapping:
			[u16] name length
			[u8[]] name
	ZLib deflated {
	For each node in schematic:  (for z, y, x)
		[u16] content
	For each node in schematic:
		[u8] param1
		  bit 0-6: probability
		  bit 7:   specific node force placement
	For each node in schematic:
		[u8] param2
	}

	Version changes:
	1 - Initial version
	2 - Fixed messy never/always place; 0 probability is now never, 0xFF is always
	3 - Added y-slice probabilities; this allows for variable height structures
	4 - Compressed range of node occurence prob., added per-node force placement bit
*/

//// Schematic constants
#define MTSCHEM_FILE_SIGNATURE 0x4d54534d // 'MTSM'
#define MTSCHEM_FILE_VER_HIGHEST_READ  4
#define MTSCHEM_FILE_VER_HIGHEST_WRITE 4

#define MTSCHEM_PROB_MASK       0x7F

#define MTSCHEM_PROB_NEVER      0x00
#define MTSCHEM_PROB_ALWAYS     0x7F
#define MTSCHEM_PROB_ALWAYS_OLD 0xFF

#define MTSCHEM_FORCE_PLACE     0x80

enum SchematicType
{
	SCHEMATIC_NORMAL,
};

enum SchematicFormatType {
	SCHEM_FMT_HANDLE,
	SCHEM_FMT_MTS,
	SCHEM_FMT_LUA,
};

class Schematic : public ObjDef, public NodeResolver {
public:
	Schematic();
	virtual ~Schematic();

	ObjDef *clone() const;

	virtual void resolveNodeNames();

	bool loadSchematicFromStream(std::istream *is, const std::string &filename,
		const NodeDefManager *ndef, StringMap *replace_names = NULL);
	bool loadSchematicFromFile(const std::string &filename,
		const NodeDefManager *ndef, StringMap *replace_names = NULL);
	bool saveSchematicToFile(const std::string &filename,
		const NodeDefManager *ndef);
	bool getSchematicFromMap(Map *map, v3s16 p1, v3s16 p2);

	bool deserializeFromMts(std::istream *is, std::vector<std::string> *names);
	bool serializeToMts(std::ostream *os,
		const std::vector<std::string> &names) const;
	bool serializeToLua(std::ostream *os, const std::vector<std::string> &names,
		bool use_comments, u32 indent_spaces) const;

	void blitToVManip(MMVManip *vm, v3s16 p, Rotation rot, bool force_place, ServerMap *map = nullptr);
	bool placeOnVManip(MMVManip *vm, v3s16 p, u32 flags, Rotation rot, bool force_place);
	void placeOnMap(ServerMap *map, v3s16 p, u32 flags, Rotation rot, bool force_place);

	void applyProbabilities(v3s16 p0,
		std::vector<std::pair<v3s16, u8> > *plist,
		std::vector<std::pair<s16, u8> > *splist);

	std::vector<content_t> c_nodes;
	u32 flags = 0;
	v3s16 size;
	MapNode *schemdata = nullptr;
	u8 *slice_probs = nullptr;
};

class SchematicManager : public ObjDefManager {
public:
	SchematicManager(Server *server);
	virtual ~SchematicManager() = default;

	SchematicManager *clone() const;

	virtual void clear();

	const char *getObjectTitle() const
	{
		return "schematic";
	}

	static Schematic *create(SchematicType type)
	{
		return new Schematic;
	}

private:
	SchematicManager() {};

	Server *m_server;
};

void generate_nodelist_and_update_ids(MapNode *nodes, size_t nodecount,
	std::vector<std::string> *usednodes, const NodeDefManager *ndef);
