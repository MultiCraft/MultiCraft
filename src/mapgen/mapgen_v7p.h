/*
Minetest
Copyright (C) 2013-2016 kwolekr, Ryan Kwolek <kwolekr@minetest.net>
Copyright (C) 2014-2017 paramat

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

#ifndef MAPGEN_V7P_HEADER
#define MAPGEN_V7P_HEADER

#include "mapgen.h"

//////////// Mapgen V7P flags
#define MGV7P_MOUNTAINS  0x01
#define MGV7P_RIDGES     0x02

class BiomeManager;

extern FlagDesc flagdesc_mapgen_v7p[];


struct MapgenV7PParams : public MapgenParams
{
	NoiseParams np_terrain_base;
	NoiseParams np_terrain_alt;
	NoiseParams np_terrain_persist;
	NoiseParams np_height_select;
	NoiseParams np_filler_depth;
	NoiseParams np_mount_height;
	NoiseParams np_ridge_uwater;
	NoiseParams np_mountain;
	NoiseParams np_ridge;

	MapgenV7PParams();
	~MapgenV7PParams() {}

	void readParams(const Settings *settings);
	void writeParams(Settings *settings) const;
	void setDefaultSettings(Settings *settings);
};

class MapgenV7P : public MapgenBasic
{
public:
	MapgenV7P(MapgenV7PParams *params, EmergeParams *emerge);
	~MapgenV7P();

	virtual MapgenType getType() const { return MAPGEN_V7P; }

	virtual void makeChunk(BlockMakeData *data);
	int getSpawnLevelAtPoint(v2s16 p);

	float baseTerrainLevelAtPoint(s16 x, s16 z);
	float baseTerrainLevelFromMap(int index);
	float mountainLevelAtPoint(s16 x, s16 z);
	float mountainLevelFromMap(int idx_xz);

	void generateBedrock();
	int generateTerrain();
	void generateRidgeTerrain();
	virtual void generateCaves(s16 max_stone_y, s16 large_cave_depth);

private:
	Noise *noise_terrain_base;
	Noise *noise_terrain_alt;
	Noise *noise_terrain_persist;
	Noise *noise_height_select;
	Noise *noise_mount_height;
	Noise *noise_ridge_uwater;
	Noise *noise_mountain;
	Noise *noise_ridge;

	content_t c_bedrock;

	u32 small_caves_count;
	s16 bedrock_level;
};

#endif
