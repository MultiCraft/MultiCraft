// SPDX-FileCopyrightText: 2024 Luanti Contributors
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "hashing.h"

#include "debug.h"
#include "config.h"
#if USE_OPENSSL
#include <openssl/sha.h>
#include <openssl/evp.h>
#else
#include "util/sha1.h"
#endif

namespace hashing
{

std::string sha1(const std::string &data)
{
#if USE_OPENSSL
	std::string digest(SHA1_DIGEST_SIZE, '\000');
	auto src = reinterpret_cast<const uint8_t *>(data.data());
	auto dst = reinterpret_cast<uint8_t *>(&digest[0]);
	SHA1(src, data.size(), dst);
	return digest;
#else
	SHA1 sha1;
	sha1.addBytes(data.c_str(), data.size());
	unsigned char *digest = sha1.getDigest();
	std::string ret(reinterpret_cast<char*>(digest), SHA1_DIGEST_SIZE);
	free(digest);
	return ret;
#endif
}

}
