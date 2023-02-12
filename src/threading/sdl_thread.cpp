/*
MultiCraft

Copyright (C) 2000-2006  Jori Liesenborgs <jori.liesenborgs@gmail.com>
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

#include "threading/sdl_thread.h"
#include "threading/mutex_auto_lock.h"
#include "log.h"
#include "porting.h"

#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_

Thread::Thread(const std::string &name) :
		m_name(name), m_request_stop(false), m_running(false)
{
}

Thread::~Thread()
{
	m_running = false;
	wait();

	// Make sure start finished mutex is unlocked before it's destroyed
	if (m_start_finished_mutex.try_lock())
		m_start_finished_mutex.unlock();
}

bool Thread::start()
{
	MutexAutoLock lock(m_mutex);

	if (m_running)
		return false;

	m_request_stop = false;

	// The mutex may already be locked if the thread is being restarted
	m_start_finished_mutex.try_lock();

	m_thread_obj = SDL_CreateThread(&threadProc, m_name.c_str(), this);

	if (!m_thread_obj)
		return false;

	// Allow spawned thread to continue
	m_start_finished_mutex.unlock();

	while (!m_running)
		sleep_ms(1);

	m_joinable = true;

	return true;
}

bool Thread::stop()
{
	m_request_stop = true;
	return true;
}

bool Thread::wait()
{
	MutexAutoLock lock(m_mutex);

	if (!m_joinable)
		return false;

	SDL_WaitThread(m_thread_obj, NULL);

	m_thread_obj = nullptr;

	assert(m_running == false);
	m_joinable = false;
	return true;
}

bool Thread::getReturnValue(void **ret)
{
	if (m_running)
		return false;

	*ret = m_retval;
	return true;
}

int Thread::threadProc(void *data)
{
	Thread *thr = (Thread *)data;

	g_logger.registerThread(thr->m_name);
	thr->m_running = true;

	// Wait for the thread that started this one to finish initializing the
	// thread handle so that getThreadId/getThreadHandle will work.
	thr->m_start_finished_mutex.lock();

	thr->m_retval = thr->run();

	thr->m_running = false;
	// Unlock m_start_finished_mutex to prevent data race condition on Windows.
	// On Windows with VS2017 build TerminateThread is called and this mutex is not
	// released. We try to unlock it from caller thread and it's refused by system.
	thr->m_start_finished_mutex.unlock();
	g_logger.deregisterThread();

	return 0;
}

void Thread::setName(const std::string &name)
{
	// Name can be set only during thread creation.
}

unsigned int Thread::getNumberOfProcessors()
{
	return SDL_GetCPUCount();
}

bool Thread::bindToProcessor(unsigned int proc_number)
{
	// Not available in SDL
	return false;
}

bool Thread::setPriority(SDL_ThreadPriority prio)
{
	int result = SDL_SetThreadPriority(prio);
	return result == 0;
}

#endif
