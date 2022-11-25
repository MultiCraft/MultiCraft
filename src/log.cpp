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

#include "log.h"

#include "threading/mutex_auto_lock.h"
#include "debug.h"
#include "gettime.h"
#include "porting.h"
#include "settings.h"
#include "config.h"
#include "exceptions.h"
#include "util/numeric.h"
#include "log.h"

#include <sstream>
#include <iostream>
#include <algorithm>
#include <cerrno>
#include <cstring>

const int BUFFER_LENGTH = 256;

class StringBuffer : public std::streambuf {
public:
	StringBuffer() {
		buffer_index = 0;
	}

	int overflow(int c);
	virtual void flush(const std::string &buf) = 0;
	std::streamsize xsputn(const char *s, std::streamsize n);
	void push_back(char c);

private:
	char buffer[BUFFER_LENGTH];
	int buffer_index;
};


class LogBuffer : public StringBuffer {
public:
	LogBuffer(Logger &logger, LogLevel lev) :
		logger(logger),
		level(lev)
	{}

	void flush(const std::string &buffer);

private:
	Logger &logger;
	LogLevel level;
};


class RawLogBuffer : public StringBuffer {
public:
	void flush(const std::string &buffer);
};

////
//// Globals
////

Logger g_logger;

StreamLogOutput stdout_output(std::cout);
StreamLogOutput stderr_output(std::cerr);
std::ostream null_stream(NULL);

RawLogBuffer raw_buf;

LogBuffer none_buf(g_logger, LL_NONE);
LogBuffer error_buf(g_logger, LL_ERROR);
LogBuffer warning_buf(g_logger, LL_WARNING);
LogBuffer action_buf(g_logger, LL_ACTION);
LogBuffer info_buf(g_logger, LL_INFO);
LogBuffer verbose_buf(g_logger, LL_VERBOSE);

// Connection
std::ostream *dout_con_ptr = &null_stream;
std::ostream *derr_con_ptr = &verbosestream;

// Common streams
std::ostream rawstream(&raw_buf);
std::ostream dstream(&none_buf);
std::ostream errorstream(&error_buf);
std::ostream warningstream(&warning_buf);
std::ostream actionstream(&action_buf);
std::ostream infostream(&info_buf);
std::ostream verbosestream(&verbose_buf);

// Android
#ifdef __ANDROID__

static unsigned int g_level_to_android[] = {
	ANDROID_LOG_INFO,     // LL_NONE
	//ANDROID_LOG_FATAL,
	ANDROID_LOG_ERROR,    // LL_ERROR
	ANDROID_LOG_WARN,     // LL_WARNING
	ANDROID_LOG_WARN,     // LL_ACTION
	//ANDROID_LOG_INFO,
	ANDROID_LOG_DEBUG,    // LL_INFO
	ANDROID_LOG_VERBOSE,  // LL_VERBOSE
};

class AndroidSystemLogOutput : public ICombinedLogOutput {
	public:
		AndroidSystemLogOutput()
		{
			g_logger.addOutput(this);
		}
		~AndroidSystemLogOutput()
		{
			g_logger.removeOutput(this);
		}
		void logRaw(LogLevel lev, const std::string &line)
		{
			STATIC_ASSERT(ARRLEN(g_level_to_android) == LL_MAX,
				mismatch_between_android_and_internal_loglevels);
			__android_log_print(g_level_to_android[lev],
				PROJECT_NAME_C, "%s", line.c_str());
		}
};

AndroidSystemLogOutput g_android_log_output;

#endif

///////////////////////////////////////////////////////////////////////////////


////
//// Logger
////

LogLevel Logger::stringToLevel(const std::string &name)
{
	if (name == "none")
		return LL_NONE;
	else if (name == "error")
		return LL_ERROR;
	else if (name == "warning")
		return LL_WARNING;
	else if (name == "action")
		return LL_ACTION;
	else if (name == "info")
		return LL_INFO;
	else if (name == "verbose")
		return LL_VERBOSE;
	else
		return LL_MAX;
}

void Logger::addOutput(ILogOutput *out)
{
	addOutputMaxLevel(out, (LogLevel)(LL_MAX - 1));
}

