/*
MultiCraft

Copyright (C) 2013 sapier <sapier AT gmx DOT net>
Copyright (C) 2023 Dawid Gan <deveee@gmail.com>

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

#include "threading/sdl_semaphore.h"

#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_

#include <cassert>

#define UNUSED(expr)                                                                     \
	do {                                                                             \
		(void)(expr);                                                            \
	} while (0)

Semaphore::Semaphore(int val)
{
	semaphore = SDL_CreateSemaphore(val);
}

Semaphore::~Semaphore()
{
	SDL_DestroySemaphore(semaphore);
}

void Semaphore::post(unsigned int num)
{
	assert(num > 0);
	for (unsigned i = 0; i < num; i++) {
		int ret = SDL_SemPost(semaphore);
		assert(!ret);
		UNUSED(ret);
	}
}

void Semaphore::wait()
{
	int ret = SDL_SemWait(semaphore);
	assert(!ret);
	UNUSED(ret);
}

bool Semaphore::wait(unsigned int time_ms)
{
	int ret;
	if (time_ms > 0) {
		ret = SDL_SemWaitTimeout(semaphore, time_ms);
	} else {
		ret = SDL_SemTryWait(semaphore);
	}
	assert(!ret || ret == SDL_MUTEX_TIMEDOUT);
	return !ret;
}

#endif
