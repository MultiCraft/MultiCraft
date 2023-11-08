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

#pragma once

#include "chacha.h"
#include "sha256.h"

#include <cstring>
#include <string>

class Encryption
{
private:
	static const unsigned int salt_size = SHA256_BLOCK_SIZE;
	static const unsigned int mac_size = SHA256_BLOCK_SIZE;

	static void hmacInit(SHA256_CTX *ctx, const uint8_t *key);
	static void hmacFinal(SHA256_CTX *ctx, const uint8_t *key, uint8_t *hash);
	static void generateSalt(unsigned char *salt, unsigned int size);

public:
	struct EncryptedData
	{
		const unsigned char id[4] = {'E', 'N', 'C', '1'};
		unsigned char salt[salt_size] = {};
		unsigned char mac[mac_size] = {};
		std::string data;

		void toString(std::string &raw_data)
		{
			uint64_t data_size = data.size();

			raw_data.clear();
			raw_data.append((char *)id, sizeof(id));
			raw_data.append((char *)salt, salt_size);
			raw_data.append((char *)mac, mac_size);
			raw_data.append((char *)(&data_size), sizeof(uint64_t));
			raw_data.append((char *)data.c_str(), data_size);
		}

		bool fromString(const std::string &raw_data)
		{
			if (raw_data.size() < sizeof(id) + salt_size + mac_size +
							      sizeof(uint64_t))
				return false;

			unsigned char id_from_data[sizeof(id)] = {};
			memcpy(id_from_data, &raw_data[0], sizeof(id));

			if (memcmp(id_from_data, id, sizeof(id)))
				return false;

			uint64_t data_size = 0;

			memcpy(salt, &raw_data[sizeof(id)], salt_size);
			memcpy(mac, &raw_data[sizeof(id) + salt_size], mac_size);
			memcpy(&data_size, &raw_data[sizeof(id) + salt_size + mac_size],
					sizeof(uint64_t));

			if (data_size == 0)
				return false;

			if (raw_data.size() < sizeof(id) + salt_size + mac_size +
							      sizeof(uint64_t) +
							      data_size)
				return false;

			data.clear();
			data.append(&raw_data[sizeof(id) + salt_size + mac_size +
						    sizeof(uint64_t)],
					data_size);

			return true;
		}
	};

	static bool encrypt(const std::string &data, EncryptedData &encrypted_data);
	static bool decrypt(EncryptedData &encrypted_data, std::string &data);
	static void setKey(uint8_t new_key[32]);
};