void Logger::addOutput(ILogOutput *out, LogLevel lev)
{
	m_outputs[lev].push_back(out);
}

void Logger::addOutputMasked(ILogOutput *out, LogLevelMask mask)
{
	for (size_t i = 0; i < LL_MAX; i++) {
		if (mask & LOGLEVEL_TO_MASKLEVEL(i))
			m_outputs[i].push_back(out);
	}
}

void Logger::addOutputMaxLevel(ILogOutput *out, LogLevel lev)
{
	assert(lev < LL_MAX);
	for (size_t i = 0; i <= lev; i++)
		m_outputs[i].push_back(out);
}

LogLevelMask Logger::removeOutput(ILogOutput *out)
{
	LogLevelMask ret_mask = 0;
	for (size_t i = 0; i < LL_MAX; i++) {
		std::vector<ILogOutput *>::iterator it;

		it = std::find(m_outputs[i].begin(), m_outputs[i].end(), out);
		if (it != m_outputs[i].end()) {
			ret_mask |= LOGLEVEL_TO_MASKLEVEL(i);
			m_outputs[i].erase(it);
		}
	}
	return ret_mask;
}

void Logger::setLevelSilenced(LogLevel lev, bool silenced)
{
	m_silenced_levels[lev] = silenced;
}

void Logger::registerThread(const std::string &name)
{
	std::thread::id id = std::this_thread::get_id();
	MutexAutoLock lock(m_mutex);
	m_thread_names[id] = name;
}

void Logger::deregisterThread()
{
	std::thread::id id = std::this_thread::get_id();
	MutexAutoLock lock(m_mutex);
	m_thread_names.erase(id);
}

const std::string Logger::getLevelLabel(LogLevel lev)
{
	static const std::string names[] = {
		"",
		"ERROR",
		"WARNING",
		"ACTION",
		"INFO",
		"VERBOSE",
	};
	assert(lev < LL_MAX && lev >= 0);
	STATIC_ASSERT(ARRLEN(names) == LL_MAX,
		mismatch_between_loglevel_names_and_enum);
	return names[lev];
}

LogColor Logger::color_mode = LOG_COLOR_AUTO;

const std::string Logger::getThreadName()
{
	std::map<std::thread::id, std::string>::const_iterator it;

	std::thread::id id = std::this_thread::get_id();
	it = m_thread_names.find(id);
	if (it != m_thread_names.end())
		return it->second;

	std::ostringstream os;
	os << "#0x" << std::hex << id;
	return os.str();
}

void Logger::log(LogLevel lev, const std::string &text)
{
	if (m_silenced_levels[lev])
		return;

	const std::string thread_name = getThreadName();
	const std::string label = getLevelLabel(lev);
	const std::string timestamp = getTimestamp();
	std::ostringstream os(std::ios_base::binary);
	os << timestamp << ": " << label << "[" << thread_name << "]: " << text;

	logToOutputs(lev, os.str(), timestamp, thread_name, text);
}

void Logger::logRaw(LogLevel lev, const std::string &text)
{
	if (m_silenced_levels[lev])
		return;

	logToOutputsRaw(lev, text);
}

void Logger::logToOutputsRaw(LogLevel lev, const std::string &line)
{
	MutexAutoLock lock(m_mutex);
	for (size_t i = 0; i != m_outputs[lev].size(); i++)
		m_outputs[lev][i]->logRaw(lev, line);
}

void Logger::logToOutputs(LogLevel lev, const std::string &combined,
	const std::string &time, const std::string &thread_name,
	const std::string &payload_text)
{
	MutexAutoLock lock(m_mutex);
	for (size_t i = 0; i != m_outputs[lev].size(); i++)
		m_outputs[lev][i]->log(lev, combined, time, thread_name, payload_text);
}


////
//// *LogOutput methods
////

