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

#include <functional>
#include <vector>
#include "util/string.h"
#include "config.h"

// Can be used in place of "caller" in asynchronous transfers to discard result
// (used as default value of "caller")
#define HTTPFETCH_DISCARD 0
#define HTTPFETCH_SYNC 1

//  Methods
enum HttpMethod : u8
{
	HTTP_GET,
	HTTP_POST,
	HTTP_PUT,
	HTTP_DELETE,
};

struct HTTPFetchRequest
{
	std::string url = "";

	// Identifies the caller (for asynchronous requests)
	// Ignored by httpfetch_sync
	unsigned long caller = HTTPFETCH_DISCARD;

	// Some number that identifies the request
	// (when the same caller issues multiple httpfetch_async calls)
	unsigned long request_id = 0;

	// Timeout for the whole transfer, in milliseconds
	long timeout;

	// Timeout for the connection phase, in milliseconds
	long connect_timeout;

	// Indicates if this is multipart/form-data or
	// application/x-www-form-urlencoded.  POST-only.
	bool multipart = false;

	//  The Method to use default = GET
	//  Avaible methods GET, POST, PUT, DELETE
	HttpMethod method = HTTP_GET;

	// Fields of the request
	StringMap fields;

	// Raw data of the request overrides fields
	std::string raw_data;

	// If not empty, should contain entries such as "Accept: text/html"
	std::vector<std::string> extra_headers;

	// useragent to use
	std::string useragent;

	HTTPFetchRequest();
};

struct HTTPFetchResult
{
	bool succeeded = false;
	bool timeout = false;
	long response_code = 0;
	std::string data = "";
	// The caller and request_id from the corresponding HTTPFetchRequest.
	unsigned long caller = HTTPFETCH_DISCARD;
	unsigned long request_id = 0;

	HTTPFetchResult() = default;

	HTTPFetchResult(const HTTPFetchRequest &fetch_request) :
			caller(fetch_request.caller), request_id(fetch_request.request_id)
	{
	}
};

// Initializes the httpfetch module
void httpfetch_init(int parallel_limit);

// Stops the httpfetch thread and cleans up resources
void httpfetch_cleanup();

// Starts an asynchronous HTTP fetch request
void httpfetch_async(const HTTPFetchRequest &fetch_request);

// If any fetch for the given caller ID is complete, removes it from the
// result queue, sets the fetch result and returns true. Otherwise returns false.
bool httpfetch_async_get(unsigned long caller, HTTPFetchResult &fetch_result);

// Allocates a caller ID for httpfetch_async
// Not required if you want to set caller = HTTPFETCH_DISCARD
unsigned long httpfetch_caller_alloc();

// Allocates a non-predictable caller ID for httpfetch_async
unsigned long httpfetch_caller_alloc_secure();

// Frees a caller ID allocated with httpfetch_caller_alloc
// Note: This can be expensive, because the httpfetch thread is told
// to stop any ongoing fetches for the given caller.
void httpfetch_caller_free(unsigned long caller);

// Performs a synchronous HTTP request that is interruptible if the current
// thread is a Thread object. interval is the completion check interval in ms.
// This blocks and therefore should only be used from background threads.
// Returned is whether the request completed without interruption.
bool httpfetch_sync_interruptible(const HTTPFetchRequest &fetch_request,
		HTTPFetchResult &fetch_result, long interval = 25,
		std::function<bool()> is_cancelled = {});
