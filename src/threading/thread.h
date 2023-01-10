/*
This file is a part of the JThread package, which contains some object-
oriented thread wrappers for different thread implementations.

Copyright (c) 2000-2006  Jori Liesenborgs (jori.liesenborgs@gmail.com)

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

#include "porting.h"
#include "util/basic_macros.h"

#include <string>
#include <atomic>
#include <thread>
#include <mutex>

#ifdef _AIX
	#include <sys/thread.h> // for tid_t
#endif

#ifdef __HAIKU__
	#include <kernel/OS.h>
#endif

#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
#include <SDL.h>
#endif

/*
 * On platforms using pthreads, these five priority classes correlate to
 * even divisions between the minimum and maximum reported thread priority.
 */
#if !defined(_WIN32)
	#define THREAD_PRIORITY_LOWEST       0
	#define THREAD_PRIORITY_BELOW_NORMAL 1
	#define THREAD_PRIORITY_NORMAL       2
	#define THREAD_PRIORITY_ABOVE_NORMAL 3
	#define THREAD_PRIORITY_HIGHEST      4
#endif



class Thread {
public:
	Thread(const std::string &name="");
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
#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
	bool isCurrentThread() { return SDL_ThreadID() == getThreadId(); }
#else
	bool isCurrentThread() { return std::this_thread::get_id() == getThreadId(); }
#endif

	bool isRunning() { return m_running; }
	bool stopRequested() { return m_request_stop; }

#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
	SDL_threadID getThreadId() { return SDL_GetThreadID(m_thread_obj); }
#else
	std::thread::id getThreadId() { return m_thread_obj->get_id(); }
#endif

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

#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
	/*
	 * Sets the thread priority to the specified priority.
	 *
	 * prio can be one of: SDL_THREAD_PRIORITY_LOW, SDL_THREAD_PRIORITY_NORMAL,
	 * SDL_THREAD_PRIORITY_HIGH.
	 */
	bool setPriority(SDL_ThreadPriority prio);
#else
	/*
	 * Sets the thread priority to the specified priority.
	 *
	 * prio can be one of: THREAD_PRIORITY_LOWEST, THREAD_PRIORITY_BELOW_NORMAL,
	 * THREAD_PRIORITY_NORMAL, THREAD_PRIORITY_ABOVE_NORMAL, THREAD_PRIORITY_HIGHEST.
	 * On Windows, any of the other priorites as defined by SetThreadPriority
	 * are supported as well.
	 *
	 * Note that it may be necessary to first set the threading policy or
	 * scheduling algorithm to one that supports thread priorities if not
	 * supported by default, otherwise this call will have no effect.
	 */
	bool setPriority(int prio);
#endif

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
#ifndef _IRR_COMPILE_WITH_SDL_DEVICE_
	std::thread::native_handle_type getThreadHandle()
		{ return m_thread_obj->native_handle(); }
#endif

#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
	static int threadProc(void *data);
#else
	static void threadProc(Thread *thr);
#endif

	void *m_retval = nullptr;
	bool m_joinable = false;
	std::atomic<bool> m_request_stop;
	std::atomic<bool> m_running;
	std::mutex m_mutex;
	std::mutex m_start_finished_mutex;

#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
	SDL_Thread *m_thread_obj = nullptr;
#else
	std::thread *m_thread_obj = nullptr;
#endif

#ifdef _AIX
	// For AIX, there does not exist any mapping from pthread_t to tid_t
	// available to us, so we maintain one ourselves.  This is set on thread start.
	tid_t m_kernel_thread_id;
#endif
};

