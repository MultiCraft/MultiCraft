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

#pragma once

#include "IrrCompileConfig.h"

#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_

#include "porting.h"
#include "util/basic_macros.h"

#include <SDL.h>

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
	SDL_sem *semaphore;
};

#endif
