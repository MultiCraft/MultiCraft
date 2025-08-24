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

#pragma once

#include "IrrCompileConfig.h"

#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_

#include "util/basic_macros.h"

#include <string>
#include <atomic>
#include <mutex>

#include <SDL3/SDL.h>

class Thread
{
public:
	Thread(const std::string &name = "");
	virtual ~Thread();
	DISABLE_CLASS_COPY(Thread)

	/*
	 * Begins execution of a new thread at the pure virtual method Thread::run().
	 * Execution of the thread is guaranteed to have started after this function
	 * returns.
	 */
	bool start();

	/*
	 * Requests that the thread exit gracefully.
	 * Returns immediately; thread execution is guaranteed to be complete after
	 * a subsequent call to Thread::wait.
	 */
	bool stop();

	/*
	 * Waits for thread to finish.
	 * Note:  This does not stop a thread, you have to do this on your own.
	 * Returns false immediately if the thread is not started or has been waited
	 * on before.
	 */
	bool wait();

	/*
	 * Returns true if the calling thread is this Thread object.
	 */
	bool isCurrentThread() { return SDL_GetCurrentThreadID() == getThreadId(); }

	bool isRunning() { return m_running; }
	bool stopRequested() { return m_request_stop; }

	SDL_ThreadID getThreadId() { return SDL_GetThreadID(m_thread_obj); }

	/*
	 * Gets the thread return value.
	 * Returns true if the thread has exited and the return value was available,
	 * or false if the thread has yet to finish.
	 */
	bool getReturnValue(void **ret);

	/*
	 * Binds (if possible, otherwise sets the affinity of) the thread to the
	 * specific processor specified by proc_number.
	 */
	bool bindToProcessor(unsigned int proc_number);

	/*
	 * Sets the thread priority to the specified priority.
	 *
	 * prio can be one of: SDL_THREAD_PRIORITY_LOW, SDL_THREAD_PRIORITY_NORMAL,
	 * SDL_THREAD_PRIORITY_HIGH.
	 */
	bool setPriority(SDL_ThreadPriority prio);

	/*
	 * Returns the thread object of the current thread if it exists.
	 */
	static Thread *getCurrentThread();

	/*
	 * Sets the currently executing thread's name to where supported; useful
	 * for debugging.
	 */
	static void setName(const std::string &name);

	/*
	 * Returns the number of processors/cores configured and active on this machine.
	 */
	static unsigned int getNumberOfProcessors();

protected:
	std::string m_name;

	virtual void *run() = 0;

private:
	static int threadProc(void *data);

	void *m_retval = nullptr;
	bool m_joinable = false;
	std::atomic<bool> m_request_stop;
	std::atomic<bool> m_running;
	std::mutex m_mutex;
	std::mutex m_start_finished_mutex;

	SDL_Thread *m_thread_obj = nullptr;
};

#endif
