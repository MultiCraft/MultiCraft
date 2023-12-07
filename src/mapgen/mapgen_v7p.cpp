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


#include "mapgen.h"
#include "voxel.h"
#include "noise.h"
#include "mapblock.h"
#include "mapnode.h"
#include "map.h"
#include "nodedef.h"
#include "voxelalgorithms.h"
//#include "profiler.h" // For TimeTaker
#include "settings.h" // For g_settings
#include "emerge.h"
#include "dungeongen.h"
#include "cavegen.h"
#include "mg_biome.h"
#include "mg_ore.h"
#include "mg_decoration.h"
#include "mapgen_v7p.h"


FlagDesc flagdesc_mapgen_v7p[] = {
	{"mountains",  MGV7P_MOUNTAINS},
	{"ridges",     MGV7P_RIDGES},
	{NULL,         0}
};


////////////////////////////////////////////////////////////////////////////////


MapgenV7P::MapgenV7P(MapgenV7PParams *params, EmergeParams *emerge)
	: MapgenBasic(MAPGEN_V7P, params, emerge)
{
	spflags = params->spflags;

	// Initialise some values to hardcoded defaults
	cave_width         = 0.09;
	large_cave_depth   = -33;
	small_cave_num_min = 0;
	small_cave_num_max = 0;
	large_cave_num_min = 0;
	large_cave_num_max = 2;
	large_cave_flooded = 0.5;
	cavern_limit       = -256;
	cavern_taper       = 256;
	cavern_threshold   = 0.7;
	dungeon_ymin       = -31000;
	dungeon_ymax       = 31000;

	// Average of mgv6 small caves count
	small_caves_count = 6 * csize.X * csize.Z * MAP_BLOCKSIZE / 50000;

	// Highest level of bedrock
	bedrock_level = water_level - 64;

	// 2D noise
	noise_terrain_base    = new Noise(&params->np_terrain_base,    seed, csize.X, csize.Z);
	noise_terrain_alt     = new Noise(&params->np_terrain_alt,     seed, csize.X, csize.Z);
	noise_terrain_persist = new Noise(&params->np_terrain_persist, seed, csize.X, csize.Z);
	noise_height_select   = new Noise(&params->np_height_select,   seed, csize.X, csize.Z);
	noise_filler_depth    = new Noise(&params->np_filler_depth,    seed, csize.X, csize.Z);

	if (spflags & MGV7P_MOUNTAINS) {
		noise_mount_height = new Noise(&params->np_mount_height, seed, csize.X, csize.Z);
		noise_mountain     = new Noise(&params->np_mountain,     seed, csize.X, csize.Z);
	}

	if (spflags & MGV7P_RIDGES) {
		noise_ridge_uwater = new Noise(&params->np_ridge_uwater, seed, csize.X, csize.Z);
		noise_ridge        = new Noise(&params->np_ridge,        seed, csize.X, csize.Z);
	}

	// 3D noise, 1 down overgeneration
	MapgenBasic::np_cave1    = NoiseParams(0.0, 12.0, v3f(61.0, 61.0, 61.0), 52534, 3, 0.5, 2.0);
	MapgenBasic::np_cave2    = NoiseParams(0.0, 12.0, v3f(67.0, 67.0, 67.0), 10325, 3, 0.5, 2.0);
	MapgenBasic::np_cavern   = NoiseParams(0.0, 1.0, v3f(384.0, 128.0, 384.0), 723, 5, 0.63, 2.0);
	// 3D noise
	MapgenBasic::np_dungeons = NoiseParams(0.9, 0.5, v3f(500.0, 500.0, 500.0), 0, 2, 0.8, 2.0);

	// Resolve additional nodes
	c_bedrock = ndef->getId("mapgen_bedrock");
}


MapgenV7P::~MapgenV7P()
{
	delete noise_terrain_base;
	delete noise_terrain_alt;
	delete noise_terrain_persist;
	delete noise_height_select;
	delete noise_filler_depth;

	if (spflags & MGV7P_MOUNTAINS) {
		delete noise_mount_height;
		delete noise_mountain;
	}

	if (spflags & MGV7P_RIDGES) {
		delete noise_ridge_uwater;
		delete noise_ridge;
	}
}


