/*
This file is a part of the JThread package, which contains some object-
oriented thread wrappers for different thread implementations.

Copyright (c) 2000-2006  Jori Liesenborgs (jori.liesenborgs@gmail.com)
Copyright (c) 2023 Dawid Gan <deveee@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include "threading/sdl_thread.h"
#include "threading/mutex_auto_lock.h"
#include "log.h"

#include <cassert>

#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_

thread_local Thread *current_thread = nullptr;

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

	if (m_running || m_thread_obj)
		return false;

	m_request_stop = false;
	m_started = false;

	MutexAutoLock start_finished_lock(m_start_finished_mutex);

	m_thread_obj = SDL_CreateThread(&threadProc, m_name.c_str(), this);

	if (!m_thread_obj)
		return false;

	// Allow spawned thread to continue
	start_finished_lock.unlock();

	// start() promises that the thread has begun executing once it returns
	m_running_cv.wait(lock, [this] { return m_started; });

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
	current_thread = thr;

	g_logger.registerThread(thr->m_name);

	{
		MutexAutoLock lock(thr->m_mutex);
		thr->m_running = true;
		thr->m_started = true;
	}
	thr->m_running_cv.notify_all();

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

Thread *Thread::getCurrentThread()
{
	return current_thread;
}

void Thread::setName(const std::string &name)
{
	// Name can be set only during thread creation.
}

unsigned int Thread::getNumberOfProcessors()
{
	return SDL_GetNumLogicalCPUCores();
}

bool Thread::bindToProcessor(unsigned int proc_number)
{
	// Not available in SDL
	return false;
}

bool Thread::setPriority(SDL_ThreadPriority prio)
{
	return SDL_SetCurrentThreadPriority(prio);
}

#endif
