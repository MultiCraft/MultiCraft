/*
Minetest
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

#include "encryption.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>

uint8_t key[32] = {230, 46, 18, 105, 49, 123, 150, 84, 225, 49, 77, 254, 203, 120, 242,
		155, 53, 173, 77, 54, 45, 160, 169, 194, 204, 219, 104, 10, 165, 53, 215,
		234};

void Encryption::generateSalt(unsigned char *salt, unsigned int size)
{
	std::random_device rd;

	for (unsigned int i = 0; i < size; i++) {
		salt[i] = static_cast<unsigned char>(rd());
	}
}

void Encryption::hmacInit(SHA256_CTX *ctx, const uint8_t *key)
{
	uint8_t pad[SHA256_BLOCK_SIZE];
	sha256_init(ctx);
	for (int i = 0; i < SHA256_BLOCK_SIZE; i++)
		pad[i] = key[i] ^ 0x36U;
	sha256_update(ctx, pad, sizeof(pad));
}

void Encryption::hmacFinal(SHA256_CTX *ctx, const uint8_t *key, uint8_t *hash)
{
	uint8_t pad[SHA256_BLOCK_SIZE];
	sha256_final(ctx, hash);
	sha256_init(ctx);
	for (int i = 0; i < SHA256_BLOCK_SIZE; i++)
		pad[i] = key[i] ^ 0x5cU;
	sha256_update(ctx, pad, sizeof(pad));
	sha256_update(ctx, hash, SHA256_BLOCK_SIZE);
	sha256_final(ctx, hash);
}

bool Encryption::encrypt(const std::string &data, EncryptedData &encrypted_data)
{
	static uint8_t buffer[2][CHACHA_BLOCKLENGTH * 1024];
	size_t data_size = data.size();

	encrypted_data.data.clear();
	encrypted_data.data.reserve(data_size);

	generateSalt(encrypted_data.salt, salt_size);

	chacha_ctx ctx[1];
	chacha_keysetup(ctx, key, 256);
	chacha_ivsetup(ctx, encrypted_data.salt);

	SHA256_CTX hmac[1];
	hmacInit(hmac, key);

	for (uint32_t i = 0; i < data_size; i += sizeof(buffer[0])) {
		uint32_t z;
		if (i + sizeof(buffer[0]) >= data_size)
			z = data_size - i;
		else
			z = sizeof(buffer[0]);

		memcpy(buffer[0], &data[i], z);
		sha256_update(hmac, buffer[0], z);
		chacha_encrypt(ctx, buffer[0], buffer[1], z);

		encrypted_data.data.append((char *)buffer[1], z);
	}

	hmacFinal(hmac, key, encrypted_data.mac);

	return true;
}

bool Encryption::decrypt(EncryptedData &encrypted_data, std::string &data)
{
	static uint8_t buffer[2][CHACHA_BLOCKLENGTH * 1024];
	size_t data_size = encrypted_data.data.size();

	data.clear();
	data.reserve(data_size);

	chacha_ctx ctx[1];
	chacha_keysetup(ctx, key, 256);
	chacha_ivsetup(ctx, encrypted_data.salt);

	SHA256_CTX hmac[1];
	hmacInit(hmac, key);

	for (uint32_t i = 0; i < data_size; i += sizeof(buffer[0])) {
		uint32_t z;
		if (i + sizeof(buffer[0]) >= data_size)
			z = data_size - i;
		else
			z = sizeof(buffer[0]);

		memcpy(buffer[0], &(encrypted_data.data)[i], z);
		chacha_encrypt(ctx, buffer[0], buffer[1], z);
		sha256_update(hmac, buffer[1], z);

		data.append((char *)buffer[1], z);
	}

	unsigned char mac[mac_size];
	hmacFinal(hmac, key, mac);

	if (memcmp(encrypted_data.mac, mac, mac_size))
		return false;

	return true;
}

void Encryption::setKey(uint8_t new_key[32])
{
	memcpy(key, new_key, 32);
}