MapgenV7PParams::MapgenV7PParams()
{
	spflags            = MGV7P_MOUNTAINS | MGV7P_RIDGES;

	np_terrain_base    = NoiseParams(4,    35,  v3f(600,  600,  600),  82341, 5, 0.6,  2.0);
	np_terrain_alt     = NoiseParams(4,    25,  v3f(600,  600,  600),  5934,  5, 0.6,  2.0);
	np_terrain_persist = NoiseParams(0.6,  0.1, v3f(2000, 2000, 2000), 539,   3, 0.6,  2.0);
	np_height_select   = NoiseParams(-8,   16,  v3f(500,  500,  500),  4213,  6, 0.7,  2.0);
	np_filler_depth    = NoiseParams(0,    1.2, v3f(150,  150,  150),  261,   3, 0.7,  2.0);
	np_mount_height    = NoiseParams(128,  56,  v3f(1000, 1000, 1000), 72449, 3, 0.6,  2.0);
	np_ridge_uwater    = NoiseParams(0,    1,   v3f(1000, 1000, 1000), 85039, 5, 0.6,  2.0);
	np_mountain        = NoiseParams(-0.6, 1,   v3f(250,  250,  250),  5333,  5, 0.63, 2.0);
	np_ridge           = NoiseParams(0,    1,   v3f(100,  100,  100),  6467,  4, 0.75, 2.0);
}


void MapgenV7PParams::readParams(const Settings *settings)
{
	settings->getFlagStrNoEx("mgv7p_spflags",              spflags, flagdesc_mapgen_v7p);

	settings->getNoiseParams("mgv7p_np_terrain_base",      np_terrain_base);
	settings->getNoiseParams("mgv7p_np_terrain_alt",       np_terrain_alt);
	settings->getNoiseParams("mgv7p_np_terrain_persist",   np_terrain_persist);
	settings->getNoiseParams("mgv7p_np_height_select",     np_height_select);
	settings->getNoiseParams("mgv7p_np_filler_depth",      np_filler_depth);
	settings->getNoiseParams("mgv7p_np_mount_height",      np_mount_height);
	settings->getNoiseParams("mgv7p_np_ridge_uwater",      np_ridge_uwater);
	settings->getNoiseParams("mgv7p_np_mountain",          np_mountain);
	settings->getNoiseParams("mgv7p_np_ridge",             np_ridge);
}


void MapgenV7PParams::writeParams(Settings *settings) const
{
	settings->setFlagStr("mgv7p_spflags",           spflags, flagdesc_mapgen_v7p);

	settings->setNoiseParams("mgv7p_np_terrain_base",      np_terrain_base);
	settings->setNoiseParams("mgv7p_np_terrain_alt",       np_terrain_alt);
	settings->setNoiseParams("mgv7p_np_terrain_persist",   np_terrain_persist);
	settings->setNoiseParams("mgv7p_np_height_select",     np_height_select);
	settings->setNoiseParams("mgv7p_np_filler_depth",      np_filler_depth);
	settings->setNoiseParams("mgv7p_np_mount_height",      np_mount_height);
	settings->setNoiseParams("mgv7p_np_ridge_uwater",      np_ridge_uwater);
	settings->setNoiseParams("mgv7p_np_mountain",          np_mountain);
	settings->setNoiseParams("mgv7p_np_ridge",             np_ridge);
}


void MapgenV7PParams::setDefaultSettings(Settings *settings)
{
	settings->setDefault("mgv7p_spflags", flagdesc_mapgen_v7p,
		MGV7P_MOUNTAINS | MGV7P_RIDGES);
}


///////////////////////////////////////////////////////////////////////////////


int MapgenV7P::getSpawnLevelAtPoint(v2s16 p)
{
	// If enabled, first check if inside a river
	if (spflags & MGV7P_RIDGES) {
		float width = 0.2;
		float uwatern =
				NoisePerlin2D(&noise_ridge_uwater->np, p.X, p.Y, seed) * 2;
		if (fabs(uwatern) <= width)
			return MAX_MAP_GENERATION_LIMIT; // Unsuitable spawn point
	}

	// Base/mountain terrain calculation
	s16 y = baseTerrainLevelAtPoint(p.X, p.Y);
	if (spflags & MGV7P_MOUNTAINS)
		y = MYMAX(mountainLevelAtPoint(p.X, p.Y), y);

	if (y <= water_level || y > water_level + 16)
		return MAX_MAP_GENERATION_LIMIT; // Unsuitable spawn point
	else
		return y + 2; // +2 because surface is at y and due to biome 'dust'
}


