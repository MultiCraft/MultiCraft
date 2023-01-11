/*
Minetest
Copyright (C) 2013 sapier <sapier AT gmx DOT net>

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

#if defined(_WIN32)
#include <windows.h>
#elif defined(__MACH__) && defined(__APPLE__)
#include <mach/semaphore.h>
#else
#include <semaphore.h>
#endif

#include "porting.h"
#include "util/basic_macros.h"

#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
#include <SDL.h>
#endif

class Semaphore
{
public:
	Semaphore(int val = 0);
	~Semaphore();

	DISABLE_CLASS_COPY(Semaphore);

	void post(unsigned int num = 1);
	void wait();
	bool wait(unsigned int time_ms);

private:
#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
	SDL_sem* semaphore;
#elif defined(WIN32)
	HANDLE semaphore;
#elif defined(__MACH__) && defined(__APPLE__)
	semaphore_t semaphore;
#else
	sem_t semaphore;
#endif
};
