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

#include <memory>

#include "sound.h"

#define AL_ALEXT_PROTOTYPES
#if defined(_WIN32)
	#include <al.h>
	#include <alc.h>
	#include <alext.h>
#else
	#include <AL/al.h>
	#include <AL/alc.h>
	#include <AL/alext.h>
#endif

typedef std::unique_ptr<ALCdevice, void (*)(ALCdevice *p)> unique_ptr_alcdevice;
typedef std::unique_ptr<ALCcontext, void(*)(ALCcontext *p)> unique_ptr_alccontext;

class SoundManagerSingleton
{
private:
	unique_ptr_alcdevice m_device;
	unique_ptr_alccontext m_context;

public:
	SoundManagerSingleton();
	~SoundManagerSingleton();

	bool init();
	void pauseDevice();
	void resumeDevice();
};

extern SoundManagerSingleton* g_sound_manager_singleton;

SoundManagerSingleton* createSoundManagerSingleton();
void deleteSoundManagerSingleton(SoundManagerSingleton* smg);
ISoundManager *createOpenALSoundManager(
		SoundManagerSingleton *smg, OnDemandSoundFetcher *fetcher);