void MapgenV7P::makeChunk(BlockMakeData *data)
{
	// Pre-conditions
	assert(data->vmanip);
	assert(data->nodedef);

	this->generating = true;
	this->vm = data->vmanip;
	this->ndef = data->nodedef;

	v3s16 blockpos_min = data->blockpos_min;
	v3s16 blockpos_max = data->blockpos_max;
	node_min = blockpos_min * MAP_BLOCKSIZE;
	node_max = (blockpos_max + v3s16(1, 1, 1)) * MAP_BLOCKSIZE - v3s16(1, 1, 1);
	full_node_min = (blockpos_min - 1) * MAP_BLOCKSIZE;
	full_node_max = (blockpos_max + 2) * MAP_BLOCKSIZE - v3s16(1, 1, 1);

	blockseed = getBlockSeed2(full_node_min, seed);

	if (node_max.Y <= bedrock_level) {
		// Only generate bedrock
		generateBedrock();
	} else {
		// Generate base and mountain terrain
		s16 stone_surface_max_y = generateTerrain();

		// Generate rivers
		if (spflags & MGV7P_RIDGES)
			generateRidgeTerrain();

		// Create heightmap
		updateHeightmap(node_min, node_max);

		// Init biome generator, place biome-specific nodes, and build biomemap
		if (flags & MG_BIOMES) {
			biomegen->calcBiomeNoise(node_min);
			generateBiomes();
		}

		// Generate mgv6 caves but not deep into bedrock
		if (flags & MG_CAVES)
			generateCaves(stone_surface_max_y, water_level);

		// Generate dungeons
		if (flags & MG_DUNGEONS)
			generateDungeons(stone_surface_max_y);

		// Generate the registered decorations
		if (flags & MG_DECORATIONS)
			m_emerge->decomgr->placeAllDecos(
					this, blockseed, node_min, node_max);

		// Generate the registered ores
		m_emerge->oremgr->placeAllOres(this, blockseed, node_min, node_max);

		// Sprinkle some dust on top after everything else was generated
		dustTopNodes();

		// Update liquids
		updateLiquid(&data->transforming_liquid, full_node_min, full_node_max);
	}

	// Calculate lighting
	if (flags & MG_LIGHT)
		calcLighting(node_min - v3s16(0, 1, 0), node_max + v3s16(0, 1, 0),
				full_node_min, full_node_max, true);

	this->generating = false;
}


float MapgenV7P::baseTerrainLevelAtPoint(s16 x, s16 z)
{
	float hselect = NoisePerlin2D(&noise_height_select->np, x, z, seed);
	hselect = rangelim(hselect, 0.0, 1.0);

	float persist = NoisePerlin2D(&noise_terrain_persist->np, x, z, seed);

	noise_terrain_base->np.persist = persist;
	float height_base = NoisePerlin2D(&noise_terrain_base->np, x, z, seed);

	noise_terrain_alt->np.persist = persist;
	float height_alt = NoisePerlin2D(&noise_terrain_alt->np, x, z, seed);

	if (height_alt > height_base)
		return height_alt;

	return (height_base * hselect) + (height_alt * (1.0 - hselect));
}


float MapgenV7P::baseTerrainLevelFromMap(int index)
{
	float hselect = rangelim(noise_height_select->result[index], 0.0, 1.0);
	float height_base = noise_terrain_base->result[index];
	float height_alt = noise_terrain_alt->result[index];

	if (height_alt > height_base)
		return height_alt;

	return (height_base * hselect) + (height_alt * (1.0 - hselect));
}


float MapgenV7P::mountainLevelAtPoint(s16 x, s16 z)
{
	float mnt_h_n = NoisePerlin2D(&noise_mount_height->np, x, z, seed);
	float mnt_n = NoisePerlin2D(&noise_mountain->np, x, z, seed);

	return mnt_n * mnt_h_n;
}


float MapgenV7P::mountainLevelFromMap(int idx_xz)
{
	float mounthn = noise_mount_height->result[idx_xz];
	float mountn = noise_mountain->result[idx_xz];

	return mountn * mounthn;
}


void MapgenV7P::generateBedrock()
{
	MapNode n_bedrock(c_bedrock);

	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 y = node_min.Y - 1; y <= node_max.Y + 1; y++) {
		u32 vi = vm->m_area.index(node_min.X, y, z);

		for (s16 x = node_min.X; x <= node_max.X; x++, vi++) {
			if (vm->m_data[vi].getContent() == CONTENT_IGNORE)
				vm->m_data[vi] = n_bedrock;
		}
	}
}


