/*
MultiCraft
Copyright (C) 2025 Dawid Gan <deveee@gmail.com>

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

#if defined(__APPLE__)

#include <CommonCrypto/CommonDigest.h>
#include <stddef.h>

#define SHA_DIGEST_LENGTH 20
#define SHA224_DIGEST_LENGTH 28
#define SHA256_DIGEST_LENGTH 32
#define SHA384_DIGEST_LENGTH 48
#define SHA512_DIGEST_LENGTH 64

typedef CC_SHA1_CTX SHA_CTX; // It's in sha2.h for some reason
typedef CC_SHA256_CTX SHA256_CTX;

static inline int SHA224_Init(SHA256_CTX *c)
{
	return CC_SHA224_Init(c);
}

static inline int SHA224_Update(SHA256_CTX *c, const void *data, size_t len)
{
	return CC_SHA224_Update(c, data, (CC_LONG)len);
}

static inline int SHA224_Final(unsigned char *md, SHA256_CTX *c)
{
	return CC_SHA224_Final(md, c);
}

static inline unsigned char *SHA224(const unsigned char *d, size_t n, unsigned char *md)
{
	static unsigned char m[CC_SHA224_DIGEST_LENGTH];

	if (md == NULL)
		md = m;

	return CC_SHA224(d, (CC_LONG)n, md);
}

static inline int SHA256_Init(SHA256_CTX *c)
{
	return CC_SHA256_Init(c);
}

static inline int SHA256_Update(SHA256_CTX *c, const void *data, size_t len)
{
	return CC_SHA256_Update(c, data, (CC_LONG)len);
}

static inline int SHA256_Final(unsigned char *md, SHA256_CTX *c)
{
	return CC_SHA256_Final(md, c);
}

static inline unsigned char *SHA256(const unsigned char *d, size_t n, unsigned char *md)
{
	static unsigned char m[CC_SHA256_DIGEST_LENGTH];

	if (md == NULL)
		md = m;

	return CC_SHA256(d, (CC_LONG)n, md);
}

#endif
