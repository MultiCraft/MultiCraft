/*
MultiCraft
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

#if defined(__ANDROID__) || defined(__APPLE__)
#include <porting.h>
#endif

uint8_t Encryption::key[32] = {};

void Encryption::generateSalt(unsigned char *salt, unsigned int size)
{
	std::random_device rd;

	for (unsigned int i = 0; i < size; i++) {
		salt[i] = static_cast<unsigned char>(rd());
	}
}

void Encryption::hmacInit(SHA256_CTX *ctx, const uint8_t *key)
{
	uint8_t pad[SHA256_DIGEST_LENGTH];
	SHA256_Init(ctx);
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
		pad[i] = key[i] ^ 0x36U;
	SHA256_Update(ctx, pad, sizeof(pad));
}

void Encryption::hmacFinal(SHA256_CTX *ctx, const uint8_t *key, uint8_t *hash)
{
	uint8_t pad[SHA256_DIGEST_LENGTH];
	SHA256_Final(hash, ctx);
	SHA256_Init(ctx);
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
		pad[i] = key[i] ^ 0x5cU;
	SHA256_Update(ctx, pad, sizeof(pad));
	SHA256_Update(ctx, hash, SHA256_DIGEST_LENGTH);
	SHA256_Final(hash, ctx);
}

bool Encryption::encrypt(const std::string &data, EncryptedData &encrypted_data)
{
	static uint8_t buffer[2][ECRYPT_BLOCKLENGTH * 1024];
	size_t data_size = data.size();

	encrypted_data.data.clear();
	encrypted_data.data.reserve(data_size);

	// generateSalt(encrypted_data.salt, salt_size);

	ECRYPT_ctx ctx[1];
	ECRYPT_keysetup(ctx, key, 256, 0);
	ECRYPT_ivsetup(ctx, encrypted_data.salt, 0);

	SHA256_CTX hmac[1];
	hmacInit(hmac, key);

	for (uint32_t i = 0; i < data_size; i += sizeof(buffer[0])) {
		uint32_t z;
		if (i + sizeof(buffer[0]) >= data_size)
			z = data_size - i;
		else
			z = sizeof(buffer[0]);

		memcpy(buffer[0], &data[i], z);
		SHA256_Update(hmac, buffer[0], z);
		ECRYPT_encrypt_bytes(ctx, buffer[0], buffer[1], z, 20);

		encrypted_data.data.append((char *)buffer[1], z);
	}

	hmacFinal(hmac, key, encrypted_data.mac);

	return true;
}

bool Encryption::decrypt(EncryptedData &encrypted_data, std::string &data)
{
	static uint8_t buffer[2][ECRYPT_BLOCKLENGTH * 1024];
	size_t data_size = encrypted_data.data.size();

	data.clear();
	data.reserve(data_size);

	ECRYPT_ctx ctx[1];
	ECRYPT_keysetup(ctx, key, 256, 0);
	ECRYPT_ivsetup(ctx, encrypted_data.salt, 0);

	SHA256_CTX hmac[1];
	hmacInit(hmac, key);

	for (uint32_t i = 0; i < data_size; i += sizeof(buffer[0])) {
		uint32_t z;
		if (i + sizeof(buffer[0]) >= data_size)
			z = data_size - i;
		else
			z = sizeof(buffer[0]);

		memcpy(buffer[0], &(encrypted_data.data)[i], z);
		ECRYPT_encrypt_bytes(ctx, buffer[0], buffer[1], z, 20);
		SHA256_Update(hmac, buffer[1], z);

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

void Encryption::setKey(std::string new_key)
{
	std::string resized_key = new_key;
	resized_key.resize(32, '0');

	setKey((uint8_t *)&resized_key[0]);
}

#if defined(__ANDROID__) || defined(__APPLE__)
bool Encryption::decryptSimple(const std::string &data, std::string &decrypted_data)
{
#ifdef SIGN_KEY
	static std::string secret_key = porting::getSecretKey(SIGN_KEY);
#else
	static std::string secret_key = porting::getSecretKey("");
#endif
	setKey(secret_key);

	EncryptedData encrypted_data;
	if (!encrypted_data.fromString(data))
		return false;

	return decrypt(encrypted_data, decrypted_data);
}
#endif