void FileLogOutput::setFile(const std::string &filename, s64 file_size_max)
{
	// Only move debug.txt if there is a valid maximum file size
	bool is_too_large = false;
	if (file_size_max > 0) {
		std::ifstream ifile(filename, std::ios::binary | std::ios::ate);
		is_too_large = ifile.tellg() > file_size_max;
		ifile.close();
	}

	if (is_too_large) {
		std::string filename_secondary = filename + ".1";
		actionstream << "The log file grew too big; it is moved to " <<
			filename_secondary << std::endl;
		remove(filename_secondary.c_str());
		rename(filename.c_str(), filename_secondary.c_str());
	}
	m_stream.open(filename, std::ios::app | std::ios::ate);

	if (!m_stream.good())
		throw FileNotGoodException("Failed to open log file " +
			filename + ": " + strerror(errno));
	m_stream << "\n\n"
		"-------------" << std::endl <<
		"  Separator" << std::endl <<
		"-------------\n" << std::endl;
}

void StreamLogOutput::logRaw(LogLevel lev, const std::string &line)
{
	bool colored_message = (Logger::color_mode == LOG_COLOR_ALWAYS) ||
		(Logger::color_mode == LOG_COLOR_AUTO && is_tty);
#if defined(__MACH__) && defined(__APPLE__)
	if (colored_message) {
		switch (lev) {
		case LL_ERROR:
			// error is red
			m_stream << "📕 ";
			break;
		case LL_WARNING:
			// warning is yellow
			m_stream << "📙 ";
			break;
		case LL_INFO:
			// info is a green
			m_stream << "📗 ";
			break;
		case LL_VERBOSE:
			// verbose is blue
			m_stream << "📘 ";
			break;
		default:
			// action is white
			m_stream << "📔 ";
		}
	}

	m_stream << line << std::endl;
#else
	if (colored_message) {
		switch (lev) {
		case LL_ERROR:
			// error is red
			m_stream << "\033[91m";
			break;
		case LL_WARNING:
			// warning is yellow
			m_stream << "\033[93m";
			break;
		case LL_INFO:
			// info is a bit dark
			m_stream << "\033[37m";
			break;
		case LL_VERBOSE:
			// verbose is darker than info
			m_stream << "\033[2m";
			break;
		default:
			// action is white
			colored_message = false;
		}
	}

	m_stream << line << std::endl;

	if (colored_message) {
		// reset to white color
		m_stream << "\033[0m";
	}
#endif
}

void LogOutputBuffer::updateLogLevel()
{
	const std::string &conf_loglev = g_settings->get("chat_log_level");
	LogLevel log_level = Logger::stringToLevel(conf_loglev);
	if (log_level == LL_MAX) {
		warningstream << "Supplied unrecognized chat_log_level; "
			"showing none." << std::endl;
		log_level = LL_NONE;
	}

	m_logger.removeOutput(this);
	m_logger.addOutputMaxLevel(this, log_level);
}

void LogOutputBuffer::logRaw(LogLevel lev, const std::string &line)
{
	std::string color;

	if (!g_settings->getBool("disable_escape_sequences")) {
		switch (lev) {
		case LL_ERROR: // red
			color = "\x1b(c@#F00)";
			break;
		case LL_WARNING: // yellow
			color = "\x1b(c@#EE0)";
			break;
		case LL_INFO: // grey
			color = "\x1b(c@#BBB)";
			break;
		case LL_VERBOSE: // dark grey
			color = "\x1b(c@#888)";
			break;
		default: break;
		}
	}

	m_buffer.push(color.append(line));
}

////
//// *Buffer methods
////

int StringBuffer::overflow(int c)
{
	push_back(c);
	return c;
}


std::streamsize StringBuffer::xsputn(const char *s, std::streamsize n)
{
	for (int i = 0; i < n; ++i)
		push_back(s[i]);
	return n;
}

void StringBuffer::push_back(char c)
{
	if (c == '\n' || c == '\r') {
		if (buffer_index)
			flush(std::string(buffer, buffer_index));
		buffer_index = 0;
	} else {
		buffer[buffer_index++] = c;
		if (buffer_index >= BUFFER_LENGTH) {
			flush(std::string(buffer, buffer_index));
			buffer_index = 0;
		}
	}
}


void LogBuffer::flush(const std::string &buffer)
{
	logger.log(level, buffer);
}

void RawLogBuffer::flush(const std::string &buffer)
{
	g_logger.logRaw(LL_NONE, buffer);
}
