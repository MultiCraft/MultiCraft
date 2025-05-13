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

#pragma once

#include "irrlichttypes.h"
#include "exceptions.h"
#include <iostream>
#include "util/pointer.h"

/*
	Map format serialization version
	--------------------------------

	For map data (blocks, nodes, sectors).

	NOTE: The goal is to increment this so that saved maps will be
	      loadable by any version. Other compatibility is not
		  maintained.

	0: original networked test with 1-byte nodes
	1: update with 2-byte nodes
	2: lighting is transmitted in param
	3: optional fetching of far blocks
	4: block compression
	5: sector objects NOTE: block compression was left accidentally out
	6: failed attempt at switching block compression on again
	7: block compression switched on again
	8: server-initiated block transfers and all kinds of stuff
	9: block objects
	10: water pressure
	11: zlib'd blocks, block flags
	12: UnlimitedHeightmap now uses interpolated areas
	13: Mapgen v2
	14: NodeMetadata
	15: StaticObjects
	16: larger maximum size of node metadata, and compression
	17: MapBlocks contain timestamp
	18: new generator (not really necessary, but it's there)
	19: new content type handling
	20: many existing content types translated to extended ones
	21: dynamic content type allocation
	22: minerals removed, facedir & wallmounted changed
	23: new node metadata format
	24: 16-bit node ids and node timers (never released as stable)
	25: Improved node timer format
	26: Never written; read the same as 25
	27: Added light spreading flags to blocks
	28: Added "private" flag to NodeMetadata
	29: Switched compression to zstd, a bit of reorganization
*/
// This represents an uninitialized or invalid format
#define SER_FMT_VER_INVALID 255

// Highest supported serialization version
#if USE_ZSTD
#define SER_FMT_VER_HIGHEST_READ 29
#else
#define SER_FMT_VER_HIGHEST_READ 28
#endif

// Saved on disk version
#if USE_ZSTD
#define SER_FMT_VER_HIGHEST_WRITE 29
#else
#define SER_FMT_VER_HIGHEST_WRITE 28
#endif

// Lowest supported serialization version
#define SER_FMT_VER_LOWEST_READ 0
// Lowest serialization version for writing
// Can't do < 24 anymore; we have 16-bit dynamically allocated node IDs
// in memory; conversion just won't work in this direction.
#define SER_FMT_VER_LOWEST_WRITE 24

inline bool ser_ver_supported(s32 v) {
	return v >= SER_FMT_VER_LOWEST_READ && v <= SER_FMT_VER_HIGHEST_READ;
}

/*
	Misc. serialization functions
*/

void compressZlib(const u8 *data, size_t data_size, std::ostream &os, int level = -1);
void compressZlib(const std::string &data, std::ostream &os, int level = -1);
void decompressZlib(std::istream &is, std::ostream &os, size_t limit = 0);

#if USE_ZSTD
void compressZstd(const u8 *data, size_t data_size, std::ostream &os, int level = 0);
void compressZstd(const std::string &data, std::ostream &os, int level = 0);
void decompressZstd(std::istream &is, std::ostream &os);
#endif

// These choose between zlib and a self-made one according to version
void compress(const SharedBuffer<u8> &data, std::ostream &os, u8 version, int level = -1);
void compress(const std::string &data, std::ostream &os, u8 version, int level = -1);
void compress(u8 *data, u32 size, std::ostream &os, u8 version, int level = -1);
void decompress(std::istream &is, std::ostream &os, u8 version);