int MapgenV7P::generateTerrain()
{
	MapNode n_stone(c_stone);
	MapNode n_bedrock(c_bedrock);
	MapNode n_water(c_water_source);
	MapNode n_air(CONTENT_AIR);

	//// Calculate noise for terrain generation
	noise_terrain_persist->perlinMap2D(node_min.X, node_min.Z);
	float *persistmap = noise_terrain_persist->result;

	noise_terrain_base ->perlinMap2D(node_min.X, node_min.Z, persistmap);
	noise_terrain_alt  ->perlinMap2D(node_min.X, node_min.Z, persistmap);
	noise_height_select->perlinMap2D(node_min.X, node_min.Z);

	if (spflags & MGV7P_MOUNTAINS) {
		noise_mount_height->perlinMap2D(node_min.X, node_min.Z);
		noise_mountain    ->perlinMap2D(node_min.X, node_min.Z);
	}

	//// Place nodes
	const v3s16 &em = vm->m_area.getExtent();
	s16 stone_surface_max_y = -MAX_MAP_GENERATION_LIMIT;
	u32 index2d = 0;

	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 x = node_min.X; x <= node_max.X; x++, index2d++) {
		s16 surface_y = baseTerrainLevelFromMap(index2d);
		if (spflags & MGV7P_MOUNTAINS)
			surface_y = MYMAX(mountainLevelFromMap(index2d), surface_y);

		if (surface_y > stone_surface_max_y)
			stone_surface_max_y = surface_y;

		u32 vi = vm->m_area.index(x, node_min.Y - 1, z);

		for (s16 y = node_min.Y - 1; y <= node_max.Y + 1; y++) {
			if (vm->m_data[vi].getContent() == CONTENT_IGNORE) {
				if (y <= surface_y) {
					if (y <= bedrock_level)
						vm->m_data[vi] = n_bedrock; // Bedrock
					else
						vm->m_data[vi] = n_stone; // Base and mountain terrain
				} else if (y <= water_level) {
					vm->m_data[vi] = n_water; // Water
				} else {
					vm->m_data[vi] = n_air; // Air
				}
			}
			vm->m_area.add_y(em, vi, 1);
		}
	}

	return stone_surface_max_y;
}


void MapgenV7P::generateRidgeTerrain()
{
	if (node_max.Y < water_level - 16)
		return;

	noise_ridge->perlinMap2D(node_min.X, node_min.Z);
	noise_ridge_uwater->perlinMap2D(node_min.X, node_min.Z);

	MapNode n_water(c_water_source);
	MapNode n_air(CONTENT_AIR);

	float width = 0.2;

	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 y = node_min.Y - 1; y <= node_max.Y + 1; y++) {
		u32 vi = vm->m_area.index(node_min.X, y, z);

		for (s16 x = node_min.X; x <= node_max.X; x++, vi++) {
			int index2d = (z - node_min.Z) * csize.X + (x - node_min.X);

			float uwatern = noise_ridge_uwater->result[index2d] * 2;
			if (fabs(uwatern) > width)
				continue;

			float altitude = y - water_level;
			float height_mod = (altitude + 17) / 2.5;
			float width_mod  = width - fabs(uwatern);
			float nridge = noise_ridge->result[index2d] * MYMAX(altitude, 0) / 7.0;

			if (nridge + width_mod * height_mod < 0.6)
				continue;

			vm->m_data[vi] = (y > water_level) ? n_air : n_water;
		}
	}
}


void MapgenV7P::generateCaves(s16 max_stone_y, s16 large_cave_depth)
{
	if (max_stone_y < node_min.Y)
		return;

	PseudoRandom ps(blockseed + 21343);
	PseudoRandom ps2(blockseed + 1032);

	u32 large_caves_count = (node_max.Y <= large_cave_depth) ? ps.range(0, 2) : 0;

	for (u32 i = 0; i < small_caves_count + large_caves_count; i++) {
		CavesV6 cave(ndef, &gennotify, water_level, c_water_source, c_lava_source);

		bool large_cave = (i >= small_caves_count);
		cave.makeCave(vm, node_min, node_max, &ps, &ps2,
			large_cave, max_stone_y, heightmap);
	}
}
