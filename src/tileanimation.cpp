/*
Minetest
Copyright (C) 2016 sfan5 <sfan5@live.de>

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
#include "tileanimation.h"
#include "util/serialize.h"

void TileAnimationParams::serialize(std::ostream &os, u8 tiledef_version) const
{
	if (tiledef_version < 3 && type != TAT_VERTICAL_FRAMES) {
		writeU8(os, TAT_NONE);
		writeU16(os, 1);
		writeU16(os, 1);
		writeF1000(os, 1.0f);
		return;
	}

	writeU8(os, type);

	// Approximate protocol version
	const u16 protocol_version = tiledef_version >= 6 ? 37 : 32;

	if (type == TAT_VERTICAL_FRAMES) {
		writeU16(os, vertical_frames.aspect_w);
		writeU16(os, vertical_frames.aspect_h);
		writeF(os, vertical_frames.length, protocol_version);
	} else if (type == TAT_SHEET_2D) {
		writeU8(os, sheet_2d.frames_w);
		writeU8(os, sheet_2d.frames_h);
		writeF(os, sheet_2d.frame_length, protocol_version);
	}
}

void TileAnimationParams::deSerialize(std::istream &is, u8 tiledef_version)
{
	type = (TileAnimationType) readU8(is);

	if (type == TAT_VERTICAL_FRAMES) {
		vertical_frames.aspect_w = readU16(is);
		vertical_frames.aspect_h = readU16(is);
		vertical_frames.length = fabs(readF32(is));
	} else if (type == TAT_SHEET_2D) {
		sheet_2d.frames_w = readU8(is);
		sheet_2d.frames_h = readU8(is);
		sheet_2d.frame_length = fabs(readF32(is));
	}
}

void TileAnimationParams::determineParams(v2u32 texture_size, int *frame_count,
		int *frame_length_ms, v2u32 *frame_size) const
{
	if (type == TAT_VERTICAL_FRAMES) {
		int frame_height = (float)texture_size.X /
				(float)vertical_frames.aspect_w *
				(float)vertical_frames.aspect_h;
		int _frame_count = texture_size.Y / frame_height;
		if (frame_count)
			*frame_count = _frame_count;
		if (frame_length_ms)
			*frame_length_ms = 1000.0 * vertical_frames.length / _frame_count;
		if (frame_size)
			*frame_size = v2u32(texture_size.X, frame_height);
	} else if (type == TAT_SHEET_2D) {
		if (frame_count)
			*frame_count = sheet_2d.frames_w * sheet_2d.frames_h;
		if (frame_length_ms)
			*frame_length_ms = 1000 * sheet_2d.frame_length;
		if (frame_size)
			*frame_size = v2u32(texture_size.X / sheet_2d.frames_w, texture_size.Y / sheet_2d.frames_h);
	}
	// caller should check for TAT_NONE
}

void TileAnimationParams::getTextureModifer(std::ostream &os, v2u32 texture_size, int frame) const
{
	if (type == TAT_NONE)
		return;
	if (type == TAT_VERTICAL_FRAMES) {
		int frame_count;
		determineParams(texture_size, &frame_count, NULL, NULL);
		os << "^[verticalframe:" << frame_count << ":" << frame;
	} else if (type == TAT_SHEET_2D) {
		int q, r;
		q = frame / sheet_2d.frames_w;
		r = frame % sheet_2d.frames_w;
		os << "^[sheet:" << sheet_2d.frames_w << "x" << sheet_2d.frames_h
			<< ":" << r << "," << q;
	}
}

v2f TileAnimationParams::getTextureCoords(v2u32 texture_size, int frame) const
{
	v2u32 ret(0, 0);
	if (type == TAT_VERTICAL_FRAMES) {
		int frame_height = (float)texture_size.X /
				(float)vertical_frames.aspect_w *
				(float)vertical_frames.aspect_h;
		ret = v2u32(0, frame_height * frame);
	} else if (type == TAT_SHEET_2D) {
		v2u32 frame_size;
		determineParams(texture_size, NULL, NULL, &frame_size);
		int q, r;
		q = frame / sheet_2d.frames_w;
		r = frame % sheet_2d.frames_w;
		ret = v2u32(r * frame_size.X, q * frame_size.Y);
	}
	return v2f(ret.X / (float) texture_size.X, ret.Y / (float) texture_size.Y);
}
